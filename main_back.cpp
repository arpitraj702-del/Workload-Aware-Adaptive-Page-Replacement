// ============================================================================
//  Workload-Aware Adaptive Page Replacement Simulator  (terminal version)
//
//  Algorithms : FIFO, LRU, Optimal (benchmark only), Adaptive (FIFO <-> LRU)
//  Compile    : g++ -std=c++11 -Wall -Wextra -o pagesim main.cpp
//  Run        : ./pagesim            (Windows: .\pagesim.exe)
//  Self-test  : ./pagesim --test
//
//  File layout (read top to bottom):
//    1. Constants and data structures
//    2. Small helper functions
//    3. The four algorithms
//    4. Workload generators
//    5. Printing
//    6. Input validation
//    7. Self-tests
//    8. Menu / main
// ============================================================================

#include <iostream>
#include <vector>
#include <string>
#include <sstream>
#include <iomanip>
#include <algorithm>
#include <cstdint>

using namespace std;

// ----------------------------------------------------------------------------
// 1. Constants and data structures
// ----------------------------------------------------------------------------
const int MAX_FRAMES = 20;   // largest number of frames the user may choose
const int MAX_REFS   = 200;  // longest reference string accepted
const int MAX_PAGE   = 999;  // largest page number accepted
const int EMPTY      = -1;   // marks an empty frame slot

// Adaptive rule settings (explained in the project notes):
const int    ADAPT_WINDOW    = 8;     // look at the last 8 references
const double ADAPT_THRESHOLD = 0.30;  // >= 30% repeats in window -> use LRU

// What happened on ONE page reference (used to print frame contents).
struct Step {
    int         page;     // page that was referenced
    bool        hit;      // true = hit, false = fault
    vector<int> frames;   // frame contents AFTER this reference
    int         evicted;  // page removed (EMPTY if nothing was removed)
    string      note;     // for Adaptive: which rule picked the victim
};

// The outcome of running one algorithm over the whole reference string.
struct Result {
    string       name;
    int          hits = 0;
    int          faults = 0;
    int          fifoEvictions = 0;   // used only by Adaptive
    int          lruEvictions = 0;    // used only by Adaptive
    vector<Step> steps;
};

// ----------------------------------------------------------------------------
// 2. Small helper functions
// ----------------------------------------------------------------------------

// Returns the slot index holding 'page', or -1 if the page is not in memory.
int findPage(const vector<int>& frames, int page) {
    for (size_t i = 0; i < frames.size(); i++)
        if (frames[i] == page) return (int)i;
    return -1;
}

// Returns the index of the first empty slot, or -1 if memory is full.
int findEmpty(const vector<int>& frames) {
    for (size_t i = 0; i < frames.size(); i++)
        if (frames[i] == EMPTY) return (int)i;
    return -1;
}

// Percentage helper that never divides by zero.
double percent(int part, int total) {
    return total == 0 ? 0.0 : 100.0 * part / total;
}

// Saves one step into the result.
void recordStep(Result& r, int page, bool hit, const vector<int>& frames,
                int evicted, const string& note) {
    Step s;
    s.page = page;
    s.hit = hit;
    s.frames = frames;
    s.evicted = evicted;
    s.note = note;
    r.steps.push_back(s);
}

// ----------------------------------------------------------------------------
// 3. The four algorithms
//    All of them: take the reference string + number of frames, return a Result.
// ----------------------------------------------------------------------------

// ---- FIFO: evict the page that has been in memory the longest --------------
// Pages are loaded into slots 0,1,2,... in order, so the "oldest" slot is
// always the one 'nextVictim' points to. After each eviction we move on.
Result runFIFO(const vector<int>& refs, int numFrames) {
    Result r;
    r.name = "FIFO";
    vector<int> frames(numFrames, EMPTY);
    int nextVictim = 0;

    for (size_t i = 0; i < refs.size(); i++) {
        int page = refs[i];
        int evicted = EMPTY;
        bool hit = (findPage(frames, page) != -1);

        if (hit) {
            r.hits++;                        // FIFO ignores hits completely
        } else {
            r.faults++;
            int slot = findEmpty(frames);
            if (slot == -1) {                // memory full -> evict the oldest
                slot = nextVictim;
                evicted = frames[slot];
                nextVictim = (nextVictim + 1) % numFrames;
            }
            frames[slot] = page;
        }
        recordStep(r, page, hit, frames, evicted, "");
    }
    return r;
}

