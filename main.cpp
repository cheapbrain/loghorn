
#include "argh.h"
#include "horn.hpp"

#include "utils.cpp"
#include "generate.cpp"

std::mutex stdout_mutex;
std::mutex generate_mutex;

bool print_messages = false;

void fprint(FILE *stream, InputClauses &phi) {
	fprintf(stream, "---- Rules ----\n");
	for (size_t i = 0; i < phi.rules.size(); i++) {
		printFormula(stream, phi, Formula::create(CLAUSE, i), true);
		fprintf(stream, "\n");
	}

	fprintf(stream, "---- Facts ----\n");
	for (auto fact : phi.facts) {
		printFormula(stream, phi, fact, false);
		fprintf(stream, "\n");
	}
	fprintf(stream, "---------------\n\n");
}
void print(InputClauses &phi) {
	fprint(stdout, phi);
}

void print(std::vector<int> v) {
	printf("{ ");
	for (auto i : v) {
		printf("%d ", i);
	}
	printf("} ");
}

void print(std::vector<std::vector<int>> v) {
	for (auto c : v) {
		print(c);
		printf("\n");
	}
}

void allPossibleClauses(FormulaVector &symbols, int start, std::vector<Clause> &clauses, Clause &clause) {

	if (clause.size() > 0) {
		for (auto f : symbols) {
			if (std::find(clause.begin(), clause.end(), f) != clause.end()) continue;

			Clause finalClause(clause);
			finalClause.push_back(f);
			clauses.push_back(finalClause);
		}

		Clause falseClause(clause);
		falseClause.push_back(Formula::falsehood());
		clauses.push_back(falseClause);
	}

	for (size_t i = start; i < symbols.size(); i++) {
		clause.push_back(symbols[i]);
		allPossibleClauses(symbols, i+1, clauses, clause);
		clause.pop_back();
	}
}

void allPossibleClauses(FormulaVector &symbols, int start, std::vector<Clause> &clauses) {
	Clause empty = {};
	allPossibleClauses(symbols, start, clauses, empty);
}

void allPossibleInputs(std::vector<Clause> &clauses, int start, InputClauses &phi, std::vector<InputClauses> &inputs) {
	for (size_t i = start; i < clauses.size(); i++) {
		phi.rules.push_back(clauses[i]);
		inputs.push_back(phi);
		allPossibleInputs(clauses, i+1, phi, inputs);
		phi.rules.pop_back();
	}
}

bool skipInput(std::vector<Clause> &clauses, InputClauses &phi) {
	if (phi.rules.size() == 0) {
		return false;
	}

	auto last = phi.rules.back();
	auto pos = find(clauses.begin(), clauses.end(), last) - clauses.begin() + 1;
	phi.rules.pop_back();
	if (pos >= (int)clauses.size()) {
		return skipInput(clauses, phi);
	} else {
		phi.rules.push_back(clauses[pos]);
		return true;
	}
}

bool nextInput(std::vector<Clause> &clauses, InputClauses &phi) {
	if (phi.rules.size() == 0) {
		phi.rules.push_back(clauses[0]);
		return true;
	}

	auto last = phi.rules.back();
	auto pos = find(clauses.begin(), clauses.end(), last) - clauses.begin() + 1;
	if (pos >= (int)clauses.size()) {
		return skipInput(clauses, phi);
	} else {
		phi.rules.push_back(clauses[pos]);
		return true;
	}
}

std::vector<int> setUnion(std::vector<int> &a, std::vector<int> &b) {
	if (a.size() == 0) return b;
	if (b.size() == 0) return a;

	std::vector<int> c;
	c.reserve(a.size() + b.size());

	auto first1 = a.begin();
	auto last1 = a.end();
	auto first2 = b.begin();
	auto last2 = b.end();

	while (true) {
		if (first1 == last1) { c.insert(c.end(), first2, last2); break; }
		if (first2 == last2) { c.insert(c.end(), first1, last1); break; }

		if (*first1 < *first2) { c.push_back(*first1); ++first1; }
		else if (*first1 > *first2) { c.push_back(*first2); ++first2; }
		else { c.push_back(*first1); ++first1; ++first2; }
	}


	return c;
}

