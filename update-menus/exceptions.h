/*
 * Debian menu system -- update-menus and install-menu
 * update-menus/exceptions.h
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

#ifndef EXCEPTIONS_H
#define EXCEPTIONS_H

#include <string>
#include <iostream>
#include "common.h"
#include "compose.hpp"

class genexcept {
public:
  virtual void report() {
      std::cerr << message() << std::endl;
  }
  virtual std::string message() const {
      return _("Unknown error.");
  }
  virtual ~genexcept() { }
};

class except_string : public genexcept {
protected:
  std::string msg;
public:
  except_string(std::string s) : msg(s) { }
  std::string message() const {
    return String::compose(_("Unknown error, message=%1"), msg);
  }
};

class ferror_open : public except_string {
public:
  ferror_open(std::string s):except_string(s) { }
  std::string message() const {
    return String::compose(_("Unable to open file \"%1\"."), msg);
  }
};

class informed_fatal : public genexcept { 
  public:
    std::string message() const { return ""; }
};

#endif /* EXCEPTIONS_H */
