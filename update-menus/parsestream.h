/*
 * Debian menu system -- update-menus and install-menu
 * update-menus/parsestream.h
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

#ifndef PARSESTREAM_H
#define PARSESTREAM_H

#include <vector>
#include <string>
#include <regex.h>
#include "exceptions.h"

class Regex {
  struct re_pattern_buffer* patt;
public:
  Regex(const char *s);
  ~Regex() { delete patt; }

  struct re_pattern_buffer* pattern() const { return patt; }
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

// Exception classes for parsestream class.
class except_pi : public genexcept {
public:
  parsestream *pi;
  except_pi(parsestream *p) : pi(p) { }
  void report();
};

class except_pi_string : public except_pi {
protected:
  std::string msg;
public:
  except_pi_string(parsestream *p, std::string s) : except_pi(p), msg(s) { }
  std::string message() const {
    return String::compose(_("Unknown error, message=%1"), msg);
  }
};

class endoffile : public except_pi {
public:
  endoffile(parsestream *p) : except_pi(p) { }
  std::string message() const { return _("Unexpected end of file."); }
};

class endofline : public except_pi {
public:
  endofline(parsestream *p) : except_pi(p) { }
  std::string message() const { return _("Unexpected end of line."); }
};

class ident_expected : public except_pi {
public:
  ident_expected(parsestream *p) : except_pi(p) { }
  const char *errormsg() const { return _("Identifier expected."); }
};

class char_expected : public except_pi_string {
public:
  char_expected(parsestream *p, std::string s):except_pi_string(p,s) { }
  std::string message() const {
    return String::compose(_("Expected: \"%1\""), msg);
  }
};

class char_unexpected : public except_pi_string {
public:
  char_unexpected(parsestream *p, std::string s):except_pi_string(p,s) { }
  std::string message() const {
    return String::compose(_("Unexpected character: \"%1\""), msg);
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

class unknown_compat : public except_pi_string {
public:
  unknown_compat(parsestream *p, std::string s):except_pi_string(p,s) { }
  std::string message() const {
    return String::compose(_("Unknown compat mode: \"%1\""), msg);
  }
};


#endif /* PARSESTREAM_H */