void buildSet(std::vector<std::vector<int>> &old, std::vector<std::vector<int>> &out, std::vector<int> &temp, int start, int depth, unsigned int size) {
	auto tempsize = temp.size();
	for (size_t i = start; i < old.size() - depth; i++) {
		auto newSet = setUnion(old[i], temp);

		if (depth > 0) {
			if (tempsize == 0 || newSet.size() <= tempsize+1) {
				buildSet(old, out, newSet, i + 1, depth - 1, size);
			}
		} else if (newSet.size() == size+1) {
			out.push_back(newSet);
		}
	}
}

void printPropertyError(InputClauses &phi, Model &model, int s, int t, int w, int z) {
	stdout_mutex.lock();
	fprint(stderr, phi);
	fprintf(stderr, "The property doesn't hold true at {%d, %d} and {%d, %d}\n", s, t, w, z);
	printState(stderr, phi, model.lo, model.lo.size());
	stdout_mutex.unlock();
}

bool checkMinimumModelAndLog(InputClauses &phi, Model &model) {

	if (!model.satisfied) {
		return true;
	}

	for (auto t = 1; t < (int)model.lo.size() - 1; t++) {
		if (t == model.start.first || t == model.start.second) continue;

		const auto &aRequestsCurrent = model.lo.get(0, t);
		const auto &aRequestsNext = model.lo.get(0, t+1);
		for (auto f: aRequestsCurrent) {
			if (f.type == BOXA && aRequestsNext.count(f) == 0) {
				printPropertyError(phi, model, 0, t, 0, t+1);
				return false;
			}
		}
	}

	for (auto t = 2; t < (int)model.lo.size(); t++) {
		if (t == model.start.first || t == model.start.second) continue;

		const auto &aRequestsCurrent = model.lo.get(0, t);
		const auto &aRequestsPrevious = model.lo.get(0, t-1);
		for (auto f: aRequestsCurrent) {
			if (f.type == BOXA_BAR && aRequestsPrevious.count(f) == 0) {
				printPropertyError(phi, model, 0, t-1, 0, t);
				return false;
			}
		}
	}

	return true;
}

std::vector<InputClauses> genInputBatch(std::vector<Clause> &clauses, InputClauses &inputTemplate, int OriginalTargetLength, int batchSize, int maxFalseClauses) {

	generate_mutex.lock();
	std::vector<InputClauses> batch(batchSize);

	for (auto &phi : batch) {
		phi = inputTemplate;
		int remaining = clauses.size();
		int targetLength = OriginalTargetLength;
		int falseClausesCount = 0;

		for (auto clause : clauses) {
			float value = (float)rand() / RAND_MAX;
			float k = (float)targetLength / remaining;

			if (value < k) {

				if (maxFalseClauses > 0 && clause.back().id == FALSEHOOD) {

					if (falseClausesCount >= maxFalseClauses) {
						--remaining;
						continue;
					} else {
						++falseClausesCount;
					}
				}

				phi.rules.push_back(clause);
				--targetLength;
			}
			--remaining;
		}
	}
	generate_mutex.unlock();

	return batch;
}

void workerLoop(std::vector<Clause> &clauses, InputClauses &inputTemplate, Case caseType, int numClauses, int batchSize, int maxFalseClauses) {
	using namespace std::chrono;

	while (true) {

		auto batch = genInputBatch(clauses, inputTemplate, 
			numClauses, batchSize, maxFalseClauses);

		for (auto &phi : batch) {

			auto t1 = high_resolution_clock::now();
			Model model = check(phi, caseType);
			auto t2 = high_resolution_clock::now();

			double time = (duration_cast<duration<double>>(t2 - t1)).count();
			printf("%d\t%d\t%d\t%s\t%.7f\n", 
				(int)phi.labels.size()-2, (int)phi.rules.size(), 
				(int)model.lo.size(), model.satisfied ? "YES" : "NO", time);

			checkMinimumModelAndLog(phi, model);
		}

	}

}

Case parseCaseType(const std::string &caseName) {
	for (int i = 0; i <= INVALID_CASE; i++) {
		if (!strcmp(caseStrings[i], caseName.c_str())) {
			return (Case)i;
		}
	}
	return INVALID_CASE;
}

void runCheckAndLog(InputClauses &phi, Case caseType) {
	printf("Starting check of the %s case.\n", caseStrings[caseType]);

	Model model = check(phi, caseType);

	if (model.satisfied) {
		printf("The clause set is SATISFIABLE in the %s case, "
			"with size %d and starting interval [%d, %d]\n", 
			caseStrings[caseType], (int)model.lo.size(), 
			model.start.first, model.start.second );
	} else {
		printf("The clause set is NOT SATISFIABLE in the %s case\n", caseStrings[caseType]);
	}
}

