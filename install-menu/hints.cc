#include <iostream>
#include <algorithm>
#include <functional>
#include "hints.h"

/* Possible speedups:

   In class correlation:
   - simply `forget' hints that only occur once or less than
     N/(nopt^2) times, or less than nopt-? times. 
     Or, those that occur more than 3*N/(nopt) times.
     -> More general: don't sort hints by frequency f, but
        by abs(f-1/nopt). This way the hints with the most
	usefull frequency will appear first.
   -Maybe this helps:
     For hints that occur more then N/2 times, don't store the hints
     themselves, but store "!hint" in those hint_input's where the
     hint doens't occur. Should give identical results, but the
     procedures may be confursed by hints that occur > N/2.
   Overall speedup: 
   - change string into `int'! (and somewhere store a table with the
     string - rope<char> mapping). This way, many data types will be
     much smaller, and comparisions and searches will be much faster.
     (especially the building of the correlation table!)
   More try_shortcut heuristics!
   Get rid of the exp(sqr(n)) behaviour:
   - As long as there are still many (> nopt^2) children, don't 
     calculate the `real' penalty for all possible_divisions, but
     only estimate heuristically. Then only use the `best' of the
     few possibilities, discarding the others. This should make 
     time-complexity linear, if I'm right.
   - If possible_divisions for example is:
       AB
       ABC
       ACG
       BCQ
     then we now 3 times run process('A'). Only the first time we need
     to actually do that. The other times we already know penalty(),
     and we can simply copy the produced tree. (Same for B, C here.
     with recursion this should give a good speedup, I hope).
     Should make a _global_ set of set of hints, whith the `best'
     tree for each set of hints.

*/

/* Bugs:

   While writing, I wasn't aware that after a vector.push_back(), all
   itterators to vector are (possibly) invalidated (that is, if a
   reallocation occurs). Check this! (STL tutorial & refernence, page 130).
*/

using std::vector;
using std::map;
using std::multimap;
using std::set;
using std::pair;
using std::cout;
using std::string;
using std::endl;

class correlation {
  // this class combines the hints, and calculates a correlation table.

public:
  typedef vector<vector<int> >  table;

private:
  bool debugopt;
  vector<vector<string> > hint_list; //raw input for this correlation class.
  vector<int>     raw_count; //raw input frequency vector.
  int n_raw;                 //sum of raw_count[i] for all i.
  
  // hints sorted by name, to be able to quickly find out a 
  // hints' freq. The frequency of each hint is the sum of their 
  // appearencies in hint_list (multiplied by raw_count).
  map<string, int> hint_frequency;

  // Same as hint_frequency, but sorted by frequency.
  vector<string> frequency_hint; //array of hints, sorted by frequency.
  vector<int> frequency_freq; //frequencies of above hints.

  table tab; 
  //tab[i][j] contains the number of occurences of both
  //hint i and hint j in one menuentry. (hint_list[]). 
  // Multiplied by raw_count. Here (and in the rest) 
  // hint i refers to frequency_hint[i].

  int hint_input_n;  //number of lines (menuentries) in hint_input.

  void calc_correlations();

public:
  correlation(const vector<vector<string> > &hint_input,
              const vector<int> &count_input, 
              double min_hint_freq,
              bool debug);

  int frequency(string h) { return hint_frequency[h]; }
  int frequency(unsigned int i) { return frequency_freq[i]; }
  string hint_i(unsigned int i) { return frequency_hint[i]; }
  vector<string> &hint_input_i(unsigned int i) { return hint_list[i]; }
  int raw_total() const { return n_raw; }
  unsigned int n_input() const { return hint_input_n; }
  unsigned int nhints() const { return tab.size(); }
  table &get_table() { return tab; }
  void debug();
};


