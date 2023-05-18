/*
LCR - Main Program
Author: Yuzheng Cai
2022-09-12
------------------------------
C++ 11 
*/ 


#include "Index/Index.cc"
#include "Index/IndexL.cc"


int main(int argc, char* argv[]) {

    // parameters
    if (argc<2) {
        printf("Usage: ./%s <graphFilename>\n", argv[0]);
        exit(-1);
    } 
    graphFilename = datasetPath+argv[1];
    
    // read in graph
    Graph* graph = new Graph(graphFilename);

    // for graphs with small number of labels
    if (graph->labelNum <= 2*THRESHOLD) {
        Index* index = new Index(graph);

        // build index with pruning techniques 
        double indexTime = index->buildIndex();
        double indexSize = index->getIndexSizeInBytes();
        double indexEntryCnt = index->getIndexEntryCnt();

        // write to log file
        logFile<<graphFilename<<","<<(graph->VN)<<","<<(graph->EN)<<","<<(graph->labelNum)<<","<<indexTime<<","<<indexEntryCnt<<","<<indexSize;

        // for each query file 
        for (int k=0; k<3; ++k) {
            
            // load all queries
            vector<PerQuery> queries = loadQueryFile(graphFilename+"-"+to_string(k)+".query", false);

            // run all queries
            double queryTime = index->runAllQueries(queries)*1000;

            // write to log file
            logFile<<","<<queries.size()<<","<<queryTime;
        }
        
        // clean up
        index->freeIndex();
        delete index;

    // for graphs with large number of labels
    } else {
        IndexL* index = new IndexL(graph);

        // build index with pruning techniques 
        double indexTime = index->buildIndex();
        double indexSize = index->getIndexSizeInBytes();
        double indexEntryCnt = index->getIndexEntryCnt();

        // write to log file
        logFile<<graphFilename<<","<<(graph->VN)<<","<<(graph->EN)<<","<<(graph->labelNum)<<","<<indexTime<<","<<indexEntryCnt<<","<<indexSize;

        // for each query file 
        for (int k=0; k<3; ++k) {
            
            // load all queries
            vector<PerQuery> queries = loadQueryFile(graphFilename+"-"+to_string(k)+".query", true);

            // run all queries
            double queryTime = index->runAllQueries(queries)*1000;

            // write to log file
            logFile<<","<<queries.size()<<","<<queryTime;
        }
        
        // clean up
        index->freeIndex();
        delete index;
    }
    
    // clean up
    delete graph;
    cout<<endl;
    logFile<<endl;
    logFile.close();

    return 0;
}