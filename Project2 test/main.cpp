/*
    Author:Main
    Date: 2025-08-19
    Description:
      Console UI for StatsArray:
      - Pointer shown as uppercase zero-padded hex
      - Exceptions are printed as "Exception Error: ..."
      - Frequency table with percentage
*/

#include <iostream>
#include <fstream>
#include <random>
#include <string>
#include <limits>
#include <cstdlib>   
#include <cerrno>    
#include <cstddef>   
#include <iomanip>   
#include <sstream>   
#include <cstdint>
#include <tuple>
#include <exception>
#include "StatsArray.h"
#include "input.h"

using namespace std;

// Pre : none
// Post: app starts as Sample with an empty StatsArray
enum class DataSetType { SAMPLE, POPULATION };
struct App {
    DataSetType type = DataSetType::SAMPLE;
    StatsArray  arr;
};

/*
  Pre : console available
  Post: clears the terminal screen buffer (platform-dependent best effort).
*/
static void clearScreen() {
    cout.flush();
#ifdef _WIN32
    system("cls");
#else
    system("clear");
#endif
}

/*
  Pre : none
  Post: prints msg, then waits for a line; input buffer is cleared.
*/
static void pauseEnter(const string& msg = "Press any key to continue . . . ") {
    cout << '\n' << msg;
    cin.clear();
    cin.ignore(numeric_limits<streamsize>::max(), '\n');
}

/*
  Pre : any pointer value (may be null)
  Post: returns uppercase, zero-padded hex string of pointer width (no 0x).
*/
static string formatPtr(const void* p) {
    ostringstream oss;
    oss << uppercase << hex << setfill('0')
        << setw(sizeof(void*) * 2)
        << static_cast<uintptr_t>(reinterpret_cast<uintptr_t>(p));
    return oss.str();
}

static void printValuesInline(const StatsArray& a) {
    if (a.size() == 0) {
        cout << "\n";     // no data: just keep it blank like your screenshot
        return;
    }
    for (size_t i = 0; i < a.size(); ++i) {
        if (i) cout << "  ";          // double-space between numbers (matches look)
        cout << a.at(i);              // default stream formatting -> 13 51 98
    }
    cout << '\n';
}

/*
  Pre : app is valid
  Post: renders the main menu and prints pointer + dataset type.
*/
static void drawMain(const App& app) {

    srand(static_cast<unsigned>(time(0)));

    cout << "Descriptive Statistics Calculator Main Menu\n";
    cout << "Address of Dynamic array: " << formatPtr(app.arr.dataAddress()) << "\n";
    cout << "Dataset: (" << (app.type == DataSetType::SAMPLE ? "Sample" : "Population") << ")\n\n";
    printValuesInline(app.arr);
    cout << "\n";
    cout << "____________________________________________________________________\n\n";
    cout << "0. Exit\n";
    cout << "1. Configure Dataset to Sample or Polulation\n";
    cout << "2. Insert sort value(s) to the Dataset\n";
    cout << "3. Delete value(s) from the Dataset\n";
    cout << "--------------------------------------------------------------------\n";
    cout << "A. Find Minimum                N. Find Outliers\n";
    cout << "B. Find Maximum                O. Find Sum of Squares\n";
    cout << "C. Find Range                  P. Find Mean Absolute Deviation\n";
    cout << "D. Find Size                   Q. Find Root Mean Square\n";
    cout << "E. Find Sum                    R. Find Standard Error of Mean\n";
    cout << "F. Find Mean                   S. Find Skewness\n";
    cout << "G. Find Median                 T. Find Kurtosis\n";
    cout << "H. Find Mode(s)                U. Find Kurtosis Excess\n";
    cout << "I. Find Standard Deviation     V. Find Coefficient of Variation\n";
    cout << "J. Find Variance               W. Find Relative Standard Deviation\n";
    cout << "K. Find Midrange               X. Display Frequency Table\n";
    cout << "L. Find Quartiles              Y. Display ALL statical results\n";
    cout << "M. Find Interquartile Range    Z. Output ALL statical results to text file\n";
    cout << "____________________________________________________________________\n\n";

}

