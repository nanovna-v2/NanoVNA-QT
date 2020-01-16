#define TOKEN_TO_STRING(TOK) # TOK
using namespace std;

typedef array<complex<double>,2> complex2;
static const int reflIndex=0, thruIndex=1;


// to be implemented by ui code
// pop up an info dialog with msg
void alert(string msg);
// update the ith point on the graph
void updateGraph(int i, complex2 values);
// update the ith point on the time domain graph
void updateTimeGraph(int i, complex2 values);
// called whenever a full frequency sweep is done
void sweepCompleted();


// exported functions
extern double Z0;

extern void* xavna_dev;
extern int nPoints;		// how many frequency points; do not modify; call resizeVectors()
extern int startFreq;	// start frequency in kHz
extern int freqStep;	// frequency step in kHz
extern bool use_cal;	// whether calibration is in effect; do not modify
extern bool refreshThreadShouldExit; // set to true to cause refreshThread to exit


static const int timeScale=3;
static const int nCal = 4;
static const double freqMultiplier=0.001;

// indexed by CAL_* constants
extern array<vector<complex2>, 4> calibrationReferences;

// you must call this function from a secondary thread (non UI thread)
// to perform continuous refresh
// make sure xavna_dev is initialized before calling
void* refreshThread(void* v);
// take a measurement over the complete frequency range
void takeMeasurement(function<void(vector<complex2>)> cb);
// using 4 measurements (short, open, load, thru) apply calibration to the graph
void applySOLT();
// disable calibration
void clearCalibration();
string saveCalibration();
bool loadCalibration(char* data, int size);
// change nPoints to n; do not call when refreshThread is running
void resizeVectors(int n);

vector<complex<double> > extract(vector<complex2> in, int i);


enum {
	CAL_SHORT=0,
	CAL_OPEN,
	CAL_LOAD,
	CAL_THRU
};



// convenience functions

// freq is in Hz, Z is in ohms
inline double capacitance_inductance(double freq, double Z) {
	if(Z>0) return Z/(2*M_PI*freq);
	return 1./(2*Z*M_PI*freq);
}
// freq is in Hz, Y is in mhos
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
    vsnprintf((char*)tmp.data(), maxLen, fmt, args);
    va_end(args);
    return tmp;
}


inline double dB(double power) {
	return log10(power)*10;
}
inline double gauss(double x, double m, double s) {
    static const double inv_sqrt_2pi = 0.3989422804014327;
    double a = (x - m) / s;
    return inv_sqrt_2pi / s * std::exp(-0.5d * a * a);
}

inline int timePoints() {
	//return (nPoints*3-1);
	return nPoints*timeScale;
}

// returns MHz
inline double freqAt(int i) {
	return (startFreq+i*freqStep)*freqMultiplier;
}
// returns ns
inline double timeAt(int i) {
	double fs=double(freqStep)*freqMultiplier; // MHz
	double totalTime = 1000./fs/2; // ns
	return double(i)*totalTime/double(timePoints())/2;
}