correlation::correlation(const vector<vector<string> > &hint_input,
                         const vector<int> &raw_input_count,
                         double min_hint_freq,
                         bool bug)
{
  // hint_input: has been sorted by calling routine,
  //             every hint in hint_input only occurs once
  // raw_input_count[i]: records frequency of hint_input[i].

  unsigned int i,j;
  vector<vector<string> >::const_iterator k;
  vector<string>::const_iterator m,l;
  map<string, int>::iterator hi;
  multimap<int, string, std::greater<int> > fh;
  multimap<int, string, std::greater<int> >::const_iterator fhi;
  unsigned int total, n;
  string c;
  set<string> used_hints;

  debugopt=bug;
  hint_input_n=hint_input.size();
  hint_list=hint_input;
  raw_count=raw_input_count;
  n_raw=0;
  // First, calculate the frequencies of the hints.
  // This information (and more) will later also be calculated
  // in the correlation table, but things are easier if we have
  // the frequencies now already.
  total=0;
  for(i=0; i<hint_input.size(); i++){
    for(j=0; j<hint_input[i].size(); j++){
      c=hint_input[i][j];
      hi=hint_frequency.find(c);
      n=raw_count[i];
      total+=n;
      if(hi==hint_frequency.end())
          hint_frequency[c]=n;
      else
          (*hi).second+=n;
    }
    n_raw += raw_input_count[i];
  }
  for(hi=hint_frequency.begin(); hi!=hint_frequency.end(); hi++){
    if( ((double)(*hi).second / total) >= min_hint_freq){
      fh.insert(pair<int,string>((*hi).second,(*hi).first));
      used_hints.insert((*hi).first);
    }
  }

  // search for items in hint_input that (due to the min_hint_freq
  // limitation) are left out in used_hints.
  for(k=hint_input.begin(); k!=hint_input.end(); k++){
    if (!k->empty()) {
      for(l=m=(*k).begin(); m!=(*k).end(); m++)
      {
        if(hint_frequency[*l]<hint_frequency[*m])
            l=m;
        if(used_hints.find(*m)!=used_hints.end())
            break;
      }
      if(m==(*k).end()){
        fh.insert(pair<int,string>(hint_frequency[*l],*l));
        used_hints.insert(*l);
      }
    }
  }

  // store the information in frequency_hint etc.
  for(i=0, fhi=fh.begin(); 
      fhi!=fh.end(); 
      i++, fhi++){
    frequency_hint.push_back((*fhi).second);
    frequency_freq.push_back((*fhi).first);
  }
  calc_correlations();
  debug();
}

void correlation::calc_correlations()
{

  unsigned int i,j,k;

  // fill tab with zerro's
  for(i=0; i<frequency_hint.size(); i++){
    vector<int> h(frequency_hint.size());
    tab.push_back(h);
  }

  // now build the correlation table.
  // (could be done faster...).
  for(i=0; i<frequency_hint.size(); i++){
    string c=frequency_hint[i];
    vector<int> &tab_i=tab[i];
    for(j=0; j<frequency_hint.size();j++){
      if(i==j)
          tab_i[j]=hint_frequency[c];
      else
          for(k=0;k<hint_list.size(); k++){
            vector<string>::const_iterator b=hint_list[k].begin();
            vector<string>::const_iterator e=hint_list[k].end();
            if((find(b, e, c)!=e) && (find(b, e, frequency_hint[j])!=e)){
              tab_i[j]+=raw_count[k];
            }
          }
    }
  }
}

