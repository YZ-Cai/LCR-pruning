/*
LCR - Answering with Pruning Strategies
Author: Yuzheng Cai
2022-10-27
------------------------------
C++ 11
Implementations for graphs with large number of labels
*/ 


#ifndef INDEXL_CC
#define INDEXL_CC
#include "IndexL.h"


IndexL::IndexL(Graph* inputGraph) {
    graph = inputGraph;
    VN = graph->VN;
    EN = graph->EN;
    labelNum = graph->labelNum;
    neighbors = graph->neighbors;
}


double IndexL::buildIndex() {
    if (builtIndex) {
        cout << "! Index already exists" <<endl;
        return 0;
    }
    index = new IndexNode[VN];
    
    // memory reused by several subtasks
     int* intVNreuse = new int[VN]();
    VertexID* vidVNreuse1 = new VertexID[VN+1];
    VertexID* vidVNreuse2 = new VertexID[VN];
    bool* boolVNreuse = new bool[VN]();

    // build DAG for unreachable query filter (UQF)
    cout<<"Start building DAG ..."<<endl;
    startRecordTime();
    GenerateDAG dag(graph, intVNreuse, vidVNreuse1, vidVNreuse2, boolVNreuse);
    double genDAGtime = getElapsedTimeInMs();
    cout<<"- Finished. DAG has "<<dag.DAGVM<<" vertices and "<<dag.DAGEN<<" edges, degree="<<(dag.DAGEN)/float(dag.DAGVM)<<". Time cost: "<<genDAGtime<<" ms"<<endl;

    // obtain vertex id mapping from original graph to DAG
    DAGVN = dag.DAGVM;
    for (VertexID i=0; i<VN; ++i)
        index[i].raw2DAG = dag.raw2DAG[i];
    dag.freeMemory();
    
    // generate unreachable query filter index (UQF)
    cout<<"Start building unreachable query filter index ..."<<endl;
    startRecordTime();
    UQF UQFindex(&dag, vidVNreuse1, vidVNreuse2, boolVNreuse);
    double UQFindexTime = getElapsedTimeInMs();
    cout<<"- Finished. Time cost: "<<UQFindexTime<<" ms"<<endl;

    // obtain unreachable query filter (UQF) index
    UQForders = UQFindex.UQForders;
    UQFindex.freeMemory();
    delete[] dag.DAGneighbors;

    // free unuseful memory reused by several subtasks
    visited = intVNreuse;
    isProcessed = boolVNreuse;
    visitedS = new int[VN]();
    Q = vidVNreuse1;
    delete[] vidVNreuse2;

    // build 2-hop index using degree-one reduction (DOR)
    printf("Start building P2H+ index with degree-one reduction ...\n");
    startRecordTime();
    divideLabels();
    build2hop();
    double P2HindexTime = getElapsedTimeInMs();
    printf("- Finished, time cost: %.2fms\n", P2HindexTime);

    // return total time cost
    builtIndex = true;
    return genDAGtime + UQFindexTime + P2HindexTime;
}


void IndexL::freeIndex() {
    if (builtIndex) {
        delete[] index;
        delete[] UQForders;
        delete[] visited;
        delete[] reverseMapping;
        delete[] visitedS;
        delete[] Q;
        builtIndex = false;    
    }
}


double IndexL::runAllQueries(const vector<PerQuery>& queries) {
    printf("Start running %d queries ...\n", int(queries.size()));
    startRecordTime();

    for (int i=0; i<queries.size(); ++i) {
        const PerQuery& q = queries[i];
        VertexID s=q.s, t=q.t;
        const vector<LabelID>& lls=q.lls;
        
        if (query(s, t, lls)==q.ans)
            continue;
        printf("! Error in %d-th query: %d->%d, label set: %s. Answer should be %s\n", i, s, t, labelSetToString(lls).c_str(), q.ans?"true":"false");
    }

    double queryTime = getElapsedTimeInMs();
    printf("- Finished, time cost: %.2fms\n", queryTime);
    return queryTime;
}


