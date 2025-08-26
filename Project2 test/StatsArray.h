#pragma once
/*
    Author: Minh Tran
    Date: 2025-08-19
    Program: StatsArray (Dynamic Array for Numbers) — C++14 header-only

    Description:
      - Stores double values in a dynamic array, kept in ASCENDING order.
      - Full set of descriptive statistics.
      - Rule of Three implemented.
      - Throws exceptions for invalid dataset sizes.
      - Uses std::tie (C++14) rather than structured bindings.
*/

#pragma once
#include <iostream>
#include <cassert>
#include <cmath>      // isfinite, sqrt, fabs
#include <cstring>    // memcpy, memmove
#include <cstddef>    // size_t
#include <new>        // nothrow
#include <vector>
#include <tuple>      // tuple, tie
#include <utility>
#include <fstream>
#include <stdexcept>
#include <string>
#include <iomanip>

using namespace std;

// ---------------- Exceptions ----------------

/*
  Pre : none
  Post: an exception type representing an empty dataset.
*/
class DatasetEmptyException : public runtime_error {
public:
    explicit DatasetEmptyException(const string& msg) : runtime_error(msg) {}
};

/*
  Pre : none
  Post: an exception type representing insufficient data size for an operation.
*/
class InsufficientDataException : public runtime_error {
public:
    explicit InsufficientDataException(const string& msg) : runtime_error(msg) {}
};

// ---------------- StatsArray ----------------
class StatsArray {
public:
    // =========================== Rule of Three ============================

    /*
      Pre : none
      Post: creates empty container; no allocation until first insert.
    */
    StatsArray() : _data(nullptr), _used(0), _cap(0) {}

    /*
      Pre : cap0 >= 0
      Post: creates empty container with capacity = max(8, cap0).
    */
    explicit StatsArray(size_t cap0)
        : _data(new (nothrow) double[cap0 > 0 ? cap0 : 8]),
        _used(0),
        _cap(cap0 > 0 ? cap0 : 8) {
        assert(_data != nullptr);
    }

    /*
      Pre : other is valid
      Post: *this is a deep copy of other.
    */
    StatsArray(const StatsArray& other)
        : _data(new (nothrow) double[other._cap]),
        _used(other._used),
        _cap(other._cap) {
        assert(_data != nullptr);
        if (_used) memcpy(_data, other._data, _used * sizeof(double));
    }

    /*
      Pre : both objects valid
      Post: *this becomes deep copy of other; old memory released.
    */
    StatsArray& operator=(const StatsArray& other) {
        if (this == &other) return *this;
        double* nd = new (nothrow) double[other._cap];
        assert(nd != nullptr);
        if (other._used) memcpy(nd, other._data, other._used * sizeof(double));
        delete[] _data;
        _data = nd; _used = other._used; _cap = other._cap;
        return *this;
    }

    /*
      Pre : object valid
      Post: dynamic memory released.
    */
    ~StatsArray() { delete[] _data; }

    // ============================== Modifiers =============================

    /*
      Pre : isfinite(x)
      Post: x inserted at sorted position; size() increases by 1.
    */
    void insert(double x) {
        assert(isfinite(x));
        size_t pos = lowerBound(x);
        insertAt(pos, x);
    }

    /*
      Pre : count >= 1
      Post: removes up to 'count' occurrences of v; returns number removed.
    */
    size_t eraseValue(double v, size_t count = 1) {
        assert(count >= 1);
        size_t pos = lowerBound(v), removed = 0;
        while (pos < _used && _data[pos] == v && removed < count) {
            for (size_t i = pos + 1; i < _used; ++i) _data[i - 1] = _data[i];
            --_used; ++removed;
        }
        return removed;
    }

    /*
      Pre : idx < size()
      Post: value at idx removed; order preserved; size() decreases by 1.
    */
    void eraseAt(size_t idx) {
        assert(idx < _used);
        for (size_t i = idx + 1; i < _used; ++i) _data[i - 1] = _data[i];
        --_used;
    }

