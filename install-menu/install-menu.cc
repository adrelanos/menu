/*
 * Debian menu system -- install-menu
 * install-menu/install-menu.cc
 *
 * Copyright (C) 1996-2003  Joost Witteveen, 
 * Copyright (C) 2002-2004  Bill Allombert and Morten Brix Pedersen.
 * 
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License with
 * the Debian GNU/Linux distribution in file /usr/share/common-licenses/GPL;
 * if not, write to the Free Software Foundation, Inc., 59 Temple Place,
 * Suite 330, Boston, MA  02111-1307  USA
 *
 *
 * Written by Joost Witteveen.
 */

#include <set>
#include <sstream>
#include <iostream>
#include <fstream>
#include <cctype>
#include <cstdlib>
#include <getopt.h>
#include <limits.h>
#include <sys/types.h>
#include <dirent.h>
#include <unistd.h>
#include <sys/stat.h>
#include <errno.h>
#include <pwd.h>
#include "config.h"
#include <iconv.h>
#include <langinfo.h>

#include "install-menu.h"
#include "menu-tree.h"

using std::vector;
using std::string;
using std::map;
using std::set;
using std::ostream;
using std::ofstream;
using std::ifstream;
using std::cerr;
using std::cout;
using std::endl;

int verbose = 0;
const char * menuencoding = "UTF-8";

map<string, func *> func_data;

menuentry root_menu;
configinfo *config;
supportedinfo *supported;

std::ofstream *genoutputfile = 0;
set<string> outputnames; // all names of files ever written to

struct option long_options[] = { 
  { "verbose", no_argument, NULL, 'v' }, 
  { "help", no_argument, NULL, 'h' }, 
  { NULL, 0, NULL, 0 } };

// quick hack to get the 'repeat_lang="LOCALE"' working.
// main() checks for value of repeat_lang, and sets
// this variable if repeat_lang==LOCALE.
bool do_translate_hack = false;

void store_func(func *f)
{
  func_data[f->name()]=f;
}

// This function is used to convert a string from one encoding to another.
//
// The function could probably be prettier, but it works. One of the things
// I dislike about it is that the cleanup operations are duplicated three
// times (e.g. delete[]).
std::string convert(const std::string& str)
{
  //std::cerr << "Original string: " << str << std::endl;
  size_t insize = str.length();
  char *inbuf = new char[insize + 1];
  char *inbuf_t = inbuf;
  strcpy(inbuf, str.c_str());

  // Max length of a UTF-8 character is 6 per input character.
  size_t outsize = insize * 6;
  size_t outsize_t = outsize;

  char *outbuf = new char[outsize];
  char *outbuf_t = outbuf;

  iconv_t conversion = iconv_open(config->outputencoding().c_str(), menuencoding);

  if (conversion == (iconv_t)(-1)) {
      delete []inbuf;
      delete []outbuf;
      throw conversion_error(strerror(errno));
  }

  size_t retval = iconv(conversion, &inbuf_t, &insize, &outbuf_t, &outsize_t);
  if (retval == (size_t)(-1)) {
      delete []inbuf;
      delete []outbuf;
      iconv_close(conversion);
      throw conversion_error(strerror(errno));
  }

  std::string conv_str = string(outbuf, outsize - outsize_t);
  //std::cerr << "Converted string: " << conv_str << std::endl;
  iconv_close(conversion);
  delete []inbuf;
  delete []outbuf;
  return conv_str;
}

void add_functions()
{
  store_func(new prefix_func);
  store_func(new ifroot_func);

  store_func(new print_func);
  store_func(new add_func);
  store_func(new sub_func);
  store_func(new mult_func);
  store_func(new div_func);
  store_func(new ifempty_func);
  store_func(new ifnempty_func);
  store_func(new iffile_func);
  store_func(new ifelsefile_func);
  store_func(new ifelse_func);
  store_func(new catfile_func);
  store_func(new forall_func);

  store_func(new ifeq_func);
  store_func(new ifneq_func);
  store_func(new ifeqelse_func);

  store_func(new cond_surr_func);
  store_func(new esc_func);
  store_func(new escwith_func);
  store_func(new escfirst_func);
  store_func(new tolower_func);  
  store_func(new toupper_func);
  store_func(new replacewith_func);
  store_func(new nstring_func);  
  store_func(new cppesc_func);
  store_func(new parent_func);
  store_func(new basename_func);
  store_func(new stripdir_func);
  store_func(new entrycount_func);
  store_func(new entryindex_func);
  store_func(new firstentry_func);
  store_func(new lastentry_func);
  store_func(new level_func);

  store_func(new rcfile_func);
  store_func(new examplercfile_func);
  store_func(new mainmenutitle_func);
  store_func(new rootsection_func);
  store_func(new rootprefix_func);
  store_func(new userprefix_func);
  store_func(new treewalk_func);
  store_func(new postoutput_func);
  store_func(new preoutput_func);
  store_func(new cwd_func);
  store_func(new translate_func);
}

bool empty_string(const string &s)
{
  if (s.empty() || s == "none")
    return true;
  else
    return false;
}

cat_str *get_eq_cat_str(parsestream &i)
{
  i.skip_space();
  i.skip_char('=');
  return new cat_str(i);
}