// ---- LRU: evict the page whose last use is the oldest ----------------------
// lastUsed[slot] stores the index of the most recent reference to that slot.
Result runLRU(const vector<int>& refs, int numFrames) {
    Result r;
    r.name = "LRU";
    vector<int> frames(numFrames, EMPTY);
    vector<int> lastUsed(numFrames, -1);

    for (size_t i = 0; i < refs.size(); i++) {
        int page = refs[i];
        int evicted = EMPTY;
        int slot = findPage(frames, page);
        bool hit = (slot != -1);

        if (hit) {
            r.hits++;
        } else {
            r.faults++;
            slot = findEmpty(frames);
            if (slot == -1) {                // full -> evict smallest lastUsed
                slot = 0;
                for (int s = 1; s < numFrames; s++)
                    if (lastUsed[s] < lastUsed[slot]) slot = s;
                evicted = frames[slot];
            }
            frames[slot] = page;
        }
        lastUsed[slot] = (int)i;             // hit OR load counts as a "use"
        recordStep(r, page, hit, frames, evicted, "");
    }
    return r;
}

// ---- Optimal: evict the page used farthest in the future -------------------
// THEORETICAL BENCHMARK ONLY: it looks at refs[i+1...], which a real OS can
// never know. It is here to show the best possible result.
Result runOptimal(const vector<int>& refs, int numFrames) {
    Result r;
    r.name = "Optimal";
    vector<int> frames(numFrames, EMPTY);
    int n = (int)refs.size();

    for (int i = 0; i < n; i++) {
        int page = refs[i];
        int evicted = EMPTY;
        bool hit = (findPage(frames, page) != -1);

        if (hit) {
            r.hits++;
        } else {
            r.faults++;
            int slot = findEmpty(frames);
            if (slot == -1) {
                // For each resident page find when it is next used.
                // A page never used again gets distance n (= "infinitely far").
                int farthest = -1;
                slot = 0;
                for (int s = 0; s < numFrames; s++) {
                    int nextUse = n;
                    for (int j = i + 1; j < n; j++) {
                        if (refs[j] == frames[s]) { nextUse = j; break; }
                    }
                    if (nextUse > farthest) { farthest = nextUse; slot = s; }
                }
                evicted = frames[slot];
            }
            frames[slot] = page;
        }
        recordStep(r, page, hit, frames, evicted, "");
    }
    return r;
}

// ---- Adaptive: choose FIFO or LRU using ONLY past/current references -------
// The rule (called only when a fault happens and memory is full):
//   Look at the last ADAPT_WINDOW references up to and including the current
//   one (refs[i-7 .. i]). Count how many of them repeat a page that already
//   appeared earlier inside the same window.
//       repeatRatio = repeats / window size
//   repeatRatio >= 0.30  -> program is re-using pages (locality)  -> use LRU
//   repeatRatio <  0.30  -> mostly new pages (scan-like)          -> use FIFO
// It never reads refs[i+1...], so it needs no knowledge of the future.
bool shouldUseLRU(const vector<int>& refs, int i) {
    int start = max(0, i - ADAPT_WINDOW + 1);
    int windowSize = i - start + 1;
    if (windowSize < 2) return false;        // not enough history -> FIFO

    int repeats = 0;
    for (int a = start; a <= i; a++) {
        for (int b = start; b < a; b++) {    // only looks at earlier refs
            if (refs[b] == refs[a]) { repeats++; break; }
        }
    }
    double repeatRatio = (double)repeats / windowSize;
    return repeatRatio >= ADAPT_THRESHOLD;
}

