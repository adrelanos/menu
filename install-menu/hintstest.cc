#include "hints.h"

void lees (vector<vector<string> > &hint_input, vector <int> &hint_count)
{
  vector<string> h;
  char c;
  //  map <char, int>::iterator hi;

  while (cin.get(c))
  {
    switch (c)
    {
      case '\n':
        hint_input.push_back(h);
        hint_count.push_back(1);
        h.erase(h.begin(), h.end());

        break;
      default:
        h.push_back(c);
        break;
    }
  }
}

int main()
{
  vector<vector<string> > hint_input, hint_output;
  vector<hint_tree> children;
  vector<int> hint_count;
  hints h;

  hint_tree t('-',children);

  lees(hint_input, hint_count);
  h.set_nentry(3);
  h.set_topnentry(3);
  h.set_mixedpenalty(0);
  h.set_max_local_penalty(50);
  h.set_minhintfreq(0.0001);
  h.set_debug(true);
  h.calc_tree(hint_input, hint_output);
}
