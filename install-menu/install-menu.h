//   -*- mode: c++; -*-
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

// container classes:

class str {
public:
  virtual std::ostream &output(std::ostream &o, std::map<string, string> &menuentry) = 0;
  virtual std::ostream &debuginfo(std::ostream &) = 0;
  virtual ~str() { };
};

class const_str : public str {
  string data;
public:
  const_str(parsestream &i) : data(i.get_stringconst()) { }
  virtual std::ostream &output(std::ostream &o, std::map<string, string> &menuentry);
  std::ostream &debuginfo(std::ostream &o);
};

class cat_str : public str {
  std::vector<str *> v;
public:
  cat_str(parsestream &);
  std::ostream &output(std::ostream &o, std::map<string, string> &menuentry);
  void output(std::map<string, string> &menuentry);
  string soutput(std::map<string, string> &menuentry);
  std::ostream &debuginfo(std::ostream &o);
};

class var_str : public str {
  string var_name;
public:
  var_str(parsestream &i);
  std::ostream &output(std::ostream &o, std::map<string, string> &menuentry);
  std::ostream &debuginfo(std::ostream &o);
};

class func;

class func_str : public str {
  std::vector<cat_str *> args;
  func *f;
public:
  func_str(parsestream &);
  std::ostream &output(std::ostream &o, std::map<string, string> &menuentry);
  std::ostream &debuginfo(std::ostream &o);
};

class supportedinfo {
  class supinf {
  public:
    cat_str *c;
    int prec;
  };
  std::map<string, supinf> sup;
public:
  supportedinfo(parsestream &);
  int prec(string &);
  void subst(std::map<string, string> );
  std::ostream &debuginfo(std::ostream &);
};

class configinfo {
  string compt, rcf, exrcf, roots, mainmt, treew;
  string preout, postout;

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
  
  const string &rcfile() const { return rcf; }
  const string &examplercfile() const { return exrcf; }
  const string &mainmenutitle() const { return mainmt; }
  const string &rootsection() const { return roots; }
  const string &treewalk() const { return treew; }
  const string &preoutput() const { return preout; }
  const string &postoutput() const { return postout; }
  cat_str *rootprefix();
  cat_str *userprefix();

  string prefix();
};

extern configinfo *config;
extern supportedinfo *supported;
extern bool testuniqueness(std::map<string, string> &menuentry);

/////////////////////////////////////////////////////
//  prototypes of install-menu functions:
//

class func {
public: 
  virtual unsigned int nargs() const = 0;
  virtual std::ostream &output(std::ostream &, std::vector<cat_str *> &,
                                  std::map<string, string> &) = 0;
  virtual const char * name() const = 0;
};

template<unsigned int N>
struct funcN : public func {
  unsigned int nargs() const { return N; }
};

class func_def : public func {
  cat_str *f;
  string func_name;
  StrVec args_name;
public:
  func_def(parsestream &i);
  unsigned int nargs() const { return args_name.size(); }
  std::ostream &output(std::ostream &, std::vector<cat_str *> &,std::map<string, string> &);
  const char * name() const { return func_name.c_str(); }
  virtual ~func_def() { }
};

/////////////////////////////////////////////////////
//  Function that can be used in install-menu (declarations)
//

struct prefix_func : public funcN<0> {
  std::ostream &output(std::ostream &o, std::vector<cat_str *> &, std::map<string, string> &);
  const char * name() const { return "prefix"; }
};
struct ifroot_func: public funcN<2> {
  std::ostream &output(std::ostream &o, std::vector<cat_str *> &, std::map<string, string> &);
  const char * name() const { return "ifroot"; }
};

struct print_func : public funcN<1> {
  std::ostream &output(std::ostream &o, std::vector<cat_str *> &, std::map<string, string> &);
  const char * name() const { return "print"; }
};
struct ifempty_func : public funcN<2> {
  std::ostream &output(std::ostream &o, std::vector<cat_str *> &, std::map<string, string> &);
  const char * name() const { return "ifempty"; }
};
struct ifnempty_func : public funcN<2> {
  std::ostream &output(std::ostream &o, std::vector<cat_str *> &, std::map<string, string> &);
  const char * name() const { return "ifnempty"; }
};
struct iffile_func : public funcN<2> {
  std::ostream &output(std::ostream &o, std::vector<cat_str *> &, std::map<string, string> &);
  const char * name() const { return "iffile"; }
};
struct ifelsefile_func : public funcN<3> {
  std::ostream &output(std::ostream &o, std::vector<cat_str *> &, std::map<string, string> &);
  const char * name() const { return "ifelsefile"; }
};
struct catfile_func : public funcN<1> {
  std::ostream &output(std::ostream &o, std::vector<cat_str *> &, std::map<string, string> &);
  const char * name() const { return "catfile"; }
};
struct forall_func : public funcN<3> {
  std::ostream &output(std::ostream &o, std::vector<cat_str *> &, std::map<string, string> &);
  const char * name() const { return "forall"; }
};

