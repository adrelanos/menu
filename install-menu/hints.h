#ifndef HINTS_H
#define HINTS_H

#include <vector>
#include <map>
#include <set>
#include <string>
#include "adstring.h"

using std::string;

bool operator<(const std::set<string> &left, const std::set<string> &right);

std::ostream& operator<<(std::ostream &, const StrVec &);


class hint_tree;

class correlation;

typedef std::multimap <double, std::vector<hint_tree> > possible_divisions;
typedef std::multimap <double, possible_divisions::iterator> possible_divisions_it;

class hint_tree {
public:
  hint_tree(string k, std::vector<hint_tree> &c) : key(k), children(c) { }
  hint_tree(string k) : key(k) { }

  string key;
  std::vector<StrVec> entries;
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
  
  std::vector<StrVec> hint_list;
  std::vector<int> raw_count;

public:
  hints()
      : root_tree("-"), min_hint_freq(0.3 / hint_nentry),
  hint_nentry(6), hint_topnentry(6), max_ntry(5), max_iter_hint(5) { }

  void set_nentry(double n){hint_nentry=n;};
  void set_topnentry(double n){hint_topnentry=n;};  
  void set_mixedpenalty(double p){mixed_penalty=p;};
  void set_minhintfreq(double m){min_hint_freq=m/hint_nentry;};
  void set_max_local_penalty(double p){max_local_penalty=p;};
  void set_max_ntry(int n){max_ntry=n;};
  void set_max_iter_hint(int n){max_iter_hint=n;};
  void set_debug(bool opt){debugopt=opt;};
  void calc_tree(const std::vector<StrVec> &hint_input,
      std::vector<StrVec> &hint_output);

  void debug();

private:
  void order();
  double sqr(double x) const { return x*x; }
  double calc_penalty(int level, int n, int unused);
  double nopt(int level) {
    if(level != 0) 
      return hint_nentry; 
    else 
      return hint_topnentry;
  }
  void add_division(int level, possible_divisions &division_list, 
		    const std::vector <unsigned int> &division, 
		    correlation &h, 
		    int unused, 
		    double &worst_penalty,
		    unsigned int itteration);
  bool try_shortcut(int level,
			const std::vector <StrVec > &hint_input, 
			const std::vector <int> &raw_input_count,
			hint_tree &t,
			int already_used);
  void find_possible_divisions(int level, possible_divisions &division_list,
			       const std::vector <StrVec> &hint_input,
			       const std::vector <int> &raw_input_count,
			       int already_used);
  void postprocess(int level, 
		   const std::vector <StrVec> &hint_input, 
		   const std::vector <int> &raw_input_count,
		   hint_tree &t,
		   int already_used);
  void search_hint(hint_tree &t,
		   const StrVec &hint_in,
		         StrVec &hint_out);
  void debug_hint_tree(const hint_tree &t, int level);
  void nspace(int);
};


#endif /*  HINTS_H */
