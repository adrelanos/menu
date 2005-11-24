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
#include <cstdio>
#include <unistd.h>
#include <sys/stat.h>
#include <errno.h>
#include <pwd.h>
#include "config.h"
#include <iconv.h>
#include <langinfo.h>

#include "install-menu.h"
#include "menu-tree.h"
#include <stringtoolbox.h>

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
using namespace exceptions;

int verbose = 0;
const char * menuencoding = "UTF-8";
bool is_root;
const char *pw_dir;

map<string, functions::func *> func_data;

menuentry root_menu;
methodinfo *menumethod;
supportedinfo *supported;

std::ofstream *genoutputfile = 0;
set<string> outputnames; // all names of files ever written to

struct option long_options[] = { 
  { "verbose", no_argument, NULL, 'v' }, 
  { "help", no_argument, NULL, 'h' }, 
  { "remove", no_argument, NULL, 'r' }, 
  { NULL, 0, NULL, 0 } };

// Should we do translations? This is set to 'true' when both outputencoding
// and outputlanguage is set.
bool do_translation = false;

void store_func(functions::func *f)
{
  func_data[f->getName()] = f;
}

/** This function is used to convert a string from one encoding to another.
 *
 * The function could probably be prettier, but it works. One of the things
 * I dislike about it is that the cleanup operations are duplicated three
 * times (e.g. delete[]).
 */
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

  iconv_t conversion = iconv_open(menumethod->outputencoding().c_str(), menuencoding);

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
  store_func(new functions::prefix);
  store_func(new functions::shell);
  store_func(new functions::ifroot);

  store_func(new functions::print);
  store_func(new functions::add);
  store_func(new functions::sub);
  store_func(new functions::mult);
  store_func(new functions::div);
  store_func(new functions::ifempty);
  store_func(new functions::ifnempty);
  store_func(new functions::iffile);
  store_func(new functions::ifelsefile);
  store_func(new functions::ifelse);
  store_func(new functions::catfile);
  store_func(new functions::forall);

  store_func(new functions::ifeq);
  store_func(new functions::ifneq);
  store_func(new functions::ifeqelse);

  store_func(new functions::cond_surr);
  store_func(new functions::esc);
  store_func(new functions::escwith);
  store_func(new functions::escfirst);
  store_func(new functions::tolower);  
  store_func(new functions::toupper);
  store_func(new functions::replace);
  store_func(new functions::replacewith);
  store_func(new functions::nstring);  
  store_func(new functions::cppesc);
  store_func(new functions::parent);
  store_func(new functions::basename);
  store_func(new functions::stripdir);
  store_func(new functions::entrycount);
  store_func(new functions::entryindex);
  store_func(new functions::firstentry);
  store_func(new functions::lastentry);
  store_func(new functions::level);

  store_func(new functions::rcfile);
  store_func(new functions::examplercfile);
  store_func(new functions::mainmenutitle);
  store_func(new functions::rootsection);
  store_func(new functions::rootprefix);
  store_func(new functions::userprefix);
  store_func(new functions::treewalk);
  store_func(new functions::postoutput);
  store_func(new functions::preoutput);
  store_func(new functions::cwd);
  store_func(new functions::translate);
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
          throw except_string(String::compose(_("Could not create directory(%1): %2"), t, strerror(errno)));
      if (chdir(t.c_str()))
          throw except_string(String::compose(_("Could not change directory(%1): %2"), t, strerror(errno)));
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
    f << menumethod->postoutput();
  }
}

