# generate random queries
cd Datasets/GenQuery
sh GenQuery.sh
cd ../..

# answering LCR queries
make
make clear
ulimit -s 2097152
./main TestGraph1.edge
./main TestGraph2.edge
