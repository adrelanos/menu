/*
 * Debian menu system -- install-menu
 * install-menus/menu-tree.cc
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

#include <cctype>
#include <list>
#include <set>
#include "menu-tree.h"
#include "install-menu.h"
#include "hints.h"
#include <stringtoolbox.h>

using std::vector;
using std::string;
using std::list;
using std::set;
using std::map;

string sort_hotkey(const string& str)
{
  string t;
  string::size_type i;
  string::size_type l = str.length();
  char *s = strdup(str.c_str());

  if (str.empty())
      return t;

  t = s[0];
  s[0] = '\0';
  for (i=1; i!=l ;i++)
    if ((isspace(s[i-1]) || ispunct(s[i-1])) && isupper(s[i])) {
      t += s[i];
      s[i] = '\0';
    }
  for (i=1; i!=l; i++)
    if ((isspace(s[i-1]) || ispunct(s[i-1])) && isalnum(s[i])) {
      t += s[i];
      s[i] = '\0';
    }
  for (i=1; i!=l; i++)
    if (isupper(s[i])) {
      t += s[i];
      s[i] = '\0';
    }
  for (i=1; i!=l; i++)
    if (isalpha(s[i])) {
      t += s[i];
      s[i] = '\0';
    }
  for (i=1; i!=l; i++)
    if (isalnum(s[i])) {
      t += s[i];
      s[i] = '\0';
    }
  for (i=1; i!=l; i++)
    if (s[i]) {
      t += s[i];
      s[i] = '\0';
    }
  free(s);
  return t;
}

/** Adds a new entry in the menu hierarchy.
 *
 * Arguments:
 *
 *     sections: A vector of strings, holding the section names.
 *     entry_vars: Variables of the entry to be added.
 * 
 * There are two ways to store the menuentries in the `tree'
 * - `flat', all menuentries in the toplevel submenus map.
 *   This is used when working with `hints'. In this case, we
 *   will later sort out the tree.
 * - as a true tree.
 * However, even in the `flat', hint-processing case, if there already
 * exists a submenu with exactly the right sections[0], then we do desent,
 * if it has a entry_vars[FORCED_VAR] set.
 */
void menuentry::add_entry(std::vector<std::string> sections, std::map<std::string, std::string> &entry_vars)
{
  map<string, string>::iterator vi;
  map<string, string>::iterator vj;
  bool use_forced = false;

  if (sections.empty()) {
    // No sections defined, this means that we probably are at the last
    // node, so we just need to fill in variables that aren't already
    // defined.
    for (vi = entry_vars.begin(); vi != entry_vars.end(); ++vi)
    {
      if (vi->first == TITLE_VAR)
          continue;
      if (vi->first == SECTION_VAR)
          continue;

      vj = vars.find(vi->first);
      if ((vj == vars.end()) || vj->second.empty() || (vj->second == "none"))
          vars[vi->first] = vi->second;
    }
    return;
  }

  vector<string> firstsection;
  firstsection.push_back(sections[0]);
  submenu_container::iterator firstsection_i = submenus.find(firstsection);

  if (firstsection_i != submenus.end())
      if (firstsection_i->second->forced)
          use_forced = true;

  if ((!menumethod->hint_optimize || use_forced) && (sections.size() > 1)) {
    // We are either not using hint_optimize, or we have to add this
    // section, because it's forced to be created by the forcetree.
    //
    // And because sections.size() > 1, there are more sections to add.

    // Add new subsections to our new section (but strip the first one)
    vector<string> subsections;
    for (vector<string>::iterator i = sections.begin()+1; i != sections.end(); ++i)
        if (!i->empty())
            subsections.push_back(*i);

    // If the first subsection doesn't already exist, then create a new one.
    if (firstsection_i == submenus.end()) {
      submenus[firstsection] = new menuentry;
      submenus[firstsection]->vars[TITLE_VAR] = firstsection[0];
    }

    submenus[firstsection]->add_entry(subsections, entry_vars);
  } else {
    submenu_container::iterator f = submenus.find(sections);
    if (f == submenus.end()) {
      // Add a new entry, because these sections doesn't exist.
      submenus[sections] = new menuentry;
      submenus[sections]->vars = entry_vars;
    } else {
      // Modifying an already existing entry. Add all variables
      // that aren't already defined.
      for (vi = entry_vars.begin(); vi != entry_vars.end(); ++vi)
      {
        menuentry *m = f->second;
        vj = m->vars.find(vi->first);
        if ((vj==m->vars.end()) || vj->second.empty() || (vj->second == "none"))
            m->vars[vi->first] = vi->second;
      }
    }
  }
}

