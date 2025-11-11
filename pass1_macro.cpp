#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <unordered_map>
#include <string>
#include <cctype>

using namespace std;

struct MNTEntry {
    string macroName;
    int mdtIndex; // 0-based index into MDT where the macro body starts
};

vector<MNTEntry> MNT;
vector<string> MDT;
// store ALA list per macro (ordered list of formal parameter names, without &)
vector<vector<string>> ALA_per_macro;

string trim(const string &str) {
    int first = str.find_first_not_of(" \t\r\n");
    if (first == string::npos) return "";
    int last = str.find_last_not_of(" \t\r\n");
    return str.substr(first, (last - first + 1));
}

vector<string> split(const string &line, char delimiter = ',') {
    vector<string> tokens;
    stringstream ss(line);
    string token;
    while (getline(ss, token, delimiter)) {
        tokens.push_back(trim(token));
    }
    return tokens;
}

// Replace occurrences of "&param" in a line with placeholder "#idx"
string replaceAmpParam(const string &line, const string &param, int idx) {
    string out;
    string pattern = "&" + param;
    string placeholder = "#" + to_string(idx);

    size_t pos = 0;
    while (pos < line.size()) {
        size_t found = line.find(pattern, pos);
        if (found == string::npos) {
            out.append(line.substr(pos));
            break;
        }
        // append up to found, then placeholder, then continue
        out.append(line.substr(pos, found - pos));
        out.append(placeholder);
        pos = found + pattern.length();
    }
    return out;
}

// parse macro header line: "MACNAME &A, &B"
// returns pair(macroName, ordered vector of formal parameter names (no &))
pair<string, vector<string>> parseMacroHeader(const string &headerLine) {
    stringstream ss(headerLine);
    string macroName;
    ss >> macroName;
    string rest;
    getline(ss, rest);
    rest = trim(rest);
    vector<string> params;
    if (!rest.empty()) {
        vector<string> parts = split(rest, ',');
        for (string p : parts) {
            p = trim(p);
            // expect leading '&' — remove it
            if (!p.empty() && p.front() == '&') p = p.substr(1);
            if (!p.empty()) params.push_back(p);
        }
    }
    return {macroName, params};
}

void writeTables() {
    ofstream mntFile("mnt.txt");
    for (auto &e : MNT) {
        mntFile << e.macroName << " " << e.mdtIndex << endl;
    }
    mntFile.close();

    ofstream mdtFile("mdt.txt");
    for (auto &line : MDT) {
        mdtFile << line << endl;
    }
    mdtFile.close();

    ofstream alaFile("ala.txt");
    // Write ALA per macro in a readable way:
    for (int m = 0; m < (int)MNT.size(); ++m) {
        alaFile << MNT[m].macroName << ":" << endl;
        for (int i = 0; i < (int)ALA_per_macro[m].size(); ++i) {
            alaFile << i << " " << ALA_per_macro[m][i] << endl;
        }
        alaFile << endl;
    }
    alaFile.close();
}

void pass1(const string &inputFile = "source.asm") {
    ifstream input(inputFile);
    if (!input) {
        cerr << "Error: could not open " << inputFile << endl;
        return;
    }

    ofstream intermediate("intermediate.txt");
    if (!intermediate) {
        cerr << "Error: could not create intermediate.txt" << endl;
        return;
    }

    string line;
    bool inMacroDef = false;

    while (getline(input, line)) {
        string rawLine = line;
        string tline = trim(line);

        if (tline.empty()) {
            // preserve blank lines in intermediate (only outside macro defs)
            if (!inMacroDef) intermediate << rawLine << endl;
            continue;
        }

        if (tline == "MACRO") {
            // start macro definition; next line must be header
            if (!getline(input, line)) {
                cerr << "Unexpected EOF after MACRO" << endl;
                break;
            }
            string header = trim(line);
            auto parsed = parseMacroHeader(header);
            string macroName = parsed.first;
            vector<string> formals = parsed.second;

            // record MNT entry pointing to the index where body will start
            int mdtIndex = (int)MDT.size();
            MNT.push_back({macroName, mdtIndex});
            // store ALA for this macro
            ALA_per_macro.push_back(formals);

            inMacroDef = true;
            continue; // do not write header into MDT
        }

        if (tline == "MEND") {
            // add MEND to MDT and finish macro
            MDT.push_back("MEND");
            inMacroDef = false;
            continue;
        }

        if (inMacroDef) {
            // process body line: replace only "&param" with positional placeholders
            string processed = line;
            const vector<string> &formals = ALA_per_macro.back();
            for (int i = 0; i < (int)formals.size(); ++i) {
                processed = replaceAmpParam(processed, formals[i], i);
            }
            MDT.push_back(processed);
        } else {
            // outside macro: copy to intermediate (macro calls remain as-is)
            intermediate << rawLine << endl;
        }
    }

    input.close();
    intermediate.close();

    writeTables();
    cout << "Pass 1 complete. Files written: mnt.txt, mdt.txt, ala.txt, intermediate.txt" << endl;
}

int main() {
    pass1("input.txt"); // change to your source filename if needed
    return 0;
}