    /*
      Pre : none
      Post: size() becomes 0; capacity unchanged.
    */
    void clear() { _used = 0; }

    // ============================== Accessors =============================

    /*
      Pre : none
      Post: returns number of elements.
    */
    size_t size() const { return _used; }

    /*
      Pre : none
      Post: returns current capacity.
    */
    size_t capacity() const { return _cap; }

    /*
      Pre : idx < size()
      Post: returns value at idx.
    */
    double at(size_t idx) const { assert(idx < _used); return _data[idx]; }

    /*
      Pre : none
      Post: returns underlying array address (for display).
    */
    const void* dataAddress() const { return static_cast<const void*>(_data); }

    // ============================== Statistics ============================

    /*
      Pre : size() >= 1
      Post: returns smallest value.
    */
    double min() const { requireSize(1, "Minimum"); return _data[0]; }

    /*
      Pre : size() >= 1
      Post: returns largest value.
    */
    double max() const { requireSize(1, "Maximum"); return _data[_used - 1]; }

    /*
      Pre : size() >= 1
      Post: returns max - min.
    */
    double range() const { requireSize(1, "Range");   return _data[_used - 1] - _data[0]; }

    /*
      Pre : size() >= 1
      Post: returns sum of all values.
    */
    double sum() const {
        requireSize(1, "Sum");
        long double s = 0.0L; for (size_t i = 0;i < _used;++i) s += _data[i];
        return (double)s;
    }

    /*
      Pre : size() >= 1
      Post: returns arithmetic mean.
    */
    double mean() const { requireSize(1, "Mean"); return sum() / (double)_used; }

    /*
      Pre : size() >= 1
      Post: returns median of sorted data.
    */
    double median() const {
        requireSize(1, "Median");
        size_t n = _used, m = n / 2; return (n % 2) ? _data[m] : (_data[m - 1] + _data[m]) / 2.0;
    }

    /*
      Pre : size() >= 1
      Post: returns list of modes; empty if no mode.
    */
    vector<double> modes() const {
        requireSize(1, "Mode(s)");
        vector<double> res; size_t best = 0, i = 0;
        while (i < _used) {
            size_t j = i + 1; while (j < _used && _data[j] == _data[i]) ++j;
            if (j - i > best) best = j - i; i = j;
        }
        if (best <= 1) return res;
        i = 0; while (i < _used) {
            size_t j = i + 1; while (j < _used && _data[j] == _data[i]) ++j;
            if (j - i == best) res.push_back(_data[i]); i = j;
        }
        return res;
    }

    /*
      Pre : sample ? size() >= 2 : size() >= 1
      Post: returns variance (sample uses n-1; population uses n).
    */
    double variance(bool sample) const {
        if (sample) requireSize(2, "Variance (sample)"); else requireSize(1, "Variance (population)");
        const long double mu = mean();
        long double ss = 0.0L; for (size_t i = 0;i < _used;++i) { long double d = _data[i] - mu; ss += d * d; }
        const long double denom = sample ? (long double)(_used - 1) : (long double)_used;
        long double ans = (denom > 0.0L ? ss / denom : 0.0L);
        assert(isfinite((double)ans)); assert(ans >= -1e-12L);
        if (!sample && _used == 1) assert(ans == 0.0L);
        return (double)ans;
    }

    /*
      Pre : sample ? size() >= 2 : size() >= 1
      Post: returns standard deviation.
    */
    double stdev(bool sample) const { return sqrt(variance(sample)); }

    /*
      Pre : size() >= 1
      Post: returns (min + max)/2.
    */
    double midrange() const { requireSize(1, "Midrange"); return (min() + max()) / 2.0; }

