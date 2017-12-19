#include <iostream>
#include <string>
#include <vector>
#include <queue>
#include <unordered_map>
#include <unordered_set>
#include <iterator>

enum FormulaType {
  LETTER,
  BOXA,
  BOXA_BAR,
  CLAUSE,
};

enum Case {
  FINITE,
  NATURAL,
  DISCRETE,
};

struct Formula {
  FormulaType type;
  int id;

  static Formula create(FormulaType type, int id) { return { type, id }; }
  static Formula truth() { return { LETTER, 1 }; }
  static Formula falsehood() { return { LETTER, 0 }; }
};

typedef std::pair<int, int> Interval;
typedef std::vector<Formula> Clause;

struct InputClauses {
  std::vector<Clause> rules;
  std::vector<Formula> facts;
};

struct State {
  Case caseType;
  InputClauses& phi;
  std::vector<Formula> boxa;
  std::vector<Formula> boxaBar;
};

inline bool operator==(const Formula& lhs, const Formula& rhs) {
  return lhs.type == rhs.type && lhs.id == rhs.id;
}
bool operator!=(const Formula& lhs, const Formula& rhs) {
  return !operator==(lhs, rhs);
}
std::size_t i2hash(int a, int b) {
  return std::hash<int>()(a) ^ std::hash<int>()(b);
}
struct IntervalHash {
  std::size_t operator()(const Interval &i) const { return i2hash(i.first, i.second); }
};
struct FormulaHash {
  std::size_t operator()(const Formula &f) const { return i2hash(f.id, f.type); }
};

typedef std::unordered_set<Formula, FormulaHash> FormulaSet;
typedef std::unordered_map<Interval, FormulaSet, IntervalHash> IntervalMap;
typedef std::queue<Interval> IntervalQueue;

bool check(InputClauses& phi, Case caseType);
bool saturate(int d, int x, int y, const State& phi);
bool extend(int d, IntervalMap& hi, IntervalMap& lo, const State& phi);

void printFormula(const State& state, const Formula f) {
  if (f.type == CLAUSE) {
    Clause c = state.phi.rules[f.id];
    for (auto& l : c) {
      if (l == c.back()) {
        printf(" -> ");
      } else if (l == c.front()) {
        printf("[U] ");
      } else {
        printf(" & ");
      }
      printFormula(state, l);
    }
    return;
  } else if (f.type == BOXA) {
    printf("[A]");
  } else if (f.type == BOXA_BAR) {
    printf("[B]");
  }

  if (f.id == 0) {
    printf("F");
  } else if (f.id == 1) {
    printf("T");
  } else {
    printf("p%d", f.id);
  }
}

void printInterval(const State& state, const Interval& interval, const FormulaSet& formulas) {
  printf("[%d, %d]: ",interval.first, interval.second);
  for(auto f: formulas) {
    printf("\n\t");
    printFormula(state, f);
  }
  printf("\n");
}

void printState(const State& state, const IntervalMap &map) {

  for(auto &i : map) {
    printInterval(state, i.first, i.second);
  }
  printf("\n");
}

Clause newClause(std::vector<int> arr) {
  Clause c;
  for (auto i = 0U; i < arr.size(); i += 2) {
    c.push_back(Formula::create(static_cast<FormulaType>(arr[i]), arr[i+1]));
  }
  return c;
}

int main(int argc, char **argv) {

  InputClauses phi;

  phi.rules = {
    newClause({LETTER, 2, BOXA_BAR, 0}),
    newClause({LETTER, 2, BOXA, 0, LETTER, 0}),
    newClause({LETTER, 2, BOXA, 3}),
    newClause({LETTER, 3, BOXA, 0}),
  };
  phi.facts = {
    Formula::create(LETTER, 2),
  };

  Case caseType = FINITE;

  bool result = check(phi, caseType);

  printf("result: %d", result);

  return 0;
}

bool check(InputClauses & phi, Case caseType) {
  int min, max;
  switch (caseType) {
    case FINITE:	min = 2; break;
    case NATURAL:	min = 3; break;
    case DISCRETE:	min = 4; break;
    default:		return false;
  }
  max = min + 6 * phi.rules.size(); 
  // devo considerare solo il numero delle clausole?

  State state = {caseType, phi};
  FormulaSet literals(phi.facts.begin(), phi.facts.end());
  for (auto& clause : phi.rules)
    std::copy(clause.begin(), clause.end(), std::inserter(literals, literals.end()));
  for (auto l : literals) {
    if (l.type == BOXA)
      state.boxa.push_back(l);
    else if (l.type == BOXA_BAR)
      state.boxaBar.push_back(l);
  }

  for (int k = min; k <= max; k++) {
    for (int x = 1; x < k - 1; x++)
      for (int y = x + 1; y < k; y++)
        if (saturate(k, x, y, state))
          return true;
  }

  return false;
}

