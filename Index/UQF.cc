/*
LCR - Generate Unreachable Query Filter (UQF) Index
Author: Yuzheng Cai
2020-12-23
------------------------------
C++ 11 
*/ 

#ifndef  UQF_CC
#define  UQF_CC
#include "UQF.h"
using namespace std;


UQF::UQF(GenerateDAG* inputDAG, VertexID* inputVidVNreuse1, VertexID* inputVidVNreuse2, bool* inputBoolVNreuse) : 
    vidVNreuse1(inputVidVNreuse1), vidVNreuse2(inputVidVNreuse2), boolVNreuse(inputBoolVNreuse) {

    // obtain basic information from DAG
    DAG = inputDAG;
    DAGVN = DAG->DAGVM;
    DAGEN = DAG->DAGEN;
    DAGneighbors = DAG->DAGneighbors;
    UQForders = new UQFindexNode[DAGVN];

    // find roots in DAG
    findRoots();

    // compute X topological order and level
    topoXandLevel();

    // compute Y topological order and get FPC index
    topoYandFPC();

    // clear unuseful memory
    stack<VertexID> tmp;
    R.swap(tmp);

    // sort FPC according to level index
    sort(FPC.begin(), FPC.end());

    // get outcoming FPCs for finding affiliate nodes (ANs)
    getOutFPC();

    // get incoming FPCs for finding affiliate nodes (ANs)
    getInFPCdominate();

    // memory reuse
    processedFPC = boolDAGVNreuse2;     // whether a FPC has been processed or not
    hasH = boolDAGVNreuse3;             // whether a vertex has H(1) or H(2) 

    // generate H(1) index
    generateH1();

    // generate H(2) index
    generateH2(); 

    builtUQFindex = true;
}


// finding roots in DAG, i.e., find those don't have in-edges
void UQF::findRoots() {
    for (int i=DAGVN-1; i>=0; --i)
        if (DAGneighbors[i].in.size()==0)  
            roots.push_back(i);
}


// topological order (X) coordinate and level filter index
void UQF::topoXandLevel() {
    cout<<"- Generating X topological order ..."<<endl;
    
    VertexID order=1, tmplevel;
    Xorders = vidVNreuse1;                              // permulate vertices according to topo X
    VertexID* DAGinDegree = vidVNreuse2;                // record DAG vertices' in-degree in traversal
    memset(DAGinDegree, 0, sizeof(VertexID)*DAGVN);

    // find those don't have in-edges
    for (const VertexID& u : roots)
        R.push(u);

    // the stack isn't empty
    while (!R.empty()) 
    {   
        // pop current vertex u
        VertexID u = R.top();          
        R.pop();

        // X topological order of current vertex u
        UQForders[u].X = order;
        Xorders[order] = u;
        order++;

        // level fiter index
        if (DAGneighbors[u].in.size()) {
            tmplevel = 0;
            for (int j=0; j<DAGneighbors[u].in.size(); j++)
                tmplevel = max(UQForders[DAGneighbors[u].in[j]].level, tmplevel); 
            UQForders[u].level = tmplevel+1;
        } else
            UQForders[u].level = 0;   
        
        // delete edges connect current vertex v to other unvisited vertices
        for (int j=DAGneighbors[u].out.size()-1; j>=0; j--) {

            // an out-edge from u to v
            const VertexID& v = DAGneighbors[u].out[j];            

            // the vertex v hasn't been explored, i.e., its in-degree never been stored
            if (DAGinDegree[v] == 0)
                DAGinDegree[v] = DAGneighbors[v].in.size()-1;
            else
                DAGinDegree[v] -= 1;

            // if all of vertex v's in-edges have been deleted, push v into the stack
            if (DAGinDegree[v] == 0)
                R.push(v);  
        }
    }
}


// structure to choose vertex v with largest val
struct pqNode{
    VertexID id;
    int val;
    pqNode(VertexID a, int b): id(a), val(b) {}
    bool operator>(const pqNode& t) const {
        return val > t.val;
    }
    bool operator<(const pqNode& t) const {
        return val < t.val;
    }
};