struct esc_func : public funcN<2> {
  std::ostream &output(std::ostream &o, std::vector<cat_str *> &, std::map<string, string> &);
  const char * name() const { return "esc"; }
};
struct add_func : public funcN<2> {
  std::ostream &output(std::ostream &o, std::vector<cat_str *> &, std::map<string, string> &);
  const char * name() const { return "add"; }
};
struct sub_func : public funcN<2> {
  std::ostream &output(std::ostream &o, std::vector<cat_str *> &, std::map<string, string> &);
  const char * name() const { return "sub"; }
};
struct mult_func : public funcN<2> {
  std::ostream &output(std::ostream &o, std::vector<cat_str *> &, std::map<string, string> &);
  const char * name() const { return "mult"; }
};
struct div_func : public funcN<2> {
  std::ostream &output(std::ostream &o, std::vector<cat_str *> &, std::map<string, string> &);
  const char * name() const { return "div"; }
};


struct ifelse_func: public funcN<3> {
  std::ostream &output(std::ostream &o, std::vector<cat_str *> &, std::map<string, string> &);
  const char * name() const { return "ifelse"; }
};

struct ifeq_func: public funcN<3> {
  std::ostream &output(std::ostream &o, std::vector<cat_str *> &, std::map<string, string> &);
  const char * name() const { return "ifeq"; }
};
struct ifneq_func: public funcN<3> {
  std::ostream &output(std::ostream &o, std::vector<cat_str *> &, std::map<string, string> &);
  const char * name() const { return "ifneq"; }
};
struct ifeqelse_func: public funcN<4> {
  std::ostream &output(std::ostream &o, std::vector<cat_str *> &, std::map<string, string> &);
  const char * name() const { return "ifeqelse"; }
};

struct cond_surr_func: public funcN<3> {
  std::ostream &output(std::ostream &o, std::vector<cat_str *> &, std::map<string, string> &);
  const char * name() const { return "cond_surr"; }
};
struct escwith_func : public funcN<3> {
  std::ostream &output(std::ostream &o, std::vector<cat_str *> &, std::map<string, string> &);
  const char * name() const { return "escwith"; }
};
struct escfirst_func : public funcN<3> {
  std::ostream &output(std::ostream &o, std::vector<cat_str *> &, std::map<string, string> &);
  const char * name() const { return "escfirst"; }
};
struct tolower_func : public funcN<1> {
  std::ostream &output(std::ostream &o, std::vector<cat_str *> &, std::map<string, string> &);
  const char * name() const { return "tolower"; }
};
struct toupper_func : public funcN<1> {
  std::ostream &output(std::ostream &o, std::vector<cat_str *> &, std::map<string, string> &);
  const char * name() const { return "toupper"; }
};
struct replacewith_func : public funcN<3> {
  std::ostream &output(std::ostream &o, std::vector<cat_str *> &, std::map<string, string> &);
  const char * name() const { return "replacewith"; }
};
struct nstring_func : public funcN<2> {
  std::ostream &output(std::ostream &o, std::vector<cat_str *> &, std::map<string, string> &);
  const char * name() const { return "nstring"; }
};

struct cppesc_func : public funcN<1> {
  std::ostream &output(std::ostream &o, std::vector<cat_str *> &, std::map<string, string> &);
  const char * name() const { return "cppesc"; }
};
struct parent_func : public funcN<1> {
  std::ostream &output(std::ostream &o, std::vector<cat_str *> &, std::map<string, string> &);
  const char * name() const { return "parent"; }
};
struct basename_func : public funcN<1> {
  std::ostream &output(std::ostream &o, std::vector<cat_str *> &, std::map<string, string> &);
  const char * name() const { return "basename"; }
};
struct stripdir_func : public funcN<1> {
  std::ostream &output(std::ostream &o, std::vector<cat_str *> &, std::map<string, string> &);
  const char * name() const { return "stripdir"; }
};
struct entrycount_func : public funcN<0> {
  std::ostream &output(std::ostream &o, std::vector<cat_str *> &, std::map<string, string> &);
  const char * name() const { return "entrycount"; }
};
struct entryindex_func : public funcN<0> {
  std::ostream &output(std::ostream &o, std::vector<cat_str *> &, std::map<string, string> &);
  const char * name() const { return "entryindex"; }
};
struct firstentry_func : public funcN<1> {
  std::ostream &output(std::ostream &o, std::vector<cat_str *> &, std::map<string, string> &);
  const char * name() const { return "firstentry"; }
};
struct lastentry_func : public funcN<1> {
  std::ostream &output(std::ostream &o, std::vector<cat_str *> &, std::map<string, string> &);
  const char * name() const { return "lastentry"; }
};
struct level_func : public funcN<0> {
  std::ostream &output(std::ostream &o, std::vector<cat_str *> &, std::map<string, string> &);
  const char * name() const { return "level"; }
};