/*
  Pre : app is valid
  Post: allows user to insert one value, many randoms, or file numbers; returns to caller.
*/
static void screenInsertMenu(App& app) {

    while (true) {
        clearScreen();
        cout <<
            R"(Insert (sort) Dataset Menu
____________________________________________________________________

    A. insert a value
    B. insert a specified number of random values
    C. read data from file and insert values
____________________________________________________________________

    R. return
____________________________________________________________________
)";
        char opt = inputChar("Option: ", string("ABCR"));
        if (opt == 'R') return;

        if (opt == 'A') {
            clearScreen();
            cout << "Insert (single) value\n\n";
            double v = inputDouble("Enter a number: ");
            app.arr.insert(v);
            cout << "\nCONFIRMATION: Inserted " << v << " into the Dataset.\n";
            pauseEnter();
        }
        else if (opt == 'B') {
            clearScreen();
            cout << "Insert (random) values\n\n";
            int count = inputInteger("How many random values? ", true);

            for (int i = 0; i < count; i++) {
                int r = rand() % 101;                 
                app.arr.insert(static_cast<double>(r));
            }

            cout << "\nCONFIRMATION: Inserted " << count << " random values.\n";
            pauseEnter();
        }
        else if (opt == 'C') {
            clearScreen();
            cout << "Read data from file and insert values\n\n";
            string path = inputString("Enter file path (whitespace-separated numbers): ", true);
            ifstream fin(path);
            if (!fin) { cout << "\nERROR: Could not open file: " << path << '\n'; pauseEnter(); continue; }
            size_t inserted = 0; string token;
            while (fin >> token) {
                char* endp = nullptr; const char* cstr = token.c_str(); errno = 0;
                double v = strtod(cstr, &endp);
                if (endp != cstr && errno == 0) { app.arr.insert(v); ++inserted; }
            }
            cout << "\nCONFIRMATION: Inserted " << inserted << " value(s) from file.\n";
            pauseEnter();
        }
    }
}

/*
  Pre : callable fn representing a stats action that may throw
  Post: runs fn; on exception prints "Exception Error: ..." and always pauses.
*/
template <typename Fn>
static void runStat(Fn fn) {
    try {
        fn();
    }
    catch (const DatasetEmptyException& ex) {
        cout << "Exception Error: " << ex.what() << "\n";
    }
    catch (const InsufficientDataException& ex) {
        cout << "Exception Error: " << ex.what() << "\n";
    }
    catch (const exception& ex) {
        cout << "Exception Error: " << ex.what() << "\n";
    }
    catch (...) {
        cout << "Exception Error: Unknown error.\n";
    }
    pauseEnter();
}

