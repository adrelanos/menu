//   -*- mode: c++; -*-

#ifndef UPDATE_MENUS_H
#define UPDATE_MENUS_H

#include <map>
#include <vector>
#include "adstring.h"
#include "common.h"

using std::string;

class menuentry {
  void check_req_tags(const std::string& filename);
  void check_pkg_validity(parsestream &i, std::string &);
  void read_menuentry(parsestream &i);

public:
  menuentry(parsestream &i, const std::string& file, const std::string& shortfile);

  std::map<std::string, std::string> data;
  void output(std::vector<std::string> &s);
  std::ostream &debugoutput(std::ostream &o);
};

class trans_class {
protected:
  std::string match, replace, replace_var;
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
public:
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

  void update(std::string filename);
  void report(const string &message,verbosity_type v);
  void set_verbosity(verbosity_type v) { verbosity = v; }
};

// ************* exception classes:

class unknown_cond_package : public except_pi_string {
public:
  unknown_cond_package(parsestream *p, string s) : except_pi_string(p,s) { }
  string message() {
    return Sprintf(_("Unknown install condition \"%s\" (currently, only \"package\" is supported)"),msg);
  }
};

class cond_inst_false : public genexcept { }; //conditional install returns false

class pipeerror_read : public except_string {
public:
  pipeerror_read(std::string s) : except_string(s) { }
  std::string message() {
    return Sprintf(_("Failed to pipe data through \"%s\" (pipe opened for reading)"), msg);
  }
};  //pipe open for reading failed

class missing_tag : public except_string {
  std::string file;
public:
  missing_tag(std::string f, std::string s) : except_string(s), file(f) { }
  std::string message() {
    return file + Sprintf(_(": Missing required tag: \"%s\""), msg);
  }
};

class dir_error_read {
public:
  char name[MAX_LINE];
  dir_error_read(string s){
    strcpy(name,s.c_str());
  };
};  //dir open for reading failed

#endif
