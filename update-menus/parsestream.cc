/*
 * Debian menu system -- update-menus and install-menu
 * update-menus/parsestream.cc
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

#include <fstream>
#include <cstring>
#include "parsestream.h"
#include "stringtoolbox.h"

using std::string;
using namespace exceptions;

Regex::Regex(const char *s)
{
  patt = new re_pattern_buffer;
  patt->translate = 0;
  patt->fastmap = 0;
  patt->buffer = 0;
  patt->allocated = 0;
  re_set_syntax(RE_CHAR_CLASSES);
  re_compile_pattern(s, std::strlen(s), patt);
}

parsestream::parsestream(std::istream &in, std::string other)
  : in_constructor(true), stdin_file(true), otherdir(other), doescaping(true)
{
  init(&in,_("(probably) stdin"));
  in_constructor = false;
}

parsestream::parsestream(const std::string &name, std::string other)
  : in_constructor(true), stdin_file(false), otherdir(other), doescaping(true)
{
  std::istream *f = new std::ifstream(name.c_str());

  if(!f && (name[0] != '/')) {
    string add = other + "/" + name;
    f = new std::ifstream(add.c_str());
  }
  if (!*f)
      throw ferror_open(name);

  init(f, name);
  in_constructor = false;
}

void parsestream::preprocess(std::string &s)
{
  // Disregards lines that start with a #
  // Set filename if line starts with "!F"

  string compat;
  string::size_type i = 0;
  while ((i < s.length()) && isspace(s[i]))
      i++;
  if (i)
      s = s.substr(i);

  if (s.empty())
      return;

  switch (s[0])
  {
    case '#':
      s.erase();
      return;
    case '!':
      if (!s.empty()) {
        switch (s[1])
        {
          case 'F':
            set_linenumber(0);
            set_filename(s.substr(3));
            //cerr<<"filename="<<filename()<<", s.substr="<<s.substr(2)<<endl;
            s.erase();
            return;
          case 'L':
            set_linenumber(stringtoi(s.substr(2))-1);
            //cerr<<"filename="<<filename()<<", s.substr="<<s.substr(2)<<endl;
            s.erase();
            return;
          case 'C':
            compat = s.substr(3);
            if (compat == "menu-1")
                seteolmode(eol_newline);
            else if (compat == "menu-2")
                seteolmode(eol_semicolon);
            else
                throw unknown_compat(this, compat);
            s.erase();
            return;
          default:
            if (contains(s, "!include")) {
              string t(s.substr(strlen("!include ")));
              if (t[0] == '/' || filenames.empty()) {
                new_file(t);
              } else {
                string name = string_parent(filename()) + '/' + t;
                if (std::ifstream(name.c_str()))
                    new_file(name);
                else
                    new_file(otherdir + '/' + t);
              }
            }
            return;
        }
      }
      return;
    default:;
  }
}

void parsestream::new_file(const std::string &name)
{
  std::ifstream *f = new std::ifstream(name.c_str());

  init(f,name);
}

void parsestream::init(std::istream *in, std::string name)
{
  if (!in_constructor)
      if (!in->good())
          throw ferror_open(name);
  pos = 0;
  eolmode = eol_newline;
  istreams.push_back(in);
  linenumbers.push_back(0);
  filenames.push_back(name);

  new_line();
}

void parsestream::close_file()
{
  if (!stdin_file && !istreams.empty())
      delete istreams.back();

  if (istreams.size() > 1) {
    istreams.pop_back();
    linenumbers.pop_back();
    filenames.pop_back();
  } else {
    throw endoffile(this);
  }
}

void parsestream::new_line()
{
  while (!istreams.empty())
  {
    if (!current_istr()->good()) {
      close_file();
      continue;
    }
    buffer.erase();
    pos = 0;
    while (current_istr()->good() && buffer.empty())
    {
      getline(*current_istr(), buffer);
      set_linenumber(linenumber()+1);
      preprocess(buffer);
      buffer = rmtrailingspace(buffer);
    }
    while (current_istr()->good() &&
        (((eolmode==eol_newline) && (buffer[buffer.length()-1]=='\\')) ||
         ((eolmode==eol_semicolon) && (buffer[buffer.length()-1]!=';'))))
    {
      string s;
      getline(*current_istr(), s);
      set_linenumber(linenumber()+1);
      switch(eolmode)
      {
      case eol_newline:
        buffer = buffer.substr(0,buffer.length()-1) + ' ' + rmtrailingspace(s);
        break;
      case eol_semicolon:
        buffer = buffer + ' ' + rmtrailingspace(s);
        break;
      }
    }
    if (buffer.empty()) {
      close_file();
      continue;
    }
    if (current_istr()->eof() && !buffer.empty() &&
       (((eolmode==eol_newline)&&(buffer[buffer.length()-1]=='\\'))||
        ((eolmode==eol_semicolon)&&(buffer[buffer.length()-1]!=';')))){
      //a "\" at the end of a file: unconditional error (don't unwind etc)
      //(or no ; at eof, when eolmode=; . Same unconditional error.
      throw endoffile(this);
    }
    if (eolmode == eol_semicolon)
        if (buffer[buffer.length()-1] == ';')
            buffer.erase(buffer.length()-1, 1);
    return;
  }
  if (in_constructor)
      return;
  else
      throw endoffile(this);
}

char parsestream::get_char()
{
  if (buffer.empty())
    throw endoffile(this);
  if (pos >= buffer.length())
    throw endofline(this);

  return buffer[pos++];
}

char parsestream::put_back(char c)
{
  if (c) {
    if (pos) {
      pos--;
      buffer[pos] = c;
    } else {
      buffer = c + buffer;
    }
  }
  return c;
}

string parsestream::get_line()
{
  string s = buffer.substr(pos);
  if (s.empty())
    throw endofline(this);
  buffer.erase();
  try {
    skip_line();
  } catch (endoffile) { }
  return s;
}

string parsestream::get_name()
{
  char c;
  string s;
  skip_space();
  try {
    while((c=get_char()) &&
        (isalnum(c)||(c=='_')||(c=='-')||(c=='+')||(c=='.')))
      s+=c;
    if(c)
      put_back(c);
  } catch (endofline d) { }
  return s;
}

string parsestream::get_name(const Regex &r)
{
  char str[2]={0,0};
  char &c=str[0];
  string s;
  skip_space();
  try {
    while ((c = get_char()))
    {
      if (re_match(r.pattern(), str, 1, 0, 0) > 0)
          s += c;
      else
          break;
    }
    if (c)
      put_back(c);
  } catch (endofline) { }
  return s;
}

string parsestream::get_eq_name()
{
  skip_space();
  char c = get_char();
  if (c != '=')
    throw char_expected(this, "=");
  return get_name();
}


string parsestream::get_stringconst()
{
  string s;
  skip_space();
  char c = get_char();
  try {
    if (c == '\"') { // " found at the beginning
      while ((c = get_char()) && (c != '\"'))
      {
        if (c == '\\') {
          c = get_char();
          switch (c)
          {
            case 't': c = '\t'; break;
            case 'b': c = '\b'; break;
            case 'n': c = '\n'; break;
            default:
                      if (!doescaping)
                        s += '\\';
                      break;
          }
        }
        s += c;
      }
      if (c != '\"')
          throw char_expected(this, "\"");
    } else { // no " at beginning
      s = c;
      while ((c = get_char()) && !isspace(c))
          s += c;
    }
  } catch (endofline p) { }

  return s;
}

string parsestream::get_eq_stringconst()
{
  skip_space();
  char c = get_char();
  if (c != '=')
      throw char_expected(this, "=");
  return get_stringconst();
}

bool parsestream::get_boolean()
{
  string s = get_name();

  if (s == "true")
    return true;
  else if (s == "false")
    return false;
  else
    throw boolean_expected(this, s);
}

bool parsestream::get_eq_boolean()
{
  skip_space();
  char c = get_char();
  if (c != '=')
      throw char_expected(this,"=");
  return get_boolean();
}

int parsestream::get_integer()
{
  char c;
  string s;

  try {
    skip_space();
    while((c=get_char())&&((isdigit(c)||(c=='-'))))
      s += c;
  } catch (endofline d) { }

  return atoi(s.c_str());
}

int parsestream::get_eq_integer()
{
  skip_space();
  char c = get_char();
  if (c != '=')
      throw char_expected(this, "=");
  return get_integer();
}

double parsestream::get_double()
{
  char c;
  string s;

  skip_space();
  try {
    while ((c=get_char()) &&
        (isdigit(c) || (c=='.')||(c=='E')||(c=='e')||(c=='+')||(c=='-')))
      s += c;
  } catch (endofline d) { }

  return atof(s.c_str());
}

double parsestream::get_eq_double()
{
  skip_space();
  char c = get_char();
  if(c!='=')
    throw char_expected(this, "=");

  return get_double();
}

void parsestream::skip_line()
{
  buffer.erase();
  try {
    new_line();
  } catch (endoffile d) { }
}

void parsestream::skip_space()
{
  char c;
  while (isspace(c=get_char()));
  if(c)
    put_back(c);
}

void parsestream::skip_char(char expect)
{
  char c = get_char();
  if(c != expect) {
    put_back(c);
    char buf[2] = {c, '\0'};
    throw char_expected(this, buf);
  }
}

void except_pi::report()
{
  if (pi) {
    std::cerr << String::compose(_("In file \"%1\", at (or in the definition that ends at) line %2:\n"),
        pi->filename(),
        pi->linenumber());

    string::size_type startpos = 0;
    if (pi->pos > 50)
        startpos = pi->pos - 50;

    if (startpos)
        std::cerr << "[...]";

    std::cerr << pi->buffer.substr(startpos) << std::endl;

    if (startpos)
        std::cerr << "[...]";

    for (string::size_type i = 1+startpos; i< pi->pos; ++i)
        std::cerr << ' ';

    std::cerr << '^' << std::endl;
  } else {
    std::cerr << _("Somewhere in input file:\n");
  }
  std::cerr << message() << std::endl;
}
