#ifndef MENU_TREE_H
#define MENU_TREE_H

#include <map>
#include "adstring.h"

class menuentry;

bool operator<(const StrVec &left, const StrVec &right);

/* submenu_container:
  StrVec    first   - full title, broken into a StrVec
  menuentry *second - the menu entry with this title
*/
typedef std::map<StrVec, menuentry *> submenu_container;

class menuentry {
  char hotkeyconv(char);
  void generate_hotkeys();
  void store_hints();
  StrVec menuhints;

public:
  menuentry() : forced(false) { }

  submenu_container submenus;
  std::map<string, string> vars;
  bool forced;

  void add_entry(StrVec sections, std::map<string, string>& entry_vars);
  void add_entry_ptr(StrVec sections, menuentry *entry);
  void output();
  void postprocess(int n_parent, int level, const string& prev_section);
  void process_hints();
  void debug(int);
};


#endif /* MENU_TREE_H */
