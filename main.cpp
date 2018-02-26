#include <iostream>
#include <fstream>
#include <streambuf>
#include <string>
#include <cstring>
#include <cctype>
#include <vector>
#include <queue>
#include <unordered_map>
#include <unordered_set>
#include <iterator>
#include <algorithm>
#include <thread>
#include <mutex>

enum FormulaType {
	LETTER,
	BOXA,
	BOXA_BAR,
	CLAUSE,
	INVALID,
};

enum Case {
	FINITE,
	NATURAL,
	DISCRETE,
	ALL,
};

const char *caseStrings[] = {
	"FINITE",
	"NATURAL",
	"DISCRETE",
	"ALL",
};


#define FALSEHOOD 0
#define TRUTH 1

struct Formula {
	FormulaType type;
	int id;

	static Formula create(FormulaType type, int id) { return { type, id }; }
	static Formula truth() { return { LETTER, TRUTH }; }
	static Formula falsehood() { return { LETTER, FALSEHOOD }; }
};

typedef std::pair<int, int> Interval;
typedef std::vector<Formula> Clause;

struct InputClauses {
	std::vector<Clause> rules;
	std::vector<Formula> facts;
	std::vector<std::string> labels;
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
int extend(int d, IntervalMap& hi, IntervalMap& lo, const State& phi);

void printFormula(const InputClauses& phi, const Formula f, bool universal) {
	auto prefix = universal ? "[U] " : "";
	if (f.type == CLAUSE) {
		Clause c = phi.rules[f.id];
		for (auto& l : c) {
			if (l == c.back())       printf("%s", " -> ");
			else if (l == c.front()) printf("%s", prefix);
			else                     printf("%s", " & ");
			printFormula(phi, l, false);
		}
		return;

	} else if (f.type == BOXA) {
		printf("[A]");
	} else if (f.type == BOXA_BAR) {
		printf("[P]");
	}

	printf("%s", phi.labels[f.id].c_str());
}

void printInterval(const InputClauses& phi, const Interval& interval, const FormulaSet& formulas) {
	if (formulas.size() == 0) return;
	printf("[%d, %d]: ",interval.first, interval.second);
	for(auto f: formulas) {
		printf("\n\t");
		printFormula(phi, f, false);
	}
	printf("\n");
}

void printState(const InputClauses& phi, const IntervalMap &map) {
	for(auto &i : map) {
		printInterval(phi, i.first, i.second);
	}
	printf("\n");
}

struct TokInfo {
	int pos;
	int len;
	bool alphanum;
};

bool findToken(const char* text, TokInfo& state) {
	state.pos += state.len;
	state.len = 0;

	int i = state.pos;
	while(1) {
		if (text[i] == '\0') return false;
		if (!std::isspace(text[i])) break;
		i++;
	}
	state.pos = i;

	enum {LETTER, OPERATOR, OTHER};
	int type = OTHER;
	if (std::isalnum(text[i])) type = LETTER;
	else if (text[i] == '[') type = OPERATOR;

	while (text[i] != '\0') {
		char c = text[i];
		if (std::isspace(c)) break;
		bool found = false;
		switch(type) {
			case LETTER: 
				found = (!std::isalnum(c));
				break;
			case OPERATOR: 
				if (c == ']') {
					found = true;
					i++;
				}
				break;
			case OTHER: 
				found = (std::isalnum(c) || c == '[');
				break;
		}
		if (found) break;
		i++;
	}

	state.len = i - state.pos;
	return true;
}

Formula parseFormula(const std::string& line, TokInfo& token, InputClauses& phi) {
	Formula f = {};
	std::string text = line.substr(token.pos, token.len);

	if (text.compare("[A]") == 0) {
		f.type = BOXA;
	} else if (text.compare("[B]") == 0) {
		f.type = BOXA_BAR;
	} else {
		f.type = LETTER;
	}

	if (f.type != LETTER) {
		findToken(line.c_str(), token);
		text = line.substr(token.pos, token.len);
	}

	for(auto c = text.begin(); c != text.end(); c++) {
		if (!std::isalnum(*c)) return Formula::create(INVALID, 0);
	}

	for(size_t i = 0; i < phi.labels.size(); i++) {
		if (text.compare(phi.labels[i]) == 0) {
			f.id = i;
			return f;
		}
	}

	f.id = phi.labels.size();
	phi.labels.push_back(text);
	return f;
}

void exitError(const char* text, int line, const std::string& token) {
	std::cerr << "Error on line " << line << ", at \"" << token << "\": " << text << std::endl;
	exit(-1);
}

InputClauses parseFile(const char* path) {
	std::ifstream fp(path);
	std::cout << "Reading file: " << path << "\n";
	int lineNum = 0;
	InputClauses phi = {};
	phi.labels.push_back("F");
	phi.labels.push_back("T");
	std::string line;

	while (std::getline(fp, line)) {
		auto cline = line.c_str();
		TokInfo token = {};
		lineNum++;

		bool hasNext = findToken(cline, token);
		if (!hasNext) continue;
		
		if (line.substr(token.pos, token.len).compare("[U]") != 0) {
			auto f = parseFormula(line, token, phi);
			if (f.type == INVALID) exitError("This is not a valid formula.", lineNum, line.substr(token.pos, token.len));
			phi.facts.push_back(f);
			continue;
		} 

		Clause clause = {};
		do {
			if (!findToken(cline, token)) exitError("Missing formula at the end of line.", lineNum, line);

			auto f = parseFormula(line, token, phi);
			if (f.type == INVALID) exitError("This is not a valid formula.", lineNum, line.substr(token.pos, token.len));
			clause.push_back(f);

			hasNext = findToken(cline, token);
		} while (hasNext);

		phi.rules.push_back(clause);
	}

	return phi;
}

Clause newClause(const std::vector<int>& arr) {
	Clause c;
	for (auto i = 0U; i < arr.size(); i += 2) {
		c.push_back(Formula::create(static_cast<FormulaType>(arr[i]), arr[i+1]));
	}
	return c;
}


std::mutex stdout_mutex;


int main(int argc, char **argv) {

	auto filename = "test.horn";
	auto caseType = ALL;

	if (argc > 1) {
		filename = argv[1];
	}

	if (argc > 2) {
		if (strcmp(argv[2], "FINITE") == 0) caseType = FINITE;
		else if (strcmp(argv[2], "NATURAL") == 0) caseType = NATURAL;
		else if (strcmp(argv[2], "DISCRETE") == 0) caseType = DISCRETE;
		else if (strcmp(argv[2], "ALL") == 0) caseType = ALL;
		else {
			printf("The case \"%s\" is not valid.\n", argv[2]);
			return 1;
		}
	}

	std::vector<Case> caseTypes;
	if (caseType == ALL)
		caseTypes = { FINITE, NATURAL, DISCRETE };
	else
		caseTypes = { caseType };


	InputClauses phi = parseFile(filename);

	printf("---- Rules ----\n");
	for (size_t i = 0; i < phi.rules.size(); i++) {
		printFormula(phi, Formula::create(CLAUSE, i), true);
		printf("\n");
	}

	printf("---- Facts ----\n");
	for (auto fact : phi.facts) {
		printFormula(phi, fact, false);
		printf("\n");
	}
	printf("---------------\n\n");

	std::vector<std::thread> threads;
	for (auto caseType : caseTypes) {
		printf("Starting check of the %s case.\n", caseStrings[caseType]);
		std::thread th(check, std::ref(phi), caseType);
		threads.push_back(std::move(th));
	}

	for (auto &th : threads) {
		th.join();
	};

	printf("Done.\n");
	return 0;




	return 0;
}

bool check(InputClauses &phi, Case caseType) {
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

	int xmin = (caseType == DISCRETE);

	for (int k = min; k <= max; k++) {
		int ymax = k - (caseType != FINITE);
		for (int x = xmin; x < ymax - 1; x++)
			for (int y = x + 1; y < ymax; y++)
				if (saturate(k, x, y, state))
					return true;
	}

	printf("The formula is NOT SATISFIABLE in the %s case.\n", caseStrings[caseType]);
	return false;
}

bool saturate(int d, int x, int y, const State& state) {
	IntervalMap hi, lo;

	//printf("Starting SATURATE size:%d, starting interval: [%d, %d]\n", d, x, y);

	for (int z = 0; z < d - 1; z++) {
		for (int t = z + 1; t < d; t++) {
			auto zt = std::make_pair(z, t);
			hi[zt] = FormulaSet();
			lo[zt] = FormulaSet{ Formula::truth() };

			for (auto i = 0U; i < state.phi.rules.size(); i++)
				hi[zt].insert(Formula::create(CLAUSE, i));
		}
	}

	auto xy = std::make_pair(x, y);
	for (auto f : state.phi.facts) {
		hi[xy].insert(f);
	}

	bool changed = true;
	while (changed) {
		changed = false;

		for (int z = 0; z < d -1; z++) {
			for (int t = z + 1; t < d; t++) {
				auto zt = std::make_pair(z, t);

				std::vector<Formula> formulas (hi[zt].begin(), hi[zt].end());
				for (auto f : formulas) {

					if (f.type == LETTER && f.id == TRUTH) {
						hi[zt].erase(f);

					} else if (f.type == LETTER && f.id == FALSEHOOD) {
						lo[zt].insert(f);
						return false;

					} else if (f.type == LETTER) {
						hi[zt].erase(f);
						if (lo[zt].insert(f).second) changed = true;

					} else if (f.type == BOXA) {
						hi[zt].erase(f);
						if (lo[zt].insert(f).second) changed = true;
						for (int r = t + 1; r < d; r++)
							if (hi[std::make_pair(t, r)].insert(Formula::create(LETTER, f.id)).second) changed = true;

					} else if (f.type == BOXA_BAR) {
						hi[zt].erase(f);
						if (lo[zt].insert(f).second) changed = true;
						for (int r = 0; r < z; r++)
							if (hi[std::make_pair(r, z)].insert(Formula::create(LETTER, f.id)).second) changed = true;

					} else if (f.type == CLAUSE) {
						Clause& clause = state.phi.rules[f.id];
						Formula last = clause.back();
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
							//lo[zt].insert(f);
							if (hi[zt].insert(last).second) changed = true;
						}
					}
				}

				int res = extend(d, hi, lo, state);
				changed = changed || (res == 1);

				if (res == 2) {
					return false;
				}

			}
		}

	}

