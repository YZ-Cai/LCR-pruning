/*
LCR - Generate Queries Randomly
Author: Yuzheng Cai
2021-5-19
------------------------------
C++ 11 
Implementations based on https://github.com/khaledammar/LCR
*/ 


#include "../../GraphUtils/Graph.cc"
bool largeLabelSet = false;


// Generates random labelset with exactly nK labels randomly, for small label set
LabelSet generateLabelSet(LabelSet& ls, int nK, uniform_int_distribution<LabelID>& distribution, default_random_engine& generator) {
    int nL = getNumberOfLabelsInLabelSet(ls);
    while(nL != nK) {
        int j = distribution(generator);
        if( nL < nK )
            setLabelInLabelSet(ls, j , 1);
        if( nL > nK )
            setLabelInLabelSet(ls, j , 0);
        nL = getNumberOfLabelsInLabelSet(ls);
    }
    return ls;
}


// Generates random labelset with exactly nK labels randomly, for large label set
vector<LabelID> generateLabelSet(vector<LabelID>& ls, int nK, uniform_int_distribution<LabelID>& distribution, default_random_engine& generator) {
    int nL = ls.size();
    while(nL != nK) {
        int j = distribution(generator);
        if( nL < nK && find(ls.begin(), ls.end(), j)==ls.end() )
            ls.emplace_back(j);
        if( nL > nK )
            ls.pop_back();
        nL = ls.size();
    }
    return ls;
}


// Randomly generate per query set given number of labels
void generatePerQuerySet(int numOfQueriesPerQuerySet, vector<PerQuery>& querySet, Graph* graph, LabelID numOfLabel) {
    VertexID N = graph->VN;
    LabelID L = graph->labelNum;

    // random distributions for picking a vertex
    unsigned int seed = time(NULL)*time(NULL) % 1000;
    default_random_engine vertexGenerator(seed);
    uniform_int_distribution<VertexID> vertexDistribution(0, N-1);

    // random distributions for picking a label set
    seed = (2*time(NULL)*time(NULL)*time(NULL)) % 1000;
    default_random_engine labelGenerator(seed);
    uniform_int_distribution<LabelID> labelDistribution(0, L-1);

    while (querySet.size()<numOfQueriesPerQuerySet) {
        VertexID s = vertexDistribution(vertexGenerator);
        VertexID t = vertexDistribution(vertexGenerator);
        if( s == t )
            continue;
        
        if (largeLabelSet) {
            vector<LabelID> lls;
            lls = generateLabelSet(lls, numOfLabel, labelDistribution, labelGenerator);
            if( graph->LCRsearch(s, t, lls) ) 
                querySet.emplace_back(s, t, lls, true);
            else 
                querySet.emplace_back(s, t, lls, false);
        } else {
            LabelSet ls = 0;
            ls = generateLabelSet(ls, numOfLabel, labelDistribution, labelGenerator);
            if( graph->LCRsearch(s, t, ls) ) 
                querySet.emplace_back(s, t, ls, true);
            else 
                querySet.emplace_back(s, t, ls, false);
        }
    }
    cout << "- Finished"<<endl;
}


// write each query set to file
void writeQuerySetToFile(vector<PerQuery>& querySet, string queryFilename) {
    ofstream queryFile (queryFilename);
    for(const auto& q : querySet) {
        queryFile << (q.ans?1:0) << " " << q.s << " " << q.t;
        if (largeLabelSet) {
            for (const LabelID& l : q.lls)
                queryFile << " " << to_string(l);
            queryFile << endl;
        } else {
            queryFile << " " << to_string(q.ls) << endl;
        }
    }
    queryFile.close();
}



int main(int argc, char *argv[]) {

    if ( argc < 3 ) {
        cout << "./GenQuery <edge file> <k: number of query sets> <l: number of queries> <k numbers denoting number of labels per query>"  << endl;
        return 1;
    }

    string graphFilename = argv[1];
    int numOfQuerySet = atoi(argv[2]);
    int numOfQueriesPerQuerySet = atoi(argv[3]);
    vector<LabelID> numOfLabels;

    if ( argc < (3+numOfQuerySet) ) {
        cout << "Too few label numbers !" << endl;
        cout << "./GenQuery <edge file> <k: number of query sets> <l: number of queries> <k numbers denoting number of labels per query>" << endl;
        return 1;
    }

    for (int i = 0; i < numOfQuerySet; ++i) {
        int numOfLabel = atoi(argv[4+i]);
        numOfLabels.push_back( numOfLabel );
    }

    cout << "# graph filename: " << graphFilename << endl;
    cout << "# num of query sets: " << numOfQuerySet << endl;
    cout << "# num of queries per query set: " << numOfQueriesPerQuerySet << endl;

    // initialize graph data
    Graph* graph = new Graph(graphFilename);
    graph->initializeLCRsearch();
    if ( graph->labelNum > 2*THRESHOLD )
        largeLabelSet = true;

    // generate query sets
    for (int i = 0; i < numOfQuerySet; ++i) {
        vector<PerQuery> querySet;
        cout << "Generating querysets " << i << ", number of labels: " << numOfLabels[i] << endl;
        generatePerQuerySet(numOfQueriesPerQuerySet, querySet, graph, numOfLabels[i]);
        writeQuerySetToFile(querySet, graphFilename+"-"+to_string(i)+".query");
    }
    return 0;
}
