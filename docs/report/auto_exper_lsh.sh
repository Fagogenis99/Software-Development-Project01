#!/bin/bash
# -------------------------------------------------------------------
# Locality-Sensitive Hashing (LSH) experimental script
# Runs multiple parameter combinations and saves both .txt and .csv
# -------------------------------------------------------------------

# Step 1: Move to project root
cd "$(dirname "$0")/../.."

# Step 2: Define data paths and create result folder
mkdir -p docs/report/results
DSET="-d data/train-images.idx3-ubyte"
QSET="-q data/t10k-images.idx3-ubyte"
TYPE="-type mnist"

# Step 3: Define output files
OUT_CSV=docs/report/results/lsh.csv
OUT_TXT=docs/report/results/lsh_output.txt
echo "k,L,w,Recall@N,AF,QPS,tApprox" > $OUT_CSV
echo "=== LSH EXPERIMENT RESULTS ===" > $OUT_TXT

# Step 4: Parameter loops
for k in 4 6 8; do
  for L in 5 10 15; do
    w=4.0

    echo "Running LSH (k=$k, L=$L, w=$w)"
    ./search $DSET $QSET $TYPE -lsh -k $k -L $L -w $w \
      -N 1 -R 2000 -range false -o docs/report/results/tmp_lsh.txt

    # Step 5: Extract metrics from output
    recall=$(grep "Recall@N" docs/report/results/tmp_lsh.txt | awk '{print $2}')
    af=$(grep "Average AF" docs/report/results/tmp_lsh.txt | awk '{print $3}')
    qps=$(grep "QPS" docs/report/results/tmp_lsh.txt | awk '{print $2}')
    tapp=$(grep "tApproximateAverage" docs/report/results/tmp_lsh.txt | awk '{print $2}')

    # Step 6: Append results
    echo "$k,$L,$w,$recall,$af,$qps,$tapp" >> $OUT_CSV
    echo "-------------------------------------------------" >> $OUT_TXT
    cat docs/report/results/tmp_lsh.txt >> $OUT_TXT
  done
done

echo "✅ LSH experiments done → $OUT_CSV and $OUT_TXT"