/*
  Pre : console available; input.h present
  Post: full interactive loop until user chooses Exit (0).
*/
int main() {
    ios::sync_with_stdio(false);
    cin.tie(nullptr);
    
    App app{};

    while (true) {
        clearScreen();
        drawMain(app);

        string allowed = "0123ABCDEFGHIJKLMNOPQRSTUVWXYZ";
        char choice = inputChar("Option: ", allowed);
        if (choice == '0') break;

        bool sample = (app.type == DataSetType::SAMPLE);

        switch (choice) {
        case '1': {
            clearScreen();
            cout << "Configure Dataset Type\n\n";
            char t = inputChar("Enter type (S=Sample, P=Population): ", string("SP"));
            app.type = (t == 'P') ? DataSetType::POPULATION : DataSetType::SAMPLE;
            cout << "\nDataset set to " << (app.type == DataSetType::SAMPLE ? "Sample" : "Population") << ".\n";
            pauseEnter();
            break;
        }
        case '2': screenInsertMenu(app); break;
        case '3': {
            clearScreen();
            cout << "Delete value(s)\n\n";
            double v = inputDouble("Enter a value to delete (all occurrences): ");
            size_t removed = app.arr.eraseValue(v, SIZE_MAX);
            cout << "\nRemoved " << removed << " occurrence(s).\n";
            pauseEnter();
            break;
        }

                // --- A..Z stats with exception wrapper ---
        case 'A': { clearScreen(); runStat([&] { cout << "Minimum = " << app.arr.min() << '\n'; }); break; }
        case 'B': { clearScreen(); runStat([&] { cout << "Maximum = " << app.arr.max() << '\n'; }); break; }
        case 'C': { clearScreen(); runStat([&] { cout << "Range = " << app.arr.range() << '\n'; }); break; }
        case 'D': { clearScreen(); cout << "Size = " << app.arr.size() << '\n'; pauseEnter(); break; }
        case 'E': { clearScreen(); runStat([&] { cout << "Sum = " << app.arr.sum() << '\n'; }); break; }
        case 'F': { clearScreen(); runStat([&] { cout << "Mean = " << app.arr.mean() << '\n'; }); break; }
        case 'G': { clearScreen(); runStat([&] { cout << "Median = " << app.arr.median() << '\n'; }); break; }
        case 'H': { clearScreen(); runStat([&] { auto md = app.arr.modes(); cout << "Mode(s): "; if (md.empty()) cout << "(none)\n"; else { for (size_t i = 0;i < md.size();++i) { if (i) cout << ' '; cout << md[i]; } cout << '\n'; } }); break; }
        case 'I': { clearScreen(); runStat([&] { cout << "Standard Deviation (" << (sample ? "sample" : "population") << ") = " << app.arr.stdev(sample) << '\n'; }); break; }
        case 'J': { clearScreen(); runStat([&] { cout << "Variance (" << (sample ? "sample" : "population") << ") = " << app.arr.variance(sample) << '\n'; }); break; }
        case 'K': { clearScreen(); runStat([&] { cout << "Midrange = " << app.arr.midrange() << '\n'; }); break; }
        case 'L': { clearScreen(); runStat([&] { double q1, q2, q3; tie(q1, q2, q3) = app.arr.quartiles(); cout << "Quartiles:\nQ1 = " << q1 << "\nQ2 (Median) = " << q2 << "\nQ3 = " << q3 << '\n'; }); break; }
        case 'M': { clearScreen(); runStat([&] { cout << "Interquartile Range (IQR) = " << app.arr.iqr() << '\n'; }); break; }
        case 'N': { clearScreen(); runStat([&] { auto v = app.arr.outliers(); cout << "Outliers (Tukey +/- 1.5*IQR): "; if (v.empty()) cout << "(none)\n"; else { for (size_t i = 0;i < v.size();++i) { if (i) cout << ' '; cout << v[i]; } cout << '\n'; } }); break; }
        case 'O': { clearScreen(); runStat([&] { cout << "Sum of Squares = " << app.arr.sumSquares() << '\n'; }); break; }
        case 'P': { clearScreen(); runStat([&] { cout << "Mean Absolute Deviation = " << app.arr.meanAbsDeviation() << '\n'; }); break; }
        case 'Q': { clearScreen(); runStat([&] { cout << "Root Mean Square (RMS) = " << app.arr.rms() << '\n'; }); break; }
        case 'R': { clearScreen(); runStat([&] { cout << "Standard Error of Mean (SEM) = " << app.arr.sem(sample) << '\n'; }); break; }
        case 'S': { clearScreen(); runStat([&] { cout << "Skewness = " << app.arr.skewness(sample) << '\n'; }); break; }
        case 'T': { clearScreen(); runStat([&] { cout << "Kurtosis (Pearson) = " << app.arr.kurtosis() << '\n'; }); break; }
        case 'U': { clearScreen(); runStat([&] { cout << "Kurtosis Excess = " << app.arr.kurtosisExcess() << '\n'; }); break; }
        case 'V': { clearScreen(); runStat([&] { cout << "Coefficient of Variation = " << app.arr.coefficientOfVariation(sample) << '\n'; }); break; }
        case 'W': { clearScreen(); runStat([&] { cout << "Relative Standard Deviation (%) = " << app.arr.relativeStdDeviation(sample) << '\n'; }); break; }
        case 'X': {
            clearScreen();
            runStat([&] {
                cout << "Frequency Table\n\n";
                cout << left << setw(10) << "Value" << setw(12) << "Frequency" << setw(12) << "Frequency %\n";
                auto ft = app.arr.frequencyTable(); size_t total = app.arr.size();
                for (auto& p : ft) {
                    double perc = 100.0 * (double)p.second / (double)total;
                    cout << left << setw(10) << p.first
                        << setw(12) << p.second
                        << setw(12) << fixed << setprecision(2) << perc << "\n";
                }
                });
            break;
        }
        case 'Y': { clearScreen(); runStat([&] { app.arr.printAll(cout, sample); cout << '\n'; }); break; }
        case 'Z': {
            clearScreen();
            string path = inputString("Enter output file path (e.g., results.txt): ", true);
            runStat([&] { bool ok = app.arr.writeAllToFile(path, sample);
            cout << (ok ? "\nSaved results to: " : "\nERROR writing file: ") << path << '\n'; });
            break;
        }

        default: { clearScreen(); cout << "Feature not implemented yet.\n"; pauseEnter(); break; }
        }
    }

    clearScreen();
    cout << "Goodbye!\n";
    return 0;
}
