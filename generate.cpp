
#include "horn.hpp"

std::mt19937 rng;


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
	std::uniform_int_distribution<int> dis(0, max-1);
	int n = dis(rng);
	return n;
}

float frand() {
	std::uniform_real_distribution<float> dis(0.f,1.f);
	float f = dis(rng);
	return f;
}

InputClauses randomInput(int n_clauses, int letters, int clause_len, int max_falsehood) {
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

			int put_falsehood = (max_falsehood > 0);
			if ((j == clause_len-1) && frand() < 0.5f * put_falsehood) {
				c.push_back(Formula::falsehood());
				max_falsehood--;
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
