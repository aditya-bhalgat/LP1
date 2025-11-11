#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <map>
#include <iomanip>
using namespace std;

// --- Global Data Structures (Unchanged) ---
map<string, pair<string, int>> MOT = {
    {"STOP", {"IS", 0}}, {"ADD", {"IS", 1}}, {"SUB", {"IS", 2}},
    {"MULT", {"IS", 3}}, {"MOVER", {"IS", 4}}, {"MOVEM", {"IS", 5}},
    {"COMP", {"IS", 6}}, {"BC", {"IS", 7}}, {"DIV", {"IS", 8}},
    {"READ", {"IS", 9}}, {"PRINT", {"IS", 10}}
};
map<string, pair<string, int>> AD = {
    {"START", {"AD", 1}}, {"END", {"AD", 2}},
    {"ORIGIN", {"AD", 3}}, {"EQU", {"AD", 4}}, {"LTORG", {"AD", 5}}
};
map<string, pair<string, int>> DL = { {"DC", {"DL", 1}}, {"DS", {"DL", 2}} };
map<string, int> REG = { {"AREG", 1}, {"BREG", 2}, {"CREG", 3}, {"DREG", 4} };
map<string, int> CC  = { {"LT", 1}, {"LE", 2}, {"EQ", 3}, {"GT", 4}, {"GE", 5}, {"ANY", 6} };

struct Symbol { string name; int addr; bool defined = false; };
struct Literal { string lit; int addr; };

vector<Symbol> symtab;
vector<Literal> littab;
vector<int> pooltab;

int LC = 0;
int literalPoolStart = 0;

// --- Small helpers (just 3) ---
int searchSymbol(string s){ for(int i=0;i<(int)symtab.size();++i) if(symtab[i].name==s) return i; return -1; }
int searchLiteral(string s){ for(int i=0;i<(int)littab.size();++i) if(littab[i].lit==s) return i; return -1; }

bool isNumber(const string& s){
    if(s.empty()) return false;
    int i = (s[0]=='+'||s[0]=='-')?1:0;
    if(i==(int)s.size()) return false;
    for(; i<(int)s.size(); ++i) if(!isdigit((unsigned char)s[i])) return false;
    return true;
}

// number | SYMBOL | SYMBOL+number | SYMBOL-number
int evalExpr(const string& e){
    if(isNumber(e)) return stoi(e);
    size_t p = e.find_first_of("+-");
    if(p==string::npos){ // plain symbol
        int si = searchSymbol(e);
        if(si==-1){ symtab.push_back({e,-1,false}); return 0; }
        return symtab[si].addr;
    }
    string left = e.substr(0,p), right = e.substr(p+1);
    int base = 0, si = searchSymbol(left);
    if(si==-1) symtab.push_back({left,-1,false});
    else base = symtab[si].addr;
    int off = isNumber(right)? stoi(right) : 0;
    return (e[p]=='+') ? base+off : base-off;
}

// DC operand: handles numbers and quoted chars/strings like '9' or "A"
int parseDC(const string& t){
    if(isNumber(t)) return stoi(t);
    if(t.size()>=2 && ((t.front()=='\''&&t.back()=='\'')||(t.front()=='"'&&t.back()=='"'))){
        string mid = t.substr(1, t.size()-2);
        if(isNumber(mid)) return stoi(mid);
        return mid.empty()?0:(int)mid[0];
    }
    return 0;
}

// Process pending literals
void processLiterals(ofstream &icFile) {
    if ((int)littab.size() > literalPoolStart) pooltab.push_back(literalPoolStart + 1);
    for (int i = literalPoolStart; i < (int)littab.size(); i++) {
        if (littab[i].addr == -1) {
            icFile << LC << " (DL,01) (C," << littab[i].lit.substr(2, littab[i].lit.length() - 3) << ")\n";
            littab[i].addr = LC;
            LC++;
        }
    }
    literalPoolStart = (int)littab.size();
}

