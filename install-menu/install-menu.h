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


#ifndef INSTALL_MENU_H
#define INSTALL_MENU_H

#include <iostream>
#include <vector>
#include <map>
#include <common.h>
#include <compose.hpp>
#include <parsestream.h>
#include "functions.h"

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
        ...

Example "str":
  "title=" print($title) ", mess:" \
   ifelse($longtitle, print(title), $longtitle)
*/

/** Container class. See above documentation. */
class str {
public:
  virtual std::ostream &output(std::ostream &o, std::map<std::string, std::string> &menuentry) = 0;
  virtual ~str() { };
};

/** Container class. See above documentation. */
class const_str : public str {
  std::string data;
public:
  const_str(parsestream &i) : data(i.get_stringconst()) { }
  /** Output the constant string */
  virtual std::ostream &output(std::ostream &o, std::map<std::string, std::string> &menuentry);
};

/** Container class. See above documentation. */
class cat_str : public str {
  std::vector<str *> v;
public:
  cat_str(parsestream &);
  std::ostream &output(std::ostream &o, std::map<std::string, std::string> &menuentry);
  void output(std::map<std::string, std::string> &menuentry);
  std::string soutput(std::map<std::string, std::string> &menuentry);
};

/** Container class. See above documentation. */
class var_str : public str {
  std::string var_name;
public:
  var_str(parsestream &i);
  /** Output the contents of the variable */
  std::ostream &output(std::ostream &o, std::map<std::string, std::string> &menuentry);
};

class func;

/** Container class. See above documentation. */
class func_str : public str {
  std::vector<cat_str *> args;
  functions::func *f;
public:
  func_str(parsestream &);
  /** Output the return value of the function */
  std::ostream &output(std::ostream &o, std::map<std::string, std::string> &menuentry);
};


class func_def : public functions::func {
  cat_str *f;
  std::string func_name;
  std::vector<std::string> args_name;
public:
  func_def(parsestream &i);
  unsigned int nargs() const { return args_name.size(); }
  std::ostream &output(std::ostream &, std::vector<cat_str *> &,std::map<std::string, std::string> &);
  const char * getName() const { return func_name.c_str(); }
  virtual ~func_def() { }
};

/** Class containing info of supported menu needs */
class supportedinfo {
  /** Representation of a single supported menu need */
  class supinfo {
  public:
    cat_str *name;
    int prec;
  };
  /** Map containing all supported menu needs */
  std::map<std::string, supinfo> supported;
public:
  supportedinfo(parsestream &);
  /** Checks whether a specified 'supported' exists */
  bool supports(std::string&);
  void subst(std::map<std::string, std::string> );
};

/** Class containing configurable menu-method data */
class methodinfo {
  std::string compt, rcf, exrcf, roots, mainmt, treew, outputenc;
  std::string preout, postout;

public:
  methodinfo(parsestream &);

  bool keep_sections;
  cat_str *startmenu, *endmenu, *submenutitle, *hkexclude,
    *genmenu, *postrun, *prerun, *sort, *rootpref, *userpref,
    *outputlanguage, *also_run, *preruntest;

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

extern methodinfo *menumethod;
extern supportedinfo *supported;
extern bool testuniqueness(std::map<std::string, std::string> &menuentry);
const char *ldgettext(const char *lang, const char *domain, const char *msgid);

namespace exceptions {
  /** Exception to be thrown when number of arguments to function didn't match */
  class narg_mismatch : public except_pi_string {
  public:
    narg_mismatch(parsestream *p, std::string s) : except_pi_string(p,s) {}
    std::string message() const {
      return String::compose(_("Number of arguments to function %1 does not match."), msg);
    }
  }; 
  /** Exception to be thrown when an unknown function was used */
  class unknown_function : public except_pi_string {
  public:
    unknown_function(parsestream *p, std::string s) : except_pi_string(p,s) {}
    std::string message() const {
      return String::compose(_("Unknown function: \"%1\""), msg);
    }
  };

  /** Exception to be thrown when a function was not defined but used */
  class unknown_indirect_function : public except_pi_string {
  public:
    unknown_indirect_function(parsestream *p, std::string s) : except_pi_string(p,s){}
    std::string message() const {
      return String::compose(_("Indirectly used, but not defined function: \"%1\""), msg);
    }
  };

  /** Exception to be thrown when a unknown identifier was found in menu-method */
  class unknown_ident : public except_pi_string {
  public:
    unknown_ident(parsestream *p, std::string s) : except_pi_string(p,s) {}
    std::string message() const {
      return String::compose(_("Unknown identifier: \"%1\""), msg);
    }
  };

  /** Exception to be thrown when encoding conversion failed */
  class conversion_error : public except_string {
  public:
    conversion_error(std::string s) : except_string(s) { }
    std::string message() const {
      return String::compose(_("Encoding conversion error: \"%1\""), msg);
    }
  };
}

#endif
