/*
LCR - Condense SCCs in raw graph to obtain DAG
Author: Yuzheng Cai
2020-12-13
------------------------------
C++ 11 
1. A graph with cycles is turned into a DAG by condensing Strongly Connected Components (SCC);
2. The algorithm used here is Tarjan Algorithm;
*/ 


#ifndef  GENERATEDAG_CC
#define  GENERATEDAG_CC
#include "GenerateDAG.h"
using namespace std;


GenerateDAG::GenerateDAG(Graph* inputRawGraph, int* intVNreuse, VertexID* vidVNreuse1, VertexID* vidVNreuse2, bool* boolVNreuse) {       

    // get raw graph infos
    rawGraph = inputRawGraph;
    rawVN = rawGraph->VN;
    rawEN = rawGraph->EN;
    rawNeighbors = rawGraph->neighbors;

    DAGvisited = intVNreuse;                    // remember which vertex has been visited
    instack = boolVNreuse;                      // remember which vertex is in stack
    DFN = vidVNreuse1;                          // vectors used in Tarjan Algorithm
    LOW = vidVNreuse2;                          // vectors used in Tarjan Algorithm            
    raw2DAG = new VertexID[rawVN];              // matches from old id to new id in DAG
    
    // run Tarjan to find SCCs
    order = newId = 0;
    for (int i=0; i<rawVN; ++i)
        if (DAGvisited[i]==0) 
            tarjan(i);
    DAGVM = newId;

    // to condense the edge connection
    DAGneighbors = new PerDAGneighbor[DAGVM];           // storing vertex id and edges in DAG
    VertexID *outEdges = DFN;                           // reuse memory from DFN
    VertexID *isInOutEdges = LOW;                       // reuse memory from DFN
    memset(isInOutEdges, 0, sizeof(VertexID)*rawVN);
    VertexID outEdgesEnd, offset=0;                     // set for removing redundant in-/out-edges of vertex 
    DAGEN = 0;

    // for each new id
    for (int i=0; i<DAGVM; ++i) {
        ++offset;
        outEdgesEnd = 0;

        // for each old id which is contained in new id i
        for (const VertexID& u : DAG2raw[i]) {

            // add all out-edges of u into i's out-edges
            for (auto iter=rawNeighbors[u].out.begin(); iter!=rawNeighbors[u].out.end(); ++iter)
                for (const VertexID& v : iter->second)
                    if (raw2DAG[v]!=i)                      // if the edge doesn't point to i itself
                        if (isInOutEdges[raw2DAG[v]] < offset) {
                            isInOutEdges[raw2DAG[v]] = offset;
                            outEdges[outEdgesEnd++] = raw2DAG[v];
                        }
        }
        
        // add to total edge number of DAG
        DAGEN += outEdgesEnd;          

        // dump into edges of new vertex i
        DAGneighbors[i].out.resize(outEdgesEnd);
        for (int j=0; j<outEdgesEnd; ++j) {
            DAGneighbors[i].out[j] = outEdges[j];
            DAGneighbors[outEdges[j]].in.emplace_back(i);
        }
    }
}


// free memory
void GenerateDAG::freeMemory() {
    if (generatedDAG) {
        delete[] raw2DAG;
        vector<vector<VertexID>> tmp;
        DAG2raw.swap(tmp);
        stack<VertexID> stktmp;
        stk.swap(stktmp);
        generatedDAG = false;
    }
}


// tarjan algorithm to find SCCs 
void GenerateDAG::tarjan(VertexID u) {
    DFN[u]=LOW[u]=order;
    ++order;
    DAGvisited[u] = 1;
    stk.push(u);
    instack[u] = true;

    for (auto iter=rawNeighbors[u].out.begin(); iter!=rawNeighbors[u].out.end(); ++iter)
        for (const VertexID& v : iter->second) {   
            if (DAGvisited[v]==0) {                      // haven't been visited
                tarjan(v);
                LOW[u] = LOW[u]<LOW[v]?LOW[u]:LOW[v];   // LOW should be the smallest among its childs
            } 
            else if (instack[v]) {                       // find a cycle         
                LOW[u] = LOW[u]<DFN[v]?LOW[u]:DFN[v];   // update LOW
            }
        }
    
    VertexID id;
    if (DFN[u]==LOW[u]) {                                // find a new SCC 
        vector<VertexID> SCCids;
        do {
            id = stk.top();                             // old id belong to new id
            SCCids.emplace_back(id);                    // add old id to current SCC
            instack[id] = false;
            stk.pop();
            raw2DAG[id] = newId;                        // from old id to new id
        } while (id!=u);
        DAG2raw.emplace_back(SCCids);                   // this new id contains old ids in current SCC
        newId++;                                        // next new id
    }
}


#endif