// Adaptive keeps BOTH pieces of bookkeeping (load time for FIFO, last use for
// LRU) at all times, so it can switch rule on any fault without losing state.
Result runAdaptive(const vector<int>& refs, int numFrames) {
    Result r;
    r.name = "Adaptive";
    vector<int> frames(numFrames, EMPTY);
    vector<int> loadTime(numFrames, -1);     // when the page was loaded
    vector<int> lastUsed(numFrames, -1);     // when the page was last used

    for (size_t i = 0; i < refs.size(); i++) {
        int page = refs[i];
        int evicted = EMPTY;
        string note = "";
        int slot = findPage(frames, page);
        bool hit = (slot != -1);

        if (hit) {
            r.hits++;
        } else {
            r.faults++;
            slot = findEmpty(frames);
            if (slot == -1) {
                bool useLRU = shouldUseLRU(refs, (int)i);
                slot = 0;
                for (int s = 1; s < numFrames; s++) {
                    if (useLRU ? (lastUsed[s] < lastUsed[slot])
                               : (loadTime[s] < loadTime[slot]))
                        slot = s;
                }
                evicted = frames[slot];
                note = useLRU ? "LRU" : "FIFO";
                if (useLRU) r.lruEvictions++; else r.fifoEvictions++;
            }
            frames[slot] = page;
            loadTime[slot] = (int)i;
        }
        lastUsed[slot] = (int)i;
        recordStep(r, page, hit, frames, evicted, note);
    }
    return r;
}

// ----------------------------------------------------------------------------
// 4. Workload generators (deterministic: same input -> same sequence)
// ----------------------------------------------------------------------------
uint32_t rngState = 12345;

int randBelow(int n) {                       // tiny linear-congruential RNG
    rngState = rngState * 1664525u + 1013904223u;
    return (int)((rngState >> 16) % (uint32_t)n);
}

// Scan: pages firstPage, firstPage+1, ... (never re-used)
void addScan(vector<int>& refs, int count, int& nextScanPage) {
    for (int k = 0; k < count; k++) refs.push_back(nextScanPage++);
}

// Repeated: a small "hot set" is referenced again and again (75% of the time);
// the other 25% are occasional cold pages.
void addRepeated(vector<int>& refs, int count, int numFrames, int hotBase) {
    int hotSize = max(1, numFrames - 1);
    for (int k = 0; k < count; k++) {
        if (randBelow(100) < 75) refs.push_back(hotBase + randBelow(hotSize));
        else                     refs.push_back(500 + randBelow(40));
    }
}

// type: 1 = sequential, 2 = repeated, 3 = mixed (scan, hot, scan, hot)
vector<int> generateWorkload(int type, int length, int numFrames) {
    rngState = 12345;
    vector<int> refs;
    int nextScan = 1;
    if (type == 1) {
        addScan(refs, length, nextScan);
    } else if (type == 2) {
        addRepeated(refs, length, numFrames, 1);
    } else {
        int q = length / 4;
        addScan(refs, q, nextScan);
        addRepeated(refs, q, numFrames, 300);
        addScan(refs, q, nextScan);
        addRepeated(refs, length - 3 * q, numFrames, 300);
    }
    return refs;
}

// ----------------------------------------------------------------------------
// 5. Printing
// ----------------------------------------------------------------------------
string framesToString(const vector<int>& frames) {
    ostringstream out;
    out << "[";
    for (size_t i = 0; i < frames.size(); i++) {
        if (i > 0) out << " |";
        if (frames[i] == EMPTY) out << setw(4) << "-";
        else                    out << setw(4) << frames[i];
    }
    out << " ]";
    return out.str();
}

void printSteps(const Result& r) {
    cout << "\n  Frame contents for " << r.name << ":\n";
    cout << "  Ref | Frames" << string(r.steps.empty() ? 0 : r.steps[0].frames.size() * 6 - 2, ' ')
         << " | Result\n";
    cout << "  ----+" << string(r.steps.empty() ? 8 : r.steps[0].frames.size() * 6 + 5, '-')
         << "+--------\n";
    for (size_t i = 0; i < r.steps.size(); i++) {
        const Step& s = r.steps[i];
        cout << "  " << setw(3) << s.page << " | " << framesToString(s.frames) << " | ";
        if (s.hit) {
            cout << "HIT";
        } else {
            cout << "FAULT";
            if (s.evicted != EMPTY) cout << " (evict " << s.evicted << ")";
            if (!s.note.empty())    cout << " [rule: " << s.note << "]";
        }
        cout << "\n";
    }
}

