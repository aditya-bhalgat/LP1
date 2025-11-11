#include <iostream>
#include <vector>
#include <queue>
#include <algorithm>
#include <climits>
using namespace std;

struct Process {
    int pid;
    int arrival;
    int burst;
    int priority;
    int remaining;   // used by preemptive algorithms
    int completion;
    int turnaround;
    int waiting;
};

// Print table of results
void printTable(const vector<Process>& procs) {
    cout << "\nPID\tAT\tBT\tPRI\tCT\tTAT\tWT\n";
    for (const auto &p : procs) {
        cout << p.pid << '\t' << p.arrival << '\t' << p.burst << '\t'
             << p.priority << '\t' << p.completion << '\t'
             << p.turnaround << '\t' << p.waiting << '\n';
    }
}

// 1) FCFS - First Come First Serve (non-preemptive)
void FCFS(vector<Process> procs) {
    cout << "\n--- FCFS ---\n";
    sort(procs.begin(), procs.end(), [](const Process &a, const Process &b){
        return a.arrival < b.arrival;
    });

    int currentTime = 0;
    for (auto &p : procs) {
        if (currentTime < p.arrival) currentTime = p.arrival;
        p.completion = currentTime + p.burst;
        p.turnaround = p.completion - p.arrival;
        p.waiting = p.turnaround - p.burst;
        currentTime = p.completion;
    }
    printTable(procs);
}

// 2) Preemptive SJF (Shortest Remaining Time First)
void SJF_Preemptive(vector<Process> procs) {
    cout << "\n--- SJF (Preemptive) ---\n";
    int n = procs.size();
    for (auto &p : procs) p.remaining = p.burst;

    int completed = 0;
    int time = 0;

    while (completed < n) {
        // find process with smallest remaining time that has arrived
        int idx = -1;
        int bestRem = INT_MAX;
        for (int i = 0; i < n; ++i) {
            if (procs[i].arrival <= time && procs[i].remaining > 0) {
                if (procs[i].remaining < bestRem) {
                    bestRem = procs[i].remaining;
                    idx = i;
                }
            }
        }

        if (idx == -1) {
            // no job available now
            time++;
            continue;
        }

        // execute 1 unit of time
        procs[idx].remaining--;
        time++;

        // if finished, record times
        if (procs[idx].remaining == 0) {
            completed++;
            procs[idx].completion = time;
            procs[idx].turnaround = procs[idx].completion - procs[idx].arrival;
            procs[idx].waiting = procs[idx].turnaround - procs[idx].burst;
        }
    }

    printTable(procs);
}

// 3) Priority Scheduling (Non-preemptive)
void Priority_NonPreemptive(vector<Process> procs) {
    cout << "\n--- Priority (Non-preemptive) ---\n";
    int n = procs.size();
    vector<bool> done(n, false);
    int finished = 0;
    int time = 0;

    while (finished < n) {
        int idx = -1;
        int bestPriority = INT_MAX;
        for (int i = 0; i < n; ++i) {
            if (!done[i] && procs[i].arrival <= time) {
                if (procs[i].priority < bestPriority) {
                    bestPriority = procs[i].priority;
                    idx = i;
                }
            }
        }

        if (idx == -1) {
            time++;
            continue;
        }

        // run process to completion (non-preemptive)
        time += procs[idx].burst;
        procs[idx].completion = time;
        procs[idx].turnaround = procs[idx].completion - procs[idx].arrival;
        procs[idx].waiting = procs[idx].turnaround - procs[idx].burst;
        done[idx] = true;
        finished++;
    }

    printTable(procs);
}

// 4) Round Robin (Preemptive)
void RoundRobin(vector<Process> procs, int quantum) {
    cout << "\n--- Round Robin (q = " << quantum << ") ---\n";
    int n = procs.size();
    for (auto &p : procs) p.remaining = p.burst;

    queue<int> q;
    int time = 0;
    int finished = 0;
    vector<bool> added(n, false);

    // add processes that arrive at time 0
    for (int i = 0; i < n; ++i) {
        if (procs[i].arrival == 0) {
            q.push(i);
            added[i] = true;
        }
    }

    while (finished < n) {
        if (q.empty()) {
            // no process ready — advance time and add any arrived
            time++;
            for (int i = 0; i < n; ++i) {
                if (!added[i] && procs[i].arrival <= time && procs[i].remaining > 0) {
                    q.push(i);
                    added[i] = true;
                }
            }
            continue;
        }

        int idx = q.front(); q.pop();
        int run = min(quantum, procs[idx].remaining);
        procs[idx].remaining -= run;
        time += run;

        // add newly arrived processes during this quantum
        for (int i = 0; i < n; ++i) {
            if (!added[i] && procs[i].arrival <= time && procs[i].remaining > 0) {
                q.push(i);
                added[i] = true;
            }
        }

        if (procs[idx].remaining == 0) {
            finished++;
            procs[idx].completion = time;
            procs[idx].turnaround = procs[idx].completion - procs[idx].arrival;
            procs[idx].waiting = procs[idx].turnaround - procs[idx].burst;
        } else {
            // not finished, put back to queue
            q.push(idx);
        }
    }

    printTable(procs);
}

int main() {
    int n;
    cout << "Number of processes: ";
    cin >> n;

    vector<Process> procs(n);
    for (int i = 0; i < n; ++i) {
        procs[i].pid = i + 1;
        cout << "Enter arrival, burst, priority for P" << i+1 << " : ";
        cin >> procs[i].arrival >> procs[i].burst >> procs[i].priority;
    }

    while (true) {
        cout << "\nMenu:\n1) FCFS\n2) SJF (Preemptive)\n3) Priority (Non-preemptive)\n4) Round Robin\n5) Exit\nChoose: ";
        int choice; cin >> choice;
        if (choice == 1) FCFS(procs);
        else if (choice == 2) SJF_Preemptive(procs);
        else if (choice == 3) Priority_NonPreemptive(procs);
        else if (choice == 4) {
            int q; cout << "Time quantum: "; cin >> q;
            RoundRobin(procs, q);
        }
        else if (choice == 5) break;
        else cout << "Invalid option\n";
    }

    cout << "Goodbye!\n";
    return 0;
}