/** Adds a new entry in the menu hierarchy, using an already created
 * menuentry pointer.
 *
 * Arguments:
 *
 *     sections: A vector of strings, holding the section names.
 *     entry: a pointer to a menuentry.
 */
void menuentry::add_entry_ptr(std::vector<std::string> sections, menuentry *entry)
{
  if (sections.size() > 1) {
    // There are more sections to add...

    // Add new subsections to our new section (but strip the first one)
    vector<string> subsections;
    for (vector<string>::iterator i = sections.begin()+1; i != sections.end(); ++i)
        if (!i->empty())
            subsections.push_back(*i);

    // If the first subsection doesn't already exist, then create a new one.
    vector<string> firstsection;
    firstsection.push_back(sections[0]);
    if (submenus.find(firstsection) == submenus.end()) {
      submenus[firstsection] = new menuentry;
      submenus[firstsection]->forced = entry->forced;
      submenus[firstsection]->vars[TITLE_VAR] = firstsection[0];
    }
    submenus[firstsection]->add_entry_ptr(subsections, entry);
  } else if (submenus.find(sections) == submenus.end()) {
    // Add a new entry, because this section doesn't exist.
    submenus[sections] = entry;
  }
}

struct menusort {

  bool operator()(std::string s1, std::string s2) const {
    return strcoll(s1.c_str(), s2.c_str())<0;
  }
};

/** Output menu tree.
 *
 * Uses the 'treewalk' variable to define what to output in which order,
 * this documentation was taken from the menu manual:
 *
 *   `treewalk="c(m)"'
 *        This string defines in what order to dump the `$startmenu',
 *        `$endmenu', and `$submenutitle' (and its children).  Each char in
 *        the string refers to:
 *
 *  c  : dump children of menu.
 *  m  : dump this menu's $submenutitles
 *  (  : dump $startmenu
 *  )  : dump $endmenu
 *  M  : dump all $submenutitles of this menu and this menu's children.
 *
 *       The default is "c(m)".  For olvwm, one needs: "(M)"
 */
void menuentry::output()
{
  string treew = menumethod->treewalk();
  submenu_container::iterator sub_i;
  std::multimap<string, menuentry *, menusort> sorted;
  std::multimap<string, menuentry *>::iterator i;

  // Sort the submenus in sorted:
  for (sub_i = submenus.begin(); sub_i != submenus.end(); ++sub_i)
  {
    string s;
    if (menumethod->sort)
      s = menumethod->sort->soutput(sub_i->second->vars);
    else
      s = sub_i->second->vars[SORT_VAR] + ':' + sub_i->second->vars[TITLE_VAR];

    sorted.insert(std::pair<string, menuentry *>(s, sub_i->second));
  }

  // Output the menu according to the treewalk variable.
  for (string::size_type j = 0; j < treew.length(); ++j)
  {
    bool children_too = false;
    switch (treew[j])
    {
      case 'c':
        for (i = sorted.begin(); i != sorted.end(); ++i)
            if (!i->second->submenus.empty())
                i->second->output();
        break;
      case '(':
        if (!submenus.empty())
            if (menumethod->startmenu && testuniqueness(vars))
                menumethod->startmenu->output(vars);
        break;
      case ')':
        if (!submenus.empty())
            if (menumethod->endmenu && testuniqueness(vars))
                menumethod->endmenu->output(vars);
        break;
      case 'M':
        children_too = true;
      case 'm':
        for (i = sorted.begin(); i != sorted.end(); ++i)
        {
          if (!i->second->vars[COMMAND_VAR].empty() &&
              testuniqueness(i->second->vars)) {
            supported->subst(i->second->vars);
          } else {
            if (menumethod->submenutitle && !i->second->submenus.empty() &&
                testuniqueness(i->second->vars))
                menumethod->submenutitle->output(i->second->vars);
            if (children_too)
                i->second->output();
          }
        }
    }
  }
}

