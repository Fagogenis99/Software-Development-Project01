set terminal pngcairo size 800,600
set output 'docs/report/figs/ivfpq_recall_vs_time.png'
set datafile separator ','
set title 'IVFPQ Recall vs Time'
set xlabel 'tApprox (ms)'
set ylabel 'Recall@N'
plot 'docs/report/results/ivfpq.csv' using 7:4 with linespoints title 'Recall vs Time'
