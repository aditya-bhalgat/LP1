#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <map>
#include <iomanip> 
using namespace std;

struct Symbol { string name; int addr; };
struct Literal { string lit; int addr; };

vector<Symbol> symtab;
vector<Literal> littab;

void loadSymtab() {
    ifstream file("symtab.txt");
    string line;
    getline(file, line);
    while (getline(file, line)) {
        stringstream ss(line);
        int idx, addr;
        string name;
        ss >> idx >> name >> addr;
        symtab.push_back({name, addr});
    }
}

void loadLittab() {
    ifstream file("littab.txt");
    string line;
    getline(file, line);
    while (getline(file, line)) {
        stringstream ss(line);
        int idx, addr;
        string lit;
        ss >> idx >> lit >> addr;
        littab.push_back({lit, addr});
    }
}

int getSymbolAddr(int index) {
    if (index - 1 < (int)symtab.size())
        return symtab[index - 1].addr;
    return -1;
}

int getLiteralAddr(int index) {
    if (index - 1 < (int)littab.size())
        return littab[index - 1].addr;
    return -1;
}

void pass2() {
    ifstream icFile("intermediate.txt");
    ofstream mcFile("machinecode.txt");
    if (!icFile) {
        cerr << "Error: intermediate.txt not found!\n";
        return;
    }
    string line;
    while (getline(icFile, line)) {
        stringstream ss(line);
        string token;
        vector<string> parts;
        while (ss >> token) {
            parts.push_back(token);
        }
        if (parts.empty()) continue;
        string opcodePart;
        int operandStartIdx;
        if (parts[0].find('(') != string::npos) {
            continue;
        } else {
            opcodePart = parts[1];
            operandStartIdx = 2;
        }
        if (opcodePart.find("(IS") != string::npos) {
            int opcode = stoi(opcodePart.substr(4, 2));
            string regField = "0";
            string memField = "000";
            for (int i = operandStartIdx; i < (int)parts.size(); i++) {
                if (parts[i].find("(R") != string::npos) {
                    regField = parts[i].substr(3, 1);
                } 
                else if (parts[i].find("(CC") != string::npos) {
                    regField = parts[i].substr(4, 1);
                }
                else if (parts[i].find("(S") != string::npos) {
                    int symIndex = stoi(parts[i].substr(3, parts[i].size() - 4));
                    int addr = getSymbolAddr(symIndex);
                    memField = to_string(addr);
                }
                else if (parts[i].find("(L") != string::npos) {
                    int litIndex = stoi(parts[i].substr(3, parts[i].size() - 4));
                    int addr = getLiteralAddr(litIndex);
                    memField = to_string(addr);
                }
            }
            mcFile << setfill('0') << setw(2) << opcode << " "
                   << regField << " "
                   << setfill('0') << setw(3) << memField << "\n";
        }
        else if (opcodePart.find("(DL,01") != string::npos) {
            int val = stoi(parts[operandStartIdx].substr(3, parts[operandStartIdx].size() - 4));
            mcFile << "00 0 " << setfill('0') << setw(3) << val << "\n";
        }
        else if (opcodePart.find("(DL,02") != string::npos) {
            int size = stoi(parts[operandStartIdx].substr(3, parts[operandStartIdx].size() - 4));
            for (int i = 0; i < size; i++) {
                mcFile << "00 0 000\n";
            }
        }
        else if (opcodePart.find("(AD") != string::npos) {}
    }
    cout << "Pass 2 completed \n";
    cout << "Generated: machinecode.txt\n";
    icFile.close();
    mcFile.close();
}

int main() {
    loadSymtab();
    loadLittab();
    pass2();
    return 0;
}