void correlation::debug()
{
  unsigned int i,j;
  vector<vector<string> >::const_iterator hi;
  map<string, int>::const_iterator m;

  if(debugopt){
    cout<<"Hintoj (size="<<hint_frequency.size()<<") :"<<endl;
    for(m=hint_frequency.begin(); m!=hint_frequency.end(); m++)
      cout<<(*m).first<<" - "<<(*m).second<<endl;

    cout<<"Hint_input (size="<<hint_list.size()<<") :"<<endl;
    for(i=0,hi=hint_list.begin(); hi!=hint_list.end(); hi++,i++){
      cout<<i<<" \"";
      for(j=0; j< (*hi).size(); j++)
          cout<<(*hi)[j];
      cout<<" * "<<raw_count[i]<<"\""<<endl;
    }
    cout<<"Hint_tab:"<<endl<<"   ";
    for(i=0; i<frequency_hint.size(); i++)
        cout<<frequency_hint[i];
    cout<<endl;
    for(i=0; i<frequency_hint.size(); i++){
      unsigned int k;
      cout<<" "<<frequency_hint[i]<<" ";
      for(k=0;k<frequency_hint.size(); k++){
        cout<<tab[i][k];
      }
      cout<<endl;
    }
  }
}

std::ostream& operator<<(std::ostream &o, const vector<string> &s)
{
  for(vector<string>::const_iterator i = s.begin(); i != s.end(); ++i)
      o << '[' << *i << ']';

  return o;
}

bool operator<(const set<string> &left, const set<string> &right)
{
  set<string>::const_iterator i,j;

  for(i=left.begin(), j=right.begin();
      (i!=left.end()) && (j!=right.end());
      i++, j++){
    if((*i)<(*j))
      return true;
    else if((*i)>(*j))
      return false;
  }
  if(j==right.end())
    return false;
  else
    return true;
}

// Calculate a new menu tree.
//
// Arguments:
//
//      hint_input: a vector with all sections.
//      hint_output: what is to be the final tree
//
// The magic happens in order() and postprocess().
//
void hints::calc_tree(const vector<vector<string> > &hint_input,
    vector<vector<string> > &hint_output)
{

  hint_output.erase(hint_output.begin(), hint_output.end());
  raw_count = vector<int>(hint_input.size(), 1);
  hint_list = hint_input;

  order();

  postprocess(0, hint_list, raw_count, root_tree, 0);

  vector<vector<string> >::const_iterator i;
  for (i = hint_input.begin(); i != hint_input.end(); ++i)
  {
    vector<string> out;
    search_hint(root_tree, *i, out);
    hint_output.push_back(out);
    if (debugopt)
        cout << "IN: " << *i << "   OUT: " << out << endl;
  }
  if (debugopt)
      debug();
}

// Sort the hint_list properly.
//
// This means that a vector that contained these strings, in this order:
//
//    Games, Arcade, Doom, 3D
//
// will be changed into this order, when this function completes:
//
//     3D, Arcade, Doom, Games
//
// Duplicated elements are removed, and raw_count is set to the value of
// each entry.
void hints::order()
{
  map<set<string>, int> sorted;
  map<set<string>, int>::iterator si;

  // The test below for (*).empty() is a bad hack! I don't want
  // empty entries, that probably were caused by 
  // forcetree stuff. They will cause the optimize routines
  // think that the toplevel menu already is mixed.
  
  for (unsigned int i = 0; i < hint_list.size(); ++i)
  {
    if (!hint_list[i].empty()) {
      set<string> h;

      for (unsigned int j = 0; j < hint_list[i].size(); ++j)
          h.insert(hint_list[i][j]);

      si = sorted.find(h);

      if (si == sorted.end())
          sorted.insert(pair<set<string>, int>(h, raw_count[i]));
      else
          si->second++;
    }
  }
  hint_list.erase(hint_list.begin(), hint_list.end());
  raw_count.erase(raw_count.begin(), raw_count.end());

  for (si = sorted.begin(); si != sorted.end(); ++si)
  {
    set<string>::const_iterator hi;
    vector<string> v;
    for(hi = si->first.begin(); hi != si->first.end(); ++hi)
        v.push_back(*hi);
    hint_list.push_back(v);
    raw_count.push_back(si->second);
  }
  
  if (debugopt) {
    cout << "HINT_INPUT..." << endl;
    for (unsigned int i = 0; i < hint_list.size(); ++i)
        cout << hint_list[i] << " ** " << raw_count[i] << endl;
  }
}