int main(int argc, char **argv) {
	srand((unsigned int)time(NULL));

	auto cmdl = argh::parser(argc, argv, 
		argh::parser::PREFER_PARAM_FOR_UNREG_OPTION | 
		argh::parser::SINGLE_DASH_IS_MULTIFLAG);

	bool bench, verbose;
	std::string fileName, caseName;
	int numThreads, numLetters, numClauses, batchSize, maxFalseClauses;

	bench = cmdl[{"-b", "--bench"}];
	verbose = cmdl[{"-v", "--verbose"}];
	cmdl({"-f", "--file"}, "NOFILE") >> fileName;
	cmdl({"-m", "--model_type"}, "FINITE") >> caseName;
	if (!(cmdl({"-t", "--num_threads"}, 1) >> numThreads)) 
		{ fprintf(stderr, "Pass a valid integer as the number of threads\n"); return 1; }
	if (!(cmdl({"-l", "--num_letters"}, 3) >> numLetters)) 
		{ fprintf(stderr, "Pass a valid integer as the number of letters\n"); return 1; }
	if (!(cmdl({"-c", "--num_clauses"}, 4) >> numClauses)) 
		{ fprintf(stderr, "Pass a valid integer as the number of clauses\n"); return 1; }
	if (!(cmdl({"-n", "--batch_size"}, 1) >> batchSize)) 
		{ fprintf(stderr, "Pass a valid integer as the batch size\n"); return 1; }
	if (!(cmdl({"--max_false_clauses"}, 0) >> maxFalseClauses)) 
		{ fprintf(stderr, "Pass a valid integer as the max number of false clauses\n"); return 1; }

	for (auto & c: caseName) {
		c = toupper(c); 
	}
	Case caseType = parseCaseType(caseName);
	if (caseType == INVALID_CASE) 
		{ fprintf(stderr, "Invalid model type, use: FINITE, NATURAL, DISCRETE, ALL_CASES\n"); return 1; }
	if (fileName == "NOFILE" && caseType == ALL_CASES) 
		{ fprintf(stderr, "You can't use ALL_CASES with random generated input\n"); return 1; }

	if (verbose) {
		print_messages = true;
	}

	if (fileName == "NOFILE") {

		std::vector<std::string> labels;
		FormulaVector symbols;

		labels.push_back("F");
		labels.push_back("T");

		// creates all the symbols used for generating the clauses
		for (int i = 0; i < numLetters; i++) {
			std::string label = numToLabel(i);
			labels.push_back(label);

			symbols.push_back(Formula::create(LETTER, i+2));
			symbols.push_back(Formula::create(BOXA, i+2));
			symbols.push_back(Formula::create(BOXA_BAR, i+2));
		}

		std::vector<Clause> clauses;
		allPossibleClauses(symbols, 0, clauses);

		InputClauses inputTemplate;
		inputTemplate.labels = labels;
		inputTemplate.facts.push_back(Formula::create(LETTER, 2));

		if (bench) {

			printf("%s\t%s\t%s\t%s\t%s\n", "NUM_LETTERS", "NUM_CLAUSES", "MODEL_SIZE", "SATISFIED", "TIME(s)");
			// reduce the number of clauses by removing the ones that are not satisfiable
			{
				print_messages = false;
				std::vector<Clause> tempClauses;
				for (auto i = clauses.size(); i-- > 0; ) {
					inputTemplate.rules.push_back(clauses[i]);
					Model model = check(inputTemplate, caseType);
					if (model.satisfied) { tempClauses.push_back(clauses[i]); }
					inputTemplate.rules.clear();
				}
				clauses = tempClauses;
				if (verbose) { print_messages = true; }
			}

			std::vector<std::thread> threads;
			for (int threadId = 0; threadId < numThreads; threadId++) {
				std::thread th(workerLoop, std::ref(clauses), std::ref(inputTemplate), caseType, numClauses, batchSize, maxFalseClauses);
				threads.push_back(std::move(th));
			}

			for (auto &th : threads) {
				th.join();
			}

		} else {
			auto batch = genInputBatch(clauses, inputTemplate, numClauses, batchSize, maxFalseClauses);
			for (auto &phi : batch) {
				runCheckAndLog(phi, caseType);
			}

		}

	} else {
		InputClauses phi = parseFile(fileName.c_str());

		if (caseType == ALL_CASES) {
			std::vector<std::thread> threads;
			threads.push_back(std::thread(runCheckAndLog, std::ref(phi), FINITE));
			threads.push_back(std::thread(runCheckAndLog, std::ref(phi), NATURAL));
			threads.push_back(std::thread(runCheckAndLog, std::ref(phi), DISCRETE));

			for (auto &th : threads) {
				th.join();
			}

		} else {
			runCheckAndLog(phi, caseType);

		}
	}

	return 0;
}

