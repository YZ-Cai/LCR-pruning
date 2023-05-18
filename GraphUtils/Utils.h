/*
LCR - Utils
Author: Yuzheng Cai
2021-5-18
------------------------------
C++ 11 
*/ 

// avoid redefinition error
#ifndef  UTILS_H
#define  UTILS_H
#include "../Config.h"

#include <cstdlib>
#include <iostream>
#include <sstream>
#include <fstream>

#include <cstring>
#include <string>
#include <stack>
#include <queue>
#include <vector>
#include <set>
#include <unordered_map>
#include <unordered_set>

#include <algorithm>
#include <sys/time.h>
#include <random>

using namespace std;
string graphFilename;

#if (defined _WIN32) || (defined _WIN64)
    #define lrand48() ((rand()<<16) | rand())
    #define srand48(n) srand(n)
#else
    #define INT_MAX 2147483648
#endif

#define min(a, b) ((a)<(b)?(a):(b))
#define max(a, b) ((a)>(b)?(a):(b))

bool cmpByFirstElement(pair<VertexID, LabelSet> a, pair<VertexID, LabelSet> b) {
    return a.first<b.first;
}



/*
 * check whether x is not a subset of y
 */
#define isNotSubset(x, y) (((x)^(y))&(x))
#define isSubset(x, y) (!(((x)^(y))&(x)))



/*
 * for storing index
 */
struct IndexNode {
    vector<pair<VertexID, LabelSet>> inHops, outHops;
    VertexID raw2DAG;
};



/* 
 * for recording time
 */
double startClock;
inline void startRecordTime() {
    startClock = clock();
}
inline double getElapsedTimeInMs() {
    return 1000.0 * (clock()-startClock) / CLOCKS_PER_SEC;
}



/*
 * Returns the number of labels in label set, 
 * e.g. if ls = {a, b} then this method returns 2
 */
inline int getNumberOfLabelsInLabelSet(const LabelSet& ls) {
    int size = 0;
    for(int i = 0; i < 8*sizeof(LabelSet); i++) 
        if ((ls>>i)&1)
            size++;
    return size;
};



/* 
 * set j-th position of ls as bit(0/1)
 */
inline void setLabelInLabelSet(LabelSet& ls, int j, int bit) {
    if (bit) 
        ls |= (1<<j);
    else
        ls &= ~(1<<j);
}



/*
 * prints a label set as a string
 */
// for graphs with small number of labels, e.g. L={0,2,3,5} is <0,2,3,5>
inline string labelSetToString(const LabelSet& ls) {
    string res = "<";
    for(int i=0; i<8*sizeof(LabelSet); i++)
        if ((ls>>i)&1)
            res += to_string(i)+", ";
    return res+">";
}

// for graphs with large number of labels, e.g. L={0,2,3,5} is <0,2,3,5>
inline string labelSetToString(const vector<LabelID>& lls) {
    string res = "<";
    for (const LabelID& i : lls)
        res += to_string(i)+", ";
    return res+">";
}

// for graphs with small number of labels, e.g. L={0,2,3,5} is <a,c,d,f>
inline string labelSetToCharacters(const LabelSet& ls) {
    string res = "";
    for(int i=0; i<8*sizeof(LabelSet); i++)
        if ((ls>>i)&1)
            res += 'a'+i;
    return res;
}

// for graphs with small number of labels, e.g. L={0,2,3,5} is <a,c,d,f>
inline string labelSetToCharacters(const vector<LabelID>& lls) {
    string res = "";
    for (const LabelID& i : lls)
        res += 'a'+i;
    return res;
}



/*
 * load query file for answering
 */
struct PerQuery {
    VertexID s, t;
    LabelSet ls;                // for graphs with small number of labels
    vector<LabelID> lls;        // for graphs with large number of labels
    bool ans;
    PerQuery(VertexID a, VertexID b, LabelSet c, bool d): s(a), t(b), ls(c), ans(d) {}
    PerQuery(VertexID a, VertexID b, vector<LabelID> c, bool d): s(a), t(b), lls(c), ans(d) {}
};

vector<PerQuery> loadQueryFile(const string& queryFileName, bool largeLabelSet) {
    vector<PerQuery> queries;
    string line = "";
    ifstream queryFile(queryFileName);
    
    printf("Loading query file: %s ...\n", queryFileName.c_str());
    startRecordTime();
    while ( getline (queryFile, line) ) {
        string from, label, to, answer;
        VertexID s, t;

        istringstream iss(line);
        iss >> answer >> from >> to;
        istringstream (from) >> s;
        istringstream (to) >> t;

        bool ans = true;
        if (answer=="0")
            ans = false;

        if (largeLabelSet) {
            vector<LabelID> lls;
            LabelID l;
            while (iss >> label) {
                istringstream (label) >> l;
                lls.emplace_back(l);
            }
            queries.emplace_back(s, t, lls, ans);
        } else {
            LabelSet ls;
            iss >> label;
            istringstream (label) >> ls;
            queries.emplace_back(s, t, ls, ans);
        }
    }
    queryFile.close();

    printf("- Finished, %d queries loaded. Time cost: %.2fms\n", queries.size(), getElapsedTimeInMs());
    return queries;
}



/*
 * get current time string for writing logs, e.g. "2022-08-30 15:00"
 */
string getCurrentTimeForLogs( ) {
    time_t now = time(0);
    tm *ltm = localtime(&now);
    return  to_string(1900 + ltm->tm_year) + "-" + to_string(1 + ltm->tm_mon) + "-" + to_string(ltm->tm_mday) + " " +
            to_string(ltm->tm_hour) + ":" + to_string(ltm->tm_min) + ":" + to_string(ltm->tm_sec);
}



#endif