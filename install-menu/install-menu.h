/*
 * Debian menu system -- update-menus
 * install-menu/install-menu.h
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

/*
  
Grammar of "string"'s:
  cat    = value | (value cat)
  value  = const | func | var
  const  = "\"" .* "\""
  var    = "$"name
  func   = name "(" args ")"
  args   = cat | (args "," cat)
  name   = [a-z,A-Z,_][a-z,A-Z,_,0-9]*

Structure of container classes
  str         general container
    const     contains constant string
    cat       std::vector of str's
    var       contains a named variable (like section, title, needs, ...).
    func      a function (print, ifelse, iffile, ..). 
      funcN is a templated type, where:
        funcN<1> is with 1 arguments
        funcN<2> is with 2 arguments
        funcN<3> is with 3 arguments
        etc...

Example "str":
  "title=" print($title) ", mess:" \
   ifelse($longtitle, print(title), $longtitle)
*/

#ifndef INSTALL_MENU_H
#define INSTALL_MENU_H

#include <iostream>
#include <vector>
#include <map>
#include "common.h"
#include "adstring.h"
#include "compose.hpp"

// container classes:

class str {
public:
  virtual std::ostream &output(std::ostream &o, std::map<std::string, std::string> &menuentry) = 0;
  virtual std::ostream &debuginfo(std::ostream &) = 0;
  virtual ~str() { };
};

class const_str : public str {
  std::string data;
public:
  const_str(parsestream &i) : data(i.get_stringconst()) { }
  virtual std::ostream &output(std::ostream &o, std::map<std::string, std::string> &menuentry);
  std::ostream &debuginfo(std::ostream &o);
};

class cat_str : public str {
  std::vector<str *> v;
public:
  cat_str(parsestream &);
  std::ostream &output(std::ostream &o, std::map<std::string, std::string> &menuentry);
  void output(std::map<std::string, std::string> &menuentry);
  std::string soutput(std::map<std::string, std::string> &menuentry);
  std::ostream &debuginfo(std::ostream &o);
};

class var_str : public str {
  std::string var_name;
public:
  var_str(parsestream &i);
  std::ostream &output(std::ostream &o, std::map<std::string, std::string> &menuentry);
  std::ostream &debuginfo(std::ostream &o);
};

class func;

class func_str : public str {
  std::vector<cat_str *> args;
  func *f;
public:
  func_str(parsestream &);
  std::ostream &output(std::ostream &o, std::map<std::string, std::string> &menuentry);
  std::ostream &debuginfo(std::ostream &o);
};

class supportedinfo {
  class supinf {
  public:
    cat_str *c;
    int prec;
  };
  std::map<std::string, supinf> sup;
public:
  supportedinfo(parsestream &);
  int prec(std::string &);
  void subst(std::map<std::string, std::string> );
  std::ostream &debuginfo(std::ostream &);
};

class configinfo {
  std::string compt, rcf, exrcf, roots, mainmt, treew, outputenc;
  std::string preout, postout;

  void check_config();
public:
  configinfo(parsestream &);

  bool keep_sections;
  cat_str *startmenu, *endmenu, *submenutitle, *hkexclude,
    *genmenu, *postrun, *prerun, *sort, *rootpref, *userpref,
    *repeat_lang, *also_run, *preruntest;

  bool onlyrunasroot, onlyrunasuser;
  bool onlyuniquetitles;

  // hint stuff:
  bool   hint_optimize;
  double hint_nentry;
  double hint_topnentry;
  double hint_mixedpenalty;
  double hint_minhintfreq;
  double hint_mlpenalty;
  int    hint_max_ntry;
  int    hint_max_iter_hint;
  bool   hint_debug;

  int hotkeycase; //0=insensitive, 1=sensitive
  std::ostream &debuginfo(std::ostream &);
  
  const std::string &rcfile() const { return rcf; }
  const std::string &examplercfile() const { return exrcf; }
  const std::string &mainmenutitle() const { return mainmt; }
  const std::string &rootsection() const { return roots; }
  const std::string &treewalk() const { return treew; }
  const std::string &preoutput() const { return preout; }
  const std::string &postoutput() const { return postout; }
  const std::string &outputencoding() const { return outputenc; }
  cat_str *rootprefix();
  cat_str *userprefix();

