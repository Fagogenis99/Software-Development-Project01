# Software-Development-Project1
This is the first project for Software Development for Algorithmic Problems course



to test the loader : g++ -O3 -std=c++17 main.cpp -o test_loader

to download mnist files from github : wget https://github.com/mrgloom/MNIST-dataset-in-different-formats/raw/master/data/Original%20dataset/train-images.idx3-ubyte 
and wget https://github.com/mrgloom/MNIST-dataset-in-different-formats/raw/master/data/Original%20dataset/t10k-images.idx3-ubyte

to compile : g++ -O3 -std=c++17 ./main.cpp ./kmeans.cpp ./ivf_flat.cpp -o search

to test : ./search -d ../data/train-images.idx3-ubyte -q ../data/t10k-images.idx3-ubyte   -type mnist -ivfflat -kclusters 50 -nprobe 5 -N 1 -R 2000 -range false