// Core
void processLine(vector<string> tokens, ofstream &icFile) {
    if (tokens.empty()) return;

    if (tokens[0] == "START") {
        LC = stoi(tokens[1]);                 // input uses plain number
        icFile << LC << " (AD,01) (C," << tokens[1] << ")\n";
        return;
    }

    int idx = 0;
    string potentialLabel = tokens[0];
    if (MOT.count(potentialLabel)==0 && DL.count(potentialLabel)==0 && AD.count(potentialLabel)==0) {
        int pos = searchSymbol(potentialLabel);
        if (pos == -1) { symtab.push_back({potentialLabel, LC, false}); pos = (int)symtab.size() - 1; }
        if (symtab[pos].defined) cerr << "Error: Duplicate label definition: " << potentialLabel << endl;
        else { symtab[pos].addr = LC; symtab[pos].defined = true; }
        idx = 1;
    }
    if ((int)tokens.size() <= idx) return;

    string op = tokens[idx];

    if (AD.count(op)) {
        if (op == "END") {
            processLiterals(icFile);
            icFile << "(AD,02)\n";
        }
        else if (op == "LTORG") {
            icFile << "(AD,05)\n";
            processLiterals(icFile);
        }
        else if (op == "ORIGIN") {             // changed: evalExpr
            int v = evalExpr(tokens[idx+1]);
            icFile << "(AD,03) (C," << v << ")\n";
            LC = v;
        }
        else if (op == "EQU") {                // changed: evalExpr
            int symPos = searchSymbol(tokens[idx-1]);
            if (symPos != -1) {
                int v = evalExpr(tokens[idx+1]);
                symtab[symPos].addr = v;
                icFile << "(AD,04) (C," << v << ")\n";
            }
        }
        return;
    }

    int currentLC = LC;

    if (MOT.count(op)) {
        icFile << currentLC << " (IS," << setw(2) << setfill('0') << MOT[op].second << ") ";
        for (int i = idx + 1; i < (int)tokens.size(); i++) {
            if (REG.count(tokens[i])) icFile << "(R," << REG[tokens[i]] << ") ";
            else if (CC.count(tokens[i])) icFile << "(CC," << CC[tokens[i]] << ") ";
            else if (tokens[i][0] == '=') {
                int litIndex = searchLiteral(tokens[i]);
                if (litIndex == -1) { littab.push_back({tokens[i], -1}); litIndex = (int)littab.size() - 1; }
                icFile << "(L," << litIndex + 1 << ") ";
            } else {
                int pos = searchSymbol(tokens[i]);
                if (pos == -1) { symtab.push_back({tokens[i], -1, false}); pos = (int)symtab.size() - 1; }
                icFile << "(S," << pos + 1 << ") ";
            }
        }
        icFile << "\n";
        LC++;
    }
    else if (DL.count(op)) {
        if (op == "DS") {
            int size = stoi(tokens[idx + 1]);          // input uses plain number
            icFile << currentLC << " (DL,02) (C," << size << ")\n";
            LC += size;
        }
        else if (op == "DC") {                          // changed: parseDC
            int val = parseDC(tokens[idx + 1]);
            icFile << currentLC << " (DL,01) (C," << val << ")\n";
            LC++;
        }
    }
}

int main() {
    ifstream inFile("input.txt");
    ofstream icFile("intermediate.txt");
    ofstream symFile("symtab.txt");
    ofstream litFile("littab.txt");
    ofstream poolFile("pooltab.txt");

    if (!inFile) { cerr << "Error: input.txt not found!\n"; return 1; }

    string line;
    while (getline(inFile, line)) {
        stringstream ss(line);
        vector<string> tokens; string word;
        while (ss >> word) { if (!word.empty() && word.back() == ',') word.pop_back(); tokens.push_back(word); }
        processLine(tokens, icFile);
    }

    if ((int)littab.size() > literalPoolStart) pooltab.push_back(literalPoolStart + 1);

    symFile << "Index\tSymbol\tAddress\n";
    for (int i = 0; i < (int)symtab.size(); i++)
        symFile << i + 1 << "\t" << symtab[i].name << "\t" << symtab[i].addr << "\n";

    litFile << "Index\tLiteral\tAddress\n";
    for (int i = 0; i < (int)littab.size(); i++)
        litFile << i + 1 << "\t" << littab[i].lit << "\t" << littab[i].addr << "\n";

    poolFile << "Pool#\tStartIndex\n";
    for (int i = 0; i < (int)pooltab.size(); i++)
        poolFile << i + 1 << "\t" << pooltab[i] << "\n";

    cout << "Pass 1 completed \n";
    cout << "Generated: intermediate.txt, symtab.txt, littab.txt, pooltab.txt\n";
    return 0;
}