int check_dir(string s)
{
  string t;
  if (verbose)
    cerr << String::compose(_("install-menu: checking directory %1\n"), s);
  while (!s.empty()) {
    string::size_type i = s.find('/', 1);
    if (i != string::npos) {
      t = s.substr(0, i+1);
      for(; i < s.length() && (s[i] == '/'); i++);
      s = s.substr(i);
    } else {
      t = s;
      s.erase();
    }
    if (chdir(t.c_str()) < 0) {
      if (verbose)
        cerr << String::compose(_("install-menu: creating directory %1:\n"), t) ;
      if (mkdir(t.c_str(),0755))
	throw dir_createerror(t);
      if (chdir(t.c_str()))
	throw dir_createerror(t);
    } else {
      if (verbose)
	cerr << String::compose(_("install-menu: directory %1 already exists\n"), t);
    }
  }
  return !s.length();
}

void closegenoutput()
{
  set<string>::const_iterator i;

  // OK, this will look clumsy in strace output, especially if there's
  // only output file: first we close, and immediately afterwards we
  // open and write again. But I don't see an easy and general way
  // to get round that.
  if (genoutputfile) {
    delete genoutputfile;
    genoutputfile = 0;
  }
  for(i = outputnames.begin(); i != outputnames.end(); ++i)
  {
    std::ofstream f(i->c_str(), std::ios::app);
    f << config->postoutput();
  }
}

void genoutput(const string &s, map<string, string> &v)
{
  static string lastname = "////";

  string name = config->prefix() + '/' + config->genmenu->soutput(v);
  if (name != lastname) {
    if (genoutputfile)
        delete genoutputfile;
    if (outputnames.find(name) == outputnames.end()) {
      outputnames.insert(name);
      check_dir(string_parent(name));
      // after opening a file with ios::trunc, all writes seem to fail!
      //genoutputfile=new ofstream(name.c_str(), ios::trunc);
      // So, I do it this way instead:
      unlink(name.c_str());
      genoutputfile = new std::ofstream(name.c_str());
      (*genoutputfile) << config->preoutput();
    } else {
      genoutputfile = new std::ofstream(name.c_str(), std::ios::app);
    }
    lastname = name;
  }
  (*genoutputfile) << s;
}

/////////////////////////////////////////////////////
//  Construction:
//

cat_str::cat_str(parsestream &i)
{
  char buf[2]=".";
  char c = i.get_char();
  i.put_back(c);
  try {
    while(1)
    {
      i.skip_space();
      c=i.get_char();
      i.put_back(c);
      if (isalpha(c))
	v.push_back(new func_str(i));
      else if (c == '\"') 
	v.push_back(new const_str(i));
      else if (c == '$') 
	v.push_back(new var_str(i));
      else if(c == ',')
	break;
      else if(c == ')')
	break;
      else if(c == '\0')
	break;
      else {
	buf[0] = c;
	throw char_unexpected(&i, buf);
      }
    }
  } catch(endofline p) { }
}

var_str::var_str(parsestream &i)
{
  i.skip_char('$');
  var_name = i.get_name();
}

func_str::func_str(parsestream &i)
{
  char c;
  string name = i.get_name();
  map<string, func *>::const_iterator j = func_data.find(name);
  if (j == func_data.end())
      throw unknown_function(&i, name);
  else
      f = j->second;

  i.skip_space();
  i.skip_char('(');
  do {
    i.skip_space();
    c = i.get_char();
    if(c == ')')
      break;
    i.put_back(c);
    args.push_back(new cat_str(i));
    i.skip_space();
  } while((c = i.get_char()) && (c == ','));
  i.put_back(c);
  i.skip_char(')');
  if (args.size() != f->nargs())
    throw narg_mismatch(&i, name);
}


/////////////////////////////////////////////////////
//  Output routines
//

ostream &const_str::output(ostream &o, map<string, string> &)
{
  return o << data;
}

string cat_str::soutput(map<string, string> &menuentry)
{
  std::ostringstream s;

  for(vector<str * >::const_iterator i = v.begin(); i != v.end(); ++i)
      (*i)->output(s, menuentry);

  return s.str();
}

ostream &cat_str::output(ostream &o, map<string, string> &menuentry)
{
  return o << soutput(menuentry);
}

void cat_str::output(map<string, string> &menuentry)
{
  genoutput(soutput(menuentry),menuentry);
}

ostream &var_str::output(ostream &o, map<string, string> &menuentry)
{
  return o << menuentry[var_name];
}

ostream &func_str::output(ostream &o, map<string, string> &menuentry)
{
  return f->output(o, args, menuentry);
}

/////////////////////////////////////////////////////
//  Debug routines
//
ostream &const_str::debuginfo(ostream &o)
{
  return o << "CONST_STR: " << data << endl;
}

ostream &cat_str::debuginfo(ostream &o)
{
  vector<str *>::const_iterator i;
  o << "CAT_STR: " << endl;
  for(i = v.begin(); i != v.end(); ++i)
      (*i)->debuginfo(o);

  return o;
}

ostream &var_str::debuginfo(ostream &o)
{
  return o << "VAR_STR: " << var_name << endl;
}

