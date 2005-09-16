/*
 * Debian menu system -- install-menu
 * install-menus/functions.h
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

#ifndef FUNCTIONS_H
#define FUNCTIONS_H

#include <fstream>
#include <vector>
#include <string>
#include <map>
#include <parsestream.h>

class cat_str;

/////////////////////////////////////////////////////
//  Function that can be used in install-menu (declarations)
//
//
namespace functions {

  /** Base class for function definitions */
  class func {
  public: 
    virtual ~func() { }
    /** Number of arguments that function takes as parameters */
    virtual unsigned int nargs() const = 0;
    /** Calls function with arguments and returns output in stream */
    virtual std::ostream &output(std::ostream &, std::vector<cat_str *> &,
        std::map<std::string, std::string> &) = 0;
    /** Accessor to function name */
    virtual const char * getName() const = 0;
  };

  /** Template class for functions with N arguments */
  template<unsigned int N>
  struct funcN : public func {
    unsigned int nargs() const { return N; }
  };

  /** Documentation for function can be found in menu manual */
  struct prefix : public funcN<0> {
    std::ostream &output(std::ostream &o, std::vector<cat_str *> &, std::map<std::string, std::string> &);
    const char * getName() const { return "prefix"; }
  };
  /** Documentation for function can be found in menu manual */
  struct shell : public funcN<1> {
    std::ostream &output(std::ostream &o, std::vector<cat_str *> &, std::map<std::string, std::string> &);
    const char * getName() const { return "shell"; }
  };
  /** Documentation for function can be found in menu manual */
  struct ifroot: public funcN<2> {
    std::ostream &output(std::ostream &o, std::vector<cat_str *> &, std::map<std::string, std::string> &);
    const char * getName() const { return "ifroot"; }
  };
  /** Documentation for function can be found in menu manual */
  struct print : public funcN<1> {
    std::ostream &output(std::ostream &o, std::vector<cat_str *> &, std::map<std::string, std::string> &);
    const char * getName() const { return "print"; }
  };
  /** Documentation for function can be found in menu manual */
  struct ifempty : public funcN<2> {
    std::ostream &output(std::ostream &o, std::vector<cat_str *> &, std::map<std::string, std::string> &);
    const char * getName() const { return "ifempty"; }
  };
  /** Documentation for function can be found in menu manual */
  struct ifnempty : public funcN<2> {
    std::ostream &output(std::ostream &o, std::vector<cat_str *> &, std::map<std::string, std::string> &);
    const char * getName() const { return "ifnempty"; }
  };
  /** Documentation for function can be found in menu manual */
  struct iffile : public funcN<2> {
    std::ostream &output(std::ostream &o, std::vector<cat_str *> &, std::map<std::string, std::string> &);
    const char * getName() const { return "iffile"; }
  };
  /** Documentation for function can be found in menu manual */
  struct ifelsefile : public funcN<3> {
    std::ostream &output(std::ostream &o, std::vector<cat_str *> &, std::map<std::string, std::string> &);
    const char * getName() const { return "ifelsefile"; }
  };
  /** Documentation for function can be found in menu manual */
  struct catfile : public funcN<1> {
    std::ostream &output(std::ostream &o, std::vector<cat_str *> &, std::map<std::string, std::string> &);
    const char * getName() const { return "catfile"; }
  };
  /** Documentation for function can be found in menu manual */
  struct forall : public funcN<3> {
    std::ostream &output(std::ostream &o, std::vector<cat_str *> &, std::map<std::string, std::string> &);
    const char * getName() const { return "forall"; }
  };
  /** Documentation for function can be found in menu manual */
  struct esc : public funcN<2> {
    std::ostream &output(std::ostream &o, std::vector<cat_str *> &, std::map<std::string, std::string> &);
    const char * getName() const { return "esc"; }
  };
  /** Documentation for function can be found in menu manual */
  struct add : public funcN<2> {
    std::ostream &output(std::ostream &o, std::vector<cat_str *> &, std::map<std::string, std::string> &);
    const char * getName() const { return "add"; }
  };
  /** Documentation for function can be found in menu manual */
  struct sub : public funcN<2> {
    std::ostream &output(std::ostream &o, std::vector<cat_str *> &, std::map<std::string, std::string> &);
    const char * getName() const { return "sub"; }
  };
  /** Documentation for function can be found in menu manual */
  struct mult : public funcN<2> {
    std::ostream &output(std::ostream &o, std::vector<cat_str *> &, std::map<std::string, std::string> &);
    const char * getName() const { return "mult"; }
  };
  /** Documentation for function can be found in menu manual */
  struct div : public funcN<2> {
    std::ostream &output(std::ostream &o, std::vector<cat_str *> &, std::map<std::string, std::string> &);
    const char * getName() const { return "div"; }
  };
  /** Documentation for function can be found in menu manual */
  struct ifelse: public funcN<3> {
    std::ostream &output(std::ostream &o, std::vector<cat_str *> &, std::map<std::string, std::string> &);
    const char * getName() const { return "ifelse"; }
  };
  /** Documentation for function can be found in menu manual */
  struct ifeq: public funcN<3> {
    std::ostream &output(std::ostream &o, std::vector<cat_str *> &, std::map<std::string, std::string> &);
    const char * getName() const { return "ifeq"; }
  };
  /** Documentation for function can be found in menu manual */
  struct ifneq: public funcN<3> {
    std::ostream &output(std::ostream &o, std::vector<cat_str *> &, std::map<std::string, std::string> &);
    const char * getName() const { return "ifneq"; }
  };
  /** Documentation for function can be found in menu manual */
  struct ifeqelse: public funcN<4> {
    std::ostream &output(std::ostream &o, std::vector<cat_str *> &, std::map<std::string, std::string> &);
    const char * getName() const { return "ifeqelse"; }
  };
  /** Documentation for function can be found in menu manual */
  struct cond_surr: public funcN<3> {
    std::ostream &output(std::ostream &o, std::vector<cat_str *> &, std::map<std::string, std::string> &);
    const char * getName() const { return "cond_surr"; }
  };
  /** Documentation for function can be found in menu manual */
  struct escwith : public funcN<3> {
    std::ostream &output(std::ostream &o, std::vector<cat_str *> &, std::map<std::string, std::string> &);
    const char * getName() const { return "escwith"; }
  };
  /** Documentation for function can be found in menu manual */
  struct escfirst : public funcN<3> {
    std::ostream &output(std::ostream &o, std::vector<cat_str *> &, std::map<std::string, std::string> &);
    const char * getName() const { return "escfirst"; }
  };
  /** Documentation for function can be found in menu manual */
  struct tolower : public funcN<1> {
    std::ostream &output(std::ostream &o, std::vector<cat_str *> &, std::map<std::string, std::string> &);
    const char * getName() const { return "tolower"; }
  };
  /** Documentation for function can be found in menu manual */
  struct toupper : public funcN<1> {
    std::ostream &output(std::ostream &o, std::vector<cat_str *> &, std::map<std::string, std::string> &);
    const char * getName() const { return "toupper"; }
  };
  /** Documentation for function can be found in menu manual */
  struct replace : public funcN<3> {
    std::ostream &output(std::ostream &o, std::vector<cat_str *> &, std::map<std::string, std::string> &);
    const char * getName() const { return "replace"; }
  };
  /** Documentation for function can be found in menu manual */
  struct replacewith : public funcN<3> {
    std::ostream &output(std::ostream &o, std::vector<cat_str *> &, std::map<std::string, std::string> &);
    const char * getName() const { return "replacewith"; }
  };
  /** Documentation for function can be found in menu manual */
  struct nstring : public funcN<2> {
    std::ostream &output(std::ostream &o, std::vector<cat_str *> &, std::map<std::string, std::string> &);
    const char * getName() const { return "nstring"; }
  };
  /** Documentation for function can be found in menu manual */
  struct cppesc : public funcN<1> {
    std::ostream &output(std::ostream &o, std::vector<cat_str *> &, std::map<std::string, std::string> &);
    const char * getName() const { return "cppesc"; }
  };
  /** Documentation for function can be found in menu manual */
  struct parent : public funcN<1> {
    std::ostream &output(std::ostream &o, std::vector<cat_str *> &, std::map<std::string, std::string> &);
    const char * getName() const { return "parent"; }
  };
  /** Documentation for function can be found in menu manual */
  struct basename : public funcN<1> {
    std::ostream &output(std::ostream &o, std::vector<cat_str *> &, std::map<std::string, std::string> &);
    const char * getName() const { return "basename"; }
  };
  /** Documentation for function can be found in menu manual */
  struct stripdir : public funcN<1> {
    std::ostream &output(std::ostream &o, std::vector<cat_str *> &, std::map<std::string, std::string> &);
    const char * getName() const { return "stripdir"; }
  };
  /** Documentation for function can be found in menu manual */
  struct entrycount : public funcN<0> {
    std::ostream &output(std::ostream &o, std::vector<cat_str *> &, std::map<std::string, std::string> &);
    const char * getName() const { return "entrycount"; }
  };
  /** Documentation for function can be found in menu manual */
  struct entryindex : public funcN<0> {
    std::ostream &output(std::ostream &o, std::vector<cat_str *> &, std::map<std::string, std::string> &);
    const char * getName() const { return "entryindex"; }
  };
  /** Documentation for function can be found in menu manual */
  struct firstentry : public funcN<1> {
    std::ostream &output(std::ostream &o, std::vector<cat_str *> &, std::map<std::string, std::string> &);
    const char * getName() const { return "firstentry"; }
  };
  /** Documentation for function can be found in menu manual */
  struct lastentry : public funcN<1> {
    std::ostream &output(std::ostream &o, std::vector<cat_str *> &, std::map<std::string, std::string> &);
    const char * getName() const { return "lastentry"; }
  };
  /** Documentation for function can be found in menu manual */
  struct level : public funcN<0> {
    std::ostream &output(std::ostream &o, std::vector<cat_str *> &, std::map<std::string, std::string> &);
    const char * getName() const { return "level"; }
  };
  /** Documentation for function can be found in menu manual */
  struct rcfile : public funcN<0> {
    std::ostream &output(std::ostream &o, std::vector<cat_str *> &, std::map<std::string, std::string> &);
    const char * getName() const { return "rcfile"; }
  };
  /** Documentation for function can be found in menu manual */
  struct examplercfile : public funcN<0> {
    std::ostream &output(std::ostream &o, std::vector<cat_str *> &, std::map<std::string, std::string> &);
    const char * getName() const { return "examplercfile"; }
  };
  /** Documentation for function can be found in menu manual */
  struct mainmenutitle : public funcN<0> {
    std::ostream &output(std::ostream &o, std::vector<cat_str *> &, std::map<std::string, std::string> &);
    const char * getName() const { return "mainmenutitle"; }
  };
  /** Documentation for function can be found in menu manual */
  struct rootsection : public funcN<0> {
    std::ostream &output(std::ostream &o, std::vector<cat_str *> &, std::map<std::string, std::string> &);
    const char * getName() const { return "rootsection"; }
  };
  /** Documentation for function can be found in menu manual */
  struct rootprefix : public funcN<0> {
    std::ostream &output(std::ostream &o, std::vector<cat_str *> &, std::map<std::string, std::string> &);
    const char * getName() const { return "rootprefix"; }
  };
  /** Documentation for function can be found in menu manual */
  struct userprefix : public funcN<0> {
    std::ostream &output(std::ostream &o, std::vector<cat_str *> &, std::map<std::string, std::string> &);
    const char * getName() const { return "userprefix"; }
  };
  /** Documentation for function can be found in menu manual */
  struct treewalk : public funcN<0> {
    std::ostream &output(std::ostream &o, std::vector<cat_str *> &, std::map<std::string, std::string> &);
    const char * getName() const { return "treewalk"; }
  };
  /** Documentation for function can be found in menu manual */
  struct postoutput : public funcN<0> {
    std::ostream &output(std::ostream &o, std::vector<cat_str *> &, std::map<std::string, std::string> &);
    const char * getName() const { return "postoutput"; }
  };
  /** Documentation for function can be found in menu manual */
  struct preoutput : public funcN<0> {
    std::ostream &output(std::ostream &o, std::vector<cat_str *> &, std::map<std::string, std::string> &);
    const char * getName() const { return "preoutput"; }
  };
  /** Documentation for function can be found in menu manual */
  struct cwd : public funcN<0> {
    std::ostream &output(std::ostream &o, std::vector<cat_str *> &, std::map<std::string, std::string> &);
    const char * getName() const { return "cwd"; }
  };
  /** Documentation for function can be found in menu manual */
  struct translate : public funcN<2> {
    std::ostream &output(std::ostream &o, std::vector<cat_str *> &, std::map<std::string, std::string> &);
    const char * getName() const { return "translate"; }
  };

}

#endif