  std::string prefix();
};

extern configinfo *config;
extern supportedinfo *supported;
extern bool testuniqueness(std::map<std::string, std::string> &menuentry);

/////////////////////////////////////////////////////
//  prototypes of install-menu functions:
//

class func {
public: 
  virtual unsigned int nargs() const = 0;
  virtual std::ostream &output(std::ostream &, std::vector<cat_str *> &,
                                  std::map<std::string, std::string> &) = 0;
  virtual const char * name() const = 0;
};

template<unsigned int N>
struct funcN : public func {
  unsigned int nargs() const { return N; }
};

class func_def : public func {
  cat_str *f;
  std::string func_name;
  std::vector<std::string> args_name;
public:
  func_def(parsestream &i);
  unsigned int nargs() const { return args_name.size(); }
  std::ostream &output(std::ostream &, std::vector<cat_str *> &,std::map<std::string, std::string> &);
  const char * name() const { return func_name.c_str(); }
  virtual ~func_def() { }
};

/////////////////////////////////////////////////////
//  Function that can be used in install-menu (declarations)
//

struct prefix_func : public funcN<0> {
  std::ostream &output(std::ostream &o, std::vector<cat_str *> &, std::map<std::string, std::string> &);
  const char * name() const { return "prefix"; }
};
struct ifroot_func: public funcN<2> {
  std::ostream &output(std::ostream &o, std::vector<cat_str *> &, std::map<std::string, std::string> &);
  const char * name() const { return "ifroot"; }
};
struct print_func : public funcN<1> {
  std::ostream &output(std::ostream &o, std::vector<cat_str *> &, std::map<std::string, std::string> &);
  const char * name() const { return "print"; }
};
struct ifempty_func : public funcN<2> {
  std::ostream &output(std::ostream &o, std::vector<cat_str *> &, std::map<std::string, std::string> &);
  const char * name() const { return "ifempty"; }
};
struct ifnempty_func : public funcN<2> {
  std::ostream &output(std::ostream &o, std::vector<cat_str *> &, std::map<std::string, std::string> &);
  const char * name() const { return "ifnempty"; }
};
struct iffile_func : public funcN<2> {
  std::ostream &output(std::ostream &o, std::vector<cat_str *> &, std::map<std::string, std::string> &);
  const char * name() const { return "iffile"; }
};
struct ifelsefile_func : public funcN<3> {
  std::ostream &output(std::ostream &o, std::vector<cat_str *> &, std::map<std::string, std::string> &);
  const char * name() const { return "ifelsefile"; }
};
struct catfile_func : public funcN<1> {
  std::ostream &output(std::ostream &o, std::vector<cat_str *> &, std::map<std::string, std::string> &);
  const char * name() const { return "catfile"; }
};
struct forall_func : public funcN<3> {
  std::ostream &output(std::ostream &o, std::vector<cat_str *> &, std::map<std::string, std::string> &);
  const char * name() const { return "forall"; }
};
struct esc_func : public funcN<2> {
  std::ostream &output(std::ostream &o, std::vector<cat_str *> &, std::map<std::string, std::string> &);
  const char * name() const { return "esc"; }
};
struct add_func : public funcN<2> {
  std::ostream &output(std::ostream &o, std::vector<cat_str *> &, std::map<std::string, std::string> &);
  const char * name() const { return "add"; }
};
struct sub_func : public funcN<2> {
  std::ostream &output(std::ostream &o, std::vector<cat_str *> &, std::map<std::string, std::string> &);
  const char * name() const { return "sub"; }
};
struct mult_func : public funcN<2> {
  std::ostream &output(std::ostream &o, std::vector<cat_str *> &, std::map<std::string, std::string> &);
  const char * name() const { return "mult"; }
};
struct div_func : public funcN<2> {
  std::ostream &output(std::ostream &o, std::vector<cat_str *> &, std::map<std::string, std::string> &);
  const char * name() const { return "div"; }
};
struct ifelse_func: public funcN<3> {
  std::ostream &output(std::ostream &o, std::vector<cat_str *> &, std::map<std::string, std::string> &);
  const char * name() const { return "ifelse"; }
};
struct ifeq_func: public funcN<3> {
  std::ostream &output(std::ostream &o, std::vector<cat_str *> &, std::map<std::string, std::string> &);
  const char * name() const { return "ifeq"; }
};
struct ifneq_func: public funcN<3> {
  std::ostream &output(std::ostream &o, std::vector<cat_str *> &, std::map<std::string, std::string> &);
  const char * name() const { return "ifneq"; }
};
struct ifeqelse_func: public funcN<4> {
  std::ostream &output(std::ostream &o, std::vector<cat_str *> &, std::map<std::string, std::string> &);
  const char * name() const { return "ifeqelse"; }
};
struct cond_surr_func: public funcN<3> {
  std::ostream &output(std::ostream &o, std::vector<cat_str *> &, std::map<std::string, std::string> &);
  const char * name() const { return "cond_surr"; }
};
struct escwith_func : public funcN<3> {
  std::ostream &output(std::ostream &o, std::vector<cat_str *> &, std::map<std::string, std::string> &);
  const char * name() const { return "escwith"; }
};
struct escfirst_func : public funcN<3> {
  std::ostream &output(std::ostream &o, std::vector<cat_str *> &, std::map<std::string, std::string> &);
  const char * name() const { return "escfirst"; }
};
struct tolower_func : public funcN<1> {
  std::ostream &output(std::ostream &o, std::vector<cat_str *> &, std::map<std::string, std::string> &);
  const char * name() const { return "tolower"; }
};
struct toupper_func : public funcN<1> {
  std::ostream &output(std::ostream &o, std::vector<cat_str *> &, std::map<std::string, std::string> &);
  const char * name() const { return "toupper"; }
};
struct replace_func : public funcN<3> {
  std::ostream &output(std::ostream &o, std::vector<cat_str *> &, std::map<std::string, std::string> &);
  const char * name() const { return "replace"; }
};
struct replacewith_func : public funcN<3> {
  std::ostream &output(std::ostream &o, std::vector<cat_str *> &, std::map<std::string, std::string> &);
  const char * name() const { return "replacewith"; }
};
struct nstring_func : public funcN<2> {
  std::ostream &output(std::ostream &o, std::vector<cat_str *> &, std::map<std::string, std::string> &);
  const char * name() const { return "nstring"; }
};
struct cppesc_func : public funcN<1> {
  std::ostream &output(std::ostream &o, std::vector<cat_str *> &, std::map<std::string, std::string> &);
  const char * name() const { return "cppesc"; }
};
struct parent_func : public funcN<1> {
  std::ostream &output(std::ostream &o, std::vector<cat_str *> &, std::map<std::string, std::string> &);
  const char * name() const { return "parent"; }
};
struct basename_func : public funcN<1> {
  std::ostream &output(std::ostream &o, std::vector<cat_str *> &, std::map<std::string, std::string> &);
  const char * name() const { return "basename"; }
};
struct stripdir_func : public funcN<1> {
  std::ostream &output(std::ostream &o, std::vector<cat_str *> &, std::map<std::string, std::string> &);
  const char * name() const { return "stripdir"; }
};
struct entrycount_func : public funcN<0> {
  std::ostream &output(std::ostream &o, std::vector<cat_str *> &, std::map<std::string, std::string> &);
  const char * name() const { return "entrycount"; }
};
struct entryindex_func : public funcN<0> {
  std::ostream &output(std::ostream &o, std::vector<cat_str *> &, std::map<std::string, std::string> &);
  const char * name() const { return "entryindex"; }
};
struct firstentry_func : public funcN<1> {
  std::ostream &output(std::ostream &o, std::vector<cat_str *> &, std::map<std::string, std::string> &);
  const char * name() const { return "firstentry"; }
};
struct lastentry_func : public funcN<1> {
  std::ostream &output(std::ostream &o, std::vector<cat_str *> &, std::map<std::string, std::string> &);
  const char * name() const { return "lastentry"; }
};
struct level_func : public funcN<0> {
  std::ostream &output(std::ostream &o, std::vector<cat_str *> &, std::map<std::string, std::string> &);
  const char * name() const { return "level"; }
};
struct rcfile_func : public funcN<0> {
  std::ostream &output(std::ostream &o, std::vector<cat_str *> &, std::map<std::string, std::string> &);
  const char * name() const { return "rcfile"; }
};
struct examplercfile_func : public funcN<0> {
  std::ostream &output(std::ostream &o, std::vector<cat_str *> &, std::map<std::string, std::string> &);
  const char * name() const { return "examplercfile"; }
};
struct mainmenutitle_func : public funcN<0> {
  std::ostream &output(std::ostream &o, std::vector<cat_str *> &, std::map<std::string, std::string> &);
  const char * name() const { return "mainmenutitle"; }
};
struct rootsection_func : public funcN<0> {
  std::ostream &output(std::ostream &o, std::vector<cat_str *> &, std::map<std::string, std::string> &);
  const char * name() const { return "rootsection"; }
};
struct rootprefix_func : public funcN<0> {
  std::ostream &output(std::ostream &o, std::vector<cat_str *> &, std::map<std::string, std::string> &);
  const char * name() const { return "rootprefix"; }
};
struct userprefix_func : public funcN<0> {
  std::ostream &output(std::ostream &o, std::vector<cat_str *> &, std::map<std::string, std::string> &);
  const char * name() const { return "userprefix"; }
};
struct treewalk_func : public funcN<0> {
  std::ostream &output(std::ostream &o, std::vector<cat_str *> &, std::map<std::string, std::string> &);
  const char * name() const { return "treewalk"; }
};
struct postoutput_func : public funcN<0> {
  std::ostream &output(std::ostream &o, std::vector<cat_str *> &, std::map<std::string, std::string> &);
  const char * name() const { return "postoutput"; }
};
struct preoutput_func : public funcN<0> {
  std::ostream &output(std::ostream &o, std::vector<cat_str *> &, std::map<std::string, std::string> &);
  const char * name() const { return "preoutput"; }
};
struct cwd_func : public funcN<0> {
  std::ostream &output(std::ostream &o, std::vector<cat_str *> &, std::map<std::string, std::string> &);
  const char * name() const { return "cwd"; }
};
struct translate_func : public funcN<2> {
  std::ostream &output(std::ostream &o, std::vector<cat_str *> &, std::map<std::string, std::string> &);
  const char * name() const { return "translate"; }
};