ostream &func_str::debuginfo(ostream &o)
{
  o << "FUNC_STR: " << f->name() << " (" << endl;
  vector<cat_str *>::const_iterator i;  
  for(i = args.begin(); i != args.end(); ++i)
  {
    o << ", ";
    (*i)->debuginfo(o);
  }
  return o << ")" << endl;
}


/////////////////////////////////////////////////////
//  Function definitions:
//

ostream &prefix_func::output(ostream &o, vector<cat_str *> &,
    map<string, string> &)
{
  return o << config->prefix();
}

ostream &ifroot_func::output(ostream &o, vector<cat_str *> & args,
    map<string, string> &menuentry)
{
  if(getuid())
    args[1]->output(o,menuentry);
  else
    args[0]->output(o,menuentry);

  return o;
}

ostream &print_func::output(ostream &o, vector<cat_str *> &args,
    map<string, string> &menuentry)
{
  string s=args[0]->soutput(menuentry);

  if (empty_string(s)) {
    cerr << _("Zero-size argument to print function.");
    throw informed_fatal();
  }
  return o << s;
}

ostream &ifempty_func::output(ostream &o, vector<cat_str *> &args,
    map<string, string> &menuentry)
{
  string s=args[0]->soutput(menuentry);

  if(empty_string(s))
    args[1]->output(o,menuentry);
  return o;
}

ostream &ifnempty_func::output(ostream &o, vector<cat_str *> &args,
    map<string, string> &menuentry)
{
  string s=args[0]->soutput(menuentry);
  if(!empty_string(s))
    args[1]->output(o,menuentry);
  return o;
}

ostream &iffile_func::output(ostream &o, vector<cat_str *> &args,
    map<string, string> &menuentry)
{
  string s=args[0]->soutput(menuentry);
  ifstream f(s.c_str());
  if(f)
    args[1]->output(o,menuentry);
  return o;
}

ostream &ifelsefile_func::output(ostream &o, vector<cat_str *> &args,
    map<string, string> &menuentry)
{
  string s=args[0]->soutput(menuentry);
  ifstream f(s.c_str());
  if(f)
    args[1]->output(o,menuentry);
  else
    args[2]->output(o,menuentry);
  return o;
}

ostream &catfile_func::output(ostream &o, vector<cat_str *> &args,
    map<string, string> &menuentry)
{
  string s=args[0]->soutput(menuentry);
  ifstream f(s.c_str());
  char c;

  while(f && f.get(c))
    o<<c;
  return o;
}
ostream &forall_func::output(ostream &o, vector<cat_str *> &args,
    map<string, string> &menuentry)
{
  const string &array=args[0]->soutput(menuentry);
  vector<string> vec;
  vector<string>::const_iterator i;
  string s;
  string varname=args[1]->soutput(menuentry);

  break_char(array, vec, ':');
  
  for(i = vec.begin(); i != vec.end(); ++i){
    menuentry[varname]=(*i);
    s+=args[2]->soutput(menuentry);
  }
  return o<<s;
}

ostream &esc_func::output(ostream &o, vector<cat_str *> &args,
    map<string, string> &menuentry)
{
  return o<<escape_string(args[0]->soutput(menuentry),
			  args[1]->soutput(menuentry));
}

ostream &escwith_func::output(ostream &o, vector<cat_str *> &args,
    map<string, string> &menuentry)
{
  return o<<escapewith_string(args[0]->soutput(menuentry),
			      args[1]->soutput(menuentry),
			      args[2]->soutput(menuentry));
}

ostream &escfirst_func::output(ostream &o, vector<cat_str *> &args,
    map<string, string> &menuentry)
{
  string s=args[0]->soutput(menuentry);
  string esc=args[1]->soutput(menuentry);
  string t;

  for(string::size_type i=0; i != s.length(); ++i){
    if(esc.length() && contains(esc, s[i])) {
      t=s.substr(0,i);
      t+=args[2]->soutput(menuentry);
      t+=s.substr(i);
      break;
    }
    t+=s[i];
  }
  return o<<t;
}
ostream &tolower_func::output(ostream &o, vector<cat_str *> &args,
    map<string, string> &menuentry)
{
  return o<<lowercase(args[0]->soutput(menuentry));
}

ostream &toupper_func::output(ostream &o, vector<cat_str *> &args,
    map<string, string> &menuentry)
{
  return o<<uppercase(args[0]->soutput(menuentry));
}
ostream &replacewith_func::output(ostream &o, vector<cat_str *> &args,
    map<string, string> &menuentry)
{
  return o<<replacewith_string(args[0]->soutput(menuentry),
			       args[1]->soutput(menuentry),
			       args[2]->soutput(menuentry));
}

ostream &nstring_func::output(ostream &o, vector<cat_str *> &args,
    map<string, string> &menuentry)
{
  int count= stringtoi(args[0]->soutput(menuentry));
  int i;

  for(i=0; i<count; i++)
    o<<args[1]->soutput(menuentry);

  return o;
}
ostream &cppesc_func::output(ostream &o, vector<cat_str *> &args,
    map<string, string> &menuentry)
{
  return o<<cppesc_string(args[0]->soutput(menuentry));
}

ostream &add_func::output(ostream &o, vector<cat_str *> &args,
    map<string, string> &menuentry)
{
  int x=stringtoi(args[0]->soutput(menuentry));
  int y=stringtoi(args[1]->soutput(menuentry));

  return o<<itostring(x+y);
}

