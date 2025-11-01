#!/bin/bash
# -------------------------------------------------------------------
# IVF-PQ ANN experimental script
# -------------------------------------------------------------------

cd "$(dirname "$0")/../.."

mkdir -p docs/report/results
DSET="-d data/train-images.idx3-ubyte"
QSET="-q data/t10k-images.idx3-ubyte"
TYPE="-type mnist"

OUT_CSV=docs/report/results/ivfpq.csv
OUT_TXT=docs/report/results/ivfpq_output.txt
echo "kclusters,nprobe,M,nbits,Recall@N,AF,QPS,tApprox" > $OUT_CSV
echo "=== IVFPQ EXPERIMENT RESULTS ===" > $OUT_TXT

# parameter loops (feel free to adjust ranges)
for kclusters in 20 50 100; do
  for nprobe in 1 5 10; do
    for M in 8 16; do
      for nbits in 6 8; do

        echo "Running IVFPQ (kclusters=$kclusters, nprobe=$nprobe, M=$M, nbits=$nbits)"
        ./search $DSET $QSET $TYPE -ivfpq \
          -kclusters $kclusters -nprobe $nprobe \
          -M $M -nbits $nbits \
          -N 1 -R 2000 -range false \
          -o docs/report/results/tmp_ivfpq.txt > docs/report/results/tmp_ivfpq.txt

        # Extract metrics from output
        recall=$(grep "Recall@N" docs/report/results/tmp_ivfpq.txt | awk '{print $2}')
        af=$(grep "Average AF" docs/report/results/tmp_ivfpq.txt | awk '{print $3}')
        qps=$(grep "QPS" docs/report/results/tmp_ivfpq.txt | awk '{print $2}')
        tapp=$(grep "tApproximateAverage" docs/report/results/tmp_ivfpq.txt | awk '{print $2}')

        # Append results
        echo "$kclusters,$nprobe,$M,$nbits,$recall,$af,$qps,$tapp" >> $OUT_CSV
        echo "-------------------------------------------------" >> $OUT_TXT
        cat docs/report/results/tmp_ivfpq.txt >> $OUT_TXT

      done
    done
  done
done

echo "✅ IVFPQ experiments done → $OUT_CSV and $OUT_TXT"
