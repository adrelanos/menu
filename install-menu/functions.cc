/*
 * Debian menu system -- install-menu
 * install-menus/functions.cc
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

#include "functions.h"
#include "install-menu.h"
#include <stringtoolbox.h>
#include <iostream>
#include <cstdio>
#include <unistd.h>

using std::ostream;
using std::vector;
using std::string;
using std::map;
using std::cerr;

bool empty_string(const string &s)
{
  if (s.empty() || s == "none")
    return true;
  else
    return false;
}

namespace functions {

ostream &prefix::output(ostream &o, vector<cat_str *> &,
    map<string, string> &)
{
  return o << menumethod->prefix();
}

ostream &shell::output(ostream &o, vector<cat_str *> &args,
    map<string, string> &menuentry)
{
  string command = args[0]->soutput(menuentry);
  FILE *status = popen(command.c_str(), "r");

  if (!status)
      throw exceptions::pipeerror_read(command);

  while (!feof(status))
  {
    char tmp[MAX_LINE];
    if (fgets(tmp, MAX_LINE, status) != NULL)
        o << tmp;
  }
  pclose(status);
  return o;
}

ostream &ifroot::output(ostream &o, vector<cat_str *> & args,
    map<string, string> &menuentry)
{
  if(!is_root)
    args[1]->output(o,menuentry);
  else
    args[0]->output(o,menuentry);

  return o;
}

ostream &print::output(ostream &o, vector<cat_str *> &args,
    map<string, string> &menuentry)
{
  string s=args[0]->soutput(menuentry);

  if (empty_string(s)) {
    cerr << _("Zero-size argument to print function.");
    throw exceptions::informed_fatal();
  }
  return o << s;
}

ostream &ifempty::output(ostream &o, vector<cat_str *> &args,
    map<string, string> &menuentry)
{
  string s=args[0]->soutput(menuentry);

  if (empty_string(s))
    args[1]->output(o,menuentry);
  return o;
}

ostream &ifnempty::output(ostream &o, vector<cat_str *> &args,
    map<string, string> &menuentry)
{
  string s=args[0]->soutput(menuentry);
  if (!empty_string(s))
    args[1]->output(o,menuentry);
  return o;
}

ostream &iffile::output(ostream &o, vector<cat_str *> &args,
    map<string, string> &menuentry)
{
  string s=args[0]->soutput(menuentry);
  std::ifstream f(s.c_str());
  if(f)
    args[1]->output(o,menuentry);
  return o;
}

ostream &ifelsefile::output(ostream &o, vector<cat_str *> &args,
    map<string, string> &menuentry)
{
  string s=args[0]->soutput(menuentry);
  std::ifstream f(s.c_str());
  if(f)
    args[1]->output(o,menuentry);
  else
    args[2]->output(o,menuentry);
  return o;
}

ostream &catfile::output(ostream &o, vector<cat_str *> &args,
    map<string, string> &menuentry)
{
  string s=args[0]->soutput(menuentry);
  std::ifstream f(s.c_str());
  char c;

  while(f && f.get(c))
    o<<c;
  return o;
}
ostream &forall::output(ostream &o, vector<cat_str *> &args,
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

ostream &esc::output(ostream &o, vector<cat_str *> &args,
    map<string, string> &menuentry)
{
  return o << escape(args[0]->soutput(menuentry),
                     args[1]->soutput(menuentry));
}

ostream &escwith::output(ostream &o, vector<cat_str *> &args,
    map<string, string> &menuentry)
{
  return o << escapewith(args[0]->soutput(menuentry),
                         args[1]->soutput(menuentry),
                         args[2]->soutput(menuentry));
}

ostream &escfirst::output(ostream &o, vector<cat_str *> &args,
    map<string, string> &menuentry)
{
  string s=args[0]->soutput(menuentry);
  string esc=args[1]->soutput(menuentry);
  string t;

  for(string::size_type i=0; i != s.length(); ++i){
    if(!esc.empty() && contains(esc, s[i])) {
      t=s.substr(0,i);
      t+=args[2]->soutput(menuentry);
      t+=s.substr(i);
      break;
    }
    t+=s[i];
  }
  return o<<t;
}
ostream &tolower::output(ostream &o, vector<cat_str *> &args,
    map<string, string> &menuentry)
{
  return o<<lowercase(args[0]->soutput(menuentry));
}

ostream &toupper::output(ostream &o, vector<cat_str *> &args,
    map<string, string> &menuentry)
{
  return o << uppercase(args[0]->soutput(menuentry));
}
ostream &replacewith::output(ostream &o, vector<cat_str *> &args,
    map<string, string> &menuentry)
{
  return o << ::replacewith(args[0]->soutput(menuentry),
                            args[1]->soutput(menuentry),
                            args[2]->soutput(menuentry));
}

ostream &replace::output(ostream &o, vector<cat_str *> &args,
    map<string, string> &menuentry)
{
    return o << ::replace(args[0]->soutput(menuentry),
                          args[1]->soutput(menuentry),
                          args[2]->soutput(menuentry));
}

ostream &nstring::output(ostream &o, vector<cat_str *> &args,
    map<string, string> &menuentry)
{
  int count= stringtoi(args[0]->soutput(menuentry));
  int i;

  for(i=0; i<count; i++)
    o<<args[1]->soutput(menuentry);

  return o;
}
ostream &cppesc::output(ostream &o, vector<cat_str *> &args,
    map<string, string> &menuentry)
{
  return o << ::cppesc(args[0]->soutput(menuentry));
}

ostream &add::output(ostream &o, vector<cat_str *> &args,
    map<string, string> &menuentry)
{
  int x=stringtoi(args[0]->soutput(menuentry));
  int y=stringtoi(args[1]->soutput(menuentry));

  return o<<itostring(x+y);
}

ostream &sub::output(ostream &o, vector<cat_str *> &args,
    map<string, string> &menuentry)
{
  int x=stringtoi(args[0]->soutput(menuentry));
  int y=stringtoi(args[1]->soutput(menuentry));

  return o<<itostring(x-y);
}

ostream &mult::output(ostream &o, vector<cat_str *> &args,
    map<string, string> &menuentry)
{
  int x=stringtoi(args[0]->soutput(menuentry));
  int y=stringtoi(args[1]->soutput(menuentry));

  return o<<itostring(x*y);
}

ostream &div::output(ostream &o, vector<cat_str *> &args,
    map<string, string> &menuentry)
{
  int x=stringtoi(args[0]->soutput(menuentry));
  int y=stringtoi(args[1]->soutput(menuentry));
  
  if(y)
    return o<<itostring(x/y);
  else
    return o<<"0";
}

ostream &ifelse::output(ostream &o, vector<cat_str *> &args,
    map<string, string> &menuentry)
{
  string s=args[0]->soutput(menuentry);

  (!empty_string(s)) ?
    args[1]->output(o,menuentry)
    :
    args[2]->output(o,menuentry);
  return o;
}

ostream &ifeq::output(ostream &o, vector<cat_str *> &args,
    map<string, string> &menuentry)
{
  string s=args[0]->soutput(menuentry);
  string t=args[1]->soutput(menuentry);
  if(s==t)
    args[2]->output(o,menuentry);
  
  return o;
}
ostream &ifneq::output(ostream &o, vector<cat_str *> &args,
    map<string, string> &menuentry)
{
  string s=args[0]->soutput(menuentry);
  string t=args[1]->soutput(menuentry);
  if(!(s==t))
    args[2]->output(o,menuentry);
  
  return o;
}
ostream &ifeqelse::output(ostream &o, vector<cat_str *> &args,
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

ostream &cond_surr::output(ostream &o, vector<cat_str *> &args,
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

ostream &parent::output(ostream &o, vector<cat_str *> &args,
    map<string, string> &menuentry)
{
  string s=args[0]->soutput(menuentry);

  return o<<string_parent(s);
}

ostream &basename::output(ostream &o, vector<cat_str *> &args,
    map<string, string> &menuentry)
{
  string s=args[0]->soutput(menuentry);
  
  return o<<string_basename(s);
}

ostream &stripdir::output(ostream &o, vector<cat_str *> &args,
    map<string, string> &menuentry)
{
  string s=args[0]->soutput(menuentry);
  
  return o<<string_stripdir(s);
}

ostream &entrycount::output(ostream &o, vector<cat_str *> &/*args*/,
    map<string, string> &menuentry)
{
  return o<<menuentry[PRIVATE_ENTRYCOUNT_VAR];
}