ostream &sub_func::output(ostream &o, vector<cat_str *> &args,
    map<string, string> &menuentry)
{
  int x=stringtoi(args[0]->soutput(menuentry));
  int y=stringtoi(args[1]->soutput(menuentry));

  return o<<itostring(x-y);
}

ostream &mult_func::output(ostream &o, vector<cat_str *> &args,
    map<string, string> &menuentry)
{
  int x=stringtoi(args[0]->soutput(menuentry));
  int y=stringtoi(args[1]->soutput(menuentry));

  return o<<itostring(x*y);
}

ostream &div_func::output(ostream &o, vector<cat_str *> &args,
    map<string, string> &menuentry)
{
  int x=stringtoi(args[0]->soutput(menuentry));
  int y=stringtoi(args[1]->soutput(menuentry));
  
  if(y)
    return o<<itostring(x/y);
  else
    return o<<"0";
}

ostream &ifelse_func::output(ostream &o, vector<cat_str *> &args,
    map<string, string> &menuentry)
{
  string s=args[0]->soutput(menuentry);

  (!empty_string(s)) ?
    args[1]->output(o,menuentry)
    :
    args[2]->output(o,menuentry);
  return o;
}

ostream &ifeq_func::output(ostream &o, vector<cat_str *> &args,
    map<string, string> &menuentry)
{
  string s=args[0]->soutput(menuentry);
  string t=args[1]->soutput(menuentry);
  if(s==t)
    args[2]->output(o,menuentry);
  
  return o;
}
ostream &ifneq_func::output(ostream &o, vector<cat_str *> &args,
    map<string, string> &menuentry)
{
  string s=args[0]->soutput(menuentry);
  string t=args[1]->soutput(menuentry);
  if(!(s==t))
    args[2]->output(o,menuentry);
  
  return o;
}
ostream &ifeqelse_func::output(ostream &o, vector<cat_str *> &args,
    map<string, string> &menuentry)
{
  string s=args[0]->soutput(menuentry);
  string t=args[1]->soutput(menuentry);
  if(s==t)
    args[2]->output(o,menuentry);
  else
    args[3]->output(o,menuentry);

  return o;
}

ostream &cond_surr_func::output(ostream &o, vector<cat_str *> &args,
    map<string, string> &menuentry)
{
  string s=args[0]->soutput(menuentry);

  if(!empty_string(s)){
    args[1]->output(o,menuentry);
    args[0]->output(o,menuentry);
    args[2]->output(o,menuentry);
  }
  return o;
}

ostream &parent_func::output(ostream &o, vector<cat_str *> &args,
    map<string, string> &menuentry)
{
  string s=args[0]->soutput(menuentry);

  return o<<string_parent(s);
}

ostream &basename_func::output(ostream &o, vector<cat_str *> &args,
    map<string, string> &menuentry)
{
  string s=args[0]->soutput(menuentry);
  
  return o<<string_basename(s);
}

ostream &stripdir_func::output(ostream &o, vector<cat_str *> &args,
    map<string, string> &menuentry)
{
  string s=args[0]->soutput(menuentry);
  
  return o<<string_stripdir(s);
}

ostream &entrycount_func::output(ostream &o, vector<cat_str *> &/*args*/,
    map<string, string> &menuentry)
{
  return o<<menuentry[PRIVATE_ENTRYCOUNT_VAR];
}

ostream &entryindex_func::output(ostream &o, vector<cat_str *> &/*args*/,
    map<string, string> &menuentry)
{
  return o<<menuentry[PRIVATE_ENTRYINDEX_VAR];
}

ostream &firstentry_func::output(ostream &o, vector<cat_str *> &args,
    map<string, string> &menuentry)
{
  int index=stringtoi(menuentry[PRIVATE_ENTRYINDEX_VAR]);

  if(index == 0)
    args[0]->output(o,menuentry);
  return o;
}

ostream &lastentry_func::output(ostream &o, vector<cat_str *> &args,
    map<string, string> &menuentry)
{
  int index=stringtoi(menuentry[PRIVATE_ENTRYINDEX_VAR]);
  int count=stringtoi(menuentry[PRIVATE_ENTRYCOUNT_VAR]);

  if(index+1 == count)
    args[0]->output(o,menuentry);
  return o;
}

ostream &level_func::output(ostream &o, vector<cat_str *> &/*args*/,
    map<string, string> &menuentry)
{
  return o<<menuentry[PRIVATE_LEVEL_VAR];
}

ostream &rcfile_func::output(ostream &o, vector<cat_str *> &/*args*/,
    map<string, string> &menuentry)
{
  return o<<config->rcfile();
}
ostream &examplercfile_func::output(ostream &o, vector<cat_str *> &/*args*/,
    map<string, string> &menuentry)
{
  return o<<config->examplercfile();
}
ostream &mainmenutitle_func::output(ostream &o, vector<cat_str *> &/*args*/,
    map<string, string> &menuentry)
{
  return o<<config->mainmenutitle();
}
ostream &rootsection_func::output(ostream &o, vector<cat_str *> &/*args*/,
    map<string, string> &menuentry)
{
  return o<<config->rootsection();
}