/** Put the $hint variable contents in the hint of the submenu[] map. For
 * submenu entries (without a command), put the $hint variable in all
 *  menuentries that lie below that one.
 */
void menuentry::store_hints()
{
  submenu_container::iterator i, j;
  unsigned int l;

  // Make sure menuhints are empty
  for(i = submenus.begin(); i != submenus.end(); ++i)
      i->second->menuhints.erase(i->second->menuhints.begin(), i->second->menuhints.end());

  for (i = submenus.begin(); i != submenus.end(); ++i)
  {
    const string &hints_str = i->second->vars[HINTS_VAR];

    if (!hints_str.empty()) {
      vector<string> hints;

      break_char(hints_str, hints, ',');

      j = i;

      do {
        for (vector<string>::iterator k = hints.begin(); k != hints.end(); ++k)
            j->second->menuhints.push_back(*k);

        j++;
        if (j == submenus.end())
            break;

        for (l = 0; l != i->first.size(); ++l)
            if (i->first[l] != j->first[l])
                break;
      } while (l == i->first.size());

    }
  }
}

/** Process the menu hierarchy through the hints algorithms.
 *
 * Basically, it has 4 steps (also marked as comments in the code):
 *
 *  Step 1: Fetch appropriate sections to put in hint_list vector.
 *  Step 2: Initialize hints class and calculate a new tree, using hint_list.
 *  Step 3: Remove all submenus.
 *  Step 4: Using the new hint_out vector, initialize our new sorted tree,
 *  thus re-creating the previously removed submenus (but in a different
 *  order).
 */
void menuentry::process_hints()
{
  vector<vector<string> > hint_list, hint_out;
  vector<vector<string> >::iterator k, m;
  vector<string>::const_iterator l;
  submenu_container::iterator i;

  // First, process hints of children.
  for (i = submenus.begin(); i != submenus.end(); ++i)
      if (i->second->vars[COMMAND_VAR].empty())
          i->second->process_hints();

  store_hints();

  // Step 1.
  // Go through all submenus, and add their sections to hint_list.
  for (i = submenus.begin(); i != submenus.end(); ++i)
  {
      vector<string> sections;

      for (l = i->first.begin(); l != i->first.end(); ++l)
          sections.push_back(*l);

      // If there are more than one element in 'sections', pop off the last
      // one, since that's the title of the application.
      if (sections.size() > 1) {
        sections.pop_back();
        if (menumethod->hint_debug)
            std::cout << "Adding to hint_list: " << i->first << ", hints="
                << i->second->menuhints << std::endl;
        for (l=i->second->menuhints.begin(); l!=i->second->menuhints.end(); ++l)
            sections.push_back(*l);
        hint_list.push_back(sections);
      } else {
        vector<string> empty; // to make sure hint_list[i] and submenus[i] match
        hint_list.push_back(empty);
      }
  }
  // Step 2.
  hints h;
  h.set_nentry(menumethod->hint_nentry);
  h.set_topnentry(menumethod->hint_topnentry);
  h.set_mixedpenalty(menumethod->hint_mixedpenalty);
  h.set_minhintfreq(menumethod->hint_minhintfreq);
  h.set_max_local_penalty(menumethod->hint_mlpenalty);
  h.set_max_ntry(menumethod->hint_max_ntry);
  h.set_max_iter_hint(menumethod->hint_max_iter_hint);
  h.set_debug(menumethod->hint_debug);

  // Calculate our new tree, using hints. A lot of magic happens here.
  h.calc_tree(hint_list, hint_out);

  // Step 3.
  submenu_container oldsubmenus = submenus;
  submenus.erase(submenus.begin(), submenus.end());

  // Step 4.
  for (k = hint_list.begin(), m = hint_out.begin(), i = oldsubmenus.begin();
      k != hint_list.end();
      k++, m++, i++)
  {
    vector<string> sections = *m;

    // Restore title (we popped it of earlier in this function).
    sections.push_back(i->first.back());

    // Finally re-add the entry...
    add_entry_ptr(sections, i->second);
  }
}