ostream &entryindex::output(ostream &o, vector<cat_str *> &/*args*/,
    map<string, string> &menuentry)
{
  return o<<menuentry[PRIVATE_ENTRYINDEX_VAR];
}

ostream &firstentry::output(ostream &o, vector<cat_str *> &args,
    map<string, string> &menuentry)
{
  int index=stringtoi(menuentry[PRIVATE_ENTRYINDEX_VAR]);

  if(index == 0)
    args[0]->output(o,menuentry);
  return o;
}

ostream &lastentry::output(ostream &o, vector<cat_str *> &args,
    map<string, string> &menuentry)
{
  int index=stringtoi(menuentry[PRIVATE_ENTRYINDEX_VAR]);
  int count=stringtoi(menuentry[PRIVATE_ENTRYCOUNT_VAR]);

  if(index+1 == count)
    args[0]->output(o,menuentry);
  return o;
}

ostream &level::output(ostream &o, vector<cat_str *> &/*args*/,
    map<string, string> &menuentry)
{
  return o<<menuentry[PRIVATE_LEVEL_VAR];
}

ostream &rcfile::output(ostream &o, vector<cat_str *> &/*args*/,
    map<string, string> &menuentry)
{
  return o<<menumethod->rcfile();
}
ostream &examplercfile::output(ostream &o, vector<cat_str *> &/*args*/,
    map<string, string> &menuentry)
{
  return o<<menumethod->examplercfile();
}
ostream &mainmenutitle::output(ostream &o, vector<cat_str *> &/*args*/,
    map<string, string> &menuentry)
{
  return o<<menumethod->mainmenutitle();
}
ostream &rootsection::output(ostream &o, vector<cat_str *> &/*args*/,
    map<string, string> &menuentry)
{
  return o<<menumethod->rootsection();
}