ostream &rootprefix_func::output(ostream &o, vector<cat_str *> &/*args*/,
    map<string, string> &menuentry)
{
  return o<<config->rootprefix();
}

ostream &userprefix_func::output(ostream &o, vector<cat_str *> &/*args*/,
    map<string, string> &menuentry)
{
  return o<<config->userprefix();
}

ostream &treewalk_func::output(ostream &o, vector<cat_str *> &/*args*/,
    map<string, string> &menuentry)
{
  return o<<config->treewalk();
}
ostream &postoutput_func::output(ostream &o, vector<cat_str *> &/*args*/,
    map<string, string> &menuentry)
{
  return o<<config->postoutput();
}
ostream &preoutput_func::output(ostream &o, vector<cat_str *> &/*args*/,
    map<string, string> &menuentry)
{
  return o<<config->preoutput();
}
ostream &cwd_func::output(ostream &o, vector<cat_str *> &/*args*/,
    map<string, string> &menuentry)
{
  char buf[300];
  //Bug: getcwd returns NULL if strlen(cwd) > sizeof(buf), so we get
  //     a segfault here.
  return o<<string(getcwd(buf,sizeof(buf)));
}

ostream &translate_func::output(ostream &o, vector<cat_str *> &args,
    map<string, string> &menuentry)
{
  string lang=args[0]->soutput(menuentry);
  string text=args[1]->soutput(menuentry);

  return o << ldgettext(lang.c_str(), "menu-sections", text.c_str());
}


/////////////////////////////////////////////////////
//  "defined" function (macro).
//

ostream &func_def::output(ostream &o, vector<cat_str *> &args,
    map<string, string> &menuentry)
{
  map<string, string> local_menuentry = menuentry;

  for(unsigned int i = 0; i < args_name.size(); ++i)
      local_menuentry[args_name[i]] = args[i]->soutput(menuentry);

  f->output(o,local_menuentry);
  return o;
}

func_def::func_def(parsestream &i)
{
  char c;

  func_name = i.get_name();
  i.skip_space();
  i.skip_char('(');
  do {
    i.skip_space();
    c=i.get_char();
    if(c==')')
      break;
    i.put_back(c);
    i.skip_char('$');
    args_name.push_back(i.get_name());
    i.skip_space();
  } while((c=i.get_char())&&(c==','));
  i.put_back(c);
  i.skip_char(')');
  i.skip_space();
  i.skip_char('=');
  f = new cat_str(i);
}

/////////////////////////////////////////////////////
//  Supported stuff
//

supportedinfo::supportedinfo(parsestream &i)
{
  int prec=0;
  while(1)
  {
    try {
      i.skip_space();
      string name = i.get_name();
      if (name == "endsupported")
          return;

      name=uppercase(name);

      if (verbose)
          /*Do not translate supported*/
          cerr << String::compose(_("install-menu: [supported]: name=%1\n"), name);

      supinf inf;
      inf.c=get_eq_cat_str(i);
      inf.prec=prec++;
      sup[name]=inf;
      if (verbose)
          sup[name].c->debuginfo(cerr);
      i.skip_line();   //read away the final newline
    }
    catch (endofline d) { }
  }
}

void supportedinfo::subst(map<string, string> vars)
{

  map<string, string>::const_iterator i;
  map<string, supinf>::const_iterator j;

  if ((i = vars.find(NEEDS_VAR)) == vars.end()) {
    cerr << String::compose(_("Menu entry lacks mandatory field \"%1\".\n"), NEEDS_VAR);
    throw informed_fatal();
  }
  if ((j = sup.find(uppercase(i->second))) == sup.end()) {
    cerr << String::compose(_("Unknow value for field %1=\"%2\".\n"), NEEDS_VAR, i->second);
    throw informed_fatal();
  }
  genoutput(j->second.c->soutput(vars), vars);
}

int supportedinfo::prec(string &name)
{
  map<string, supinf>::const_iterator i;
  int p;
  if((i=sup.find(uppercase(name)))==sup.end())
    p = INT_MAX;
  else
    p = i->second.prec;
  return p;
}

ostream &supportedinfo::debuginfo(ostream &o)
{
  map<string, supinf>::const_iterator i;
  for (i = sup.begin(); i != sup.end(); i++)
  {
    o <<String::compose("SUPPORTED:** name=%1, prec=%2 Def=\n",i->first,i->second.prec);
    i->second.c->debuginfo(o);
  }
  return o;
}
/////////////////////////////////////////////////////
//  forcetree stuff
//

