#include <iostream>
#include <vector>
#include <queue>
#include <unordered_set>
#include <unordered_map>

using namespace std;

// FIFO page faults (unchanged)
int fifo(const vector<int>& pages, int frames) {
    if (frames <= 0) return pages.size();
    unordered_set<int> inFrame;
    queue<int> q;
    int faults = 0;
    for (int p : pages) {
        if (inFrame.find(p) == inFrame.end()) {
            if ((int)inFrame.size() == frames) {
                int old = q.front(); q.pop();
                inFrame.erase(old);
            }
            inFrame.insert(p);
            q.push(p);
            ++faults;
        }
    }
    return faults;
}

// LRU using a vector (linear-time per access)
int lru_vector(const vector<int>& pages, int frames) {
    if (frames <= 0) return pages.size();
    vector<int> recent; // front = most recent, back = least recent
    int faults = 0;

    for (int p : pages) {
        // search for page p in recent
        int idx = -1;
        for (int i = 0; i < (int)recent.size(); ++i) {
            if (recent[i] == p) { idx = i; break; }
        }

        if (idx == -1) {
            // page fault
            ++faults;
            if ((int)recent.size() == frames) {
                // remove least recently used (back)
                recent.pop_back();
            }
            // insert p at front (most recent)
            recent.insert(recent.begin(), p);
        } else {
            // page hit -> move the found element to front
            recent.erase(recent.begin() + idx);       // remove old position
            recent.insert(recent.begin(), p);         // re-insert at front
        }
    }
    return faults;
}

// Optimal page faults (unchanged)
int optimal(const vector<int>& pages, int frames) {
    if (frames <= 0) return pages.size();
    unordered_set<int> inFrame;
    int faults = 0;

    for (size_t i = 0; i < pages.size(); ++i) {
        int p = pages[i];
        if (inFrame.find(p) == inFrame.end()) {
            if ((int)inFrame.size() == frames) {
                int toReplace = -1;
                size_t farthestNext = 0;
                for (int f : inFrame) {
                    size_t j = i + 1;
                    for (; j < pages.size() && pages[j] != f; ++j);
                    if (j == pages.size()) { toReplace = f; break; }
                    if (j > farthestNext) { farthestNext = j; toReplace = f; }
                }
                inFrame.erase(toReplace);
            }
            inFrame.insert(p);
            ++faults;
        }
    }
    return faults;
}

int main() {
    ios::sync_with_stdio(false);
    cin.tie(nullptr);

    int frames;
    cout << "Enter number of frames: ";
    if (!(cin >> frames) || frames < 0) { cerr << "Invalid frame count.\n"; return 1; }

    int n;
    cout << "Enter number of pages in reference string: ";
    if (!(cin >> n) || n < 0) { cerr << "Invalid number of pages.\n"; return 1; }

    vector<int> pages(n);
    cout << "Enter the reference string pages (space separated):\n";
    for (int i = 0; i < n; ++i) cin >> pages[i];

    cout << "\nPage Faults:\n";
    cout << "FIFO:     " << fifo(pages, frames) << '\n';
    cout << "LRU(vec): " << lru_vector(pages, frames) << '\n';
    cout << "Optimal:  " << optimal(pages, frames) << '\n';

    return 0;
}