double hints::calc_penalty(int level, int n, int unused)
{
  return sqr(n-nopt(level)) + mixed_penalty*(unused!=0);
}

void hints::add_division(int level,
                          possible_divisions &division_list, 
                          const vector<unsigned int> &division, 
                          correlation &h, 
                          int unused,
                          double &worst_penalty,
                          unsigned int itteration)
{
  double penalty;
  vector<hint_tree> d;
  unsigned int i;
  possible_divisions::iterator l;

  if (!division.empty())
      penalty=calc_penalty(level, division.size()+unused, unused);
  else
      penalty=calc_penalty(level, h.raw_total()+unused,0);


  if((division_list.size()<max_ntry) ||
      (penalty<worst_penalty)){
    for(i=0; i<division.size(); i++){
      vector<hint_tree> c;
      hint_tree t(h.hint_i(division[i]), c);
      d.push_back(t);
    } 
    if(debugopt){
      vector<unsigned int>::const_iterator j;
      cout<<"itt="<<itteration<<" ";
      cout<<"Adding Division h, p="<<penalty<<" dl.s()= "
          <<division_list.size()<<", h=";
      for(j=division.begin(); j!=division.end(); j++)
          cout<<"["<<h.hint_i(*j)<<"]";
      cout<<endl;
    }
    if(division_list.size()>=max_ntry){
      l=division_list.end();
      l--;
      division_list.erase(l, division_list.end());
    }
    division_list.insert(pair<const double, vector<hint_tree> >(penalty,d));
    l=division_list.end();
    l--;
    if((*l).first<worst_penalty)
      worst_penalty=(*l).first;
  }
}

void hints::find_possible_divisions(int level,
                                    possible_divisions &division_list,
                                    const vector<vector<string> > &hint_input,
                                    const vector<int> &raw_input_count,
                                    int already_used)
{
  vector<vector<int> > remaining;
  vector<unsigned int> division;
  unsigned int i, j, used, used_tmp, total;
  double worst_penalty=(int)max_local_penalty;
  unsigned int itterations=0;

  correlation h(hint_input, raw_input_count, min_hint_freq, debugopt);
  correlation::table &tab=h.get_table();
  {
    // generate an `empty' remaining[] array, to show that all
    // hints are still available (see also comment below).
    vector<int> r;
    for(i=0; i< h.nhints(); i++)
      r.push_back(1);
    remaining.push_back(r);
  }

  // Add an empty division, this will be the division where 
  // all entries are dumpt directly in the menu.
  add_division(level, division_list, division, h, 
               already_used, worst_penalty, itterations);

  i=0;
  used=0;
  total=h.raw_total();

  while(1)
  {
    vector<int> r;
    bool do_add=false;
    bool do_remove_and_continue=false;

    itterations++;

    if (max_iter_hint>=0)
        if (itterations>(5+max_iter_hint*h.nhints()))
            break;

    // search for a hint that doesn't occur in the same
    // (menu)entry as other, already used, hints. The remaining[]
    // array maintains a list of such hints (remaining[i]=1 means
    // hint[i] still can be used).
    for(; i < tab.size(); i++)
        if(remaining[remaining.size()-1][i]){
          used_tmp=used + h.frequency(i);
          if (used_tmp == total){
            // this combination of divisions perfectly uses up all
            // menuentries: add it to the division_list, and
            // then continue searching other possibilities.
            division.push_back(i);

            add_division(level, division_list,division, h, 
                total-used_tmp+ already_used, 
                worst_penalty, itterations);
            division.pop_back();
          }
          else if(used_tmp < total){
            used=used_tmp;
            break;
          }
        }

    if(i==tab.size() && (division.size())){
      do_add=1;
      do_remove_and_continue=true;
    }
    if(division.size()>(nopt(level)-already_used)){
      double penalty=calc_penalty(level, division.size()+1,0);
      if((division.size()>=max_ntry) &&
          (penalty > worst_penalty)){
        // If after adding this hint we already exceed the maximum 
        // number of subentries, we can stop trying this hint.
        // Also the previous hint (in division[]) will not work any
        // more, as the following hints are all lower in frequency 
        // (and thus we need more of them to use all entries).
        do_remove_and_continue=true;
      }
    }
    if(do_add){
      // We didn't find a `perfect' match (i.e. one with all 
      // entries used), but insert this option in 
      // division_list anyway (with a penalty)
      add_division(level,
          division_list, division, h, 
          total-used + already_used, 
          worst_penalty, itterations);
    }
    if(do_remove_and_continue){
      // Get i out of division (will be used in next itteration)
      i=division[division.size()-1];

      used -= h.frequency(i);

      // Remove this (failed) last hint from division.
      division.pop_back();
      remaining.pop_back();
      i++;
      continue;
    }
    if (i==tab.size())
      if (division.empty()) {
        // apparently all possibilities have been seen.
        break; 
      }

    division.push_back(i);

    for(j=0; j < tab.size(); j++)
      r.push_back((tab[i][j]==0)&&remaining[remaining.size()-1][j]);
    remaining.push_back(r);
  }
  if(debugopt)
    cout<<"find_pos, level="<<level<<", itt="<<itterations<<endl;
}

