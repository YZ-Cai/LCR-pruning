/*
LCR - Graph Storage Implementations
Author: Yuzheng Cai
2020-12-12
------------------------------
C++ 11 
*/ 


#ifndef  GRAPH_CC
#define  GRAPH_CC
#include "Graph.h"


// function for reading the input txt file
Graph::Graph(const string& filename) {

    // for recording time cost
    startRecordTime();
    cout<<"Start reading in "<<filename<<" ..."<<endl;

    VertexID i, fromId, toId;
	LabelID label;

    // read in the vertex number and edge number of the input graph
    ifstream inputfile(filename);
    inputfile>>VN>>EN>>labelNum;

    // allocate space and read in input graph
    neighbors = new PerNeighbor[VN]();

    // read in each edge
    for (i=0; i<EN; ++i) { 
        inputfile>>fromId>>toId>>label;
        if (fromId!=toId) {        // do not consider edges that link to itself
            neighbors[fromId].out[label].emplace_back(toId);
            neighbors[fromId].outDegree++;
            neighbors[toId].in[label].emplace_back(fromId);
            neighbors[toId].inDegree++;
        }
        labels.insert(label);
    }

    // check the labels
    labelNum = labels.size();
    for (const LabelID& label : labels)
        if (label<0 || label>=labelNum) {
            cerr<<"! Error! The graph has "<<labelNum<<" labels, and label ID "<<label<<" is illegal!"<<endl;
            exit(-1);
        }

    // for recording time cost
    double elapsedTime = getElapsedTimeInMs();
    printf("- Finshed, |V|=%d, |E|=%d, |L|=%d, d=%.1f. Time cost: %.0fms\n", VN, EN, labelNum, float(EN)/VN, elapsedTime);
}


Graph::~Graph() {
    delete[] neighbors;
    if (initialized) {
        delete[] visited;
        delete[] Q;
    }
}


// initialize for label constrained BFS
void Graph::initializeLCRsearch() {
    visited = new VertexID[VN]();
    Q = new VertexID[VN];
    initialized = true;
}


// label constrained BFS for graphs with small number of labels
bool Graph::LCRsearch(const VertexID& s, const VertexID& t, const LabelSet& labelSet) {
    if (s==t)
        return true;  
    if (offset >= INT_MAX) {
        offset = 1;
        memset(visited, 0, sizeof visited);
    }
    
    queueBegin = 0;
    queueEnd = 1;
    visited[s] = offset;
    Q[queueBegin] = s;

    while (queueBegin<queueEnd) {
        const VertexID& cur = Q[queueBegin];
        ++queueBegin;

        for (auto iter = neighbors[cur].out.begin(); iter!=neighbors[cur].out.end(); ++iter)
            if ((1<<(iter->first)) & labelSet)
                for (int j=0; j<iter->second.size(); ++j) {
                    const VertexID& nxt = iter->second[j];
                    if (nxt==t) {
                        ++offset;
                        return true;
                    } else 
                        if (visited[nxt]<offset) {
                            visited[nxt] = offset;
                            Q[queueEnd++] = nxt;
                        }
                }
    }

    ++offset;
    return false;
}


// label constrained BFS for graphs with small number of labels
bool Graph::LCRsearch(const VertexID& s, const VertexID& t, const vector<LabelID>& lls) {
    if (s==t)
        return true;  
    if (offset >= INT_MAX) {
        offset = 1;
        memset(visited, 0, sizeof visited);
    }
    
    queueBegin = 0;
    queueEnd = 1;
    visited[s] = offset;
    Q[queueBegin] = s;

    while (queueBegin<queueEnd) {
        const VertexID& cur = Q[queueBegin];
        ++queueBegin;

        for (const LabelID& label : lls)
            if (neighbors[cur].out.find(label)!=neighbors[cur].out.end())
                for (int j=0; j<neighbors[cur].out[label].size(); ++j) {
                    const VertexID& nxt = neighbors[cur].out[label][j];
                    if (nxt==t) {
                        ++offset;
                        return true;
                    } else 
                        if (visited[nxt]<offset) {
                            visited[nxt] = offset;
                            Q[queueEnd++] = nxt;
                        }
                }
    }

    ++offset;
    return false;
}


#endif