void read_forcetree(parsestream &i)
{
  Regex r("[a-zA-Z/-_ ]");

  while(1)
  {
    try {
      i.skip_space();
      string name = i.get_name(r);
      if (name.empty())
          throw ident_expected(&i);
      if (name == "endforcetree")
          return;

      vector<string> sections;
      break_slashes(name, sections);
      menuentry *entry = new menuentry;
      entry->forced = true;
      entry->vars[TITLE_VAR] = sections.back();
      root_menu.add_entry_ptr(sections, entry);
      i.skip_line();
    }
    catch(endofline d) { }
  }

}
/////////////////////////////////////////////////////
//  configinfo
//
configinfo::configinfo(parsestream &i) 
    : roots("/Debian"), mainmt("Debian"), treew("c(m)"),
    outputenc("LOCALE"), onlyrunasroot(false),
    onlyrunasuser(false),
    onlyuniquetitles(false), hint_optimize(false), hint_nentry(6),
    hint_topnentry(5), hint_mixedpenalty(15), hint_minhintfreq(0.1),
    hint_mlpenalty(2000), hint_max_ntry(4), hint_max_iter_hint(5),
    hint_debug(false), hotkeycase(0)
{
  userpref=rootpref=sort=prerun=preruntest=postrun=genmenu=
    hkexclude=startmenu=endmenu=submenutitle=also_run=repeat_lang=0;

  /*Should not be translated as this appear in the output file that can use 
   *a different encoding*/
  preout = "# Automatically generated file. Do not edit (see /usr/share/doc/menu/html/index.html)\n\n";

  try {
      while(1)
      {
        string name = i.get_name();
	if (name=="supported") {
	  i.skip_line();
	  supported=new supportedinfo(i);
	} else if (name=="forcetree") {
	  i.skip_line();
	  read_forcetree(i);
	} 
	else if(name=="function")
	  store_func(new func_def(i));
	else if(name=="startmenu")
	  startmenu=get_eq_cat_str(i);
	else if(name=="endmenu")
	  endmenu=get_eq_cat_str(i);
	else if(name=="submenutitle")
	  submenutitle=get_eq_cat_str(i);
	else if(name=="hotkeyexclude")
	  hkexclude=get_eq_cat_str(i);
	else if(name=="genmenu")
	  genmenu=get_eq_cat_str(i);	
	else if(name=="prerun")
	  prerun=get_eq_cat_str(i);	
	else if(name=="postrun")
	  postrun=get_eq_cat_str(i);
	else if(name=="also_run")
	  also_run=get_eq_cat_str(i);
       	else if(name=="preruntest")
	  preruntest=get_eq_cat_str(i);	
	else if(name=="onlyrunasroot")
	  onlyrunasroot=i.get_eq_boolean();	
	else if(name=="onlyrunasuser")
	  onlyrunasuser=i.get_eq_boolean();	
	else if(name=="onlyuniquetitles")
	  onlyuniquetitles=i.get_eq_boolean();	
	else if(name=="sort")
	  sort=get_eq_cat_str(i);

	else if(name=="compat"){
	  compt=i.get_eq_stringconst();
	  if(compt=="menu-1")
	    i.seteolmode(parsestream::eol_newline);
	  else if(compt=="menu-2")
	    i.seteolmode(parsestream::eol_semicolon);
	  else 
	    throw unknown_compat(&i, compt);
	}
	else if(name=="rcfile")
	  rcf=i.get_eq_stringconst();
	else if(name=="examplercfile")
	  exrcf=i.get_eq_stringconst();
	else if(name=="mainmenutitle")
	  mainmt=i.get_eq_stringconst();
	else if(name=="rootsection")
	  roots=i.get_eq_stringconst();
	else if(name=="rootprefix")
	  rootpref=get_eq_cat_str(i);
	else if(name=="userprefix")
	  userpref=get_eq_cat_str(i);
	else if(name=="treewalk")
	  treew=i.get_eq_stringconst();
	else if(name=="postoutput")
	  postout=i.get_eq_stringconst();
	else if(name=="preoutput")
	  preout=i.get_eq_stringconst();
	else if(name=="outputencoding")
	  outputenc=i.get_eq_stringconst();
	else if(name=="command"){
	  system((i.get_eq_stringconst()).c_str());
	  exit(0);
	}
	else if(name=="hotkeycase"){
	  string s = i.get_eq_stringconst();
	  if(s == "sensitive")
	    hotkeycase = 1;
	  else if (s=="insensitive")
	    hotkeycase = 0;
	  else {
	    /*Do not translate quoted text*/ 
            cerr << _("install-menu: \"hotkeycase\" can only be \"sensitive\" or \"insensitive\"\n");
	    throw informed_fatal();
	  }
	} 
	else if(name=="repeat_lang")
	  repeat_lang=get_eq_cat_str(i);
	else if(name=="hint_optimize")
	  hint_optimize=i.get_eq_boolean();
	else if(name=="hint_nentry")
	  hint_nentry=i.get_eq_double();
	else if(name=="hint_topnentry")
	  hint_topnentry=i.get_eq_integer();
	else if(name=="hint_mixedpenalty")
	  hint_mixedpenalty=i.get_eq_double();
	else if(name=="hint_minhintfreq")
	  hint_minhintfreq=i.get_eq_double();
	else if(name=="hint_mlpenalty")
	  hint_mlpenalty=i.get_eq_double();
	else if(name=="hint_max_ntry"){
	  hint_max_ntry=i.get_eq_integer();
	  if(hint_max_ntry < 1)
	    hint_max_ntry=1;
	}
	else if(name=="hint_max_iter_hint")
	  hint_max_iter_hint=i.get_eq_integer();
	else if(name=="hint_debug")
	  hint_debug=i.get_eq_boolean();
	else
	  throw unknown_ident(&i, name);
      i.skip_line();//read away final newline
    }
  } catch (endoffile) { }

  if (outputenc == "LOCALE")
      outputenc = nl_langinfo(CODESET);
  
  check_config();
}

