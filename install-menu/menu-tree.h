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