Model check(InputClauses &phi, Case caseType) {
	int min, max;
	switch (caseType) {
		case FINITE:	min = 2; break;
		case NATURAL:	min = 3; break;
		case DISCRETE:	min = 4; break;
		default:		return Model::unsatisfied();
	}
	max = min + 6 * phi.rules.size(); 

	if (print_messages) {
		stdout_mutex.lock();
		print(phi);
		stdout_mutex.unlock();
	}

	State state = {caseType, phi};
	FormulaSet literals(phi.facts.begin(), phi.facts.end());
	for (auto& clause : phi.rules) {
		std::copy(clause.begin(), clause.end(), std::inserter(literals, literals.end()));
	}
	for (auto l : literals) {
		if (l.type == BOXA) {
			state.boxa.push_back(l);
		} else if (l.type == BOXA_BAR) {
			state.boxaBar.push_back(l);
		}
	}

	int xmin = (caseType == DISCRETE);

	for (int k = min; k <= max; k++) {
		int ymax = k - (caseType != FINITE);

		if (print_messages) {
			stdout_mutex.lock();
			printf("%s - checking with size: %d\n", caseStrings[caseType], k);
			stdout_mutex.unlock();
		}

		for (int x = xmin; x < ymax - 1; x++) {
			for (int y = x + 1; y < ymax; y++) {
				Model solution = saturate(k, x, y, state);
				if (solution.satisfied) {
					return solution;
				}
			}
		}
	}

	return Model::unsatisfied();
}

Model saturate(int d, int x, int y, const State& state) {
	IntervalVector<FormulaVector> hi(d);
	IntervalVector<FormulaSet> lo(d);

	for (int z = 0; z < d - 1; z++) {
		for (int t = z + 1; t < d; t++) {

			lo.get(z, t).insert(Formula::truth());

			auto& hizt = hi.get(z, t);
			for (auto i = 0U; i < state.phi.rules.size(); i++) {
				hizt.push_back(Formula::create(CLAUSE, i));
			}

		}
	}

	auto& hixy = hi.get(x, y);
	for (auto f : state.phi.facts) {
		hixy.push_back(f);
	}

	bool changed = true;
	while (changed) {
		changed = false;

		for (int z = 0; z < d -1; z++) {
			for (int t = z + 1; t < d; t++) {
				auto& hizt = hi.get(z, t);

				for (auto ii = hizt.size(); ii-- > 0; ) {
					auto f = hizt[ii];

					if (f.type == LETTER && f.id == TRUTH) {
						eraseFast(hizt, ii);

					} else if (f.type == LETTER && f.id == FALSEHOOD) {
						lo.get(z, t).insert(f);
						return Model::unsatisfied();

					} else if (f.type == LETTER) {
						eraseFast(hizt, ii);
						if (lo.get(z, t).insert(f).second) changed = true;

					} else if (f.type == BOXA) {
						eraseFast(hizt, ii);
						if (lo.get(z, t).insert(f).second) changed = true;
						for (int r = t + 1; r < d; r++) {
							if (lo.get(t, r).insert(Formula::create(LETTER, f.id)).second) changed = true;
							if (f.id == FALSEHOOD) return Model::unsatisfied();
						}

					} else if (f.type == BOXA_BAR) {
						eraseFast(hizt, ii);
						if (lo.get(z, t).insert(f).second) changed = true;
						for (int r = 0; r < z; r++) {
							if (lo.get(r, z).insert(Formula::create(LETTER, f.id)).second) changed = true;
							if (f.id == FALSEHOOD) return Model::unsatisfied();
						}

					} else if (f.type == CLAUSE) {
						Clause& clause = state.phi.rules[f.id];
						Formula last = clause.back();
						bool found = true;
						auto& lozt = lo.get(z, t);
						for (auto it = clause.begin(); it != clause.end()-1; it++) {
							auto l = *it;
							if (lozt.find(l) == lozt.end()) {
								found = false;
								break;
							}
						}

						if (found) {
							eraseFast(hizt, ii);
							lozt.insert(f);
							hizt.push_back(last); 
							changed = true;
						}
					}

				}

			}
		}

		int res = extend(d, hi, lo, state);
		changed = changed || (res == 1);

		if (res == 2) {
			return Model::unsatisfied();
		}
	}

	if (print_messages) {
		stdout_mutex.lock();
		printState(state.phi, lo, d);
		stdout_mutex.unlock();
	}
	return Model(lo, true, Interval(x, y));
}

