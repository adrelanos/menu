/*
 * Debian menu system -- install-menu
 * install-menus/menu-tree.h
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

#ifndef MENU_TREE_H
#define MENU_TREE_H

#include <vector>
#include <string>
#include <map>

class menuentry;

bool operator<(const std::vector<std::string>& left, const std::vector<std::string>& right);

/* submenu_container:
  vector<string> first   - full title, broken into a vector of strings
  menuentry*     second  - the menu entry with this title
*/
typedef std::map<std::vector<std::string>, menuentry *> submenu_container;

class menuentry {
  char hotkeyconv(char);
  void generate_hotkeys();
  void store_hints();
  std::vector<std::string> menuhints;

public:
  menuentry() : forced(false) { }

  submenu_container submenus;
  std::map<std::string, std::string> vars;
  bool forced;

  void add_entry(std::vector<std::string> sections, std::map<std::string, std::string>& entry_vars);
  void add_entry_ptr(std::vector<std::string> sections, menuentry *entry);
  void output();
  void postprocess(int n_parent, int level, const std::string& prev_section);
  void process_hints();
  void debug(int);
};


#endif /* MENU_TREE_H */
