
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

InputClauses randomInput() {
	InputClauses phi = {};
	phi.labels.push_back("F");
	phi.labels.push_back("T");

	int n_clauses = 10;
	int clause_len = 3;
	int letters = 2;

	for (int i = 0; i < letters; i++) {
		std::string label = numToLabel(i);
		phi.labels.push_back(label);
	}

	for (int i = 0; i < n_clauses; i++) {
		std::vector<int> buff;
		Clause c;
		bool has_a = false;
		bool has_abar = false;
		for (int j = 0; j < clause_len; j++) {
			FormulaType type;
			int id;
			for (;;) {
				type = (FormulaType) rand(3);
				id = (rand(letters+1) + 2) % (letters + 2);

				if (id == FALSEHOOD && type == LETTER && j < clause_len -1) continue;
				if (((has_a && type == BOXA) || (has_abar && type == BOXA_BAR)) && j < clause_len - 1) continue;
				
				if (type == BOXA) has_a = true;
				if (type == BOXA_BAR) has_abar = true;
				break;
			}

				printf("%d %d\n", type, id);
			c.push_back(Formula::create(type, id));
		}
		phi.rules.push_back(c);

	}

	phi.facts.push_back(Formula::create(LETTER, 2));

	return phi;
}
