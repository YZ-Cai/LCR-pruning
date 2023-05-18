/*
LCR - Generate Unreachable Query Filter (UQF) Index
Author: Yuzheng Cai
2020-12-23
------------------------------
C++ 11 
*/ 

#ifndef  UQF_H
#define  UQF_H
#include "GenerateDAG.h"
using namespace std;


struct UQFindexNode {
    VertexID X, Y, level;
    unsigned long long H1, H2;
};


// for sorting false-positive contributors (FPCs)
struct FPCinfo{
    VertexID id, level;
    bool operator < (const FPCinfo &t) const { return this->level < t.level; }
	bool operator > (const FPCinfo &t) const { return this->level > t.level; }
    FPCinfo(VertexID a, VertexID b): id(a), level(b) {};
};


// incoming FPC infos for finding affiliate nodes (ANs)
struct inFPCinfo{
    VertexID id;
    VertexID level;
    inFPCinfo(VertexID a, VertexID b): id(a), level(b) {}
    bool operator <(const inFPCinfo & a) const {   
        if (level==a.level)
            return id<a.id;
        else
		    return level<a.level;
	}
};


class UQF {
    public: 

        UQF(GenerateDAG* inputDAG, VertexID* vidVNreuse1, VertexID* vidVNreuse2, bool* boolVNreuse);
        UQFindexNode* UQForders;                // UQF index orders
        void freeMemory();                      // free memory

    private:

        // memory reused by several subtasks
        VertexID *vidVNreuse1, *vidVNreuse2;
        bool *boolVNreuse, *boolDAGVNreuse1, *boolDAGVNreuse2, *boolDAGVNreuse3;

        // vertex / edge infos of DAG after condensing
        GenerateDAG* DAG;
        VertexID DAGVN;
        EdgeID DAGEN;
        PerDAGneighbor* DAGneighbors;
        bool builtUQFindex = false;

        vector<VertexID> roots;                         // roots in DAG
        VertexID* Xorders;                              // permulate vertices according to topo X
        vector<FPCinfo> FPC;                            // False Positive Contributers
        bool* isFPC;                                    // whether a vertex is FPC
        stack<VertexID> R;                              // stack containing current roots

        bool* hasOutFPC;                                // whether a vertex can reach any FPC for finding affiliate nodes (ANs)
        set<inFPCinfo>* inFPCdominate;                  // whether a vertex can be reached by any FPC for finding affiliate nodes (ANs)

        unsigned long long maxH, tmpMaxH;
        bool* hasH;                                     // whether a vertex has been assigned with H(1) or H(2)
        bool* processedFPC;                             // whether a FPC has been processed or not
        unsigned long long currentFPClevel;             // currently processing FPC's level
        stack<VertexID> explored;                       // storing explored vertices
        
        void findRoots();                               // find roots in DAG
        void topoXandLevel();                           // compute X topological order and level
        void topoYandFPC();                             // compute Y topological order and get FPC index
        
        void getOutFPC();                               // get outcoming FPCs for finding affiliate nodes (ANs)
        void getInFPCdominate();                        // get incoming FPCs for finding affiliate nodes (ANs)
        
        void traceBackForH1(VertexID v);                // trace back from an FPC to assign its predcessors in H(1) index generation
        bool canBeReachedForH1(VertexID v);             // judge whether a vertex can be reached by unprocessed FPC in H(1) index generation
        bool exploreBackwardForH1(VertexID v);          // explore ANs' predcessors to asssign assign H(1)
        bool exploreForwardForH1(VertexID v);           // explore ANs to asssign assign H(1)

        void traceBackForH2(VertexID v);                // trace back from an FPC to assign FPC's predcessors in H(2) index generation
        bool canBeReachedForH2(VertexID v);             // judge whether a vertex can be reached by unprocessed FPC in H(2) index generation
        bool exploreBackwardForH2(VertexID v);          // explore ANs' predcessors to asssign assign H(2)
        bool exploreForwardForH2(VertexID v);            // explore ANs to asssign assign H(2)

        // generate H(1) and H(2) orders
        void generateH1();
        void generateH2();
};

#endif
