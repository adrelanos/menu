/*
 * Debian menu system -- install-menu
 * install-menu/hints.h
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

#ifndef HINTS_H
#define HINTS_H

#include <vector>
#include <map>
#include <set>
#include <string>

bool operator<(const std::set<std::string> &left, const std::set<std::string> &right);
std::ostream& operator<<(std::ostream &, const std::vector<std::string> &);

class hint_tree;
class correlation;

typedef std::multimap <double, std::vector<hint_tree> > possible_divisions;
typedef std::multimap <double, possible_divisions::iterator> possible_divisions_it;

class hint_tree {
public:
  hint_tree(std::string k, std::vector<hint_tree> &c) : key(k), children(c) { }
  hint_tree(std::string k) : key(k) { }

  std::string key;
  std::vector<std::vector<std::string> > entries;
  double penalty; //of the whole tree (including children, grandchildren, etc).
  std::vector<hint_tree> children;
};

class hints {
  bool debugopt;
  double max_local_penalty;
  hint_tree root_tree;
  
  double mixed_penalty;
  double min_hint_freq;
  double hint_nentry, hint_topnentry;
  unsigned int max_ntry;
  int max_iter_hint;
  
  std::vector<std::vector<std::string> > hint_list;
  std::vector<int> raw_count;

public:
  hints()
      : root_tree("-"), min_hint_freq(0.3 / hint_nentry),
  hint_nentry(6), hint_topnentry(6), max_ntry(5), max_iter_hint(5) { }

  void set_nentry(double n) { hint_nentry = n; }
  void set_topnentry(double n) { hint_topnentry = n; } 
  void set_mixedpenalty(double p) { mixed_penalty = p; }
  void set_minhintfreq(double m) { min_hint_freq = m / hint_nentry; }
  void set_max_local_penalty(double p) { max_local_penalty = p; }
  void set_max_ntry(int n) { max_ntry = n; }
  void set_max_iter_hint(int n) { max_iter_hint = n;}
  void set_debug(bool opt) { debugopt = opt; }
  void calc_tree(const std::vector<std::vector<std::string> > &hint_input,
      std::vector<std::vector<std::string> > &hint_output);

  void debug();

private:
  void sort_hints();
  double calc_penalty(int level, int n, int unused);
  double nopt(int level) const {
    if (level != 0) 
      return hint_nentry; 
    else 
      return hint_topnentry;
  }
  void add_division(int level, possible_divisions &division_list, 
                    const std::vector<unsigned int> &division, 
                    correlation &h, 
                    int unused, 
                    double &worst_penalty,
                    unsigned int itteration);
  bool try_shortcut(int level,
                    const std::vector<std::vector<std::string> > &hint_input, 
                    const std::vector<int> &raw_input_count,
                    hint_tree &t,
                    int already_used);
  void find_possible_divisions(int level, possible_divisions &division_list,
                              const std::vector<std::vector<std::string> > &hint_input,
                              const std::vector<int> &raw_input_count,
                              int already_used);
  void postprocess(int level, 
                  const std::vector<std::vector<std::string> > &hint_input, 
                  const std::vector<int> &raw_input_count,
                  hint_tree &t,
                  int already_used);
  void search_hint(hint_tree &t, const std::vector<std::string> &hint_in,
                  std::vector<std::string> &hint_out);
  void debug_hint_tree(const hint_tree &t, int level);
};


#endif /*  HINTS_H */