bool saturate(int d, int x, int y, const State& state) {
  IntervalMap hi, lo;
  IntervalQueue q;

  printf("---------------------------\n");
  printf("Starting SATURATE size:%d\n", d);

  for (int z = 0; z < d - 1; z++) {
    for (int t = z + 1; t < d; t++) {
      auto zt = std::make_pair(z, t);
      hi[zt] = FormulaSet();
      lo[zt] = FormulaSet{ Formula::truth() };
      for (auto i = 0U; i < state.phi.rules.size(); i++)
        hi[zt].insert(Formula::create(CLAUSE, i));

      q.push(zt);
    }
  }

  auto xy = std::make_pair(x, y);
  for (auto f : state.phi.facts) {
    hi[xy].insert(f);
  }


  bool changed = true;
  while (true) {
    changed = false;
    auto zt = q.front();
    int z = zt.first;
    int t = zt.second;
    q.pop();

    std::vector<Formula> formulas (hi[zt].begin(), hi[zt].end());
    for (auto f : formulas) {

      printState(state, hi);
      printState(state, lo);
      printFormula(state, f);
      printf("\n");
      getchar();

      if (f.type == LETTER && f.id == 1) {
        hi[zt].erase(f);

      } else if (f.type == LETTER) {
        hi[zt].erase(f);
        if (lo[zt].insert(f).second) changed = true;

      } else if (f.type == BOXA) {
        hi[zt].erase(f);
        if (lo[zt].insert(f).second) changed = true;
        for (int r = t + 1; r < d; r++)
          if (lo[std::make_pair(t, r)].insert(Formula::create(LETTER, f.id)).second) changed = true;

      } else if (f.type == BOXA_BAR) {
        hi[zt].erase(f);
        if (lo[zt].insert(f).second) changed = true;
        for (int r = 0; r < z; r++)
          if (lo[std::make_pair(r, z)].insert(Formula::create(LETTER, f.id)).second) changed = true;

      } else if (f.type == CLAUSE) {
        Clause& clause = state.phi.rules[f.id];
        Formula last = clause.back();
        if (last.type != LETTER || last.id != 0) {
          bool found = true;
          for (auto it = clause.begin(); it != clause.end()-1; it++) {
            auto l = *it;
            if (lo[zt].find(l) == lo[zt].end()) {
              found = false;
              break;
            }
          }
          if (found) {
            hi[zt].erase(f);
            // ho davvero bisogno di spostare la clausola in lo?
            //if (lo[zt].insert(f).second) changed = true;
            if (hi[zt].insert(last).second) changed = true;
          }

        } else {
          printf("clause with FALSE conclusion\n");
          bool found = true;
          for (auto it = clause.begin(); it != clause.end()-1; it++) {
            auto l = *it;
            if (lo[zt].find(l) == lo[zt].end()) {
              printf("didn't find literal: ");
              printFormula(state, l);
              printf("\n");
              found = false;
              break;
            }
          }
          if (found) return false;
        }
      }
    }

    changed = extend(d, hi, lo, state) || changed;
    q.push(zt);
  }

  return true;
}

bool extend(int d, IntervalMap& hi, IntervalMap& lo, const State& state) {
  bool changed = false;
  int min, max;

  if (state.caseType == FINITE) {
    min = 0;
    max = d;
  } else {
    min = 0;
    max = d - 2;

    for (int z = min; z < max; z++) {
      hi[std::make_pair(z, max+1)] = hi[std::make_pair(z, max)];
      lo[std::make_pair(z, max+1)] = lo[std::make_pair(z, max)];
    }

    Interval last = std::make_pair(max, max+1);
    for (auto f : lo[last]) {
      if (f.type == LETTER) {
        if (lo[last].insert(Formula::create(BOXA, f.id)).second) changed = true;
      } else if (f.type == BOXA) {
        if (lo[last].insert(Formula::create(LETTER, f.id)).second) changed = true;
      } else if (f.type == BOXA_BAR) {
        if (lo[last].insert(Formula::create(LETTER, f.id)).second) changed = true;
      }
    }

    if (state.caseType == DISCRETE) {
      min = 1;
      max = d - 1;

      for (int z = min + 1; z <= max; z++) {
        hi[std::make_pair(0, z)] = hi[std::make_pair(1, z)];
        lo[std::make_pair(0, z)] = lo[std::make_pair(1, z)];
      }

      Interval first = std::make_pair(0, 1);
      for (auto f : lo[first]) {
        if (f.type == LETTER) {
          if (lo[first].insert(Formula::create(BOXA_BAR, f.id)).second) changed = true;
        }
        else if (f.type == BOXA) {
          if (lo[first].insert(Formula::create(LETTER, f.id)).second) changed = true;
        }
        else if (f.type == BOXA_BAR) {
          if (lo[first].insert(Formula::create(LETTER, f.id)).second) changed = true;
        }
      }
    }
  }

  for (int z = min; z < max; z++) {
    for (auto f : state.boxa) {

      bool found = true;
      for (int t = z + 1; t < d; t++) {
        auto p = Formula::create(LETTER, f.id);
        if (lo[std::make_pair(z, t)].count(p) == 0) {
          found = false;
          break;
        }
      }
      if (found) {
        for (int r = 0; r < z; r++)
          if (lo[std::make_pair(r, z)].insert(f).second) changed = true;
      }
    }
    for (auto f : state.boxaBar) {

      bool found = true;
      for (int r = 0; r < z; r++) {
        auto p = Formula::create(LETTER, f.id);
        if (lo[std::make_pair(r, z)].count(p) == 0) {
          found = false;
          break;
        }
      }
      if (found) {
        for (int t = z + 1; t < d; t++)
          if (lo[std::make_pair(z, t)].insert(f).second) changed = true;
      }
    }
  }

  return changed;
}
