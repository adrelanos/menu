//   -*- mode: c++; -*-

#ifndef ADSTRING_H
#define ADSTRING_H

#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <clocale>
#include <cstdio>
#include "regex-c.h"
#include "common.h"

using std::string;

class Regex {
  struct re_pattern_buffer* patt;
public:
  Regex(const char *s);
  struct re_pattern_buffer* pattern() const { return patt; }
};


// ************* std::string utility functions:

typedef std::vector<string> StrVec;

bool contains(const string& str, const string &sub, string::size_type pos=0);
bool contains(const string& str, char c);

string Sprintf(const char *, const string &);

string rmtrailingspace(string &s);
string escape_string(const string &s, const string &esc);
string escapewith_string(const string &s, 
				const string &esc, 
				const string &with);
string lowercase(string s);
string uppercase(string s);
string replacewith_string(const string &s, 
				 const string &replace, 
				 const string &with);
string cppesc_string(const string &s);
string replace(string s,char match, char replace);
int stringtoi(const string &s);
string itostring(int i);
string itohexstring(int i);
string sort_hotkey(string s);
string string_parent(string s);
string string_basename(string s);
string string_stripdir(string s);
string string_lastname(string s);
void break_char(const string &sec,StrVec &sec_vec, char);
void break_slashes(const string &sec,StrVec &sec_vec);
void break_commas(const string &sec,StrVec &sec_vec);

// ************* exception classes for parsers:

class parsestream;

class genexcept {
public:
  virtual void report() {
      std::cerr << message() << std::endl;
  }
  virtual string message() const {
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
  string msg;
public:
  except_string(string s) : msg(s) { }
  string message() const {
    return Sprintf(_("Unknown Error, message=%s"),msg);
  }
};

class except_pi_string : public except_pi {
protected:
  string msg;
public:
  except_pi_string(parsestream *p, string s) : except_pi(p), msg(s) { }
  string message() const {
    return Sprintf(_("Unknown Error, message=%s"),msg);
  }
};

class endoffile : public except_pi {
public:
  endoffile(parsestream *p) : except_pi(p) { }
  string message() const {return _("Unexpected end of file");}
};

class endofline : public except_pi {
public:
  endofline(parsestream *p) : except_pi(p) { }
  string message() const {return _("Unexpected end of line");}
};

class ident_expected : public except_pi {
public:
  ident_expected(parsestream *p) : except_pi(p) { }
  const char *errormsg() const {return _("Identifier expected");}
};

class char_expected: public except_pi_string {
public:
  char_expected(parsestream *p, string s):except_pi_string(p,s){};
  string message() const {
    return Sprintf(_("Expected: \"%s\"."), msg);
  }
};

class char_unexpected: public except_pi_string {
public:
  char_unexpected(parsestream *p, string s):except_pi_string(p,s){};
  string message() const {
    return Sprintf(_("Unexpected character :\"%s\"."),msg);
  }
};  

class boolean_expected: public except_pi_string {
public:
  boolean_expected(parsestream *p, string s):except_pi_string(p,s){};
  string message() const {
    return Sprintf(_("Boolean (either true or false) expected.\n"
		     "Found: \"%s\""),msg);
  }
};  

class ferror_open: public except_string {
public:
  ferror_open(string s):except_string(s){};
  string message() const {
    return Sprintf(_("Unable to open file \"%s\""),msg);
  }
};

class unknown_compat: public except_pi_string {
public:
  unknown_compat(parsestream *p, string s):except_pi_string(p,s){};
  string message() const {
    return Sprintf(_("Unknown compat mode: \"%s\""),msg);
  }
};

class informed_fatal : public genexcept{};

// ************* parser classes

class parseinfo {
public:
  virtual ~parseinfo() { }

  std::vector<int> lineno;
  string buffer;
  std::string::size_type pos;
  std::vector<string> fname;

  int linenumber() const { return lineno.back(); }
  void set_linenumber(int l) { lineno.back() = l; }
  string filename() const { return fname.back(); }
  void set_filename(const string &s) { fname.back() = s; }
};

class parsestream : public parseinfo {
public:
  enum eol_type {eol_newline, eol_semicolon};
private:
  std::vector<std::istream *> istreams;
  bool in_constructor;
  void new_line();
  void preprocess(string &s);
  std::istream *current_istr() const { return istreams.back(); }
  void close_file();
  void new_file(const string &s);
  bool stdin_file;
  eol_type eolmode;
  string otherdir;
public:
  parsestream(std::istream &in, string other="")
      : in_constructor(true), stdin_file(true), otherdir(other)
  {
    init(&in,_("(probably) stdin"));
    in_constructor = false;
  }

  parsestream(const string &s, string other="")
      : in_constructor(true), stdin_file(false), otherdir(other)
  {
    std::istream *f = new std::ifstream(s.c_str());

    if(!f && (s[0]!='/')) {
      string add=other+"/"+s;
      f = new std::ifstream(add.c_str());
    }
    if (!*f)
        throw ferror_open(_("Cannot open file ") +s);

    init(f,s);
    in_constructor = false;
  }
  void init(std::istream *in, string s) {

    if(!in_constructor){
      if(!*in || !in->good())
	throw ferror_open(s);
    }
    pos=0;
    eolmode=eol_newline;
    istreams.push_back(in); 
    lineno.push_back(0);
    fname.push_back(s);

    new_line();
  }
  bool good() const { return istreams.size(); }
  char get_char();
  char put_back(char);
  string get_name();
  string get_name(const Regex &);
  string get_eq_name();
  string get_stringconst();
  string get_eq_stringconst();
  bool   get_boolean();
  bool   get_eq_boolean();
  int    get_integer();
  int    get_eq_integer();
  double get_double();
  double get_eq_double();
  string get_line();
  void skip_line();
  void skip_space();
  void skip_char(char expect);
  void seteolmode(eol_type mode) { eolmode = mode; }
};

const char *ldgettext(const char *language, 
			     const char *domain,
			     const char *msgid);

#endif /* ADSTRING_H */