void genoutput(const string &s, map<string, string> &v)
{
  static string lastname = "////";

  string name = menumethod->prefix() + '/' + menumethod->genmenu->soutput(v);
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
      (*genoutputfile) << menumethod->preoutput();
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
  map<string, functions::func *>::const_iterator j = func_data.find(name);
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

bool cat_str::is_constant_string() 
{
  for(int i=0; i<v.size(); i++)
  if (!dynamic_cast<const_str*>(v[i]))
    return false;
  return true;
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
  int prec = 0;
  while(1)
  {
    try {
      i.skip_space();
      string name = i.get_name();
      if (name == "endsupported")
          return;

      name = uppercase(name);

      if (verbose)
          /*Do not translate supported*/
          cerr << String::compose(_("install-menu: [supported]: name=%1\n"), name);

      supinfo info;
      info.name = get_eq_cat_str(i);
      info.prec = prec++;
      supported[name] = info;
      i.skip_line();   //read away the final newline
    }
    catch (endofline d) { }
  }
}

void supportedinfo::subst(map<string, string> vars)
{
  map<string, string>::const_iterator i = vars.find(NEEDS_VAR);

  if (i == vars.end()) {
    cerr << String::compose(_("Menu entry lacks mandatory field \"%1\".\n"), NEEDS_VAR);
    throw informed_fatal();
  }

  map<string, supinfo>::const_iterator j = supported.find(uppercase(i->second));
  if (j == supported.end()) {
    cerr << String::compose(_("Unknown value for field %1=\"%2\".\n"), NEEDS_VAR, i->second);
    throw informed_fatal();
  }
  genoutput(j->second.name->soutput(vars), vars);
}

bool supportedinfo::supports(string& name)
{
  map<string, supinfo>::const_iterator i = supported.find(uppercase(name));
  if (i == supported.end())
      return false;
  else
      return true;
}

/////////////////////////////////////////////////////
//  forcetree stuff
//

void read_forcetree(parsestream &i)
{
  Regex r("[] /0123456789:;<=>?@ABCDEFGHIJKLMNOPQRSTUVWXYZ[\\^_abcdefghijklmnopqrstuvwxyz]");

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
      break_char(name, sections, '/');
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
//  methodinfo
//
methodinfo::methodinfo(parsestream &i) 
    : roots("/Debian"), mainmt("Debian"), treew("c(m)"),
    onlyrunasroot(false), onlyrunasuser(false),
    onlyuniquetitles(false), hint_optimize(false), hint_nentry(6),
    hint_topnentry(5), hint_mixedpenalty(15), hint_minhintfreq(0.1),
    hint_mlpenalty(2000), hint_max_ntry(4), hint_max_iter_hint(5),
    hint_debug(false), hotkeycase(0)
{
  userpref=rootpref=sort=remove=prerun=preruntest=postrun=genmenu=
    hkexclude=startmenu=endmenu=submenutitle=also_run=outputlanguage=0;

  /*Should not be translated as this appear in the output file that can use 
   *a different encoding*/
  preout = "# Automatically generated file. Do not edit (see /usr/share/doc/menu/html/index.html)\n\n";

  try {
      while(1)
      {
        string name = i.get_name();
	if (name=="supported") {
	  i.skip_line();
	  supported = new supportedinfo(i);
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
	else if(name=="removemenu")
	  remove=get_eq_cat_str(i);	
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
	else if(name=="outputlanguage" 
            //We accept the deprecated name repeat_lang for the time being
            || name=="repeat_lang")
	  outputlanguage=get_eq_cat_str(i);
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
          cerr << String::compose(_("install-menu: Warning: Unknown identifier `%1' on line %2 in file %3. Ignoring.\n"), name, i.linenumber(), i.filename());

      i.skip_line(); //read away final newline
    }
  } catch (endoffile) { }

  if (outputenc == "LOCALE")
      outputenc = nl_langinfo(CODESET);
  
  if (!genmenu) {
    cerr << String::compose(_("install-menu: %1 must be defined in menu-method %2"), "genmenu", i.filename());
    throw informed_fatal();
  }
}

cat_str *methodinfo::userprefix()
{
  if (userpref == 0)
      throw unknown_indirect_function(0, "userprefix");
  return userpref;
}

cat_str *methodinfo::rootprefix()
{
  if (rootpref == 0)
      throw unknown_indirect_function(0, "rootprefix");
  return rootpref;
}

string methodinfo::prefix()
{
  if (!is_root) {
    string up = userprefix()->soutput(root_menu.vars);
    // When userprefix is prefixed by // instead of just one /, treat the
    // URL as absolute instead of relative.
    if (up.length() > 1 && up.substr(0, 2) == "//") {
      return up.substr(1);
    } else {
      return string(pw_dir)+"/"+up;
    }
  } else {
    return rootprefix()->soutput(root_menu.vars);
  }
}

/////////////////////////////////////////////////////
//  Misc
//

bool testuniqueness(map<string, string> &menuentry)
{
  static set<string> uniquetitles;

  if(menumethod->onlyuniquetitles){
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
      
      if (do_translation) {
	if (name == "title") {
	  // This won't work if ldgettext is also used (translate($lang)).
	  string title = dgettext("menu-sections", val.c_str());
          try {
            title = convert(title);
          } catch (conversion_error& err) {
            // Conversion failed, use the original, untranslated string.
            title = val;
          }
          val = title;
	} else if (name == "section") {
	  vector<string> names;
          vector<string>::const_iterator i;
	  break_char(val, names, '/');
          if (!names.empty())
              val.erase();

          for(i = names.begin(); i != names.end(); ++i)
          {
            string sectionname = dgettext("menu-sections", i->c_str());
            try {
              sectionname = convert(sectionname);
            } catch (conversion_error& err) {
              // Conversion failed, use the original, untranslated string.
              sectionname = i->c_str();
            }
            val = val + '/' + sectionname;
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
        throw missing_tag(pi.filename(), *i);
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
      break_char(entry_vars[SECTION_VAR], sections, '/');
      if (entry_vars[TITLE_VAR] != "/")
          sections.push_back(entry_vars[TITLE_VAR]);	  
      if (supported->supports(entry_vars[NEEDS_VAR]))
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

  string input_filename = menumethod->prefix() + '/' + menumethod->examplercfile(); 

  ifstream input_file(input_filename.c_str());
  if (!input_file) {
    if (!is_root) {
      // Running as non-root:
      string input_filename2 = menumethod->rootprefix()->soutput(root_menu.vars) + '/' + menumethod->examplercfile();

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

  string output_filename = menumethod->prefix() + '/' + menumethod->rcfile();
  ofstream output_file(output_filename.c_str());

  if (!output_file) {
    cerr << String::compose(_("Cannot open file %1.\n"), output_filename); 
    if (!is_root)
        cerr << _("In order to be able to create the user config file(s) for the window manager,\n" 
            "the above file needs to be writeable (and/or the directory needs to exist).\n");
    throw informed_fatal();
  }

  std::string line;
  while (getline(input_file, line))
  {
      if (contains(line, replace)) {

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

const char *ldgettext(const char *lang, const char *domain, const char *msgid)
{
  /* this code comes from the gettext info page. It looks
     very inefficient (as though for every language change a new
     catalog file is opened), but tests show adding a language
     change like this doesn't get performance down very much
     (runtime goes `only' about 70% up, if switching between 2 
     languages, as compared to not switching at all).
  */
  /* Change language.  */
  setenv ("LANGUAGE", lang, 1);
  /* Make change known.  */
  {
    extern int  _nl_msg_cat_cntr;
    ++_nl_msg_cat_cntr;
  }
  return dgettext(domain, msgid);
}

void usage(ostream &c)
{
  /*Don't translate quoted string*/
  c <<  _("install-menu [-vh] <menu-method>\n"
	  "  Read menu entries from stdin in \"update-menus --stdout\" format\n" 
          "  and generate menu files using the specified menu-method.\n"
          "  Options to install-menu:\n"
	  "     -h --help    : this message\n"
	  "        --remove  : remove the menu instead of generating it.\n"
	  "     -v --verbose : be verbose\n");
}

int main(int argc, char **argv)
{
  int remove = 0;
  uid_t uid = getuid();
  struct passwd *pw = getpwuid(uid);
  is_root = (uid == 0);
  pw_dir = pw->pw_dir;
  std::string script_name;
  parsestream *ps = 0, *psscript = 0;
  
  setlocale(LC_ALL, "");
  bindtextdomain(PACKAGE, LOCALEDIR);
  bind_textdomain_codeset("menu-sections", menuencoding);
  textdomain(PACKAGE);

  if (is_root) {
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
      case 'r': remove = 1; break;
    }
  }
    
  try {
    if (optind < argc) {
      script_name = argv[optind];
    } else {
      cerr << _("install-menu: no menu-method script specified!");
      throw informed_fatal();
    }

    add_functions();
    
    string directory;
    if (!is_root)
        directory = MENUMETHODS;

    ps = new parsestream(script_name, directory);
    
    if (!ps->good()) {
      cerr << String::compose(_("Cannot open script %1 for reading.\n"), script_name);
      throw informed_fatal();
    }
    menumethod = new methodinfo(*ps);
    if ((menumethod->onlyrunasroot || menumethod->userpref == 0) && !is_root)
        return 0;
    if ((menumethod->onlyrunasuser || menumethod->rootpref == 0) && is_root)
        return 0;
    if (remove)
    {
      if (menumethod->remove)
        system(menumethod->remove->soutput(root_menu.vars).c_str());
      else if (menumethod->genmenu 
            && menumethod->genmenu->is_constant_string())
      {
        unlink((menumethod->prefix()+"/"
               +menumethod->genmenu->soutput(root_menu.vars)).c_str());
        if (!menumethod->rcfile().empty())
          unlink((menumethod->prefix()+"/"
                 +menumethod->rcfile()).c_str());
        rmdir(menumethod->prefix().c_str());
      }
      else
        cerr << String::compose(_("Warning: script %1 does not provide removemenu, menu not deleted\n"), script_name);
      return 0;
    }
    if (menumethod->prerun)
      system((menumethod->prerun->soutput(root_menu.vars)).c_str());
    if (menumethod->preruntest) {
      int retval = system((menumethod->preruntest->soutput(root_menu.vars)).c_str());
      if (retval)
        return retval;
    }
    if (menumethod->outputlanguage && (menumethod->outputlanguage->soutput(root_menu.vars) == "LOCALE") && !menumethod->outputencoding().empty())
      do_translation = true;

    psscript = new parsestream(std::cin);
    root_menu.vars[TITLE_VAR] = menumethod->mainmenutitle();

    read_input(*psscript);
    if (menumethod->hint_optimize)
        root_menu.process_hints();

    root_menu.postprocess(1, 0, menumethod->rootsection());
    root_menu.output();
    closegenoutput();

    if (menumethod->also_run) {
      string run = menumethod->also_run->soutput(root_menu.vars);
      vector<string> run_vec;
      vector<string>::const_iterator run_i;

      break_char(run, run_vec, ':');

      methodinfo *old_menumethod = menumethod;  // save old pointer

      for (run_i = run_vec.begin(); run_i != run_vec.end(); ++run_i)
      {
        std::cout << String::compose(_("Running: \"%1\"\n"), *run_i);
        parsestream also_ps(*run_i);
        menumethod = new methodinfo(also_ps);
        root_menu.output();
        closegenoutput();
        delete menumethod;
      }
      menumethod = old_menumethod;             // restore old pointer
    }
    if (!menumethod->rcfile().empty() && !menumethod->examplercfile().empty())
        includemenus("include-menu-defs",
            menumethod->prefix()+"/"+menumethod->genmenu->soutput(root_menu.vars));
    if (menumethod->postrun)
        system((menumethod->postrun->soutput(root_menu.vars)).c_str());
  } catch (genexcept& p) {
    p.report();
    cerr << String::compose(_("install-menu: %1: aborting\n"), script_name);
    return 1;
  }
}
