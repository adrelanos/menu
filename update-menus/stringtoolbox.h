/*
 * Debian menu system -- update-menus and install-menu
 * update-menus/stringtoolbox.h
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

#ifndef STRINGTOOLBOX_H
#define STRINGTOOLBOX_H

#include <vector>
#include <string>

/* Check whether str contains sub at pos. */ 
bool contains(const std::string& str, const std::string &sub, std::string::size_type pos = 0);

/* Check whether str contains c */
bool contains(const std::string& str, char c);

/* Remove all whitespace at end of str */
std::string rmtrailingspace(std::string str);

/* Search str and escape all characters in esc with the string 'with'
 *
 * Call with: escapewith("hello $world, %dir", "$%", "\\")
 * Returns:   "hello \$world, \%dir"
 */
std::string escapewith(const std::string &str, const std::string &esc, const std::string &with);

/* Search str and escape all characters in esc with backspace (\).
 *
 * Call with: escape("hello $world, %dir", "$%")
 * Returns:   "hello \$world, \%dir"
 */
std::string escape(const std::string &s, const std::string &esc);

/* Return str in lowercase. */
std::string lowercase(std::string str);

/* Return str in uppercase. */
std::string uppercase(std::string str);

/* Search str. For each character in replace, substitute it with the
 * corresponding character in with.
 *
 * Call with: replacewith_string("hello $world, %dir", "$% ", "123")
 * Returns:   "hello31world,32dir"
 */
std::string replacewith(std::string str, const std::string &replace, const std::string &with);

/* Search str. Replace all occurences of repl with with. */
std::string replace(std::string str, const std::string& repl, const std::string& with);

/* Escape anything that isn't a letter, number or _ with $<hex-ascii-code>.
 * So for example '-' is replaced by '$2D'. */
std::string cppesc(const std::string &s);

/* Return the integer representation of str. If conversion is not possible,
 * returns 0. */
int stringtoi(const std::string &str);

/* Returns the string representation of i. */
std::string itostring(int i);

/* Return the string representation of i, in hexidecimal. */
std::string itohexstring(int i);

/* FIXME: what does this function do? */
std::string sort_hotkey(std::string s);

/* Returns the 'parent' of a string in the form /<foo>/<bar> where '/'
 * separates parents.
 *
 * Call with: string_parent("/Debian/Apps/Editors/Emacs")
 * Returns:   "/Debian/Apps/Editors"
 */
std::string string_parent(const std::string& str);

/* FIXME: what does this function do? */
std::string string_basename(const std::string& str);

/* Returns the last element of a string in the form /<foo>/<bar> where '/'
 * separates elements.
 *
 * Call with: string_stripdir("/Debian/Apps/Editors/Emacs")
 * Returns:   "Emacs"
 */
std::string string_stripdir(const std::string& str);

/* Tokenize a string str with a separater breakchar. Insert all tokens into
 * the vector passed to the function by reference. */
void break_char(const std::string& str, std::vector<std::string>& container, char breakchar);

#endif /* STRINGTOOLBOX_H */
