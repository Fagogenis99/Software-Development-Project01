#!/bin/bash
# -------------------------------------------------------------------
# IVF-Flat ANN experimental script
# -------------------------------------------------------------------

cd "$(dirname "$0")/../.."

mkdir -p docs/report/results
DSET="-d data/train-images.idx3-ubyte"
QSET="-q data/t10k-images.idx3-ubyte"
TYPE="-type mnist"

OUT_CSV=docs/report/results/ivfflat.csv
OUT_TXT=docs/report/results/ivfflat_output.txt
echo "kclusters,nprobe,_,Recall@N,AF,QPS,tApprox" > $OUT_CSV
echo "=== IVFFLAT EXPERIMENT RESULTS ===" > $OUT_TXT

for kclusters in 20 50 100; do
  for nprobe in 1 5 10; do
    echo "Running IVFFlat (kclusters=$kclusters, nprobe=$nprobe)"
    ./search $DSET $QSET $TYPE -ivfflat -kclusters $kclusters -nprobe $nprobe \
      -N 1 -R 2000 -range false -o docs/report/results/tmp_ivf.txt > docs/report/results/tmp_ivf.txt

    recall=$(grep "Recall@N" docs/report/results/tmp_ivf.txt | awk '{print $2}')
    af=$(grep "Average AF" docs/report/results/tmp_ivf.txt | awk '{print $3}')
    qps=$(grep "QPS" docs/report/results/tmp_ivf.txt | awk '{print $2}')
    tapp=$(grep "tApproximateAverage" docs/report/results/tmp_ivf.txt | awk '{print $2}')

    echo "$kclusters,$nprobe,0,$recall,$af,$qps,$tapp" >> $OUT_CSV
    echo "-------------------------------------------------" >> $OUT_TXT
    cat docs/report/results/tmp_ivf.txt >> $OUT_TXT
  done
done

echo "✅ IVFFlat experiments done → $OUT_CSV and $OUT_TXT"
