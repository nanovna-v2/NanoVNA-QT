#define _USE_MATH_DEFINES
#include "touchstone.H"
#include "utility.H"
#include <stdarg.h>
#include <stdio.h>
#include <math.h>
#include <ctype.h>
#include <sstream>

using namespace std;

#if __cplusplus < 201103L
  #error This library needs at least a C++11 compliant compiler
#endif

// append to dst
int saprintf(string& dst, const char* fmt, ...) {
    int bytesToAllocate=32;
    int originalLen=dst.length();
    while(true) {
        dst.resize(originalLen+bytesToAllocate);
        va_list args;
        va_start(args, fmt);
        // ONLY WORKS WITH C++11!!!!!!!!
        // .data() does not guarantee enough space for the null byte before c++11
        int len = vsnprintf((char*)dst.data()+originalLen, bytesToAllocate+1, fmt, args);
        va_end(args);
        if(len>=0 && len <= bytesToAllocate) {
            dst.resize(originalLen+len);
            return len;
        }
        if(len<=0) bytesToAllocate*=2;
        else bytesToAllocate = len;
    }
}



string serializeTouchstone(vector<complex<double> > data, double startFreqHz, double stepFreqHz) {
    string res;
    res += "# MHz S MA R 50\n";
    saprintf(res,"!   freq        S11       \n");
    for(int i=0;i<(int)data.size();i++) {
        double freqHz = startFreqHz + i*stepFreqHz;
        double freqMHz = freqHz*1e-6;
        complex<double> val = data[i];
        double c = 180./M_PI;
        saprintf(res,"%8.3f %8.5f %7.2f\n",
                 freqMHz, abs(val), arg(val)*c);
    }
    return res;
}

string serializeTouchstone(vector<Matrix2cd> data, double startFreqHz, double stepFreqHz) {
    string res;
    res += "# MHz S MA R 50\n";
    saprintf(res,"!   freq        S11              S21              S12              S22       \n");
    for(int i=0;i<(int)data.size();i++) {
        double freqHz = startFreqHz + i*stepFreqHz;
        double freqMHz = freqHz*1e-6;
        Matrix2cd val = data[i];
        double c = 180./M_PI;
        saprintf(res,"%8.3f %8.5f %7.2f %8.5f %7.2f %8.5f %7.2f %8.5f %7.2f\n",
                 freqMHz,
                 abs(val(0,0)), arg(val(0,0))*c,
                 abs(val(1,0)), arg(val(1,0))*c,
                 abs(val(0,1)), arg(val(0,1))*c,
                 abs(val(1,1)), arg(val(1,1))*c);
    }
    return res;
}


void parseTouchstone(string data, int& nPorts, map<double, MatrixXcd> &results) {
    istringstream iss(data);
    string line;

    double freqMultiplier = 1e9; //GHz by default
    char format = 'd';      // d: db-angle; m: mag-angle; r: real-imag
    results.clear();
    while (getline(iss, line)) {
        // if part (or all) of the line is commented out, remove the comment
        int i;
        if((i=line.find('!')) != -1) {
            line.resize(i);
        }
        trim(line);
        if(line.size() == 0) continue;
        // option line
        if(startsWith(line, "#")) {
            string tmp = line.substr(1);
            trim(tmp);

            // to lower case
            std::transform(tmp.begin(), tmp.end(), tmp.begin(), ::tolower);

            vector<string> args = split(tmp, ' ');
            int state=0;
            for(int i=0; i<(int)args.size(); i++) {
                if(args[i].size() == 0) continue;

                // expecting impedance value
                if(state == 1) {
                    if(args[i] != "50") throw logic_error("only 50 ohm impedance S parameter files supported: " + line);
                    state = 0;
                    continue;
                }
                if(args[i] == "ghz") freqMultiplier = 1e9;
                else if(args[i] == "mhz") freqMultiplier = 1e6;
                else if(args[i] == "khz") freqMultiplier = 1e3;
                else if(args[i] == "hz") freqMultiplier = 1.;
                else if(args[i] == "s") ;
                else if(args[i] == "db") format = 'd';
                else if(args[i] == "ma") format = 'm';
                else if(args[i] == "ri") format = 'r';
                else if(args[i] == "r") state = 1;
                else {
                    throw logic_error("unknown keyword \"" + args[i] + "\" in option line: " + line);
                }
            }
            continue;
        }

        // data line
        i = line.find(' ');
        if(i < 0) throw logic_error("bad line in S parameter file: " + line);
        string valuesStr = line.substr(i + 1);

        // calculate frequency
        double freq = atof(line.substr(0,i).c_str()) * freqMultiplier;

        // set the function for parsing the S parameter values depending on format
        function<complex<double>(double a, double b)> processValue;
        switch(format) {
            case 'd': // db-angle
                processValue = [](double a, double b){
                    return polar(pow(10., a/20.), b*M_PI/180.);
                };
                break;
            case 'm': // mag-angle
                processValue = [](double a, double b){
                    return polar(a, b*M_PI/180.);
                };
                break;
            case 'r': // real-imag
                processValue = [](double a, double b){
                    return complex<double>(a,b);
                };
                break;
            default: assert(false);
        }

        // parse the S parameter values
        double values[8]; // s11a, s11b, s21a, s21b, s12a, s12b, s22a, s22b;
        int ret = sscanf(valuesStr.c_str(), "%lf %lf %lf %lf %lf %lf %lf %lf",
                         &values[0], &values[1], &values[2], &values[3],
                         &values[4], &values[5], &values[6], &values[7]);
        if(results.size() == 0) {
            if(ret == 2) nPorts = 1;
            else if(ret == 8) nPorts = 2;
            else throw logic_error("bad data line in S parameter file: " + line);
        }
        Matrix2cd tmp;
        if(nPorts == 1) {
            if(ret != 2) throw logic_error("bad data line in S parameter file: " + line);
            tmp << processValue(values[0], values[1]), 0., 0., 0.;
        } else if(nPorts == 2) {
            if(ret != 8) throw logic_error("bad data line in S parameter file: " + line);
            tmp << processValue(values[0], values[1]), processValue(values[4], values[5]),
                    processValue(values[2], values[3]), processValue(values[6], values[7]);
        } else assert(false);
        results[freq] = tmp;
    }
}