void IndexL::divideLabels() {
    vector<int> distribution(labelNum, 0);
    EdgeID secondaryCnt = 0;
    for (VertexID i=0; i<VN; i++)
        for (auto iter=neighbors[i].out.begin(); iter!=neighbors[i].out.end(); ++iter)
            if (iter->first >= THRESHOLD) {
                ++secondaryCnt;
                distribution[iter->first]++;
            }
    cout<<"- Secondary label percentage: "<<(float(secondaryCnt)/EN)<<endl;

    labelMapping.resize(labelNum);
    reverseMapping = new vector<LabelID>[THRESHOLD*2];
    for (LabelID i=0; i<THRESHOLD; ++i) {
        primaryMask |= (1<<i);
        labelMapping[i] = i;
        reverseMapping[i].emplace_back(i);
    }
 
    LabelID cur=0;
    EdgeID cnt=0;
    for (LabelID i=THRESHOLD; i<labelNum; i++){
        cnt += distribution[i];
        labelMapping[i] = THRESHOLD+cur;
        reverseMapping[THRESHOLD+cur].emplace_back(i);
        if (cnt>=(cur+1)*secondaryCnt/THRESHOLD)
            cur++;
    }
}



void IndexL::build2hop() {

    // init local variables
    memset(isProcessed, 0, sizeof(bool)*VN);
    pair<VertexID, VertexID>* allHops = new pair<VertexID, VertexID>[VN];

    // sort by degree
    for (VertexID id=0; id<VN; ++id)
        allHops[id] = {id, neighbors[id].inDegree+neighbors[id].outDegree};
    sort(allHops, allHops+VN, 
         [](const pair<VertexID, VertexID>& a, const pair<VertexID, VertexID>& b) {
                return a.second>b.second;
            });

    // process each hop
    for (VertexID order=0; order<VN; ++order) {
        const VertexID& hopId = allHops[order].first;
        isProcessed[hopId] = true;

        // backward BFS
        frontier.emplace_back(hopId, 0);
        while (!frontier.empty()) {
            exploreBackwardWithCurLabels(hopId, order);
            exploreBackwardPlusOneLabel(hopId, order);
        }

        // forward BFS
        frontier.emplace_back(hopId, 0);
        while (!frontier.empty()) {
            exploreForwardWithCurLabels(hopId, order);
            exploreForwardPlusOneLabel(hopId, order);
        }

        if (neighbors[hopId].inDegree>1)
            index[hopId].inHops.emplace_back(order, 0);
        if (neighbors[hopId].outDegree>1)
            index[hopId].outHops.emplace_back(order, 0);
    }
    
    // free memory
    delete[] isProcessed;
    delete[] allHops;
    vector<pair<VertexID, LabelSet>> tmp1, tmp2;
    frontier.swap(tmp1);
    nxtFrontier.swap(tmp2);
}


void IndexL::exploreBackwardWithCurLabels(const VertexID& hopId, const VertexID& order) {
    VertexID curIdx = 0;
    while (curIdx<frontier.size()) {
        const VertexID u = frontier[curIdx].first;
        const LabelSet ls = frontier[curIdx].second;
        ++curIdx;
        for (auto iter=neighbors[u].in.begin(); iter!=neighbors[u].in.end(); ++iter) {
            const LabelSet label = 1<<(labelMapping[iter->first]);
            if (ls & label)
                for (const VertexID& v : iter->second) {
                    if (isProcessed[v] || queryForIndexBackward(order, v, hopId, ls))
                        continue;
                    if (neighbors[v].outDegree!=1) 
                        index[v].outHops.emplace_back(order, ls);
                    frontier.emplace_back(v, ls);
                }
        }
    }
}


void IndexL::exploreBackwardPlusOneLabel(const VertexID& hopId, const VertexID& order) {
    VertexID curIdx = 0;
    nxtFrontier.clear();
    while (curIdx<frontier.size()) {
        const VertexID& u = frontier[curIdx].first;
        const LabelSet& ls = frontier[curIdx].second;
        ++curIdx;
        for (auto iter=neighbors[u].in.begin(); iter!=neighbors[u].in.end(); ++iter) {
            const LabelSet newLabel = 1<<(labelMapping[iter->first]);
            if ( ( ls & newLabel ) == 0 )
                for (const auto& v : iter->second) { 
                    LabelSet newLabelSet = ls | newLabel;
                    if (isProcessed[v] || queryForIndexBackward(order, v, hopId, newLabelSet))
                        continue;
                    if (neighbors[v].outDegree!=1)
                        index[v].outHops.emplace_back(order, newLabelSet);
                    nxtFrontier.emplace_back(v, newLabelSet);
                } 
        }
    }
    frontier.swap(nxtFrontier);
}