/** Postprocess will set or correct some variables in the
 * menuentry tree (vars), that were not known at the time of
 * creation of the menuentry classes (due to hints, or other reasons)
 *
 * Arguments:
 *
 *      n_parent: number of elements in menu of parent
 *      level: how deep this entry is nested in menu tree.
 *      prev_section: parent section name
 */
void menuentry::postprocess(int n_parent, int level, const std::string& prev_section)
{
  submenu_container::iterator i, i_next;
  int index = 0;

  vars[PRIVATE_LEVEL_VAR] = itostring(level);

  // If we are at the bottom of the hierarchy, use the "parents" section name.
  if (!level)
      vars[SECTION_VAR] = prev_section;

  for (i = submenus.begin(); i != submenus.end(); ++index)
  {
    i_next=i;
    i_next++;

    string newsection = prev_section;
    for(vector<string>::const_iterator j = i->first.begin(); j != i->first.end(); ++j)
        newsection += '/' + *j;

    menuentry *me = i->second;
    me->vars[SECTION_VAR] = newsection;

    // Get the real section name by removing title of the section name. This
    // is useful when for example the title is "Foo version/456".
    me->vars[BASESECTION_VAR] = newsection.substr(0, newsection.rfind(me->vars[TITLE_VAR]) - 1);

    me->vars[PRIVATE_ENTRYINDEX_VAR] = itostring(index);

    if (!me->submenus.empty())
        me->postprocess(submenus.size(), level+1, newsection);

    // Number of entries may have been changed by above call, so we need to
    // test again.
    if (me->submenus.empty()) {
      if (me->vars.find(COMMAND_VAR) == me->vars.end()) {
        // This is an empty menu (without comand or submenus), so delete it.
        submenus.erase(i);

        index--;
      } else {
        i->second->vars[PRIVATE_ENTRYCOUNT_VAR] = itostring(submenus.size());
        i->second->vars[PRIVATE_LEVEL_VAR] = itostring(level+1);
      }
    }
    // don't use i here any more, it may have been erased above
    i = i_next; // don't do i++, as *i now may not be defined
    index++;
  }

  generate_hotkeys();

  vars[PRIVATE_ENTRYCOUNT_VAR] = itostring(n_parent);
}

char menuentry::hotkeyconv(char h)
{
  if (menumethod->hotkeycase)
    return h;
  else
    return tolower(h);
}

void menuentry::generate_hotkeys()
{
  string::size_type i,j;
  list<int>::iterator k, old_k;
  submenu_container::iterator subi;
  map<string, string>::iterator l;
  vector<string> keys;
  list<int> todo;
  set<char> used_chars;
  string s;
  char c;

  string str0 = "0";
  str0[0]='\0';
  if (menumethod->hkexclude)
      s = menumethod->hkexclude->soutput(vars);
  for (i = 0; i != s.length(); i++)
    used_chars.insert(hotkeyconv(s[i]));

  for(subi = submenus.begin(), i = 0; subi != submenus.end(); subi++, i++)
  {
    todo.push_back(i);
    l = subi->second->vars.find(HOTKEY_VAR);
    if (l != subi->second->vars.end())
        keys.push_back(l->second);
    else
        keys.push_back(str0);
    keys[i] += sort_hotkey(subi->second->vars[TITLE_VAR]);
  }
  j = 0;
  while (!todo.empty())
  {
    for (k = todo.begin();k != todo.end();)
    {
      i = *k; 
      old_k = k++; //k++ here, to be able to todo.erase(old_k) safely.
      if (j >= keys[i].length()) {
        keys[i] = str0;
        todo.erase(old_k);  //no hotkey found -- give up on this entry.
        continue;
      }
      c = keys[i][j];
      if (c) {
        if (used_chars.find(hotkeyconv(c)) == used_chars.end()) {
          todo.erase(old_k); //found a hotkey for this entry.
          keys[i] = c;
          used_chars.insert(hotkeyconv(c));
          continue;
        } else {
          keys[i].replace(j,1,str0);
        }
      }
    }
    j++;
  }
  for (subi = submenus.begin(), i = 0; subi != submenus.end(); subi++, i++)
  {
    c = keys[i][0];
    if (c)
        subi->second->vars[HOTKEY_VAR] = c;
  }
}