// ************* exception classes:

class narg_mismatch : public except_pi_string { // number of args mismatch to func.
public:
  narg_mismatch(parsestream *p, std::string s) : except_pi_string(p,s) {}
  std::string message() const {
    return String::compose(_("Number of arguments to function %1 does not match."), msg);
  }
};  
class unknown_function : public except_pi_string {
public:
  unknown_function(parsestream *p, std::string s) : except_pi_string(p,s) {}
  std::string message() const {
    return String::compose(_("Unknown function: \"%1\""), msg);
  }
};

class unknown_indirect_function : public except_pi_string {
public:
  unknown_indirect_function(parsestream *p, std::string s) : except_pi_string(p,s){}
  std::string message() const {
    return String::compose(_("Indirectly used, but not defined function: \"%1\""), msg);
  }
};

class unknown_ident : public except_pi_string {
public:
  unknown_ident(parsestream *p, std::string s) : except_pi_string(p,s) {}
  std::string message() const {
    return String::compose(_("Unknown identifier: \"%1\""), msg);
  }
};

class dir_createerror : public except_string {
public:
  dir_createerror(std::string s) : except_string(s) {}
  std::string message() const {
    return String::compose(_("Could not open directory \"%1\"."), msg);
  }
};

class missing_tag:public except_pi_string {
public:
  missing_tag(parsestream *p, std::string s) : except_pi_string(p,s) {}
  std::string message() const {
    return String::compose(_("Missing (or empty) tag: %1\n"
          "This tag needs to be defined for the menu entry to make sense.\n"
          "Note that update-menus rearranges the order of the tags found\n" 
          "in the menu-entry files, so that the part above isn't literal."),
        msg);
  }
};

class conversion_error : public except_string {
public:
  conversion_error(std::string s) : except_string(s) { }
  std::string message() const {
    return String::compose(_("Encoding conversion error: \"%1\""), msg);
  }
};

#endif
