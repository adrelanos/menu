#include <cctype>
#include <list>
#include <set>
#include "menu-tree.h"
#include "install-menu.h"
#include "hints.h"

using std::vector;
using std::string;
using std::list;
using std::set;
using std::map;

bool operator<(const vector<string> &left, const vector<string> &right)
{
  vector<string>::const_iterator i,j;

  for (i = left.begin(), j = right.begin();
      (i != left.end()) && (j != right.end());
      i++, j++)
  {
    if (*i < *j)
        return true;
    else if (*i > *j)
        return false;
  }
  if (j == right.end())
      return false;
  else
      return true;
}

// Adds a new entry in the menu hierarchy.
//
// Arguments:
//
//     sections: A vector of strings, holding the section names.
//     entry_vars: Variables of the entry to be added.
// 
// There are two ways to store the menuentries in the `tree'
// - `flat', all menuentries in the toplevel submenus map.
//   This is used when working with `hints'. In this case, we
//   will later sort out the tree.
// - as a true tree.
// However, even in the `flat', hint-processing case, if there already
// exists a submenu with exactly the right sections[0], then we do desent,
// if it has a entry_vars[FORCED_VAR] set.
void menuentry::add_entry(vector<string> sections, map<string, string> &entry_vars)
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

  if ((!config->hint_optimize || use_forced) && (sections.size() > 1)) {
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

// Adds a new entry in the menu hierarchy, using an already created
// menuentry pointer.
//
// Arguments:
//
//     sections: A vector of strings, holding the section names.
//     entry: a pointer to a menuentry.
// 
void menuentry::add_entry_ptr(vector<string> sections, menuentry *entry)
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

// Output menu tree.
//
// Uses the 'treewalk' variable to define what to output in which order,
// this documentation was taken from the menu manual:
//
//   `treewalk="c(m)"'
//        This string defines in what order to dump the `$startmenu',
//        `$endmenu', and `$submenutitle' (and its children).  Each char in
//        the string refers to:
//
//  c  : dump children of menu.
//  m  : dump this menu's $submenutitles
//  (  : dump $startmenu
//  )  : dump $endmenu
//  M  : dump all $submenutitles of this menu and this menu's children.
//
//       The default is "c(m)".  For olvwm, one needs: "(M)"
//
void menuentry::output()
{
  string treew = config->treewalk();
  submenu_container::iterator sub_i;
  std::multimap<string, menuentry *> sorted;
  std::multimap<string, menuentry *>::iterator i;

  // Sort the submenus in sorted:
  for (sub_i = submenus.begin(); sub_i != submenus.end(); ++sub_i)
  {
    string s;
    if (config->sort)
      s = config->sort->soutput(sub_i->second->vars);
    else
      s = sub_i->second->vars[SORT_VAR] + ':' + sub_i->second->vars[TITLE_VAR];

    std::cout << "Sorting on: " << s << std::endl;
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
            if (config->startmenu && testuniqueness(vars))
                config->startmenu->output(vars);
        break;
      case ')':
        if (!submenus.empty())
            if (config->endmenu && testuniqueness(vars))
                config->endmenu->output(vars);
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
            if (config->submenutitle && !i->second->submenus.empty() &&
                testuniqueness(i->second->vars))
                config->submenutitle->output(i->second->vars);
            if (children_too)
                i->second->output();
          }
        }
    }
  }
}

// Put the $hint variable contents in the hint of the submenu[] map. For
// submenu entries (without a command), put the $hint variable in all
// menuentries that lie below that one.
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

      break_commas(hints_str, hints);

      j = i;

      do {
        for (vector<string>::iterator k = hints.begin(); k != hints.end(); ++k)
            j->second->menuhints.push_back(*k);

        j++;

        for (l = 0; l != i->first.size(); ++l)
            if (i->first[l] != j->first[l])
                break;
      } while (l == i->first.size());

    }
  }
}

// Process the menu hierarchy through the hints algorithms.
//
// Basically, it has 4 steps (also marked as comments in the code):
//
//  Step 1: Fetch appropriate sections to put in hint_list vector.
//  Step 2: Initialize hints class and calculate a new tree, using hint_list.
//  Step 3: Remove all submenus.
//  Step 4: Using the new hint_out vector, initialize our new sorted tree,
//  thus re-creating the previously removed submenus (but in a different
//  order).
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
        if (config->hint_debug)
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
  h.set_nentry(config->hint_nentry);
  h.set_topnentry(config->hint_topnentry);
  h.set_mixedpenalty(config->hint_mixedpenalty);
  h.set_minhintfreq(config->hint_minhintfreq);
  h.set_max_local_penalty(config->hint_mlpenalty);
  h.set_max_ntry(config->hint_max_ntry);
  h.set_max_iter_hint(config->hint_max_iter_hint);
  h.set_debug(config->hint_debug);

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

// Postprocess will set or correct some variables in the
// menuentry tree (vars), that were not known at the time of
// creation of the menuentry classes (due to hints, or other reasons)
//
// Arguments:
//
//      n_parent: number of elements in menu of parent
//      level: how deep this entry is nested in menu tree.
//      prev_section: parent section name
void menuentry::postprocess(int n_parent, int level, const string& prev_section)
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

    string title = prev_section;
    for(vector<string>::const_iterator j = i->first.begin(); j != i->first.end(); ++j)
        title += string("/") + *j;

    menuentry *me = i->second;
    me->vars[SECTION_VAR] = title;

    me->vars[PRIVATE_ENTRYINDEX_VAR] = itostring(index);

    if (!me->submenus.empty())
        me->postprocess(submenus.size(), level+1, title);

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
  if (config->hotkeycase)
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
  if (config->hkexclude)
      s = config->hkexclude->soutput(vars);
  for (i=0;i!=s.length();i++)
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
      i= *k; 
      old_k=k++; //k++ here, to be able to todo.erase(old_k) safely.
      if (j >= keys[i].length()) {
        keys[i]=str0;
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
  for (subi=submenus.begin(), i=0; subi!=submenus.end(); subi++, i++)
  {
    c = keys[i][0];
    if (c)
        subi->second->vars[HOTKEY_VAR] = c;
  }
}

void menuentry::debug(int level)
{
  submenu_container::iterator i;

  for (int li = level; li > 0; li--)
      std::cout << "  ";

  std::cout << vars[TITLE_VAR]<< ':' << vars[SECTION_VAR] << ':' << vars[ICON_VAR] << std::endl;
  for(i = submenus.begin(); i != submenus.end(); ++i)
      i->second->debug(level+1);
}