void configinfo::check_config()
{
  if(!(genmenu && startmenu && endmenu)) {
    /* don't translate genmenu, startmenu, endmenu and don't change \"\" */
    cerr << _("install-menu: At least one of genmenu, startmenu, endmenu\n"
        "is undefined. All of these have to be defined \n"
        "(although they may be equal to \"\").\n");
    throw informed_fatal();
  }
}

cat_str *configinfo::userprefix()
{
  if (userpref == 0)
      throw unknown_indirect_function(0, "userprefix");
  return userpref;
}

cat_str *configinfo::rootprefix()
{
  if (rootpref == 0)
      throw unknown_indirect_function(0, "rootprefix");
  return rootpref;
}

string configinfo::prefix()
{
  if (getuid()) {
    struct passwd *pw = getpwuid(getuid());
    return string(pw->pw_dir)+"/"+userprefix()->soutput(root_menu.vars);
  } else {
    return rootprefix()->soutput(root_menu.vars);
  }
}

ostream &configinfo::debuginfo(ostream &o)
{
    o<<String::compose(_("Using compatibility with %1.\n"),compt)
     <<"mainmenutitle   : "<<mainmt   << endl
     <<"rootsection     : "<<roots    << endl
     <<"rcfile          : "<<rcf      << endl
     <<"examplercfile   : "<<exrcf    << endl
     <<"root-prefix     : "<<rootpref << endl
     <<"user-prefix     : "<<userpref << endl
     <<"startmenu       : "<< endl;
    if(startmenu)
      startmenu->debuginfo(o);
    o<<"endmenu         : "<< endl;
    if(endmenu)
      endmenu->debuginfo(o);
    o<<"genmenu         : "<< endl;
    if(genmenu)
      genmenu->debuginfo(o);
    o<<"submenutitle    : "<< endl;
    if(submenutitle)
      submenutitle->debuginfo(o);
  o<<"mainmenutitle   : "<<mainmt   << endl
   <<"treewalk        : "<<treew    << endl;
  return o;
}

/////////////////////////////////////////////////////
//  Misc
//

bool testuniqueness(map<string, string> &menuentry)
{
  static set<string> uniquetitles;

  if(config->onlyuniquetitles){
    set <string>::const_iterator unique_i;
    string &title=menuentry[TITLE_VAR];
    if(!title.empty()) {
      unique_i=uniquetitles.find(title);
      if(unique_i!=uniquetitles.end())
	return false;
      else
	uniquetitles.insert(title);
    }
  }
  return true;
}

map<string, string> read_vars(parsestream &i)
{
  map<string, string> m;
  try {
    while(1)
    {
      string name = i.get_name();
      string val = i.get_eq_stringconst();
      
      if (do_translate_hack) {
	if (name == "title") {
	  // This won't work if ldgettext is also used (translate($lang)).
          string oldval = val;
	  val = dgettext("menu-sections",val.c_str());
	} else if (name == "section") {
	  vector<string> v;
	  break_slashes(val,v);
	  val.erase();

          for(vector<string>::const_iterator i = v.begin(); i != v.end(); ++i)
              val = val + '/' + dgettext("menu-sections", i->c_str());

          try {
              std::string newval = convert(val);
              val = newval;
          } catch (conversion_error& err) {
              std::cerr << err.message() << std::endl;
          }

	}
      }
      
      m[name] = val;
    } 
  }
  catch (endofline p) { }
  return m;
}

// This check is actually also done in update-menus, so it is sort of redundant
// here. But we will still have trouble if they don't exists, so still make
// the check here.
void check_vars(parsestream &pi, map<string, string> &m)
{
  vector<string> need;

  need.push_back(SECTION_VAR);
  need.push_back(TITLE_VAR);
  need.push_back(NEEDS_VAR);

  for(vector<string>::iterator i = need.begin(); i != need.end(); ++i)
  {
    map<string, string>::const_iterator j = m.find(*i);
    if ((j == m.end()) || j->second.empty())
        throw missing_tag(&pi, *i);
  }
}

void read_input(parsestream &i)
{
  try {
    while(1)
    {
      
      map<string, string> entry_vars = read_vars(i);

      // check presence of section, title, needs vars. Later we will blindly
      // assume they are defined.
      check_vars(i, entry_vars);

      vector<string> sections;
      break_slashes(entry_vars[SECTION_VAR], sections);
      if (entry_vars[TITLE_VAR] != "/")
          sections.push_back(entry_vars[TITLE_VAR]);	  
      if (supported->prec(entry_vars[NEEDS_VAR]) != INT_MAX)
          root_menu.add_entry(sections, entry_vars);
      i.skip_line();  // read away the final newline
    }
  }
  catch(endoffile p) { }
}