	printf("The formula is SATISFIABLE in the %s case, with size %d and starting interval [%d, %d]:\n", 
			caseStrings[state.caseType], d, x, y);
	printState(state.phi, lo);
	return true;
}

int extend(int d, IntervalMap& hi, IntervalMap& lo, const State& state) {
	int changed = false;
	int min, max;

	if (state.caseType == FINITE) {
		min = 0;
		max = d;
	} else {
		min = 0;
		max = d - 2;

		for (int z = min; z < max; z++) {
			for (auto f : hi[std::make_pair(z, max)]) {
				if (hi[std::make_pair(z, max+1)].insert(f).second) changed = 1;
			}
			for (auto f : lo[std::make_pair(z, max)]) {
				if (lo[std::make_pair(z, max+1)].insert(f).second) changed = 1;
			}
		}

		Interval last = std::make_pair(max, max+1);
		for (auto f : lo[last]) {
			if (f.type == LETTER) {
				if (lo[last].insert(Formula::create(BOXA, f.id)).second) changed = 1;
			} 
			else if (f.type == BOXA) {
				if (f.id == FALSEHOOD) return 2;
				if (lo[last].insert(Formula::create(LETTER, f.id)).second) changed = 1;
			} 
			else if (f.type == BOXA_BAR) {
				if (f.id == FALSEHOOD) return 2;
				if (lo[last].insert(Formula::create(LETTER, f.id)).second) changed = 1;
			}
		}

		if (state.caseType == DISCRETE) {
			min = 1;
			max = d - 1;

			for (int z = min + 1; z <= max; z++) {
				for (auto f : hi[std::make_pair(1, z)]) {
					if (hi[std::make_pair(0, z)].insert(f).second) changed = 1;
				}
				for (auto f : lo[std::make_pair(1, z)]) {
					if (lo[std::make_pair(0, z)].insert(f).second) changed = 1;
				}
			}

			Interval first = std::make_pair(0, 1);
			for (auto f : lo[first]) {
				if (f.type == LETTER) {
					if (lo[first].insert(Formula::create(BOXA_BAR, f.id)).second) changed = 1;
				}
				else if (f.type == BOXA) {
					if (f.id == FALSEHOOD) return 2;
					if (lo[first].insert(Formula::create(LETTER, f.id)).second) changed = 1;
				}
				else if (f.type == BOXA_BAR) {
					if (f.id == FALSEHOOD) return 2;
					if (lo[first].insert(Formula::create(LETTER, f.id)).second) changed = 1;
				}
			}
		}
	}

	for (int z = min; z < max; z++) {

		for (auto f : state.boxa) {
			bool found = 1;
			for (int t = z + 1; t < d; t++) {
				auto p = Formula::create(LETTER, f.id);
				if (lo[std::make_pair(z, t)].count(p) == 0) {
					found = false;
					break;
				}
			}
			if (found) {
				for (int r = 0; r < z; r++)
					if (lo[std::make_pair(r, z)].insert(f).second) changed = 1;
			}
		}

		for (auto f : state.boxaBar) {
			bool found = 1;
			for (int r = 0; r < z; r++) {
				auto p = Formula::create(LETTER, f.id);
				if (lo[std::make_pair(r, z)].count(p) == 0) {
					found = false;
					break;
				}
			}
			if (found) {
				for (int t = z + 1; t < d; t++)
					if (lo[std::make_pair(z, t)].insert(f).second) changed = 1;
			}
		}

	}

	return changed;
}