void printSummary(const vector<Result>& results, int refCount) {
    cout << "\n  Simulation Results  (" << refCount << " references)\n";
    cout << "  " << string(66, '=') << "\n";
    cout << "  " << left << setw(12) << "Algorithm" << right
         << setw(13) << "Page Faults" << setw(11) << "Page Hits"
         << setw(13) << "Hit Ratio" << setw(14) << "Fault Ratio" << "\n";
    cout << "  " << string(66, '-') << "\n";
    for (size_t i = 0; i < results.size(); i++) {
        const Result& r = results[i];
        int total = r.hits + r.faults;
        cout << "  " << left << setw(12) << r.name << right
             << setw(13) << r.faults << setw(11) << r.hits
             << setw(12) << fixed << setprecision(2) << percent(r.hits, total) << "%"
             << setw(13) << percent(r.faults, total) << "%" << "\n";
    }
    cout << "  " << string(66, '=') << "\n";
}

void printSequence(const vector<int>& refs) {
    size_t shown = min<size_t>(refs.size(), 30);
    for (size_t i = 0; i < shown; i++) cout << refs[i] << " ";
    if (refs.size() > shown) cout << "... (" << refs.size() << " total)";
}

// ----------------------------------------------------------------------------
// 6. Input validation
// ----------------------------------------------------------------------------

// Reads one line. Returns false if input has ended (Ctrl+D / Ctrl+Z / pipe end).
bool readLine(const string& prompt, string& line) {
    cout << prompt;
    if (!getline(cin, line)) return false;
    return true;
}

bool allDigits(const string& s) {
    if (s.empty()) return false;
    for (size_t i = 0; i < s.size(); i++)
        if (s[i] < '0' || s[i] > '9') return false;
    return true;
}

// Asks until the user enters an integer in [lo, hi]. false = input ended.
bool readInt(const string& prompt, int lo, int hi, int& value) {
    string line;
    while (true) {
        if (!readLine(prompt, line)) return false;
        stringstream ss(line);
        string tok, extra;
        ss >> tok;
        bool ok = allDigits(tok) && tok.size() <= 6 && !(ss >> extra);
        if (ok) {
            int v = stoi(tok);
            if (v >= lo && v <= hi) { value = v; return true; }
        }
        cout << "  Invalid input. Please enter a whole number from "
             << lo << " to " << hi << ".\n";
    }
}

// Turns "7 0 1, 2 0" into numbers. Returns an error message ("" if fine).
string parseReferences(string line, vector<int>& out) {
    for (size_t i = 0; i < line.size(); i++)
        if (line[i] == ',') line[i] = ' ';    // allow commas as separators

    stringstream ss(line);
    string tok;
    vector<int> refs;
    while (ss >> tok) {
        if (!allDigits(tok))
            return "'" + tok + "' is not a valid page number (use whole numbers >= 0).";
        if (tok.size() > 4 || stoi(tok) > MAX_PAGE)
            return "Page '" + tok + "' is too large (maximum page number is "
                   + to_string(MAX_PAGE) + ").";
        refs.push_back(stoi(tok));
        if ((int)refs.size() > MAX_REFS)
            return "Too many references (maximum is " + to_string(MAX_REFS) + ").";
    }
    if (refs.empty()) return "The reference string is empty.";
    out = refs;
    return "";
}

// ----------------------------------------------------------------------------
// 7. Self-tests  (run with:  ./pagesim --test)
// ----------------------------------------------------------------------------
int testsRun = 0, testsFailed = 0;

void check(const string& name, bool condition) {
    testsRun++;
    if (!condition) {
        testsFailed++;
        cout << "  [FAIL] " << name << "\n";
    } else {
        cout << "  [ OK ] " << name << "\n";
    }
}