// function for topo Y index and FPC set
void UQF::topoYandFPC() {
    cout<<"- Optimized FELINE Y topology order and FPC set ..."<<endl;
    
    // min heap for Y to select those with smaller X value
    priority_queue<pqNode, vector<pqNode>, greater<pqNode>> minHeap;

    // Candidate free nodes for generating FPC set
    set<pqNode> CFN;

    VertexID order=1;
    VertexID* DAGinDegree = vidVNreuse2;                    // record DAG vertices' in-degree in traversal
    // memset(DAGinDegree, 0, sizeof(VertexID)*DAGVN);      // already all zero
    queue<pqNode> toBeAdd;                                  // nodes to be added to CFN

    isFPC = boolVNreuse;                                    // whether a vertex is FPC 
    // memset(isProcessed, 0, sizeof(bool)*DAGVN);          // already all zero

    boolDAGVNreuse1 = new bool[DAGVN]();
    boolDAGVNreuse2 = new bool[DAGVN]();
    boolDAGVNreuse3 = new bool[DAGVN]();
    bool* isProcessed = boolDAGVNreuse1;                    // whether a vertex has been processed
    bool* isCFNorFPC = boolDAGVNreuse2;                     // whether a vertex is CR or FPC
    bool* hasBeenR = boolDAGVNreuse3;                       // whether a vertex has been in Roots set

    // find those don't have in-edges to initialize R
    for (const VertexID& u : roots)
        minHeap.emplace(u, UQForders[u].X);
    while (!minHeap.empty()) {
        R.push(minHeap.top().id);
        hasBeenR[minHeap.top().id] = true;
        minHeap.pop();
    }

    // the R isn't empty
    while (!R.empty()) 
    {   
        // pop current vertex u
        VertexID u = R.top();          
        R.pop();
        isProcessed[u] = true;

        // topology order of current vertex u
        UQForders[u].Y = order;
        order++;

        // find FPC
        auto iter = CFN.begin();
        while (iter!=CFN.end() && iter->val<UQForders[u].X) {
            const VertexID& v = iter->id;
            if (hasBeenR[v]==true)
                iter = CFN.erase(iter);
            else { 
                if (UQForders[v].X>UQForders[u].X) {
                    FPC.emplace_back(v, UQForders[v].level);        // push into FPC
                    isFPC[v] = true;
                    iter = CFN.erase(iter);                         // delete from CR
                } else
                    ++iter;   
            }      
        }

        // since there may be some X_w=X_u, we need to update CFN
        while (iter!=CFN.end() && iter->val==UQForders[u].X) {
            const VertexID& v = iter->id;
            if (hasBeenR[v]==true)
                iter = CFN.erase(iter);
            else {            
                VertexID maxX = 0;
                for (const VertexID& w : DAGneighbors[v].in)
                    if (isProcessed[w]==false && UQForders[w].X>maxX)
                        maxX = UQForders[w].X;
                if (maxX>0) {
                    iter = CFN.erase(iter);
                    toBeAdd.emplace(v, maxX);                       // to avoid changing iterator
                } else 
                    ++iter;
            }
        }
        while (!toBeAdd.empty()) {
            CFN.insert(toBeAdd.front());
            toBeAdd.pop();
        }
        
        // delete edges connect current vertex v to other unvisited vertices
        for (const VertexID& v : DAGneighbors[u].out) {

            // the vertex v hasn't been explored, i.e., its in-degree never been stored
            if (DAGinDegree[v] == 0)
                DAGinDegree[v] = DAGneighbors[v].in.size()-1;
            else
                DAGinDegree[v] -= 1;

            // if all of vertex v's in-edges have been removed, push v into the min-heap
            if (DAGinDegree[v] == 0) {
                hasBeenR[v] = true;
                minHeap.emplace(v, UQForders[v].X);
            } else if (isCFNorFPC[v]==false) {                       // v should be pushed into CFN
                VertexID maxX = 0;
                for (VertexID& w : DAGneighbors[v].in) 
                    if (isProcessed[w]==false && UQForders[w].X>maxX)
                        maxX = UQForders[w].X;
                CFN.emplace(v, maxX);
                isCFNorFPC[v] = true;
            }            
        }

        while (!minHeap.empty()) {
            R.push(minHeap.top().id);
            minHeap.pop();
        }     
    }

    vector<VertexID> tmp;
    roots.swap(tmp);
}


// outcoming FPCs for finding affiliate nodes (ANs)
void UQF::getOutFPC() {
    cout<<"- Computing outcoming FPC for finding affiliate nodes (ANs) ..."<<endl;

    // allocate space
    hasOutFPC = boolDAGVNreuse1;
    memset(hasOutFPC, 0, sizeof(bool)*DAGVN);

    // assign outFPC in descending X topo order
    for (int i=DAGVN; i>0; i--) {
        VertexID& u = Xorders[i];
        if (hasOutFPC[u]==true || isFPC[u]==true) {
            hasOutFPC[u] = true;
            for (const VertexID& v : DAGneighbors[u].in)
                hasOutFPC[v] = true;
        }
    }
}


