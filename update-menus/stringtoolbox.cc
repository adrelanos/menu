/*
 * Debian menu system -- update-menus and install-menu
 * update-menus/stringtoolbox.cc
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
#include "exceptions.h"
#include "stringtoolbox.h"

using std::string;

bool contains(const string& str, const string& sub, string::size_type pos)
{
  if (str.length() < (sub.length()+pos))
      return false;

  return (str.substr(pos, sub.length()) == sub);
}

bool contains(const string& str, char c)
{
  return str.find(c) != string::npos;
}

string rmtrailingspace(string str)
{
  while(!str.empty() && (isspace(str[str.length()-1])))
    str.erase(str.length()-1);
  return str;
}

string escapewith(const string& str, const string& esc, const string& with)
{
  string t;
  for (string::size_type i = 0; i != str.length(); ++i)
  {
    if (esc.find(str[i]) != string::npos)
        t += with;
    t += str[i];
  }
  return t;
}

string escape(const string &str, const string &esc)
{
  return escapewith(str, esc, "\\");
}

string lowercase(string str)
{
  std::transform(str.begin(), str.end(), str.begin(), tolower);
  return str;
}

string uppercase(string str)
{
  std::transform(str.begin(), str.end(), str.begin(), toupper);
  return str;
}

string replacewith(string str, const string &replace, const string &with)
{
  if (replace.length() != with.length())
      throw except_string(_("replacewith($string, $replace, $with): $replace and $with must have the same length."));

  for (string::size_type i = 0; i <= replace.length(); ++i)
  {
    std::replace(str.begin(), str.end(), replace[i], with[i]);
  }
  return str;
}

string replace(string str, const string& repl, const string& with)
{
  string::size_type pos = str.find(repl);

  if (pos == string::npos)
      return str;

  return replace(str.replace(pos, repl.length(), with), repl, with);
}

string cppesc(const string &s)
{
  string t;
  for (string::size_type i = 0; i!= s.length(); ++i)
  {
    if (!(isalnum(s[i]) || (s[i] == '_')))
        t += '$' + itohexstring(int(s[i]));
    else
        t += s[i];
  }
  return t;
}

int stringtoi(const string &str)
{
  return atoi(str.c_str());
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

string string_parent(const string& str)
{
  string::size_type pos = str.find_last_of('/');
  if (pos == string::npos)
      return "";
  else
      return str.substr(0, pos);
}

string string_basename(const string& str)
{
  string t;
  string::size_type i;
  string::size_type p; //points to last encountered '/'
  string::size_type q; //points to last-but-one encountered '/'.

  for(i=0,p=string::npos,q=0;i!=str.length();i++)
    if(str[i]=='/') {
      q=p;
      p=i;
    }
  if(p==string::npos)
    return "";
  else
    return str.substr(q+1,p);
}

string string_stripdir(const string& str)
{
  string::size_type pos = str.find_last_of('/');
  if (pos == string::npos)
      return str;
  else
      return str.substr(pos+1);
}

void break_char(const string &str, std::vector<string>& container, char breakchar)
{
  string::size_type lastPos = str.find_first_not_of(breakchar, 0);
  string::size_type pos = str.find_first_of(breakchar, lastPos);

  while (string::npos != pos || string::npos != lastPos)
  {
    container.push_back(str.substr(lastPos, pos - lastPos));

    lastPos = str.find_first_not_of(breakchar, pos);
    pos = str.find_first_of(breakchar, lastPos);
  }
}
