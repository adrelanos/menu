//   -*- mode: c++; -*-

#ifndef UPDATE_MENUS_H
#define UPDATE_MENUS_H

#include <map>
#include <vector>
#include "adstring.h"
#include "common.h"

class menuentry {
  bool check_validity(parsestream &i, string &);
  void read_menuentry(parsestream &i);

public:
  menuentry(parsestream &i, const string &);

  std::map<string, string> data;
  void output(std::vector<string> &s);
  std::ostream &debugoutput(std::ostream &o);
};

class trans_class {
protected:
  string match, replace, replace_var;
public:
  trans_class(const string &m, const string &r, const string &rv)
      : match(m), replace(r), replace_var(rv) { }

  bool check(string &s);
  virtual void process(menuentry &m, const string &v) = 0;
  string debuginfo();
  virtual ~trans_class() { }
};

class translate : public trans_class {
public:
  translate(const string &m, const string &r, const string &rv)
      : trans_class(m,r,rv) { }

  void process(menuentry &m, const string &v);
};

class substitute : public trans_class {
public:
  substitute(const string &m, const string &r, const string &rv)
      : trans_class(m,r,rv) { }

  void process(menuentry &m, const string &v);
};

class subtranslate : public trans_class {
public:
  subtranslate(string &m, const string &r, const string &rv)
      : trans_class(m,r,rv) { }

  void process(menuentry &m, const string &v);
};

class translateinfo {
  typedef std::multimap<string, trans_class*> trans_map;
  std::map<string, trans_map> trans;
  void init(parsestream &i);
public:
  translateinfo(parsestream &i);
  translateinfo(const string &filename);
  void process(menuentry &m);
  void debuginfo();
};

class configinfo {
public:
  typedef enum { report_quiet, report_normal, report_verbose, report_debug} verbosity_type;
  parsestream::eol_type compat;
  std::vector<std::string> menufilesdir;
  string menumethod;
  bool usedefaultmenufilesdirs;
  bool onlyoutput_to_stdout;

private:
  typedef enum { method_stdout, method_stderr, method_syslog} method_type;

  verbosity_type verbosity;
  method_type method;
  int syslog_facility, syslog_priority;
  void parse_def(const string &var, const string& value);

public:
  configinfo()
      : compat(parsestream::eol_newline), usedefaultmenufilesdirs(true),
        onlyoutput_to_stdout(false), verbosity(report_quiet),
        method(method_stderr)
    { }

  void update(string filename);
  void report(const string &message,verbosity_type v);
  void set_verbosity(verbosity_type v) { verbosity = v; }
};

// ************* exception classes:

class unknown_cond_package:public except_pi_string {
public:
  unknown_cond_package(parsestream *p, string s):except_pi_string(p,s){};
  string message(){
    return Sprintf(_("Unknown install condition \"%s\" (currently, only \"package\" is supported)"),msg);
  };
};

class cond_inst_false: public genexcept{};//conditional install returns false

class pipeerror_read: public except_string {
public:
  pipeerror_read(string s):except_string(s){};
  string message(){
    return Sprintf(_("Failed to pipe data through \"%s\" (pipe opened for reading)"),msg);
  };
};  //pipe open for reading failed

class dir_error_read {
public:
  char name[MAX_LINE];
  dir_error_read(string s){
    strcpy(name,s.c_str());
  };
};  //dir open for reading failed

#endif
