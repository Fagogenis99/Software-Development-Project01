#!/bin/bash
# IVF-Flat ANN experimental script — SIFT version


cd "$(dirname "$0")/../.."
mkdir -p docs/report/results

# --- Dataset paths ---
DSET="-d data/sift_base.fvecs"
QSET="-q data/sift_query.fvecs"
TYPE="-type sift"

OUT_CSV=docs/report/results/ivfflat_sift.csv
OUT_TXT=docs/report/results/ivfflat_sift_output.txt

echo "kclusters,nprobe,_,Recall@N,AF,QPS,tApprox" > $OUT_CSV
echo "=== IVFFLAT SIFT EXPERIMENT RESULTS ===" > $OUT_TXT

# --- Parameter grid ---
for kclusters in 20 50 100; do
  for nprobe in 5 10 20; do
    echo "Running IVFFlat SIFT (kclusters=$kclusters, nprobe=$nprobe)"
    ./search $DSET $QSET $TYPE -ivfflat -kclusters $kclusters -nprobe $nprobe \
      -N 1 -R 2 -range false -o docs/report/results/tmp_ivf_sift.txt > docs/report/results/tmp_ivf_sift.txt

    recall=$(grep "Recall@N" docs/report/results/tmp_ivf_sift.txt | awk '{print $2}')
    af=$(grep "Average AF" docs/report/results/tmp_ivf_sift.txt | awk '{print $3}')
    qps=$(grep "QPS" docs/report/results/tmp_ivf_sift.txt | awk '{print $2}')
    tapp=$(grep "tApproximateAverage" docs/report/results/tmp_ivf_sift.txt | awk '{print $2}')

    echo "$kclusters,$nprobe,0,$recall,$af,$qps,$tapp" >> $OUT_CSV
    echo "-------------------------------------------------" >> $OUT_TXT
    cat docs/report/results/tmp_ivf_sift.txt >> $OUT_TXT
  done
done

echo "✅ IVFFlat SIFT experiments done → $OUT_CSV and $OUT_TXT"