    /*
      Pre : size() >= 2
      Post: returns (Q1, Q2, Q3) using Tukey method; Q1 <= Q2 <= Q3.
    */
    tuple<double, double, double> quartiles() const {
        requireSize(2, "Quartiles");
        const double q2 = median();
        const size_t n = _used, m = n / 2;
        struct H {
            static double subMed(const double* a, size_t L, size_t R) {
                size_t len = R - L + 1, mid = L + len / 2; return (len % 2) ? a[mid] : (a[mid - 1] + a[mid]) / 2.0;
            }
        };
        double q1, q3;
        if (n % 2 == 0) { q1 = H::subMed(_data, 0, m - 1); q3 = H::subMed(_data, m, n - 1); }
        else { q1 = H::subMed(_data, 0, m - 1); q3 = H::subMed(_data, m + 1, n - 1); }
        assert(q1 <= q2 + 1e-12 && q2 <= q3 + 1e-12);
        assert(q1 >= _data[0] - 1e-12 && q3 <= _data[_used - 1] + 1e-12);
        return make_tuple(q1, q2, q3);
    }

    /*
      Pre : size() >= 2
      Post: returns Q3 - Q1.
    */
    double iqr() const { requireSize(2, "Interquartile Range"); double q1, q2, q3; tie(q1, q2, q3) = quartiles(); (void)q2; return q3 - q1; }

    /*
      Pre : size() >= 2
      Post: returns values < (Q1 - 1.5*IQR) or > (Q3 + 1.5*IQR).
    */
    vector<double> outliers() const {
        requireSize(2, "Outliers"); double q1, q2, q3; tie(q1, q2, q3) = quartiles(); (void)q2;
        double w = 1.5 * (q3 - q1), lo = q1 - w, hi = q3 + w; vector<double> res;
        for (size_t i = 0;i < _used;++i) if (_data[i]<lo || _data[i]>hi) res.push_back(_data[i]);
        return res;
    }

    /*
      Pre : size() >= 1
      Post: returns sum of squares.
    */
    double sumSquares() const {
        requireSize(1, "Sum of Squares");
        long double s = 0.0L; for (size_t i = 0;i < _used;++i) s += (long double)_data[i] * _data[i]; return (double)s;
    }

    /*
      Pre : size() >= 1
      Post: returns mean absolute deviation from mean.
    */
    double meanAbsDeviation() const {
        requireSize(1, "Mean Absolute Deviation");
        long double mu = mean(), s = 0.0L; for (size_t i = 0;i < _used;++i) s += fabsl(_data[i] - mu);
        return (double)(s / (long double)_used);
    }

    /*
      Pre : size() >= 1
      Post: returns root mean square.
    */
    double rms() const { requireSize(1, "Root Mean Square"); return sqrt(sumSquares() / (double)_used); }

    /*
      Pre : sample ? size() >= 2 : size() >= 1
      Post: returns standard error of mean.
    */
    double sem(bool sample) const {
        if (sample) requireSize(2, "Standard Error of Mean (sample)");
        else        requireSize(1, "Standard Error of Mean (population)");
        return stdev(sample) / sqrt((double)_used);
    }

    /*
      Pre : sample ? size() >= 3 : size() >= 1
      Post: returns skewness (bias-corrected for sample).
    */
    double skewness(bool sample) const {
        if (sample) requireSize(3, "Skewness (sample)"); else requireSize(1, "Skewness (population)");
        long double mu = mean(), m2 = 0.0L, m3 = 0.0L, n = (long double)_used;
        for (size_t i = 0;i < _used;++i) { long double d = _data[i] - mu; m2 += d * d; m3 += d * d * d; }
        if (sample) {
            long double s2 = m2 / (n - 1.0L), s = sqrt(s2), g1 = (m3 / n) / (s * s * s);
            return (double)(sqrt(n * (n - 1.0L)) / (n - 2.0L) * g1);
        }
        else {
            long double s2 = m2 / n, s = sqrt(s2); return (double)((m3 / n) / (s * s * s));
        }
    }