ostream &rootprefix::output(ostream &o, vector<cat_str *> &/*args*/,
    map<string, string> &menuentry)
{
  return o<<menumethod->rootprefix()->soutput(menuentry);
}

ostream &userprefix::output(ostream &o, vector<cat_str *> &/*args*/,
    map<string, string> &menuentry)
{
  return o<<menumethod->userprefix()->soutput(menuentry);
}

ostream &treewalk::output(ostream &o, vector<cat_str *> &/*args*/,
    map<string, string> &menuentry)
{
  return o<<menumethod->treewalk();
}
ostream &postoutput::output(ostream &o, vector<cat_str *> &/*args*/,
    map<string, string> &menuentry)
{
  return o<<menumethod->postoutput();
}
ostream &preoutput::output(ostream &o, vector<cat_str *> &/*args*/,
    map<string, string> &menuentry)
{
  return o<<menumethod->preoutput();
}
ostream &cwd::output(ostream &o, vector<cat_str *> &/*args*/,
    map<string, string> &menuentry)
{
  char buf[300];
  //Bug: getcwd returns NULL if strlen(cwd) > sizeof(buf), so we get
  //     a segfault here.
  return o<<string(getcwd(buf,sizeof(buf)));
}

ostream &translate::output(ostream &o, vector<cat_str *> &args,
    map<string, string> &menuentry)
{
  string lang=args[0]->soutput(menuentry);
  string text=args[1]->soutput(menuentry);

  return o << ldgettext(lang.c_str(), "menu-sections", text.c_str());
}

}