void IndexL::exploreForwardWithCurLabels(const VertexID& hopId, const VertexID& order) {
    VertexID curIdx = 0;
    while (curIdx<frontier.size()) {
        const VertexID u = frontier[curIdx].first;
        const LabelSet ls = frontier[curIdx].second;
        ++curIdx;
        for (auto iter=neighbors[u].out.begin(); iter!=neighbors[u].out.end(); ++iter) {
            const LabelSet label = 1<<(labelMapping[iter->first]);
            if (ls & label)
                for (const VertexID& v : iter->second) {
                    if (isProcessed[v] || queryForIndexForward(order, hopId, v, ls))
                        continue; 
                    if (neighbors[v].inDegree!=1) 
                        index[v].inHops.emplace_back(order, ls);
                    frontier.emplace_back(v, ls);
                }
        }
    }
}


void IndexL::exploreForwardPlusOneLabel(const VertexID& hopId, const VertexID& order) {
    VertexID curIdx = 0;
    nxtFrontier.clear();
    while (curIdx<frontier.size()) {
        const VertexID& u = frontier[curIdx].first;
        const LabelSet& ls = frontier[curIdx].second;
        ++curIdx;
        for (auto iter=neighbors[u].out.begin(); iter!=neighbors[u].out.end(); ++iter) {
            const LabelSet newLabel = 1<<(labelMapping[iter->first]);
            if ( ( ls & newLabel ) == 0 )
                for (const auto& v : iter->second) { 
                    LabelSet newLabelSet = ls | newLabel;
                    if (isProcessed[v] || queryForIndexForward(order, hopId, v, newLabelSet))
                        continue;
                    if (neighbors[v].inDegree!=1) 
                        index[v].inHops.emplace_back(order, newLabelSet);
                    nxtFrontier.emplace_back(v, newLabelSet);
                } 
        }
    }
    frontier.swap(nxtFrontier);
}


inline bool IndexL::queryForIndexForward(const VertexID& order, const VertexID& hopId, const VertexID& v, const LabelSet& ls) {

    if (neighbors[v].inDegree!=1) {
        auto iter = index[v].inHops.rbegin();
        while (iter!=index[v].inHops.rend() && iter->first==order) {
            if (isSubset(iter->second, ls))
                return true;
            ++iter;
        }
    }

    VertexID cur = hopId;
    while (neighbors[cur].outDegree==1) {
        cur = neighbors[cur].out.begin()->second[0];
        if (cur==v) return false;
    }

    // query 2-hop index
    return query2hop(cur, v, ls);
}


inline bool IndexL::queryForIndexBackward(const VertexID& order, const VertexID& v, const VertexID& hopId, const LabelSet& ls) {

    if (neighbors[v].outDegree!=1) {
        auto iter = index[v].outHops.rbegin();
        while (iter!=index[v].outHops.rend() && iter->first==order) {
            if (isSubset(iter->second, ls))
                return true;
            ++iter;
        }
    } 

    VertexID cur = hopId;
    while (neighbors[cur].inDegree==1) {
        cur = neighbors[cur].in.begin()->second[0];
        if (cur==v) return false;
    }

    // query 2-hop index
    return query2hop(v, cur, ls);
}


bool IndexL::query2hop(const VertexID& s, const VertexID& t, const LabelSet& ls) {
    auto i=index[s].outHops.begin(), j=index[t].inHops.begin();
    while (i!=index[s].outHops.end() && j!=index[t].inHops.end()) {

        // same hop id
        if (i->first==j->first) {
            const VertexID& hopId = i->first;

            // check for i
            bool iPass = false;
            do {
                if (isSubset(i->second, ls)) {
                    iPass = true;
                    break;
                }
                ++i;
            } while (i!=index[s].outHops.end() && i->first==hopId);

            // check for j
            if (iPass) {
                do {
                    if (isSubset(j->second, ls)) 
                        return true;
                    ++j;
                } while (j!=index[t].inHops.end() && j->first==hopId);

                // move i to next hop id
                while (i!=index[s].outHops.end() && i->first==hopId)
                    ++i;

            // move j to next hop id
            } else 
                while (j!=index[t].inHops.end() && j->first==hopId)
                    ++j;

        // hop id not the same
        } else if (i->first<j->first)
            i = lower_bound(i, index[s].outHops.end(), *j, cmpByFirstElement);
        else
            j = lower_bound(j, index[t].inHops.end(), *i, cmpByFirstElement);
    }
    return false;
}