    /*
   Pre : size() >= 4
   Post: Returns Excel's bias-corrected "term1" value
         (some sites label this as β2).
 */
    double kurtosis() const {
        requireSize(4, "Kurtosis Excel term1");

        long double n = static_cast<long double>(_used);
        long double mu = mean();

        // sample variance (denominator n-1)
        long double s2 = 0.0L;
        for (size_t i = 0; i < _used; ++i) {
            long double d = _data[i] - mu;
            s2 += d * d;
        }
        long double s = sqrt(s2 / (n - 1.0L));
        if (s == 0.0L) return 0.0;

        // standardized deviations
        long double sumZ4 = 0.0L;
        for (size_t i = 0; i < _used; ++i) {
            long double z = (_data[i] - mu) / s;
            sumZ4 += z * z * z * z;
        }

        // Excel bias-corrected term1
        long double term1 = (n * (n + 1.0L)) /
            ((n - 1.0L) * (n - 2.0L) * (n - 3.0L)) * sumZ4;

        return (double)term1;   // ≈ 14.944851 for {0,9,34,92}
    }

    /*
  Pre : size() >= 4
  Post: Returns Excel-style Excess Kurtosis (α4).
*/
    double kurtosisExcess() const {
        requireSize(4, "Kurtosis Excess Excel");

        long double n = static_cast<long double>(_used);
        long double mu = mean();

        long double s2 = 0.0L;
        for (size_t i = 0; i < _used; ++i) {
            long double d = _data[i] - mu;
            s2 += d * d;
        }
        long double s = sqrt(s2 / (n - 1.0L));
        if (s == 0.0L) return 0.0;

        long double sumZ4 = 0.0L;
        for (size_t i = 0; i < _used; ++i) {
            long double z = (_data[i] - mu) / s;
            sumZ4 += z * z * z * z;
        }

        long double term1 = (n * (n + 1.0L)) /
            ((n - 1.0L) * (n - 2.0L) * (n - 3.0L)) * sumZ4;
        long double term2 = (3.0L * (n - 1.0L) * (n - 1.0L)) /
            ((n - 2.0L) * (n - 3.0L));

        return (double)(term1 - term2);   // ≈ 1.444851
    }


    /*
      Pre : size() >= 1 and mean() != 0
      Post: returns stdev/mean.
    */
    double coefficientOfVariation(bool sample) const {
        requireSize(1, "Coefficient of Variation");
        double mu = mean(); if (mu == 0.0) throw InsufficientDataException("Coefficient of Variation undefined when mean is 0.");
        return stdev(sample) / mu;
    }

    /*
      Pre : size() >= 1 and mean() != 0
      Post: returns 100 * stdev/mean.
    */
    double relativeStdDeviation(bool sample) const { return 100.0 * coefficientOfVariation(sample); }

    /*
      Pre : size() >= 1
      Post: returns (value,count) pairs in ascending order.
    */
    vector<pair<double, size_t>> frequencyTable() const {
        requireSize(1, "Frequency Table");
        vector<pair<double, size_t>> ft; size_t i = 0;
        while (i < _used) {
            size_t j = i + 1; while (j < _used && _data[j] == _data[i]) ++j;
            ft.push_back(make_pair(_data[i], j - i)); i = j;
        }
        return ft;
    }

