/*
LCR - Condense SCCs in raw graph to obtain DAG
Author: Yuzheng Cai
2020-12-13
------------------------------
C++ 11 
1. A graph with cycles is turned into a DAG by condensing Strongly Connected Components (SCC);
2. The algorithm used here is Tarjan Algorithm;
*/ 


#ifndef  GENERATEDAG_H
#define  GENERATEDAG_H
#include "../GraphUtils/Graph.cc"
using namespace std;

class GenerateDAG {
    public:
        GenerateDAG(Graph* inputRawGraph, int* intVNreuse, VertexID* vidVNreuse1, VertexID* vidVNreuse2, bool* boolVNreuse);
        
        VertexID DAGVM, DAGEN;                  // vertex / edge number of DAG after condensing SCCs
        PerDAGneighbor* DAGneighbors;           // connection inside DAG
        VertexID* raw2DAG;                      // match raw vertex id to new vertex id 
        void freeMemory();                      // free memory

    private:

        // vertex / edge infos of input raw graph
        Graph* rawGraph;
        VertexID rawVN;
        EdgeID rawEN;   
        PerNeighbor* rawNeighbors;
        bool generatedDAG = false;

        VertexID order;                         // traversal order of DFS
        VertexID newId;                         // new vertex id after condensing SCC
        int* DAGvisited;                        // remember which vertex has been visited
        VertexID *DFN, *LOW;                    // vectors used in Tarjan Algorithm
        bool* instack;                          // remember which vertex is in stack
        stack<VertexID> stk;                    // stack for DFS
        vector<vector<VertexID>> DAG2raw;       // match new vertex id to raw vertex id
        void tarjan(VertexID u);                // tarjan algorithm
};

#endif