bool IndexL::query(const VertexID& s, const VertexID& t, const vector<LabelID>& lls) {
    if (builtIndex==false) {
        cout << "! Index does not exist" <<endl;
        return false;
    }
    if (s==t) return true;

    // unreachable query filter (UQF)
    const VertexID& sDAG = index[s].raw2DAG;
    const VertexID& tDAG = index[t].raw2DAG;
    const UQFindexNode& nT = UQForders[tDAG];
    if (sDAG!=tDAG) {
        const UQFindexNode& nS = UQForders[sDAG];
        if ( nS.X>=nT.X || nS.Y>=nT.Y || nS.level>=nT.level || nS.H1>=nT.H1 || nS.H2>=nT.H2 )
            return false;
    }

    // transform to unique neighbors
    VertexID curS = s, curT = t;
    visited[curS] = ++offset;
    while (neighbors[curS].outDegree==1) {
        if ( find(lls.begin(), lls.end(), neighbors[curS].out.begin()->first)==lls.end() ) return false;
        curS = neighbors[curS].out.begin()->second[0];
        if (curS==curT) return true;
        if (visited[curS]==offset) return false;
        visited[curS] = offset;
    }
    visited[curT] = ++offset;
    while (neighbors[curT].inDegree==1) {
        if ( find(lls.begin(), lls.end(), neighbors[curT].in.begin()->first)==lls.end() ) return false;
        curT = neighbors[curT].in.begin()->second[0];
        if (curS==curT) return true;
        if (visited[curT]==offset) return false;
        visited[curT] = offset;
    }

    LabelSet ls = 0;
    for (LabelID label:lls)
        ls |= (1<<(labelMapping[label]));
    if (query2hop(s, t, ls&primaryMask))
        return true;

    if (query2hop(curS, curT, ls)==false)
        return false;

    if (offsetS >= INT_MAX) {
        offsetS = 0;
        memset(visitedS, 0, sizeof visitedS);
    }
    ++offsetS;

    queueBegin = 0;
    queueEnd = 1;
    visitedS[s] = offsetS;
    Q[queueBegin] = s;

    while (queueBegin<queueEnd) {
        VertexID& cur = Q[queueBegin++];

        for (const LabelID& label : lls)
            if (neighbors[cur].out.find(label)!=neighbors[cur].out.end())
                for (int j=0; j<neighbors[cur].out[label].size(); ++j) {
                    VertexID nxt = neighbors[cur].out[label][j];
                    
                    if (visitedS[nxt]<offsetS) {
                        if (nxt==t) return true;
                        visitedS[nxt] = offsetS;
                    
                        bool fail = false;
                        while (neighbors[nxt].outDegree==1) {
                            if ( find(lls.begin(), lls.end(), neighbors[nxt].out.begin()->first)==lls.end() ) { 
                                fail = true;
                                break;
                            }
                            nxt = neighbors[nxt].out.begin()->second[0];
                            if (visitedS[nxt]<offsetS) {
                                if (nxt==t) return true;
                                visitedS[nxt] = offsetS;
                            } else {
                                fail = true;
                                break;
                            }
                        }
                        if (fail) continue;
                        
                        if (index[nxt].raw2DAG!=tDAG) {
                            const UQFindexNode& nCur = UQForders[index[nxt].raw2DAG];
                            if ( nCur.X>=nT.X || nCur.Y>=nT.Y || nCur.level>=nT.level || nCur.H1>=nT.H1 || nCur.H2>=nT.H2 )
                                continue;
                        }
                        if (query2hop(nxt, curT, ls))
                            Q[queueEnd++] = nxt;
                    }
                }
    }

    return false;
}


double IndexL::getIndexSizeInBytes() {
    if (builtIndex==false) {
        cout << "! Index does not exist" <<endl;
        return 0;
    }
    double size = 0;
    for (VertexID i=0; i<VN; ++i) 
        size += sizeof(pair<VertexID, LabelSet>)*(index[i].inHops.size()+index[i].outHops.size()); //+ sizeof(inHops[i]) + sizeof(outHops[i]);
    size += sizeof(VertexID)*VN + sizeof(UQFindexNode)*DAGVN;
    return size;
}


double IndexL::getIndexEntryCnt() {
    if (builtIndex==false) {
        cout << "! Index does not exist" <<endl;
        return 0;
    }
    double cnt = 0;
    for (VertexID i=0; i<VN; ++i) 
        cnt += index[i].inHops.size()+index[i].outHops.size();
    return cnt;
}


#endif