int extend(int d, IntervalVector<FormulaVector>& hi, IntervalVector<FormulaSet>& lo, const State& state) {
	int changed = false;
	int min, max;

	//printState(state.phi, lo, d);
	//printState(state.phi, hi, d);

	if (state.caseType == FINITE) {
		min = 0;
		max = d;
	} else {
		min = 0;
		max = d - 2;

		for (int z = min; z < max; z++) {
			auto& hizm1 = hi.get(z, max+1);
			for (auto f : hi.get(z, max)) {
				if (f.type != CLAUSE) {
					hizm1.push_back(f);
					changed = 1;
				}
			}
			auto& lozm1 = lo.get(z, max+1);
			for (auto f : lo.get(z, max)) {
				if (f.type != CLAUSE) {
					if (lozm1.insert(f).second) changed = 1;
				}
			}
		}

		std::vector<Formula> temp;
		auto& last = lo.get(max, max+1);
		for (auto f : last) {
			if (f.type == LETTER) {
				temp.push_back(Formula::create(BOXA, f.id));
			} else if (f.type == BOXA) {
				if (f.id == FALSEHOOD) return 2;
				temp.push_back(Formula::create(LETTER, f.id));
			} else if (f.type == BOXA_BAR) {
				if (f.id == FALSEHOOD) return 2;
				temp.push_back(Formula::create(LETTER, f.id));
			}
		}
		for (auto f: temp) {
			if (last.insert(f).second) changed = 1;
		}
		temp.clear();

		if (state.caseType == DISCRETE) {
			min = 1;
			max = d - 1;

			for (int z = min + 1; z <= max; z++) {
				auto& hi0z = hi.get(0, z);
				for (auto f : hi.get(1, z)) {
					if (f.type != CLAUSE) {
						hi0z.push_back(f);
						changed = 1;
					}
				}
				auto& lo0z = lo.get(0, z);
				for (auto f : lo.get(1, z)) {
					if (f.type != CLAUSE) {
						if (lo0z.insert(f).second) changed = 1;
					}
				}
			}

			auto& first = lo.get(0, 1);
			for (auto f : first) {
				if (f.type == LETTER) {
					temp.push_back(Formula::create(BOXA_BAR, f.id));
				} else if (f.type == BOXA) {
					if (f.id == FALSEHOOD) return 2;
					temp.push_back(Formula::create(LETTER, f.id));
				} else if (f.type == BOXA_BAR) {
					if (f.id == FALSEHOOD) return 2;
					temp.push_back(Formula::create(LETTER, f.id));
				}
			}
			for (auto f: temp) {
				if (first.insert(f).second) changed = 1;
			}
		}
	}

	for (int z = min; z < max; z++) {

		for (auto f : state.boxa) {
			bool found = 1;
			for (int t = z + 1; t < d; t++) {
				auto p = Formula::create(LETTER, f.id);
				if (lo.get(z, t).count(p) == 0) {
					found = false;
					break;
				}
			}
			if (found) {
				for (int r = 0; r < z; r++) {
					if (lo.get(r, z).insert(f).second) changed = 1;
				}
			}
		}

		for (auto f : state.boxaBar) {
			bool found = 1;
			for (int r = 0; r < z; r++) {
				auto p = Formula::create(LETTER, f.id);
				if (lo.get(r, z).count(p) == 0) {
					found = false;
					break;
				}
			}
			if (found) {
				for (int t = z + 1; t < d; t++) {
					if (lo.get(z, t).insert(f).second) changed = 1;
				}
			}
		}

	}

	return changed;
}
