/*
 * Debian menu system -- update-menus and install-menu
 * update-menus/adstring.cc
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

#include <algorithm>
#include <iomanip>
#include <sstream>
#include <cctype>
#include <clocale>
#include <cstdio>
#include <cstdlib>
#include "adstring.h"

using std::cerr;
using std::endl;
using std::string;

Regex::Regex(const char *s)
{
  patt = new re_pattern_buffer;
  patt->translate = 0;
  patt->fastmap = 0;
  patt->buffer = 0;
  patt->allocated = 0;
  re_compile_pattern(s, strlen(s), patt);
}

// ************* std::string utility functions:

bool contains(const std::string& str, const std::string &sub, std::string::size_type pos)
{
  std::string::size_type i;

  if (str.length() < (sub.length()+pos))
      return false;

  for (i=0; (i < sub.length()) && (sub[i] == str[i+pos]); ++i);

  return i == sub.length();
}

bool contains(const std::string& str, char c)
{
  return str.find(c) != std::string::npos;
}

string rmtrailingspace(string &s)
{
  while(!s.empty() && (isspace(s[s.length()-1])))
    s.erase(s.length()-1,1);
  return s;
}

string escapewith_string(const string& s, const string& esc, const string& with)
{
  // call with: escape_string("hello $world, %dir", "$%", "\\")
  // returns:   "hello \$world, \%dir"
  string t;
  for (string::size_type i = 0; i != s.length(); ++i)
  {
    if (esc.find(s[i]) != string::npos)
        t += with;
    t += s[i];
  }
  return t;
}

string escape_string(const string &s, const string &esc)
{
  // call with: escape_string("hello $world, %dir", "$%")
  // returns:   "hello \$world, \%dir"
  return escapewith_string(s, esc, "\\");
}

string cppesc_string(const string &s)
{
  string t;
  for (string::size_type i = 0; i!= s.length(); ++i)
  {
    if (!(isalnum(s[i])||(s[i]=='_')))
        t += '$'+itohexstring(int(s[i]));
    else
        t += s[i];
  }
  return t;
}

string lowercase(string s)
{
  string t;
  for (string::size_type i = 0; i < s.length(); ++i)
      t += char(tolower(s[i]));
  return t;
}

string uppercase(string s)
{
  string t;
  for (string::size_type i = 0; i < s.length(); ++i)
      t += char(toupper(s[i]));
  return t;
}

string replacewith_string(const string &s, const string &replace, const string &with)
{
  // call with: replacewith_string("hello $world, %dir", "$% ", "123")
  // returns:   "hello31world,32dir"

  if (replace.length() != with.length())
      throw except_string(_("replacewith($string, $replace, $with): $replace and $with must have the same length."));

  string t = s;
  for (string::size_type i = 0; i <= replace.length(); ++i)
  {
    std::replace(t.begin(), t.end(), replace[i], with[i]);
  }
  return t;
}

string replace(string s,char match, char replace)
{
  std::replace(s.begin(), s.end(), match, replace);
  return s;
}

string sort_hotkey(string str)
{
  string t;
  string::size_type i;
  string::size_type l = str.length();
  char *s=strdup(str.c_str());

  if(!l)
    return t;

  t=string("")+s[0];
  s[0]='\0';
  for(i=1;i!=l;i++)
    if((isspace(s[i-1])||ispunct(s[i-1]))&&isupper(s[i])){
      t+=s[i];
      s[i]='\0';
    }
  for(i=1;i!=l;i++)
    if((isspace(s[i-1])||ispunct(s[i-1]))&&isalnum(s[i])){
      t+=s[i];
      s[i]='\0';
    }
  for(i=1;i!=l;i++)
    if(isupper(s[i])){
      t+=s[i];
      s[i]='\0';
    }
  for(i=1;i!=l;i++)
    if(isalpha(s[i])){
      t+=s[i];
      s[i]='\0';
    }
  for(i=1;i!=l;i++)
    if(isalnum(s[i])){
      t+=s[i];
      s[i]='\0';
    }
  for(i=1;i!=l;i++)
    if(s[i]){
      t+=s[i];
      s[i]='\0';
    }
  free(s);
  return t;
}

int stringtoi(const string &s)
{
  return atoi(s.c_str());
}

string itostring(int i)
{
  std::ostringstream o;
  o << i;
  return o.str();
}

string itohexstring(int i)
{
  std::ostringstream o;
  o << std::hex << i;
  return o.str();
}


string string_parent(string s)
{
  // string_parent("/Debian/Apps/Editors/Emacs") = "/Debian/Apps/Editors
  string::size_type  i,p;

  for (i = 0, p = string::npos; i != s.length(); i++)
      if (s[i] == '/')
          p = i;
  if (p == string::npos)
      return "";
  else
      return s.substr(0, p);
}

string string_basename(string s)
{
  string t;
  string::size_type i;
  string::size_type p; //points to last encountered '/'
  string::size_type q; //points to last-but-one encountered '/'.

  for(i=0,p=string::npos,q=0;i!=s.length();i++)
    if(s[i]=='/') {
      q=p;
      p=i;
    }
  if(p==string::npos)
    return "";
  else
    return s.substr(q+1,p);
}
string string_stripdir(string s)
{
  string t;
  string::size_type i; 
  string::size_type p; //points to last encountered '/'
  
  for(i=0,p=string::npos;(string::size_type)i!=s.length();i++)
    if(s[i]=='/')
      p=i;
  if(p==string::npos)
    return s;
  else
    return s.substr(p+1,s.length());
}

void break_char(const string &sec, std::vector<string>& sec_vec, char breakchar)
{
  string::size_type i,j;

  if (sec.empty())
      return;

  if (sec[0] == breakchar) // ignore first occurence of breachar
      i = 1;
  else
      i = 0;

  while (true)
  {
    while ((i<sec.size())&&(isspace(sec[i])))
        i++;
    j = sec.find(breakchar,i);
    if (j != string::npos) {
        sec_vec.push_back(sec.substr(i, j-i));
    } else {
      if (i != sec.size())
          sec_vec.push_back(sec.substr(i));
      break;
    }
    i = j + 1;
  }
}

void break_slashes(const string &sec, std::vector<string>& sec_vec)
{
  break_char(sec,sec_vec,'/');
}

void break_commas(const string &sec, std::vector<string>& sec_vec)
{
  break_char(sec,sec_vec,',');
}

void except_pi::report()
{
  if (pi) {
    cerr << String::compose(_("In file \"%1\", at (or in the definition that ends at) line %2:\n"),
        pi->filename(),
        pi->linenumber());

    string::size_type startpos = 0;
    if (pi->pos > 50)
        startpos = pi->pos - 50;

    if (startpos)
        cerr << "[...]";

    cerr << pi->buffer.substr(startpos) << endl;

    if (startpos)
        cerr << "[...]";

    for (string::size_type i = 1+startpos; i< pi->pos; ++i)
        cerr << ' ';

    cerr << '^' << endl;
  } else {
    cerr << _("Somewhere in input file:\n");
  }
  cerr << message() << endl;
}

parsestream::parsestream(std::istream &in, string other)
  : in_constructor(true), stdin_file(true), otherdir(other), doescaping(true)
{
  init(&in,_("(probably) stdin"));
  in_constructor = false;
}

parsestream::parsestream(const string &name, string other)
  : in_constructor(true), stdin_file(false), otherdir(other), doescaping(true)
{
  std::istream *f = new std::ifstream(name.c_str());

  if(!f && (name[0] != '/')) {
    string add = other + "/" + name;
    f = new std::ifstream(add.c_str());
  }
  if (!*f)
      throw ferror_open(String::compose(_("Cannot open file %1."), name));

  init(f, name);
  in_constructor = false;
}

void parsestream::preprocess(string &s)
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

void parsestream::new_file(const string &name)
{
  std::ifstream *f = new std::ifstream(name.c_str());

  init(f,name);
}

void parsestream::init(std::istream *in, string name)
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
      // buffer.at(pos,1)=string(c);
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
    while((c = get_char())) {
      if(re_match(r.pattern(), str, 1, 0, 0) > 0)
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
  if(c != '=')
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
  char buf[2]="a";
  char c = get_char();
  if(c!=expect){
    put_back(c);
    buf[0]=c;
    throw char_expected(this, buf);
  }
}

const char *ldgettext(const char *lang, const char *domain, const char *msgid)
{
  /* this code comes from the gettext info page. It looks
     very inefficient (as though for every language change a new
     catalog file is opened), but tests show adding a language
     change like this doesn't get performance down very much
     (runtime goes `only' about 70% up, if switching between 2 
     languages, as compared to no swiching at all).
  */
  /* Change language.  */
  setenv ("LANGUAGE", lang, 1);
  /* Make change known.  */
  {
    extern int  _nl_msg_cat_cntr;
    ++_nl_msg_cat_cntr;
  }
  return dgettext(domain, msgid);
}
