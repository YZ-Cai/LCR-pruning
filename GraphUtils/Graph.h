/*
LCR - Graph Storage
Author: Yuzheng Cai
2021-5-18
------------------------------
C++ 11 
*/ 

// avoid redefinition error
#ifndef GRAPH_H
#define GRAPH_H
#include "Utils.h"

// storage structure for raw graph data 
struct PerNeighbor{
    unordered_map<LabelID, vector<VertexID>> in;    // vector of in-neighbors of vertex v
    unordered_map<LabelID, vector<VertexID>> out;   // vector of out-neighbors of vertex v
    VertexID inDegree=0, outDegree=0;
};

// storage structure for DAG 
struct PerDAGneighbor{
    vector<VertexID> in;                            // vector of in-neighbors of vertex v
    vector<VertexID> out;                           // vector of out-neighbors of vertex v
};


class Graph{

    public:
        // graph infos
        VertexID VN;
        EdgeID EN;
        PerNeighbor* neighbors;
        LabelID labelNum;

        // read in graph
        Graph(const string& filename);
        ~Graph();

        // online label constrained BFS
        void initializeLCRsearch();
        bool LCRsearch(const VertexID& s, const VertexID& t, const LabelSet& labelSet);
        bool LCRsearch(const VertexID& s, const VertexID& t, const vector<LabelID>& lls);

    private:
        unordered_set<LabelID> labels;

        // for online label constrained BFS
        VertexID offset=1;
        VertexID queueBegin=0, queueEnd=0;
        VertexID *visited, *Q; 
        bool initialized = false;
};

#endif