#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <unordered_map>
#include <string>
#include <algorithm>
using namespace std;

string trim(const string &s) {
    int a = s.find_first_not_of(" \t\r\n");
    if (a == -1) return "";
    int b = s.find_last_not_of(" \t\r\n");
    return s.substr(a, b - a + 1);
}

void replaceAll(string &line, const string &from, const string &to) {
    int pos = 0;
    while ((pos = line.find(from, pos)) != string::npos) {
        line.replace(pos, from.size(), to);
        pos += to.size();
    }
}

vector<string> splitArgs(const string &s) {
    string t = trim(s);
    vector<string> args;
    if (t.empty()) return args;

    bool hasComma = (t.find(',') != string::npos);
    string token;
    stringstream ss(t);

    if (hasComma) {
        while (getline(ss, token, ',')) {
            token = trim(token);
            if (!token.empty()) args.push_back(token);
        }
    } else {
        while (ss >> token) args.push_back(token);
    }
    return args;
}

int main() {
    ifstream mntFile("mnt.txt");
    if (!mntFile) { cerr << "Cannot open mnt.txt\n"; return 1; }

    unordered_map<string, int> MNT;
    string macroName;
    int mdtIndex;
    while (mntFile >> macroName >> mdtIndex) MNT[macroName] = mdtIndex;
    mntFile.close();

    ifstream mdtFile("mdt.txt");
    if (!mdtFile) { cerr << "Cannot open mdt.txt\n"; return 1; }

    vector<string> MDT;
    string line;
    getline(mdtFile, line); // handle stray newline
    while (getline(mdtFile, line)) MDT.push_back(line);
    mdtFile.close();

    ifstream inter("intermediate.txt");
    ofstream out("expanded.txt");
    if (!inter || !out) { cerr << "File error\n"; return 1; }

    string raw;
    while (getline(inter, raw)) {
        string s = trim(raw);
        if (s.empty()) { out << "\n"; continue; }

        stringstream ss(s);
        vector<string> tokens;
        string tok;
        while (ss >> tok) tokens.push_back(tok);

        string label = "", macro = "";

        if (!tokens.empty() && tokens[0].back() == ':') {
            label = tokens[0];
            if (tokens.size() > 1) macro = tokens[1];
        } else if (tokens.size() > 1 && MNT.find(tokens[1]) != MNT.end()) {
            label = tokens[0];
            macro = tokens[1];
        } else if (!tokens.empty()) {
            macro = tokens[0];
        }

        if (macro.empty() || MNT.find(macro) == MNT.end()) {
            out << raw << "\n";
            continue;
        }

        // Extract arguments
        int pos = raw.find(macro);
        string argStr = (pos != -1) ? trim(raw.substr(pos + macro.size())) : "";
        vector<string> args = splitArgs(argStr);

        int start = MNT[macro];
        bool firstLine = true;
        for (int i = start; i < MDT.size(); i++) {
            string body = MDT[i];
            if (trim(body) == "MEND") break;

            for (int j = 0; j < args.size(); j++) {
                string ph = "#" + to_string(j);
                replaceAll(body, ph, args[j]);
            }

            body = trim(body);
            if (firstLine && !label.empty()) {
                out << label << " " << body << "\n";
                firstLine = false;
            } else {
                out << body << "\n";
            }
        }
    }

    cout << "Pass 2 complete → expanded.txt\n";
    return 0;
}