struct rcfile_func : public funcN<0> {
  std::ostream &output(std::ostream &o, std::vector<cat_str *> &, std::map<string, string> &);
  const char * name() const { return "rcfile"; }
};
struct examplercfile_func : public funcN<0> {
  std::ostream &output(std::ostream &o, std::vector<cat_str *> &, std::map<string, string> &);
  const char * name() const { return "examplercfile"; }
};
struct mainmenutitle_func : public funcN<0> {
  std::ostream &output(std::ostream &o, std::vector<cat_str *> &, std::map<string, string> &);
  const char * name() const { return "mainmenutitle"; }
};
struct rootsection_func : public funcN<0> {
  std::ostream &output(std::ostream &o, std::vector<cat_str *> &, std::map<string, string> &);
  const char * name() const { return "rootsection"; }
};
struct rootprefix_func : public funcN<0> {
  std::ostream &output(std::ostream &o, std::vector<cat_str *> &, std::map<string, string> &);
  const char * name() const { return "rootprefix"; }
};
struct userprefix_func : public funcN<0> {
  std::ostream &output(std::ostream &o, std::vector<cat_str *> &, std::map<string, string> &);
  const char * name() const { return "userprefix"; }
};
struct treewalk_func : public funcN<0> {
  std::ostream &output(std::ostream &o, std::vector<cat_str *> &, std::map<string, string> &);
  const char * name() const { return "treewalk"; }
};
struct postoutput_func : public funcN<0> {
  std::ostream &output(std::ostream &o, std::vector<cat_str *> &, std::map<string, string> &);
  const char * name() const { return "postoutput"; }
};
struct preoutput_func : public funcN<0> {
  std::ostream &output(std::ostream &o, std::vector<cat_str *> &, std::map<string, string> &);
  const char * name() const { return "preoutput"; }
};

struct cwd_func : public funcN<0> {
  std::ostream &output(std::ostream &o, std::vector<cat_str *> &, std::map<string, string> &);
  const char * name() const { return "cwd"; }
};

struct translate_func : public funcN<2> {
  std::ostream &output(std::ostream &o, std::vector<cat_str *> &, std::map<string, string> &);
  const char * name() const { return "translate"; }
};


// ************* exception classes:

class narg_mismatch:public except_pi_string{ //number of args mismatch to func.
public:
  narg_mismatch(parsestream *p, string s):except_pi_string(p,s){};
  string message() const {
    return Sprintf(_("Number of arguments to function %s doesn't match"),
		   msg);
  };
};  
class unknown_function:public except_pi_string {
public:
  unknown_function(parsestream *p, string s):except_pi_string(p,s){};
  string message() const {
    return Sprintf(_("Unknown function: \"%s\""),
		   msg);
  };
};  
class unknown_indirect_function:public except_pi_string {
public:
  unknown_indirect_function(parsestream *p, string s):except_pi_string(p,s){};
  string message() const {
    return Sprintf(_("Indirectly used, but not defined function: \"%s\""),
		   msg);
  };
};  
class unknown_ident: public except_pi_string {
public:
  unknown_ident(parsestream *p, string s):except_pi_string(p,s){};
  string message() const {
    return Sprintf(_("Unknown identifier: \"%s\""),
		   msg);
  };
};  

class dir_createerror: public except_string{ //Cannot create dir
public:
  dir_createerror(string s):except_string(s){};
  string message() const {
    return Sprintf(_("Could not open dir \"%s\""),msg);
  };
};  

class missing_tag:public except_pi_string {
public:
  missing_tag(parsestream *p, string s):except_pi_string(p,s){};
  string message() const {
    return Sprintf(_("Missing (or empty) tag: %s\n"
		     "This tag needs to defined for the menuentry to make sense.\n"
		     "Note, BTW, that update-menus re-arranges the order of the\n"
                     "tags found in the menu entry files, so that the part above\n"
                     "isn't literal"),
		   msg);
  };
};  

#endif /* INSTALL_MENU_H */