void includemenus(string replace, string menu_filename)
{
  // copy examplercfile to rcfile, replacing lines with "rep" with the
  // menu-file menu_filename
  bool changed = false;

  string input_filename = config->prefix() + '/' + config->examplercfile(); 

  ifstream input_file(input_filename.c_str());
  if (!input_file) {
    if (getuid()) {
      // Running as non-root:
      string input_filename2 = config->rootprefix()->soutput(root_menu.vars) + '/' + config->examplercfile();

      input_file.clear(); // to clear the bad state of the old stream
      input_file.open(input_filename2.c_str());
      if (!input_file) {
        cerr << String::compose(_("Cannot open file %1 (also tried %2).\n"), input_filename, input_filename2);
        throw informed_fatal();
      } else {
	input_filename = input_filename2;
      }
    }else 
      // Running as root:
    {
      cerr << String::compose(_("Cannot open file %1.\n"), input_filename);
      throw informed_fatal();
    }
  }

  ifstream menu_file(menu_filename.c_str());
  if (!menu_file) {
      cerr << String::compose(_("Cannot open file %1.\n"), menu_filename);
      throw informed_fatal();
  }

  string output_filename = config->prefix() + '/' + config->rcfile();
  ofstream output_file(output_filename.c_str());

  if (!output_file) {
    cerr << String::compose(_("Cannot open file %1.\n"), output_filename); 
    if (getuid())
        cerr << _("In order to be able to create the user config file(s) for the window manager,\n" 
            "the above file needs to be writeable (and/or the directory needs to exist).\n");
    throw informed_fatal();
  }

  std::string line;
  while (getline(input_file, line))
  {
      if (contains(line, replace, 0)) {

          std::string menu_line;
          while (getline(menu_file, menu_line))
              output_file << menu_line << endl;

          changed = true;

      } else {
          output_file << line << endl;
      }
  }

  if (!changed)
      cerr << String::compose(_("Warning: the string %1 did not occur in template file %2\n"), replace, input_filename);
}

void usage(ostream &c)
{
  /*Don't translate quoted string*/
  c <<  _("install-menu [-vh] <menu-method>\n"
	  "  Read menu entries from stdin in \"update-menus --stdout\" format\n" 
          "  and generate menu files using the specified menu-method.\n"
          "  Options to install-menu:\n"
	  "     -h --help    : this message\n"
	  "     -v --verbose : be verbose\n");
}

int main(int argc, char **argv)
{
  std::string script_name;
  parsestream *ps = 0, *psscript = 0;
  
  setlocale(LC_ALL, "");
  bindtextdomain(PACKAGE, LOCALEDIR);
  bind_textdomain_codeset("menu-sections", menuencoding);
  textdomain(PACKAGE);

  if (!getuid()) {
    // When we are root, we set umask to 022 to ignore the real root umask.
    // This is to ensure that the menu file can be read by users.
    umask(0022);
  }

  while(1)
  {
    int c = getopt_long (argc, argv, "vh", long_options, NULL);
    if(c == -1) 
        break;
    switch (c) {
      case '?': usage(cerr); return 1; break;
      case 'h': usage(cout); return 0; break;
      case 'v': verbose = 1; break;
    }
  }
    
  try {
    if (optind < argc) {
      script_name = argv[optind];
    } else {
      cerr << _("install-menu: no menu-method script specified !");
      throw informed_fatal();
    }

    add_functions();
    
    string directory;
    if (getuid())
        directory = MENUMETHODS;

    ps = new parsestream(script_name, directory);
    
    if (!ps->good()) {
      cerr << String::compose(_("Cannot open script %1 for reading.\n"), script_name);
      throw informed_fatal();
    }
    config = new configinfo(*ps);
    if (verbose) {
      config->debuginfo(cerr);
      supported->debuginfo(cerr);
    }
    if ((config->onlyrunasroot || config->userpref == 0) && getuid())
        return 0;
    if ((config->onlyrunasuser || config->rootpref == 0) && !getuid())
        return 0;
    if (config->prerun)
        system((config->prerun->soutput(root_menu.vars)).c_str());
    if (config->preruntest) {
      int retval = system((config->preruntest->soutput(root_menu.vars)).c_str());
      if (retval)
          return retval;
    }
    if (config->repeat_lang && (config->repeat_lang->soutput(root_menu.vars) == "LOCALE"))
        do_translate_hack=true;

    psscript = new parsestream(std::cin);
    root_menu.vars[TITLE_VAR] = config->mainmenutitle();

    read_input(*psscript);
    if (config->hint_optimize)
        root_menu.process_hints();

    root_menu.postprocess(1, 0, config->rootsection());
    root_menu.output();
    closegenoutput();

    if (config->also_run) {
      string run=config->also_run->soutput(root_menu.vars);
      vector<string> run_vec;
      vector<string>::const_iterator run_i;

      break_char(run, run_vec, ':');

      configinfo *old_config = config;  // save old pointer

      for (run_i = run_vec.begin(); run_i != run_vec.end(); ++run_i)
      {
        std::cout << String::compose(_("Running: \"%1\"\n"), *run_i);
        parsestream also_ps(*run_i);
        config = new configinfo(also_ps);
        root_menu.output();
        closegenoutput();
        delete config;
      }
      config = old_config;             // restore old pointer
    }
    if (!config->rcfile().empty() && !config->examplercfile().empty())
        includemenus("include-menu-defs",
            config->prefix()+"/"+config->genmenu->soutput(root_menu.vars));
    if (config->postrun)
        system((config->postrun->soutput(root_menu.vars)).c_str());
  } catch (genexcept& p) {
    p.report();
    cerr << String::compose(_("install-menu: %1: aborting\n"), script_name);
    return 1;
  }
}
