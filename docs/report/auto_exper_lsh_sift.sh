#!/bin/bash
# Locality-Sensitive Hashing (LSH) experimental script — SIFT version


cd "$(dirname "$0")/../.."
mkdir -p docs/report/results

# --- Dataset paths ---
DSET="-d data/sift_base.fvecs"
QSET="-q data/sift_query.fvecs"
TYPE="-type sift"

# --- Output files ---
OUT_CSV=docs/report/results/lsh_sift.csv
OUT_TXT=docs/report/results/lsh_sift_output.txt

echo "k,L,w,Recall@N,AF,QPS,tApprox" > $OUT_CSV
echo "=== LSH SIFT EXPERIMENT RESULTS ===" > $OUT_TXT

# --- Parameter grid ---
for k in 4 6 8; do
  for L in 5 10 15; do
    w=4.0
    echo "Running LSH SIFT (k=$k, L=$L, w=$w)"
    ./search $DSET $QSET $TYPE -lsh -k $k -L $L -w $w \
      -N 1 -R 2 -range false -o docs/report/results/tmp_lsh_sift.txt

    recall=$(grep "Recall@N" docs/report/results/tmp_lsh_sift.txt | awk '{print $2}')
    af=$(grep "Average AF" docs/report/results/tmp_lsh_sift.txt | awk '{print $3}')
    qps=$(grep "QPS" docs/report/results/tmp_lsh_sift.txt | awk '{print $2}')
    tapp=$(grep "tApproximateAverage" docs/report/results/tmp_lsh_sift.txt | awk '{print $2}')

    echo "$k,$L,$w,$recall,$af,$qps,$tapp" >> $OUT_CSV
    echo "-------------------------------------------------" >> $OUT_TXT
    cat docs/report/results/tmp_lsh_sift.txt >> $OUT_TXT
  done
done

echo "✅ LSH SIFT experiments done → $OUT_CSV and $OUT_TXT"