int runSelfTests() {
    cout << "Running self-tests...\n";

    // Known textbook answers (Silberschatz) - 20 refs, 3 frames.
    int textbook[] = {7,0,1,2,0,3,0,4,2,3,0,3,2,1,2,0,1,7,0,1};
    vector<int> tb(textbook, textbook + 20);
    check("Textbook FIFO    = 15 faults", runFIFO(tb, 3).faults == 15);
    check("Textbook LRU     = 12 faults", runLRU(tb, 3).faults == 12);
    check("Textbook Optimal =  9 faults", runOptimal(tb, 3).faults == 9);

    // Belady's anomaly string.
    int belady[] = {1,2,3,4,1,2,5,1,2,3,4,5};
    vector<int> bl(belady, belady + 12);
    check("Belady FIFO 3 frames = 9 faults",  runFIFO(bl, 3).faults == 9);
    check("Belady FIFO 4 frames = 10 faults", runFIFO(bl, 4).faults == 10);
    check("Belady LRU  3 frames = 10 faults", runLRU(bl, 3).faults == 10);
    check("Belady LRU  4 frames = 8 faults",  runLRU(bl, 4).faults == 8);
    check("Belady OPT  3 frames = 7 faults",  runOptimal(bl, 3).faults == 7);
    check("Belady OPT  4 frames = 6 faults",  runOptimal(bl, 4).faults == 6);

    // Edge cases.
    vector<int> same(10, 5);
    check("Same page x10, 1 frame: 1 fault, 9 hits",
          runFIFO(same, 1).faults == 1 && runLRU(same, 1).hits == 9);
    vector<int> one(1, 4);
    check("Single reference = 1 fault", runAdaptive(one, 3).faults == 1);
    int alt[] = {1,2,1,2,1,2};
    vector<int> al(alt, alt + 6);
    check("1 frame, alternating pages: all faults",
          runFIFO(al, 1).faults == 6 && runLRU(al, 1).faults == 6 &&
          runOptimal(al, 1).faults == 6 && runAdaptive(al, 1).faults == 6);
    check("Frames >= distinct pages: faults = distinct (6 and 5)",
          runFIFO(tb, 8).faults == 6 && runAdaptive(bl, 5).faults == 5);

    // Invariants over all generated workloads and several frame counts.
    bool sumOk = true, optOk = true, floorOk = true;
    for (int type = 1; type <= 3; type++) {
        for (int frames = 1; frames <= 8; frames++) {
            vector<int> refs = generateWorkload(type, 40, frames);
            vector<int> sorted = refs;
            sort(sorted.begin(), sorted.end());
            int distinct = (int)(unique(sorted.begin(), sorted.end()) - sorted.begin());

            Result res[4] = { runFIFO(refs, frames), runLRU(refs, frames),
                              runOptimal(refs, frames), runAdaptive(refs, frames) };
            for (int k = 0; k < 4; k++) {
                if (res[k].hits + res[k].faults != (int)refs.size()) sumOk = false;
                if (res[k].faults < distinct) floorOk = false;
                if (res[2].faults > res[k].faults) optOk = false;
            }
        }
    }
    check("hits + faults == number of references (all algorithms)", sumOk);
    check("Optimal never has more faults than any other algorithm", optOk);
    check("Faults >= number of distinct pages (cold misses)", floorOk);

    // Adaptive must not use the future: running on a prefix of the sequence
    // must give exactly the same first k steps as running on the full one.
    bool prefixOk = true;
    vector<int> full = generateWorkload(3, 60, 3);
    Result fullRun = runAdaptive(full, 3);
    for (size_t k = 1; k <= full.size(); k++) {
        vector<int> prefix(full.begin(), full.begin() + k);
        Result p = runAdaptive(prefix, 3);
        for (size_t i = 0; i < k; i++)
            if (p.steps[i].frames != fullRun.steps[i].frames ||
                p.steps[i].hit != fullRun.steps[i].hit) prefixOk = false;
    }
    check("Adaptive uses no future info (prefix test)", prefixOk);

    // Adaptive rule sanity: pure scan -> FIFO, heavy repetition -> LRU.
    int scan[]  = {1,2,3,4,5,6,7,8};
    int hotArr[] = {1,2,1,2,1,2,1,2};
    vector<int> sc(scan, scan + 8), hot(hotArr, hotArr + 8);
    check("Rule: scan window chooses FIFO", !shouldUseLRU(sc, 7));
    check("Rule: repeating window chooses LRU", shouldUseLRU(hot, 7));

    cout << "\n" << (testsRun - testsFailed) << "/" << testsRun << " checks passed.\n";
    return testsFailed;
}

