#ifndef ADSTRING_H
#define ADSTRING_H

#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <clocale>
#include <cstdio>
#include <sys/types.h>
#include <regex.h>
#include "common.h"
#include "compose.hpp"

class Regex {
  struct re_pattern_buffer* patt;
public:
  Regex(const char *s);
  ~Regex() { delete patt; }

  struct re_pattern_buffer* pattern() const { return patt; }
};


// ************* std::string utility functions:

bool contains(const std::string& str, const std::string &sub, std::string::size_type pos = 0);
bool contains(const std::string& str, char c);

std::string rmtrailingspace(std::string &s);
std::string escape_string(const std::string &s, const std::string &esc);
std::string escapewith_string(const std::string &s, const std::string &esc, const std::string &with);
std::string lowercase(std::string s);
std::string uppercase(std::string s);
std::string replacewith_string(const std::string &s, const std::string &replace, const std::string &with);
std::string cppesc_string(const std::string &s);
std::string replace(std::string s, char match, char replace);
int stringtoi(const std::string &s);
std::string itostring(int i);
std::string itohexstring(int i);
std::string sort_hotkey(std::string s);
std::string string_parent(std::string s);
std::string string_basename(std::string s);
std::string string_stripdir(std::string s);
std::string string_lastname(std::string s);
void break_char(const std::string &sec, std::vector<std::string> &sec_vec, char);
void break_slashes(const std::string &sec, std::vector<std::string> &sec_vec);
void break_commas(const std::string &sec, std::vector<std::string> &sec_vec);

// ************* exception classes for parsers:

class parsestream;

class genexcept {
public:
  virtual void report() {
      std::cerr << message() << std::endl;
  }
  virtual std::string message() const {
      return _("Unknown Error");
  }
  virtual ~genexcept() { }
};

class except_pi : public genexcept {
public:
  parsestream *pi;
  except_pi(parsestream *p) : pi(p) { }
  void report();
};

class except_string : public genexcept {
protected:
  std::string msg;
public:
  except_string(std::string s) : msg(s) { }
  std::string message() const {
    return String::compose(_("Unknown Error, message=%1"), msg);
  }
};

class except_pi_string : public except_pi {
protected:
  std::string msg;
public:
  except_pi_string(parsestream *p, std::string s) : except_pi(p), msg(s) { }
  std::string message() const {
    return String::compose(_("Unknown Error, message=%1"), msg);
  }
};

class endoffile : public except_pi {
public:
  endoffile(parsestream *p) : except_pi(p) { }
  std::string message() const { return _("Unexpected end of file"); }
};

class endofline : public except_pi {
public:
  endofline(parsestream *p) : except_pi(p) { }
  std::string message() const { return _("Unexpected end of line"); }
};

class ident_expected : public except_pi {
public:
  ident_expected(parsestream *p) : except_pi(p) { }
  const char *errormsg() const { return _("Identifier expected"); }
};

class char_expected : public except_pi_string {
public:
  char_expected(parsestream *p, std::string s):except_pi_string(p,s) { }
  std::string message() const {
    return String::compose(_("Expected: \"%1\"."), msg);
  }
};

class char_unexpected : public except_pi_string {
public:
  char_unexpected(parsestream *p, std::string s):except_pi_string(p,s) { }
  std::string message() const {
    return String::compose(_("Unexpected character :\"%1\"."), msg);
  }
};

class boolean_expected : public except_pi_string {
public:
  boolean_expected(parsestream *p, std::string s):except_pi_string(p,s) { }
  std::string message() const {
    return String::compose(_("Boolean (either true or false) expected.\n"
                     "Found: \"%1\""), msg);
  }
};

class ferror_open : public except_string {
public:
  ferror_open(std::string s):except_string(s) { }
  std::string message() const {
    return String::compose(_("Unable to open file \"%1\""), msg);
  }
};

class unknown_compat : public except_pi_string {
public:
  unknown_compat(parsestream *p, std::string s):except_pi_string(p,s) { }
  std::string message() const {
    return String::compose(_("Unknown compat mode: \"%1\""), msg);
  }
};

class informed_fatal : public genexcept { 
  public:
    std::string message() const { return ""; }
};

class parsestream {
public:
  enum eol_type { eol_newline, eol_semicolon };
private:
  std::vector<int> linenumbers;
  std::vector<std::string> filenames;
  std::vector<std::istream *> istreams;

  void set_linenumber(int l) { linenumbers.back() = l; }
  void set_filename(const std::string &s) { filenames.back() = s; }
  std::istream *current_istr() const { return istreams.back(); }

  bool in_constructor;
  void new_line();
  void preprocess(std::string &s);
  void new_file(const std::string &name);
  void init(std::istream *in, std::string name);
  void close_file();
  bool stdin_file;
  eol_type eolmode;
  std::string otherdir;

public:
  parsestream(std::istream &in, std::string other = "");
  parsestream(const std::string &name, std::string other = "");

  bool doescaping;
  std::string::size_type pos;
  std::string buffer;
  std::string filename() const { return filenames.back(); }
  int linenumber() const { return linenumbers.back(); }

  bool good() const { return istreams.size(); }
  char get_char();
  char put_back(char);
  std::string get_name();
  std::string get_name(const Regex &);
  std::string get_eq_name();
  std::string get_stringconst();
  std::string get_eq_stringconst();
  bool   get_boolean();
  bool   get_eq_boolean();
  int    get_integer();
  int    get_eq_integer();
  double get_double();
  double get_eq_double();
  std::string get_line();
  void skip_line();
  void skip_space();
  void skip_char(char expect);
  void seteolmode(eol_type mode) { eolmode = mode; }
};

const char *ldgettext(const char *lang, const char *domain, const char *msgid);

#endif /* ADSTRING_H */
