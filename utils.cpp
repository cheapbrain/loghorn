
#include "horn.hpp"

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

void printState(const InputClauses& phi, const IntervalMap &map, int d) {
	for (int z = 0; z < d - 1; z++) {
		for (int t = z + 1; t < d; t++) {
			auto i = map.find(std::make_pair(z, t));
			printInterval(phi, i->first, i->second);
		}
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


