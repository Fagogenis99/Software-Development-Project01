🧠 Nearest Neighbor Search Algorithms — LSH, Hypercube, IVFFlat & IVFPQ

🧑‍💻 Ομάδα Εργασίας
Νίκος Βουρτσανής	sdi2200022 - Κωνσταντίνος Φαγογένης sdi21.....	

📘 Περιγραφή Προγράμματος

Το παρόν έργο αποτελεί την 1η εργασία του μαθήματος Ανάπτυξη Λογισμικού για Αλγοριθμικά Προβλήματα και αφορά την υλοποίηση και πειραματική αξιολόγηση αλγορίθμων αναζήτησης πλησιέστερων γειτόνων (Nearest Neighbor Search).

Στόχος είναι η ανάπτυξη ενός ενιαίου προγράμματος που υλοποιεί και συγκρίνει τέσσερις διαφορετικές προσεγγιστικές μεθόδους:

Locality Sensitive Hashing (LSH)
Χρησιμοποιεί  gaussian προβολές και πολλαπλά hash tables για να εντοπίσει παρόμοια διανύσματα με υψηλή πιθανότητα συγκέντρωσης στο ίδιο bucket.

Hypercube Hashing
Προβάλλει τα διανύσματα σε έναν δυαδικό χώρο k' διαστάσεων και αποθηκεύει τα σημεία σε κορυφές ενός υπερκύβου. Η αναζήτηση γίνεται εξερευνώντας γειτονικές κορυφές (probes).

IVFFlat (Inverted File Index + Flat Quantization)
Εκτελεί  ομαδοποίηση (k-means) των δεδομένων σε kclusters. Κάθε ερώτημα αναζητά πρώτα στα nprobe κοντινότερα clusters, μειώνοντας δραστικά το πλήθος των υποψηφίων.

IVFPQ (Inverted File + Product Quantization)
Επεκτείνει τον IVFFlat εφαρμόζοντας Product Quantization εντός κάθε inverted list. Τα διανύσματα χωρίζονται σε M υποδιανύσματα και κβαντίζονται με 2^nbits κεντροειδή ανά υποχώρο, επιτρέποντας περαιτέρω συμπίεση και ταχύτερους υπολογισμούς αποστάσεων.

Το πρόγραμμα λαμβάνει ένα σύνολο δεδομένων (π.χ. MNIST ή SIFT1M), δημιουργεί τη δομή ευρετηρίου για κάθε μέθοδο και εκτελεί:

K-Nearest Neighbors Search (KNN)

Range Search (με ακτίνα R)

Παράγει αρχεία εξόδου με μετρικές όπως Recall@N, Approximation Factor (AF), Queries per Second (QPS), και χρόνους προσεγγιστικής / ακριβούς αναζήτησης.



~Δομή Αρχείων~

Software-Development-Project1/
├── Makefile
├── README.md
├── include/                  # Header files (.h / .hpp)
│   ├── vector_utils.h        # Διανυσματικές πράξεις (dot product, L2 distance, randoms)
│   ├── lsh.h                 # Ορισμοί LSH
│   ├── hypercube.h           # Ορισμοί Hypercube hashing
│   ├── ivf_flat.hpp          # Ορισμοί IVFFlat (k-means + inverted lists)
│   ├── ivf_pq.hpp            # Ορισμοί IVFPQ (Product Quantization + IVF)
│   ├── dataset_io.hpp        # Συναρτήσεις φόρτωσης MNIST & SIFT
│   └── bruteForce.h          # Ακριβής KNN αναζήτηση για ground truth
│
├── src/                      # Υλοποιήσεις (.cpp)
│   ├── vector_utils.cpp
│   ├── lsh.cpp
│   ├── hypercube.cpp
│   ├── ivf_flat.cpp
│   ├── ivf_pq.cpp
│   ├── bruteForce.cpp
│   └── main.cpp              # main πρόγραμμα 
│
├── data/                     # Δεδομένα MNIST & SIFT
│   ├── train-images.idx3-ubyte
│   ├── t10k-images.idx3-ubyte
│   ├── sift_base.fvecs
│   └── sift_query.fvecs
│
├── docs/report/
  ├── plot_commands.gp   # Gnuplot script
  ├── plot_res.cpp       # παραγωγη plots
  ├── figs/              # Output images (plots)                    
