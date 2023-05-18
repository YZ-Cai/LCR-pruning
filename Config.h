/*
LCR - Configurations
Author: Yuzheng Cai
2022-09-08
------------------------------
C++ 11 
*/ 


#ifndef CONFIG_H
#define CONFIG_H
#include <fstream>
using namespace std;


// Vertex IDs
typedef unsigned int VertexID;

// Edge IDs
typedef unsigned int EdgeID;

// Edge label IDs
typedef unsigned int LabelID;

// bits for representing label set, e.g., L={0,2,3,5} -> 00101101
typedef unsigned int LabelSet;

// log file
ofstream logFile("Results/Logs.csv", ios::app);

// dataset path
string datasetPath = "Datasets/";

// Merge all secondary labels into THRESHOLD virtual labels, when |L|>2*THRESHOLD
#define THRESHOLD 6


#endif