#ifndef UTILITY_H
#define UTILITY_H
#define _USE_MATH_DEFINES
#include <stdio.h>
#include <stdarg.h>
#include <cmath>
#include <string>
#include <vector>
#include <sstream>
#include <QString>
#include <time.h>

using namespace std;

inline QString qs(const string& s) {
    return QString::fromStdString(s);
}
inline double dB(double power) {
    if(power == 0.) return -300;
    return log10(power)*10;
}

inline double swr(double power) {
    float d = powf(10,(abs(dB(power)))/20);
    return (double)fmin(11.0,(d+1)/(d-1));
}

// freq is in Hz, Z is in ohms.
// if return value is positive, it is in henries;
// if return value is negative, it is in farads.
inline double capacitance_inductance(double freq, double Z) {
    if(Z>0) return Z/(2*M_PI*freq);
    return 1./(2*Z*M_PI*freq);
}
// freq is in Hz, Y is in mhos
// if return value is positive, it is in henries;
// if return value is negative, it is in farads.
inline double capacitance_inductance_Y(double freq, double Y) {
    if(Y<0) return -1./(2*Y*M_PI*freq);
    return -Y/(2*M_PI*freq);
}
inline double si_scale(double val) {
    double val2 = fabs(val);
    if(val2>1e12) return val*1e-12;
    if(val2>1e9) return val*1e-9;
    if(val2>1e6) return val*1e-6;
    if(val2>1e3) return val*1e-3;
    if(val2>1e0) return val;
    if(val2>1e-3) return val*1e3;
    if(val2>1e-6) return val*1e6;
    if(val2>1e-9) return val*1e9;
    if(val2>1e-12) return val*1e12;
    return val*1e15;
}
inline const char* si_unit(double val) {
    val = fabs(val);
    if(val>1e12) return "T";
    if(val>1e9) return "G";
    if(val>1e6) return "M";
    if(val>1e3) return "k";
    if(val>1e0) return "";
    if(val>1e-3) return "m";
    if(val>1e-6) return "u";
    if(val>1e-9) return "n";
    if(val>1e-12) return "p";
    return "f";
}
inline string ssprintf(int maxLen, const char* fmt, ...) {
    string tmp(maxLen, '\0');
    va_list args;
    va_start(args, fmt);
    int len = vsnprintf((char*)tmp.data(), maxLen, fmt, args);
    va_end(args);
    tmp.resize(len);
    return tmp;
}
inline string sstrftime(const char *format, const struct tm &tm) {
    char buf[256] = {0};
    strftime(buf, 256, format, &tm);
    return buf;
}


// trim from start (in place)
static inline void ltrim(std::string &s) {
    s.erase(s.begin(), std::find_if(s.begin(), s.end(), [](int ch) {
        return !isspace(ch);
    }));
}

// trim from end (in place)
static inline void rtrim(std::string &s) {
    s.erase(std::find_if(s.rbegin(), s.rend(), [](int ch) {
        return !isspace(ch);
    }).base(), s.end());
}

// trim from both ends (in place)
static inline void trim(std::string &s) {
    ltrim(s);
    rtrim(s);
}

static inline bool startsWith(string s, string sub) {
    if(sub.length() > s.length()) return false;
    return s.substr(0, sub.length()) == sub;
}

template<typename Out>
inline void split(const std::string &s, char delim, Out result) {
    std::stringstream ss(s);
    std::string item;
    while (std::getline(ss, item, delim)) {
        *(result++) = item;
    }
}

inline std::vector<std::string> split(const std::string &s, char delim) {
    std::vector<std::string> elems;
    split(s, delim, std::back_inserter(elems));
    return elems;
}

#endif // UTILITY_H