│ ├── report.md          # Πειραματική μελέτη & ανάλυση
   
   --docs/report/results/
   ├── lsh_output.txt #formatted output
   ├── hypercube_output.txt #-||-
   ├── ivfflat_output.txt  #-||-
   ├── lsh.csv #csv απο όπου παίρνονται τα data για να φταιχτεί το εκάστοτε plot
   ├── hypercube.csv   #-||-
   ├── ivfflat.csv   #-||-
   └── tmp_*.txt
│    
│
└── tests/                    #Μικρά αρχεία δοκιμών
    ├── test_lsh.cpp
    ├── test_hypercube.cpp
    ├── test_ivf.cpp
    └── test_ivfpq.cpp


Μεταγλώττιση μέσω Makefile
make 


Δημιουργεί το εκτελέσιμο αρχείο:

./search

Εναλλακτικά (χειροκίνητα)
g++ -O3 -std=c++17 -Iinclude \
src/vector_utils.cpp src/lsh.cpp src/hypercube.cpp src/ivf_flat.cpp src/ivf_pq.cpp src/bruteForce.cpp src/main.cpp \
-o search

▶️ Οδηγίες Χρήσης
Εκτέλεση LSH
./search \
  -d data/train-images.idx3-ubyte \
  -q data/t10k-images.idx3-ubyte \
  -type mnist \
  -lsh -k 4 -L 5 -w 4.0 -N 1 -R 2000 -range false

Εκτέλεση Hypercube
./search \
  -d data/train-images.idx3-ubyte \
  -q data/t10k-images.idx3-ubyte \
  -type mnist \
  -hypercube -kproj 14 -M 10 -probes 2 -w 4.0 -N 1 -R 2000 -range false

Εκτέλεση IVFFlat
./search \
  -d data/train-images.idx3-ubyte \
  -q data/t10k-images.idx3-ubyte \
  -type mnist \
  -ivfflat -kclusters 50 -nprobe 5 -N 1 -R 2000 -range false

Εκτέλεση IVFPQ
./search \
  -d data/train-images.idx3-ubyte \
  -q data/t10k-images.idx3-ubyte \
  -type mnist \
  -ivfpq -M 16 -nbits 8 -N 1 -R 2000 -range false

Εκτέλεση σε SIFT dataset
./search \
  -d data/sift_base.fvecs \
  -q data/sift_query.fvecs \
  -type sift \
  -lsh -k 4 -L 5 -w 4.0 -N 1 -R 2 -range false

📤 Μορφή Εξόδου

Κάθε εκτέλεση παράγει ένα αρχείο αποτελεσμάτων (.txt) στο φάκελο docs/results/:

<METHOD>
Query: <query_id>
Nearest neighbor-1: <id_in_dataset>
distanceApproximate: <float>
distanceTrue: <float>
...
Average AF: <float>
Recall@N: <float>
QPS: <float>
tApproximateAverage: <float>
tTrueAverage: <float>

📈 Πειραματικές Μετρικές

Το πρόγραμμα υπολογίζει αυτόματα τις εξής μετρικές απόδοσης:

-Recall@N	Ποσοστό των πραγματικών Ν πλησιέστερων που βρέθηκαν προσεγγιστικά
-Approximation Factor (AF)	Λόγος της απόστασης προσεγγιστικού προς ακριβή γείτονα
-QPS (Queries per Second)	Αριθμός ερωτημάτων που απαντώνται ανά δευτερόλεπτο
-tApprox / tTrue	Μέσοι χρόνοι προσεγγιστικής και ακριβούς αναζήτησης

