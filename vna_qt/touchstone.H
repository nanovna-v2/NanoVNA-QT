#ifndef TOUCHSTONE_H
#define TOUCHSTONE_H
#include <string>
#include <vector>
#include <map>
#include <eigen3/Eigen/Core>
#include <xavna/common.H>
using namespace std;
using namespace Eigen;

class SParamSeries {
public:
    // map from frequency (in Hz) to value
    map<double, MatrixXcd> values;
    MatrixXcd interpolate(double freqHz) {
        assert(!values.empty());

        // find the item just right of freqHz
        auto it2 = values.lower_bound(freqHz);

        // if there is none, return the last value
        if(it2 == values.end()) return (*values.rbegin()).second;

        // if it is the first value, return the first value
        if(it2 == values.begin()) return (*it2).second;

        // otherwise interpolate
        auto it1 = it2;
        it2--;
        double freq1 = (*it1).first, freq2 = (*it2).first;
        MatrixXcd val1 = (*it1).second, val2 = (*it2).second;
        double scale = 1./(freq2-freq1);
        return val1*((freq2 - freqHz)*scale) + val2*((freqHz - freq1)*scale);
    }
};

string serializeTouchstone(vector<complex<double> > data, double startFreqHz, double stepFreqHz);
string serializeTouchstone(vector<Matrix2cd> data, double startFreqHz, double stepFreqHz);
void parseTouchstone(string data, int &nPorts, map<double, MatrixXcd>& results);

#endif // TOUCHSTONE_H