// ----------------------------------------------------------------------------
// 8. Menu / main
// ----------------------------------------------------------------------------
void showMenu(const vector<int>& refs, const string& workload, int frames, bool showFrames) {
    cout << "\n==============================================================\n";
    cout << "   Workload-Aware Adaptive Page Replacement Simulator\n";
    cout << "==============================================================\n";
    cout << "  Reference string : "; printSequence(refs); cout << "\n";
    cout << "  Workload type    : " << workload << "\n";
    cout << "  Frames           : " << frames << "\n";
    cout << "  Show frame steps : " << (showFrames ? "ON" : "OFF") << "\n";
    cout << "--------------------------------------------------------------\n";
    cout << "   1. Enter custom reference string\n";
    cout << "   2. Generate workload (sequential / repeated / mixed)\n";
    cout << "   3. Set number of frames\n";
    cout << "   4. Toggle frame display ON/OFF\n";
    cout << "   5. Run FIFO\n";
    cout << "   6. Run LRU\n";
    cout << "   7. Run Optimal (theoretical benchmark only)\n";
    cout << "   8. Run Adaptive (FIFO/LRU)\n";
    cout << "   9. Compare ALL algorithms\n";
    cout << "   0. Exit\n";
    cout << "--------------------------------------------------------------\n";
}

int main(int argc, char* argv[]) {
    if (argc > 1 && string(argv[1]) == "--test") return runSelfTests() == 0 ? 0 : 1;

    // Start with the textbook sequence so the program can be demonstrated at once.
    int sample[] = {7,0,1,2,0,3,0,4,2,3,0,3,2,1,2,0,1,7,0,1};
    vector<int> refs(sample, sample + 20);
    string workload = "Custom (textbook sample)";
    int numFrames = 3;
    bool showFrames = true;

    while (true) {
        showMenu(refs, workload, numFrames, showFrames);
        int choice;
        if (!readInt("  Choose an option (0-9): ", 0, 9, choice)) break;
        if (choice == 0) break;

        if (choice == 1) {
            string line;
            cout << "  Enter pages separated by spaces (e.g. 7 0 1 2 0 3 0 4)\n";
            if (!readLine("  > ", line)) break;
            vector<int> parsed;
            string err = parseReferences(line, parsed);
            if (!err.empty()) cout << "  Error: " << err << "\n";
            else { refs = parsed; workload = "Custom"; cout << "  Saved " << refs.size() << " references.\n"; }

        } else if (choice == 2) {
            int type, len;
            cout << "  1 = Sequential   2 = Repeated   3 = Mixed\n";
            if (!readInt("  Workload type (1-3): ", 1, 3, type)) break;
            if (!readInt("  Length (4-" + to_string(MAX_REFS) + "): ", 4, MAX_REFS, len)) break;
            refs = generateWorkload(type, len, numFrames);
            workload = type == 1 ? "Sequential" : type == 2 ? "Repeated" : "Mixed";
            cout << "  Generated: "; printSequence(refs); cout << "\n";

        } else if (choice == 3) {
            if (!readInt("  Number of frames (1-" + to_string(MAX_FRAMES) + "): ", 1, MAX_FRAMES, numFrames)) break;

        } else if (choice == 4) {
            showFrames = !showFrames;
            cout << "  Frame display is now " << (showFrames ? "ON" : "OFF") << ".\n";

        } else {   // 5..9 = run something
            if (refs.empty()) { cout << "  No reference string yet. Use option 1 or 2.\n"; continue; }

            vector<Result> results;
            if (choice == 5 || choice == 9) results.push_back(runFIFO(refs, numFrames));
            if (choice == 6 || choice == 9) results.push_back(runLRU(refs, numFrames));
            if (choice == 7 || choice == 9) results.push_back(runOptimal(refs, numFrames));
            if (choice == 8 || choice == 9) results.push_back(runAdaptive(refs, numFrames));

            if (showFrames)
                for (size_t i = 0; i < results.size(); i++) printSteps(results[i]);
            printSummary(results, (int)refs.size());

            for (size_t i = 0; i < results.size(); i++) {
                if (results[i].name == "Adaptive")
                    cout << "  Adaptive evictions: " << results[i].fifoEvictions
                         << " by FIFO rule, " << results[i].lruEvictions << " by LRU rule.\n";
                if (results[i].name == "Optimal")
                    cout << "  Note: Optimal uses future knowledge - benchmark only.\n";
            }
        }
    }
    cout << "\nGoodbye!\n";
    return 0;
}