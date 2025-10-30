#include <iostream>
#include <fstream>
#include <string>
#include <cstdlib>  // for system()

void plotWithGnuplot(const std::string& csv, const std::string& title, const std::string& output) {
    std::ofstream gp("docs/report/plot_commands.gp");
    gp << "set terminal pngcairo size 800,600\n";
    gp << "set output '" << output << "'\n";
    gp << "set datafile separator ','\n";
    gp << "set title '" << title << "'\n";
    gp << "set xlabel 'tApprox (ms)'\n";
    gp << "set ylabel 'Recall@N'\n";
    gp << "plot '" << csv << "' using 7:4 with linespoints title 'Recall vs Time'\n";
    gp.close();

    system("gnuplot docs/report/plot_commands.gp");
    std::cout << "✅ Plot saved to " << output << "\n";
}

int main() {
    plotWithGnuplot("docs/report/results/lsh.csv",
                    "LSH Recall vs Time",
                    "docs/report/figs/lsh_recall_vs_time.png");

    plotWithGnuplot("docs/report/results/hypercube.csv",
                    "Hypercube Recall vs Time",
                    "docs/report/figs/hypercube_recall_vs_time.png");

    plotWithGnuplot("docs/report/results/ivfflat.csv",
                    "IVF-Flat Recall vs Time",
                    "docs/report/figs/ivfflat_recall_vs_time.png");

    return 0;
}