// inFPC dominate for finding affiliate nodes (ANs)
void UQF::getInFPCdominate() {
    cout<<"- Computing incoming FPC for finding affiliate nodes (ANs) ..."<<endl;

    // allocate space
    inFPCdominate = new set<inFPCinfo>[DAGVN];

    // assign inFPC dominate in acscending X topo order
    for (int i=1; i<=DAGVN; ++i) {
        VertexID& u = Xorders[i];
        if (isFPC[u]==true)
            inFPCdominate[u].emplace(u, UQForders[u].level);
        else 
            for (const VertexID& v : DAGneighbors[u].in)
                set_difference( inFPCdominate[v].begin(),  inFPCdominate[v].end(), \
                                inFPCdominate[u].begin(),  inFPCdominate[u].end(),     \
                                inserter(inFPCdominate[u], inFPCdominate[u].begin()));        
        }  
}


// generate H(1) index in FP reduction
void UQF::generateH1() {
    cout<<"- Generating H(1) index ..."<<endl;

    // initialization
    maxH = 0;                                           // currently max H(1)
    memset(processedFPC, 0, sizeof(bool)*DAGVN);        // whether a FPC has been processed or not
    memset(hasH, 0, sizeof(bool)*DAGVN);                // whether a vertex has H(1)
    
    // iterate each FPC in ascending level order
    for (const auto& each : FPC) {
        const VertexID& f =each.id;
        currentFPClevel = UQForders[f].level;
        tmpMaxH = UQForders[f].level+maxH;

        // assign H(1) to f's predcessors
        processedFPC[f] = true;
        traceBackForH1(f);

        // explore and assign H(1) to affiliate nodes
        while (!explored.empty()) 
            explored.pop();
        for (const VertexID& v : DAGneighbors[f].out)
            if (hasH[v]==false && hasOutFPC[v]==false && canBeReachedForH1(v)==false) {
                UQForders[v].H1 = maxH + UQForders[v].level;
                hasH[v] = true;
                explored.push(v);
                exploreForwardForH1(v);
            }

        // update maxH
        maxH = tmpMaxH+1;
    }

    // process those have nothing to do with FPC
    for (VertexID v=0; v<DAGVN; v++)
        if (hasH[v]==false)
            UQForders[v].H1 = UQForders[v].level+maxH;
}


// trace back from f to assign f's predcessors
void UQF::traceBackForH1(VertexID u)
{
    if (hasH[u]==false) {

        // assign H(1)
        UQForders[u].H1 = maxH + UQForders[u].level;
        hasH[u] = true;

        // trace back more
        for (const VertexID& v : DAGneighbors[u].in)
            traceBackForH1(v);
    }    
}


// judge whether a vertex can be reached by unprocessed f in H(1) index generation
bool UQF::canBeReachedForH1(VertexID v) {
    if (!inFPCdominate[v].empty()) 
        for (auto iter=inFPCdominate[v].rbegin(); iter!=inFPCdominate[v].rend() && (iter->level)>=currentFPClevel; ++iter) 
            if (processedFPC[iter->id]==false)
                return true;
    return false;
}


// explore affiliate nodes (ANs) to asssign assign H(1)
bool UQF::exploreForwardForH1(VertexID u) {
    VertexID w;
    tmpMaxH = max(UQForders[u].H1, tmpMaxH);

    // explore backward
    for (const VertexID& v : DAGneighbors[u].in) 
        if (hasH[v]==false) {
            if (hasOutFPC[v]==true) {
                do {
                    w = explored.top();
                    explored.pop();
                    hasH[w] = false;
                } while (!explored.empty() && w!=u);
                return false;
            } else {
                UQForders[v].H1 = maxH + UQForders[v].level;
                hasH[v] = true;
                explored.push(v);
                if (exploreBackwardForH1(v)==false) {
                    do {
                        w = explored.top();
                        explored.pop();
                        hasH[w] = false;
                    } while (!explored.empty() && w!=u);
                    return false;
                }
            }
        }

    // explore forward
    for (const VertexID& v : DAGneighbors[u].out) 
        if (hasH[v]==false && hasOutFPC[v]==false && canBeReachedForH1(v)==false) {
            UQForders[v].H1 = maxH + UQForders[v].level;
            hasH[v] = true;
            explored.push(v);
            exploreForwardForH1(v);
        }

    return true;
}


// explore affiliate nodes (ANs) to asssign assign H(1)
bool UQF::exploreBackwardForH1(VertexID u) {
    VertexID w;
    for (const VertexID& v : DAGneighbors[u].in) 
        if (hasH[v]==false) {
            if (hasOutFPC[v]==true) {
                do {
                    w = explored.top();
                    explored.pop();
                    hasH[w] = false;
                } while (!explored.empty() && w!=u);
                return false;
            } else {
                UQForders[v].H1 = maxH + UQForders[v].level;
                hasH[v] = true;
                explored.push(v);
                if (exploreBackwardForH1(v)==false) {
                    do {
                        w = explored.top();
                        explored.pop();
                        hasH[w] = false;
                    } while (!explored.empty() && w!=u);
                    return false;
                }
            }
        }
    return true;
}


