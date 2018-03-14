
#include "horn.hpp"

Clause newClause(const std::vector<int>& arr) {
	Clause c;
	for (auto i = 0U; i < arr.size(); i += 2) {
		c.push_back(Formula::create(static_cast<FormulaType>(arr[i]), arr[i+1]));
	}
	return c;
}

std::string numToLabel(int n) {
	std::string out = "";
	do {
		int ch = (n % 26) + 97;
		out = ((char)ch) + out;
		n = n / 26;
	} while (n > 0);
	return out;
}

int rand(int max) {
	return rand() % max;
}

float frand() {
	return (float)rand() / RAND_MAX;
}

InputClauses randomInput(int n_clauses, int letters, int clause_len, float falsehood_rate) {
	InputClauses phi = {};
	phi.labels.push_back("F");
	phi.labels.push_back("T");

	std::vector<Formula> symbols;
	//symbols.push_back(Formula::create(BOXA, 0));
	//symbols.push_back(Formula::create(BOXA_BAR, 0));
	
	if (letters * 3 < clause_len) {
		printf("The number of letters is too small compared to the clause length\n");
		return phi;
	}

	for (int i = 0; i < letters; i++) {
		std::string label = numToLabel(i);
		phi.labels.push_back(label);

		symbols.push_back(Formula::create(BOXA_BAR, i+2));
		symbols.push_back(Formula::create(BOXA, i+2));
		symbols.push_back(Formula::create(LETTER, i+2));
	}

	for (int i = 0; i < n_clauses; i++) {
		std::vector<int> buff;
		Clause c;

		std::vector<Formula> availableSymbols = symbols;
		for (int j = 0; j < clause_len; j++) {

			if ((j == clause_len-1) && frand() < falsehood_rate) {
				c.push_back(Formula::falsehood());

			} else {
				int ns = availableSymbols.size();
				int index = rand(ns);
				Formula f = availableSymbols[index];
				availableSymbols[index] = availableSymbols[ns-1];
				availableSymbols.resize(ns-1);
				c.push_back(f);
			}

		}
		phi.rules.push_back(c);

	}

	std::vector<Formula> availableSymbols = symbols;
	for (int i = 0; i < clause_len - 1; i++) {
		int ns = availableSymbols.size();
		int index = rand(ns);
		Formula f = availableSymbols[index];
		availableSymbols[index] = availableSymbols[ns - 1];
		availableSymbols.resize(ns - 1);
		phi.facts.push_back(f);
	}

	return phi;
}
