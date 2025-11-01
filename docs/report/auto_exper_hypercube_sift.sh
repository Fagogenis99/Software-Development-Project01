#!/bin/bash
# Hypercube ANN experimental script — SIFT version

cd "$(dirname "$0")/../.."
mkdir -p docs/report/results

# --- Dataset paths ---
DSET="-d data/sift_base.fvecs"
QSET="-q data/sift_query.fvecs"
TYPE="-type sift"

OUT_CSV=docs/report/results/hypercube_sift.csv
OUT_TXT=docs/report/results/hypercube_sift_output.txt

echo "kproj,M,probes,Recall@N,AF,QPS,tApprox" > $OUT_CSV
echo "=== HYPERCUBE SIFT EXPERIMENT RESULTS ===" > $OUT_TXT

# --- Parameter grid ---
for kproj in 10 12 14; do
  for M in 100 300 500; do
    for probes in 10 20 30; do
      echo "Running Hypercube SIFT (kproj=$kproj, M=$M, probes=$probes)"
      ./search $DSET $QSET $TYPE -hypercube -kproj $kproj -M $M -probes $probes \
        -w 4.0 -N 1 -R 2 -range false -o docs/report/results/tmp_cube_sift.txt

      recall=$(grep "Recall@N" docs/report/results/tmp_cube_sift.txt | awk '{print $2}')
      af=$(grep "Average AF" docs/report/results/tmp_cube_sift.txt | awk '{print $3}')
      qps=$(grep "QPS" docs/report/results/tmp_cube_sift.txt | awk '{print $2}')
      tapp=$(grep "tApproximateAverage" docs/report/results/tmp_cube_sift.txt | awk '{print $2}')

      echo "$kproj,$M,$probes,$recall,$af,$qps,$tapp" >> $OUT_CSV
      echo "-------------------------------------------------" >> $OUT_TXT
      cat docs/report/results/tmp_cube_sift.txt >> $OUT_TXT
    done
  done
done

echo "✅ Hypercube SIFT experiments done → $OUT_CSV and $OUT_TXT"
