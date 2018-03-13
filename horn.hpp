#pragma once

#include <iostream>
#include <fstream>
#include <streambuf>
#include <string>
#include <cstring>
#include <cctype>
#include <cstdlib>
#include <ctime>
#include <vector>
#include <queue>
#include <unordered_map>
#include <unordered_set>
#include <iterator>
#include <algorithm>
#include <thread>
#include <mutex>
#include <chrono>

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

/* Satisfiability Checker */
int check(InputClauses& phi, Case caseType);
bool saturate(int d, int x, int y, const State& phi);
int extend(int d, IntervalMap& hi, IntervalMap& lo, const State& phi);

/* Print Utilities */
void printFormula(const InputClauses& phi, const Formula f, bool universal);
void printInterval(const InputClauses& phi, const Interval& interval, const FormulaSet& formulas);
void printState(const InputClauses& phi, const IntervalMap &map, int d);

/* Parse / Generator Utilities */
InputClauses parseFile(const char* path);
InputClauses randomInput(int n_clauses, int letters, int clause_len);


