/*
 * Debian menu system -- update-menus
 * update-menus/update-menus.h
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

#ifndef UPDATE_MENUS_H
#define UPDATE_MENUS_H

#include <map>
#include <vector>
#include "compose.hpp"
#include "common.h"
#include "exceptions.h"
#include "parsestream.h"

/** Internal representation of a menu entry */
class menuentry {
  void check_req_tags(const std::string& filename);
  void check_pkg_validity(parsestream &i, std::string &);
  void read_menuentry(parsestream &i);

public:
  menuentry(parsestream &i, const std::string& file, const std::string& shortfile);

  /** Map containing the data of the menuentry. The key is the tag and the
   * value is the value of the tag.
   */
  std::map<std::string, std::string> data;
  /** Output full menu entry in menu format, and put it in vec */
  void output(std::vector<std::string>& vec);
};

/** Base class containing general methods for translation/substitution */
class trans_class {
protected:
  std::string match, replace, replace_var;

public:
  trans_class(const std::string &m, const std::string &r, const std::string &rv)
      : match(m), replace(r), replace_var(rv) { }

  /** Process the menuentry using a search string */
  virtual void process(menuentry &m, const std::string& search) = 0;
  virtual ~trans_class() { }
};

/** Class for menu's "translation" feature */
class translate : public trans_class {
public:
  translate(const std::string &m, const std::string &r, const std::string &rv)
      : trans_class(m,r,rv) { }

  void process(menuentry &m, const std::string& search);
};

/** Class for menu's "substitute" feature */
class substitute : public trans_class {
public:
  substitute(const std::string &m, const std::string &r, const std::string &rv)
      : trans_class(m,r,rv) { }

  void process(menuentry &m, const std::string& search);
};

/** Helper class for menu's "substitute" feature */
class subtranslate : public trans_class {
public:
  subtranslate(std::string &m, const std::string &r, const std::string &rv)
      : trans_class(m,r,rv) { }

  void process(menuentry &m, const std::string& search);
};

/** Class container for translation and subtitution classes */
class translateinfo {
  typedef std::pair<std::string, trans_class*> trans_pair;
  std::map<std::string, std::vector<trans_pair> > trans;

public:
  translateinfo(const std::string &filename);

  /** Process all translation classes */
  void process(menuentry &m);
};

/** Representation of configurable variables in update-menus. */
class configinfo {
  typedef enum { method_stdout, method_stderr, method_syslog} method_type;
  method_type method;
  int syslog_facility, syslog_priority;

  void parse_config(const std::string &var, const std::string& value);

public:
  configinfo()
      : compat(parsestream::eol_newline), usedefaultmenufilesdirs(true),
        onlyoutput_to_stdout(false), verbosity(report_quiet),
        method(method_stderr)
    { }

  typedef enum { report_quiet, report_normal, report_verbose} verbosity_type;
  parsestream::eol_type compat;
  std::vector<std::string> menufilesdir;
  std::string menumethod;
  bool usedefaultmenufilesdirs;
  bool onlyoutput_to_stdout;


  /** Read configuration file from filename */
  void read_file(const std::string& filename);

  /** Print message according to verbosity_type */
  void report(const std::string &message, verbosity_type v);

  /** Mutator method to verbosity */
  void set_verbosity(verbosity_type v) { verbosity = v; }

private:
  verbosity_type verbosity;
};

namespace exceptions {
  /** Exception to be thrown when an unknown install condition is found */
  class unknown_cond_package : public except_pi_string {
  public:
    unknown_cond_package(parsestream *p, std::string s) : except_pi_string(p,s) { }
    std::string message() {
      return String::compose(_("Unknown install condition \"%1\" (currently, only \"package\" is supported)."), msg);
    }
  };
}

#endif