bool hints::try_shortcut(int level,
                        const vector<vector<string> > &hint_input,
                        const vector<int> &raw_input_count,
                        hint_tree &t,
                        int already_used)
{
  // Try with heuristics to reduce the thinking time
  vector<int>::const_iterator i;

  int raw_total = 0;
  for (i = raw_input_count.begin(); i != raw_input_count.end(); ++i)
      raw_total += *i;

  if ((hint_input.size() <= 1) && (already_used == 0))
    goto found;

  if ((raw_total+already_used) < (nopt(level)*1.5))
    goto found;

  return false;
found:
  t.penalty = calc_penalty(level, raw_total+already_used, 0);
  return true;
}

void hints::postprocess(int level,
                        const vector<vector<string> >& hint_input,
                        const vector<int>& raw_input_count,
                        hint_tree& t,
                        int already_used)
{

  unsigned int i,j,k;
  possible_divisions division_list;
  possible_divisions_it tmp_div_list;
  possible_divisions::iterator cl;

  // In: hint_input. For example:
  // hint_input[]=i,bc,bd,ec,ef,gh,gf
  // Then, division_list will be setup to:
  // possible_division[]={b,e,g,i}, {b,e,h,i,fg}, {b,f,h,i,ce}, 
  // {c,f,d,h,i} (and so on).
  // Then, in the cl-loop below, cl will go through the 
  // possible_divisions[] elements, and i go loop in one
  // division (c=b,e,g,i for the first cl).


  // General idea of search:
  // First, in find_possible_divisions we search for all
  // possible divisions for this node only (not it's 
  // children), and rank the divisions by looking only at. 
  // find_possible_divisions will only return the
  // 5 or so best divisions. Then we evaluate (recursively,
  // in postprocess) all children of those best divisions, 
  // and calculate the `real' penalty. The recursive search
  // here will only return the best option found,
  // in hint_tree t.

  if (try_shortcut(level, hint_input, raw_input_count, t, already_used)) {
    // We found a shortcut, just return.
    return;
  }

  find_possible_divisions(level, division_list, hint_input, raw_input_count,
                          already_used);

  if (debugopt) {
    unsigned int cl_i;
    cout << "Je fine:" << endl;
    for(cl_i=0, cl=division_list.begin(); 
        (cl!=division_list.end()) && (cl_i<7); 
        cl++, cl_i++){
      cout<<(*cl).first<<"  ";
      for(i=0;i<(*cl).second.size(); i++){
        cout << '[';
        for(j=0; j<(*cl).second[i].key.size();j++)
            cout<<((*cl).second[i]).key[j];
        cout << ']';
      }
      cout << endl; 
    }
  }
  for(cl=division_list.begin(); (cl!=division_list.end()); cl++){
    vector<bool> stored(hint_input.size());
    double penalty = cl->first;
    for(i = 0; i < cl->second.size(); ++i){
      vector<vector<string> > children_hint_input;
      vector<int> children_counts;
      //key_leftover: hints that belong to this key,
      //but aren't put in the children_hint_input.
      //This is in situations where key=A, and hints are A, AB, AC.
      //Then, [A]B and [A]C are put in children_hint_input,
      //but the first one [A] isn't.
      int key_left_over=0; 
      const string &key=(*cl).second[i].key;

      // In the example above, the first iteration key will be 
      // b, then e,...	
      // Now search entries in hint_input that have hint key in
      // them. In the above example, when key=b, the sought 
      // hint_input[] entries will be 1 and 2 (bc,bd). 
      // Once the entries have been found, strip the key 
      // from them (resulting in c,d in the example), and try
      // to (recursively) make a tree from that.
      for(j=0; j<hint_input.size(); j++)
          if (!stored[j]) {
            vector<string>::const_iterator 
                b=hint_input[j].begin(),
            e=hint_input[j].end(),
            f;
            f=find(b, e, key);
            if(f!=e){
              vector<string> tmphint;
              stored[j]=true;
              for(k=0; k<hint_input[j].size(); k++)
                  if(hint_input[j][k]!=key)
                      tmphint.push_back(hint_input[j][k]);
              if (!tmphint.empty()) {
                children_hint_input.push_back(tmphint);
                children_counts.push_back(raw_input_count[j]);
              } else
                  key_left_over+=raw_input_count[j];
            }
          }
      if (!children_hint_input.empty()) {
        if (debugopt)
            cout<<"Re-entering postpro, key="<< key 
                <<", left_over="<<key_left_over<<endl;
        postprocess(level+1, children_hint_input,  children_counts, cl->second[i], key_left_over);
        if (debugopt)
            cout<<"Eliris postpro, k="<< key <<" p="<<cl->second[i].penalty<<endl;
        penalty += cl->second[i].penalty;
      } else {
        double lpenalty=calc_penalty(level, key_left_over,0);
        cl->second[i].penalty=lpenalty;
        penalty+=lpenalty;
      }
    }
    tmp_div_list.insert(pair<double,possible_divisions::iterator>(penalty,cl));
  }
  cl=(*tmp_div_list.begin()).second;
  for(i=0; i<(*cl).second.size(); i++)
    t.children.push_back((*cl).second[i]);
  t.penalty=(*tmp_div_list.begin()).first;
}

void hints::search_hint(hint_tree &tree,
                        const vector<string> &hint_in,
                        vector<string> &hint_out)
{
  bool found = false;

  vector<string>::const_iterator begin = hint_in.begin();
  vector<string>::const_iterator end = hint_in.end();

  vector<hint_tree>::iterator t;
  for (t = tree.children.begin(); t != tree.children.end(); ++t)
  {
    vector<string>::const_iterator j = find(begin, end, t->key);
    if (j != end) {
      hint_out.push_back(*j);
      search_hint(*t, hint_in, hint_out);
      found = true;
      break;
    }
  }
  if (!found)
      tree.entries.push_back(hint_in);
}

void hints::nspace(int n)
{
  for (; n > 0; n--)
      cout << ' ';
}

void hints::debug_hint_tree(const hint_tree &t, int level)
{
  nspace(level);
  cout<<t.key<<", p="<<t.penalty<<" c="<<t.children.size()
      <<" e="<<t.entries.size()<< endl;
  for(unsigned int i = 0; i < t.children.size(); ++i)
    debug_hint_tree(t.children[i],level+2);
}

void hints::debug()
{
  std::cout << "hint_tree (root):" << std::endl;
  if (debugopt)
      debug_hint_tree(root_tree, 0);
}

