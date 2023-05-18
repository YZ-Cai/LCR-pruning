# LCR with Pruning Techniques

<br/>

This is the source codes for paper "Answering Label-Constrained Reachability Queries via Reduction Techniques", link of the paper:

[Answering Label-Constrained Reachability Queries via Reduction Techniques](https://doi.org/10.1007/978-3-031-30637-2_8)

You can directly run the codes by command:

```bash
sh Run.sh
```

<br/>

## 1 Input Graph Data

An input graph file example (`Datasets/TestGraph1.edge`):

```
13 18 6
0 4 0
1 4 5
2 1 4
3 0 1
... (14 lines omitted)
```

In first line, three numbers are the number of vertices (N), edges (M) and labels (L) of this graph, respectively.

In the following M lines, each represents an edges `(src, dst, label)`. For example, the second line is an edge from vertex 0 to vertex 4 with label 0. Note that vertex id ranges from [0, N), while label ranges from [0, L).

<br/>

## 2 Generate Queries

In `./Datasets/GenQuery/`, please run the `make` command to compile first.

**Usage**

```bash
./GenQuery <graph file> <k: number of query sets> <l: number of queries> <k numbers denoting number of labels per query>
```

**Example**

```bash
./GenQuery ../TestGraph1.edge 3 1000 1 3 4
```

It generates 3 query sets for graph `TestGraph1.edge`, each query set has 1000 queries, each of them has query label number of 1, 3 and 4, respectively.

<br/>

## 3 Answering Queries

In the root directory, please run the `make` command to compile first.

**Usage**

```bash
./main <graph filename>
```

**Example**

```bash
./main TestGraph1.edge
```

**Statistics**

After running, the index construction time, index entry number, index size, query set size and time for each query set are stored in `./Results/Logs.csv`.

<br/>

## 4 Notes

For linux, the size of running stack may need to extend before running the program, for example, set to 2048 MB:

```bash
ulimit -s 2097152 
```

In `Config.h`, you can change the input and output path, as well as the threshold of label size for using secondary label index.

Thanks for the codes provided in [khaledammar/LCR](https://github.com/khaledammar/LCR)
