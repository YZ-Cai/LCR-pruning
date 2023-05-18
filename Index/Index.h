/*
LCR - Answering with Pruning Strategies
Author: Yuzheng Cai
2022-10-27
------------------------------
C++ 11
Implementations for graphs with small number of labels
*/ 


#ifndef INDEX_H
#define INDEX_H
#include "../GraphUtils/Graph.cc"
#include "GenerateDAG.cc"
#include "UQF.cc"


class Index {
    public:
        Index(Graph* graph);

        // build and free index
        double buildIndex();
        void freeIndex();

        // answering queries
        bool query(const VertexID& s, const VertexID& t, const LabelSet& ls);
        double runAllQueries(const vector<PerQuery>& queries);
        
        // stats
        double getIndexSizeInBytes();
        double getIndexEntryCnt();

    private:

        // basic graph information
        Graph* graph;
        VertexID VN, DAGVN;
        EdgeID EN;
        LabelID labelNum;
        PerNeighbor* neighbors;
        
        // build 2-hop index with degree-one reduction (DOR)
        bool builtIndex = false;
        vector<pair<VertexID, LabelSet>> frontier, nxtFrontier;
        bool* isProcessed;
        int *visited, offset=1;
        void exploreForwardWithCurLabels(const VertexID& hopId, const VertexID& order);
        void exploreForwardPlusOneLabel(const VertexID& hopId, const VertexID& order);
        void exploreBackwardWithCurLabels(const VertexID& hopId, const VertexID& order);
        void exploreBackwardPlusOneLabel(const VertexID& hopId, const VertexID& order);
        inline bool queryForIndexForward(const VertexID& order, const VertexID& hopId, const VertexID& v, const LabelSet& ls);
        inline bool queryForIndexBackward(const VertexID& order, const VertexID& v, const VertexID& hopId, const LabelSet& ls);
        void build2hop();

        // for online query
        IndexNode* index;
        UQFindexNode* UQForders;
        bool query2hop(const VertexID& s, const VertexID& t, const LabelSet& ls);
};


#endif