// generate H(2) index in FP reduction
void UQF::generateH2() {  
    cout<<"- Generating H(2) index ..."<<endl;

    // initialization
    maxH = 0;                                           // currently max H(2)
    memset(processedFPC, 0, sizeof(bool)*DAGVN);        // whether a FPC has been processed or not
    memset(hasH, 0, sizeof(bool)*DAGVN);                // whether a vertex has H(2)
    
    // iterate each FPC in descending level order
    for (int i=FPC.size()-1; i>=0; i--) {
        const VertexID& f = FPC[i].id;
        currentFPClevel = UQForders[f].level;
        tmpMaxH = UQForders[f].level + maxH;

        // assign H(2) to f's predcessors
        processedFPC[f] = true;
        traceBackForH2(f);

        // explore and assign H(2) to affiliate nodes
        while (!explored.empty()) 
            explored.pop();

        for (const VertexID& v : DAGneighbors[f].out) 
            if (hasH[v]==false && hasOutFPC[v]==false && canBeReachedForH2(v)==false) {
                UQForders[v].H2 = maxH + UQForders[v].level;
                hasH[v] = true;
                explored.push(v);
                exploreForwardForH2(v);
            }

        // update maxH
        maxH = tmpMaxH+1;
    }

    // process those have nothing to do with FPC
    for (VertexID v=0; v<DAGVN; v++)
        if (hasH[v]==false)
            UQForders[v].H2 = UQForders[v].level+maxH;
}


// trace back from f to assign f's predcessors
void UQF::traceBackForH2(VertexID u) {
    if (hasH[u]==false) {

        // assign H(2)
        UQForders[u].H2 = maxH + UQForders[u].level;
        hasH[u] = true;

        // trace back more
        for (const VertexID& v : DAGneighbors[u].in)
            traceBackForH2(v);
    }    
}


// judge whether a vertex can be reached by unprocessed f in H(2) index generation
bool UQF::canBeReachedForH2(VertexID v) {
    if (!inFPCdominate[v].empty()) 
        for (auto iter=inFPCdominate[v].begin(); iter!=inFPCdominate[v].end() && (iter->level)<=currentFPClevel; ++iter) 
            if (processedFPC[iter->id]==false)
                return true;
    return false;
}


// explore affiliate nodes (ANs) to asssign assign H(2)
bool UQF::exploreBackwardForH2(VertexID u) {
    VertexID w;
    tmpMaxH = max(UQForders[u].H2, tmpMaxH);

    // explore backward
    for (const VertexID& v : DAGneighbors[u].in)
        if (hasH[v]==false) {
            if (hasOutFPC[v]==true) {
                do {
                    w = explored.top();
                    explored.pop();
                    hasH[w] = false;
                } while (!explored.empty() && w!=u);
                return false;
            } else {
                UQForders[v].H2 = maxH + UQForders[v].level;
                hasH[v] = true;
                explored.push(v);
                if (exploreBackwardForH2(v)==false) {
                    do {
                        w = explored.top();
                        explored.pop();
                        hasH[w] = false;
                    } while (!explored.empty() && w!=u);
                    return false;
                }
            }
        }
    return true;
}


// explore affiliate nodes (ANs) to asssign assign H(2)
bool UQF::exploreForwardForH2(VertexID u) {
    VertexID w;
    tmpMaxH = max(UQForders[u].H2, tmpMaxH);

    // explore backward
    for (const VertexID& v : DAGneighbors[u].in)
        if (hasH[v]==false) {
            if (hasOutFPC[v]==true) {
                do {
                    w = explored.top();
                    explored.pop();
                    hasH[w] = false;
                } while (!explored.empty() && w!=u);
                return false;
            } else {
                UQForders[v].H2 = maxH + UQForders[v].level;
                hasH[v] = true;
                explored.push(v);
                if (exploreBackwardForH2(v)==false) {
                    do {
                        w = explored.top();
                        explored.pop();
                        hasH[w] = false;
                    } while (!explored.empty() && w!=u);
                    return false;
                }
            }
        }

    // explore forward
    for (const VertexID& v : DAGneighbors[u].out)
        if (hasH[v]==false && hasOutFPC[v]==false && canBeReachedForH2(v)==false) {
            UQForders[v].H2 = maxH + UQForders[v].level;
            hasH[v] = true;
            explored.push(v);
            exploreForwardForH2(v);
        }

    return true;
}



void UQF::freeMemory() {
    if (builtUQFindex) {
        builtUQFindex = false;

        vector<FPCinfo> tmp1;
        FPC.swap(tmp1);

        delete[] boolDAGVNreuse1;
        delete[] boolDAGVNreuse2;
        delete[] boolDAGVNreuse3;
        delete[] inFPCdominate;

        stack<VertexID> tmp2;
        explored.swap(tmp2);
    }
}


#endif