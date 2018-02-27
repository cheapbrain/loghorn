
#include "horn.hpp"

#include "utils.cpp"
#include "generate.cpp"

std::mutex stdout_mutex;

int main(int argc, char **argv) {
	srand(time(NULL));

	auto filename = "test.horn";
	auto caseType = ALL;

	InputClauses phi;

	if (argc > 1) {
		filename = argv[1];
		phi = parseFile(filename);
	} else {
		phi = randomInput();
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
		stdout_mutex.lock();
		printf("Starting check of the %s case.\n", caseStrings[caseType]);
		stdout_mutex.unlock();
		std::thread th(check, std::ref(phi), caseType);
		threads.push_back(std::move(th));
	}

	for (auto &th : threads) {
		th.join();
	};

	printf("Done.\n");
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
		stdout_mutex.lock();
		printf("%s - checking with size: %d\n", caseStrings[caseType], k);
		stdout_mutex.unlock();
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

	stdout_mutex.lock();
	printf("The formula is SATISFIABLE in the %s case, with size %d and starting interval [%d, %d]:\n", 
			caseStrings[state.caseType], d, x, y );
	printState(state.phi, lo, d);
	stdout_mutex.unlock();
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
