# Compiler and flags
CXX := g++
CXXFLAGS := -O3 -std=c++17 -Iinclude -Wall -Wextra
LDFLAGS := 

# Executable name
OUT := search

# Source files
SRC := \
	src/vector_utils.cpp \
	src/bruteForce.cpp \
	src/lsh.cpp \
	src/hypercube.cpp \
	src/kmeans.cpp \
	src/ivf_flat.cpp \
	src/main.cpp

# Default target
all: $(OUT)

# Build rule
$(OUT): $(SRC)
	$(CXX) $(CXXFLAGS) $(SRC) -o $(OUT) $(LDFLAGS)
	@echo "✅ Build complete: ./$(OUT)"

# Run examples
run-lsh:
	./$(OUT) \
		-d data/train-images.idx3-ubyte \
		-q data/t10k-images.idx3-ubyte \
		-type mnist \
		-lsh \
		-k 6 -L 10 -w 4.0 -N 1 -R 2000 -range false \
		-o docs/results_lsh.txt

run-cube:
	./$(OUT) \
		-d data/train-images.idx3-ubyte \
		-q data/t10k-images.idx3-ubyte \
		-type mnist \
		-hypercube \
		-kproj 14 -M 10 -probes 2 -w 4.0 -N 1 -R 2000 -range false \
		-o docs/results_hypercube.txt $(ARGS)
#run-ivf:
	./$(OUT) \
		-d data/train-images.idx3-ubyte \
		-q data/t10k-images.idx3-ubyte \
		-type mnist \
		-ivfflat \
		-kclusters 50 -nprobe 5 -N 1 -range false \
		-o docs/results_ivf.txt

# Clean build files
clean:
	rm -f $(OUT)
	@echo "Cleaned up build files."
