#include <functional>
#include "xavna.h"

using namespace std;
class xavna_generic {
public:
	virtual bool is_tr()=0;
	virtual bool is_autosweep()=0;
	virtual int set_params(int freq_khz, int atten, int port, int nWait)=0;
	virtual int set_autosweep(double sweepStartHz, double sweepStepHz, int sweepPoints, int nValues=1)=0;
	virtual int set_if_freq(int freq_khz)=0;
	virtual int read_values(double* out_values, int n_samples)=0;
	virtual int read_values_raw(double* out_values, int n_samples)=0;
	virtual int read_autosweep(autoSweepDataPoint* out_values, int n_values)=0;
	virtual ~xavna_generic() {}
};

typedef function<xavna_generic*(const char* dev)> xavna_constructor;


