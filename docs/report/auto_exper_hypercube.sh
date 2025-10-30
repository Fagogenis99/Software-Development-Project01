#!/bin/bash
# -------------------------------------------------------------------
# Hypercube ANN experimental script
# -------------------------------------------------------------------

cd "$(dirname "$0")/../.."

mkdir -p docs/report/results
DSET="-d data/train-images.idx3-ubyte"
QSET="-q data/t10k-images.idx3-ubyte"
TYPE="-type mnist"

OUT_CSV=docs/report/results/hypercube.csv
OUT_TXT=docs/report/results/hypercube_output.txt
echo "kproj,M,probes,Recall@N,AF,QPS,tApprox" > $OUT_CSV
echo "=== HYPERCUBE EXPERIMENT RESULTS ===" > $OUT_TXT

for kproj in 10 12 14; do
  for M in 20 50 100; do
    for probes in 10 15 20; do
      echo "Running Hypercube (kproj=$kproj, M=$M, probes=$probes)"
      ./search $DSET $QSET $TYPE -hypercube -kproj $kproj -M $M -probes $probes \
        -w 4.0 -N 1 -R 2000 -range false -o docs/report/results/tmp_cube.txt

      recall=$(grep "Recall@N" docs/report/results/tmp_cube.txt | awk '{print $2}')
      af=$(grep "Average AF" docs/report/results/tmp_cube.txt | awk '{print $3}')
      qps=$(grep "QPS" docs/report/results/tmp_cube.txt | awk '{print $2}')
      tapp=$(grep "tApproximateAverage" docs/report/results/tmp_cube.txt | awk '{print $2}')

      echo "$kproj,$M,$probes,$recall,$af,$qps,$tapp" >> $OUT_CSV
      echo "-------------------------------------------------" >> $OUT_TXT
      cat docs/report/results/tmp_cube.txt >> $OUT_TXT
    done
  done
done

echo "✅ Hypercube experiments done → $OUT_CSV and $OUT_TXT"