    /*
      Pre : size() >= 1
      Post: writes a full, formatted report of statistics to os.
    */
    void printAll(ostream& os, bool sample) const {
        requireSize(1, "Print All");
        os << "DATA (sorted, n=" << _used << "): ";
        for (size_t i = 0;i < _used;++i) { if (i) os << ' '; os << _data[i]; }
        os << "\n\n";
        os << "Min: " << min() << "\n";
        os << "Max: " << max() << "\n";
        os << "Range: " << range() << "\n";
        os << "Sum: " << sum() << "\n";
        os << "Mean: " << mean() << "\n";
        os << "Median: " << median() << "\n";
        { auto md = modes(); os << "Mode(s): "; if (md.empty()) os << "(none)\n"; else { for (size_t i = 0;i < md.size();++i) { if (i) os << ' '; os << md[i]; } os << "\n"; } }
        os << "Variance (" << (sample ? "sample" : "population") << "): " << variance(sample) << "\n";
        os << "Std Dev (" << (sample ? "sample" : "population") << "): " << stdev(sample) << "\n";
        os << "Midrange: " << midrange() << "\n";
        {
            double q1, q2, q3; tie(q1, q2, q3) = quartiles(); os << "Quartiles (Q1,Q2,Q3): " << q1 << ", " << q2 << ", " << q3 << "\n";
            os << "IQR: " << (q3 - q1) << "\n";
        }
        { auto v = outliers(); os << "Outliers (Tukey +/- 1.5*IQR): "; if (v.empty()) os << "(none)\n"; else { for (size_t i = 0;i < v.size();++i) { if (i) os << ' '; os << v[i]; } os << "\n"; } }
        os << "Sum of Squares: " << sumSquares() << "\n";
        os << "Mean Abs Deviation: " << meanAbsDeviation() << "\n";
        os << "RMS: " << rms() << "\n";
        os << "SEM: " << sem(sample) << "\n";
        os << "Skewness: " << skewness(sample) << "\n";
        os << "Kurtosis (Pearson): " << kurtosis() << "\n";
        os << "Kurtosis Excess: " << kurtosisExcess() << "\n";
        os << "Coefficient of Variation: " << coefficientOfVariation(sample) << "\n";
        os << "Relative Std Dev (%): " << relativeStdDeviation(sample) << "\n";

        os << "\nFrequency Table\n\n";
        os << left << setw(10) << "Value" << setw(12) << "Frequency" << setw(12) << "Frequency %\n";
        auto ft = frequencyTable(); size_t total = _used;
        for (auto& p : ft) {
            double perc = 100.0 * (double)p.second / (double)total;
            os << left << setw(10) << p.first << setw(12) << p.second
                << setw(12) << fixed << setprecision(2) << perc << "\n";
        }
    }

    /*
      Pre : path not empty; size() >= 1
      Post: writes printAll() to file; returns true on success.
    */
    bool writeAllToFile(const string& path, bool sample) const {
        requireSize(1, "Write All to File");
        ofstream fout(path); if (!fout) return false; printAll(fout, sample); return true;
    }

private:
    double* _data;
    size_t  _used;
    size_t  _cap;

    /*
      Pre : need >= 1; what is a short label
      Post: throws DatasetEmptyException if size()==0;
            throws InsufficientDataException if size()<need.
    */
    void requireSize(size_t need, const char* what) const {
        if (_used == 0) throw DatasetEmptyException("Dataset is empty.");
        if (_used < need) throw InsufficientDataException(string(what) + " requires at least " + to_string(need) + " value(s).");
    }

    /*
      Pre : none
      Post: capacity >= 8 and > used when growth occurs; data preserved.
    */
    void growIfNeeded() {
        if (_used < _cap) return;
        size_t newCap = (_cap == 0 ? 8 : _cap * 2);
        double* nd = new (nothrow) double[newCap]; assert(nd != nullptr);
        if (_used) memcpy(nd, _data, _used * sizeof(double));
        delete[] _data; _data = nd; _cap = newCap;
    }

    /*
      Pre : pos <= used
      Post: places x at pos; shifts tail right; size() increases by 1.
    */
    void insertAt(size_t pos, double x) {
        assert(pos <= _used);
        growIfNeeded();
        size_t tail = _used - pos;
        if (tail) memmove(&_data[pos + 1], &_data[pos], tail * sizeof(double));
        _data[pos] = x; ++_used;
    }

    /*
      Pre : none
      Post: returns first index i where _data[i] >= x in [0..used].
    */
    size_t lowerBound(double x) const {
        size_t L = 0, R = _used;
        while (L < R) { size_t M = (L + R) / 2; if (_data[M] < x) L = M + 1; else R = M; }
        return L;
    }
};
