#include <stdint.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <math.h>
#include <assert.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <complex>
#include <tuple>
#include <map>
#include <array>
#include <functional>

#include "include/xavna_generic.H"
#include "common_types.h"
#include "include/xavna.h"
#include "include/platform_abstraction.H"
using namespace std;


// named virtual devices
map<string, xavna_constructor> xavna_virtual_devices;
xavna_constructor xavna_default_constructor;


struct complex5 {
	complex<double> val[5];
};

static u64 sx(u64 data, int bits) {
	u64 mask=~u64(u64(1LL<<bits) - 1);
	return data|((data>>(bits-1))?mask:0);
}
static complex<double> processValue(u64 data1,u64 data2) {
	return {double((ll)data2), double((ll)data1)};
}

static int readAll(int fd, void* buf, int len) {
	u8* buf1=(u8*)buf;
	int off=0;
	int r;
	while(off<len) {
		if((r=read(fd,buf1+off,len-off))<=0) break;
		off+=r;
	}
	return off;
}

static int writeAll(int fd, const void* buf, int len) {
	const u8* buf1=(const u8*)buf;
	int off=0;
	int r;
	while(off<len) {
		if((r=write(fd,buf1+off,len-off))<=0) break;
		off+=r;
	}
	return off;
}

// returns [adc0r, adc0i, adc1r, adc1i, adc2r, adc2i, ...]
// unnormalized values;
// if normalizePhase is true, divide all values by polar(arg(adc0))
static tuple<array<int64_t,8>,int> readValue2(int ttyFD, int cnt, bool normalizePhase=true) {
	array<int64_t,8> res;
	for(int i=0;i<8;i++) res[i] = 0;
	
	// we accept either 6 or 8 values in each datapoint
	int N=8;
	int N2=6;
	
	int bufsize=1024;
	u8 buf[bufsize];
	int br;
	u64 values[N+1];
	int n=0;	// how many sample groups we've read so far
	int j=0;	// bit offset into current value
	int k=0;	// which value in the sample group we are currently expecting
	
	u8 msb=1<<7;
	u8 mask=msb-1;
	u8 checksum = 0, checksumPrev = 0;
	while((br=read(ttyFD,buf,sizeof(buf)))>0) {
		for(int i=0;i<br;i++) {
			if((buf[i]&msb)==0) {
				if((k == N || k == N2) && j == 7) {
					checksumPrev &= 0b1111111;
					if(checksumPrev != u8(values[k])) {
						printf("ERROR: checksum should be %d, is %d\n", (int)checksumPrev, (int)(u8)values[6]);
					}
					int64_t valuesOut[N+1];
					for(int g=0;g<k;g++)
						valuesOut[g] = (int64_t)sx(values[g], 35);
					if(normalizePhase) {
						complex<double> val0(valuesOut[0], valuesOut[1]);
						double phase = arg(val0);
						complex<double> scale = polar(1., -phase);
						for(int g=0;g<k;g+=2) {
							complex<double> val(valuesOut[g], valuesOut[g+1]);
							val *= scale;
							res[g] += (u64)round(val.real());
							res[g+1] += (u64)round(val.imag());
						}
					} else {
						for(int g=0;g<k;g++)
							res[g] += valuesOut[g];
					}
					if(++n >= cnt) {
						return make_tuple(res, n);
					}
				}
				for(int ii=0;ii<N+1;ii++)
					values[ii] = 0;
				j=0;
				k=0;
				checksum=0b01000110;
			}
			checksumPrev = checksum;
			checksum = (checksum xor ((checksum<<1) | 1)) xor buf[i];
			if(k < N+1)
				values[k] |= u64(buf[i]&mask) << j;
			j+=7;
			if(j>=35) {
				j=0;
				k++;
			}
		}
	}
	return make_tuple(res, n);
}
// returns [adc0, adc1, adc2, adc1/adc0, adc2/adc0]
static tuple<complex5,int> readValue3(int ttyFD, int cnt) {
	complex5 result=complex5{{0.,0.,0.,0.,0.}};
	tuple<array<int64_t,8>,int> tmp = readValue2(ttyFD, cnt);
	array<int64_t,8> vals = get<0>(tmp);
	result.val[0] = processValue(vals[0], vals[1]);
	result.val[1] = processValue(vals[2], vals[3]);
	result.val[2] = processValue(vals[4], vals[5]);
	result.val[3] = result.val[1]/result.val[0];
	result.val[4] = result.val[2]/result.val[0];
	
	// TODO(xaxaxa): get rid of the explicit typing here once mxe switches
	// to newer gcc without this bug
	return tuple<complex5,int> {result, get<1>(tmp)};
}

static tuple<array<complex<double>,4>,int> readValue4(int ttyFD, int cnt) {
	array<complex<double>, 4> result = {0.,0.,0.,0.};
	tuple<array<int64_t,8>,int> tmp = readValue2(ttyFD, cnt);
	array<int64_t,8> vals = get<0>(tmp);
	result[0] = processValue(vals[0], vals[1]);
	result[1] = processValue(vals[2], vals[3]);
	result[2] = processValue(vals[4], vals[5]);
	result[3] = processValue(vals[6], vals[7]);
	
	// TODO(xaxaxa): get rid of the explicit typing here once mxe switches
	// to newer gcc without this bug
	return tuple<array<complex<double>,4>,int> 
			{result, get<1>(tmp)};
}
/*
static tuple<complex5,int> readValue3(int ttyFD, int cnt) {
	complex5 result=complex5{{0.,0.,0.,0.,0.}};
	int bufsize=1024;
	u8 buf[bufsize];
	int br;
	u64 values[7];
	int n=0;	// how many sample groups we've read so far
	int j=0;	// bit offset into current value
	int k=0;	// which value in the sample group we are currently expecting
	
	u8 msb=1<<7;
	u8 mask=msb-1;
	u8 checksum = 0;
	while((br=read(ttyFD,buf,sizeof(buf)))>0) {
		for(int i=0;i<br;i++) {
			if((buf[i]&msb)==0) {
				if(k == 6 && j == 7) {
					checksum &= 0b1111111;
					if(checksum != u8(values[6])) {
						printf("ERROR: checksum should be %d, is %d\n", (int)checksum, (int)(u8)values[6]);
					}
					for(int g=0;g<3;g++)
						result.val[g] += processValue(values[g*2],values[g*2+1]);
					//result.val[3] += result.val[1]/result.val[0];
					//result.val[4] += result.val[2]/result.val[0];
					if(++n >= cnt) {
						for(int g=0;g<5;g++)
							result.val[g] /= cnt;
						result.val[3] = result.val[1]/result.val[0];
						result.val[4] = result.val[2]/result.val[0];
						return make_tuple(result, n);
					}
				}
				values[0]=values[1]=values[2]=0;
				values[3]=values[4]=values[5]=0;
				values[6]=0;
				j=0;
				k=0;
				checksum=0b01000110;
			}
			if(k<6) checksum = (checksum xor ((checksum<<1) | 1)) xor buf[i];
			if(k < 7)
				values[k] |= u64(buf[i]&mask) << j;
			j+=7;
			if(j>=35) {
				j=0;
				k++;
			}
		}
	}
	return make_tuple(result, n);
}*/


class xavna_default: public xavna_generic {
public:
	int ttyFD;
	bool _first = true;
	bool tr = false;
	bool mirror = false;
	bool autoSweep = false;
	int _curPort = 0;
	int _nWait = 20;
	xavna_default(const char* dev) {
		fprintf(stderr, "Opening serial...\n");
		ttyFD=xavna_open_serial(dev);
		if(ttyFD < 0) {
			throw runtime_error(strerror(errno));
		}

		fprintf(stderr, "Detecting device type...\n");
		fflush(stderr);

		// check for autosweep device
		xavna_drainfd(ttyFD);
		usleep(10000);
		xavna_drainfd(ttyFD);
		usleep(10000);

		autoSweep = xavna_detect_autosweep(ttyFD);
		if(autoSweep) {
			tr = true;
			fprintf(stderr, "detected autosweep T/R vna\n");
			fflush(stderr);
			return;
		}

		set_params(100000, 31, 0, 5);
		
		xavna_drainfd(ttyFD);
		readValue2(ttyFD, 5);
		
		array<int64_t, 8> result;
		int n;
		tie(result, n) = readValue2(ttyFD, 5);
		if(result[6] != 0 && result[7] != 0) {
			tr = false;
			fprintf(stderr, "detected full two port vna\n");
		} else {
			tr = true;
			fprintf(stderr, "detected T/R vna\n");
			fprintf(stderr, "%lld %lld\n", result[0], result[1]);
		}
		
		if(!tr)
			set_if_freq(700);
	}
	virtual bool is_tr() {
		return tr;
	}
	virtual bool is_autosweep() {
		return autoSweep;
	}
	virtual int set_params(int freq_khz, int atten, int port, int nWait) {
		_nWait = nWait;
		int attenuation=atten;
		double freq = double(freq_khz)/1000.;
		int N = (int)round(freq*100);
		
		int txpower = 0b11;
		int minAtten = 5;
		
		if(attenuation >= 9+minAtten) {
			txpower = 0b00;
			attenuation -= 9;
		} else if(attenuation >= 6+minAtten) {
			txpower = 0b01;
			attenuation -= 6;
		} else if(attenuation >= 3+minAtten) {
			txpower = 0b10;
			attenuation -= 3;
		}
		
		if(attenuation > 31) attenuation = 31;
		
		if(mirror) _curPort = (port==1?0:1);
		else _curPort = port;
		u8 buf[] = {
			0, 0,
			1, u8(N>>16),
			2, u8(N>>8),
			3, u8(N),
			5, u8(attenuation*2),
			6, u8(0b00001000 | txpower),
		    7, u8((_first?0:1) | ((_curPort==1?0:1) << 2)),
			0, 0,
			4, 1
		};
		if(write(ttyFD,buf,sizeof(buf))!=(int)sizeof(buf)) return -1;
		
		xavna_drainfd(ttyFD);
		readValue2(ttyFD, nWait);

		//if(_first) usleep(1000*10);
		//_first = false;
		
		
		return 0;
	}
	virtual int set_autosweep(double sweepStartHz, double sweepStepHz, int sweepPoints, int nValues) {
		u8 buf[] = {
			// 0
			0,0,0,0,0,0,0,0,
			// 8
			0x23, 0x00, 0,0,0,0,0,0,0,0,
			// 18
			0x23, 0x10, 0,0,0,0,0,0,0,0,
			// 28
			0x21, 0x20, 0,0,
			// 32
			0x21, 0x22, 0,0,
		};
		*(uint64_t*)(buf + 8 + 2) = (uint64_t)sweepStartHz;
		*(uint64_t*)(buf + 18 + 2) = (uint64_t)sweepStepHz;
		*(uint16_t*)(buf + 28 + 2) = (uint16_t)sweepPoints;
		*(uint16_t*)(buf + 32 + 2) = (uint16_t)nValues;
		if(writeAll(ttyFD,buf,sizeof(buf)) != (int)sizeof(buf)) return -1;
		return 0;
	}
	virtual int set_if_freq(int freq_khz) {
		if(is_tr()) {
			errno = ENOTSUP;
			return -1;
		}
		freq_khz = freq_khz/10;
		freq_khz = freq_khz*10;
		
		if(freq_khz > 2550) {
			errno = EDOM;
			return -1;
		}
		
		int bb_samp_rate_khz = 19200*4;
		double bb_rate = double(freq_khz)/double(bb_samp_rate_khz);
		uint32_t rate = uint32_t(bb_rate*double(1LL<<32));
		u8 buf[] = {
			0, 0,
		    8, u8(freq_khz/10),
		    9, u8(rate>>24),
		    10, u8(rate>>16),
		    11, u8(rate>>8),
		    12, u8(rate>>0),
			0, 0,
			4, 1
		};
		if(write(ttyFD,buf,sizeof(buf))!=(int)sizeof(buf)) return -1;
		return 0;
	}

	virtual int read_values(double* out_values, int n_samples) {
		complex5 result;
		int n;
		tie(result, n) = readValue3(ttyFD, n_samples);
		out_values[0] = result.val[3].real();
		out_values[1] = result.val[3].imag();
		out_values[2] = result.val[4].real();
		out_values[3] = result.val[4].imag();
		return n;
	}
	static inline void swap(double& a, double& b) {
		double tmp = b;
		b = a;
		a = tmp;
	}
	virtual int read_values_raw(double* out_values, int n_samples) {
		int n;
		if(tr) {
			complex5 result;
			tie(result, n) = readValue3(ttyFD, n_samples);
			out_values[0] = result.val[0].real();
			out_values[1] = result.val[0].imag();
			out_values[2] = result.val[1].real();
			out_values[3] = result.val[1].imag();
			out_values[4] = 0.;
			out_values[5] = 0.;
			out_values[6] = result.val[2].real();
			out_values[7] = result.val[2].imag();
		} else {
			array<complex<double>,4> result;
			tie(result, n) = readValue4(ttyFD, n_samples);
			out_values[0] = result[1].real();
			out_values[1] = result[1].imag();
			out_values[2] = result[0].real();
			out_values[3] = result[0].imag();
			out_values[4] = result[2].real();
			out_values[5] = result[2].imag();
			out_values[6] = result[3].real();
			out_values[7] = result[3].imag();
			/*u8 reg6 = (_first?0:1) | ((_curPort==1?0:1) << 2);
			// measure outgoing wave
			tie(result, n) = readValue3(ttyFD, n_samples);
			out_values[0] = result.val[1].real();
			out_values[1] = result.val[1].imag();
			out_values[6] = result.val[2].real();
			out_values[7] = result.val[2].imag();
			
			{
				u8 buf[] = {
					0, 0,
					7, u8(reg6 | (1<<1)),
					0, 0,
				};
				
				// measure incoming wave
				if(write(ttyFD,buf,sizeof(buf))!=(int)sizeof(buf)) return -1;
				readValue3(ttyFD, _nWait);
			}
			
			tie(result, n) = readValue3(ttyFD, n_samples);
			out_values[2] = result.val[1].real();
			out_values[3] = result.val[1].imag();
			out_values[4] = result.val[2].real();
			out_values[5] = result.val[2].imag();*/
			
			if(mirror) {
				swap(out_values[0], out_values[4]);
				swap(out_values[1], out_values[5]);
				swap(out_values[2], out_values[6]);
				swap(out_values[3], out_values[7]);
			}
		}

		double scale = 1./double((int64_t(1)<<12) * (int64_t(1)<<20));

		for(int i=0;i<8;i++)
			out_values[i] *= scale;
		return n;
	}
	
	virtual int read_values_raw2(double* out_values, int n_samples) {
		complex5 result;
		int n;
		tie(result, n) = readValue3(ttyFD, n_samples);
		double scale = 1./double((int64_t(1)<<12) * (int64_t(1)<<19));
		result.val[0] *= scale;
		result.val[1] *= scale;
		result.val[2] *= scale;
		
		//FIXME: make hardware phase consistent (enable PLL phase resync)
		// and remove this code
		complex<double> refPhase = polar(1., -arg(result.val[0]));
		result.val[0] *= refPhase;
		result.val[1] *= refPhase;
		result.val[2] *= refPhase;
		
		out_values[0] = result.val[0].real();
		out_values[1] = result.val[0].imag();
		out_values[2] = result.val[1].real();
		out_values[3] = result.val[1].imag();
		out_values[4] = result.val[2].real();
		out_values[5] = result.val[2].imag();
		out_values[6] = result.val[3].real();
		out_values[7] = result.val[3].imag();
		out_values[8] = result.val[4].real();
		out_values[9] = result.val[4].imag();
		return n;
	}

	virtual int read_autosweep(autoSweepDataPoint* out_values, int n_values) {
		xavna_drainfd(ttyFD);
		u8 buf[] = {
			// 8
			0x18, 0x30, (u8)n_values
		};
		if(writeAll(ttyFD,buf,sizeof(buf)) != (int)sizeof(buf)) {
			errno = EADDRINUSE;
			return -1;
		}

		int bytes = n_values * 32;
		u8 rBuf[bytes];
		if(readAll(ttyFD, rBuf, bytes) != bytes) {
			errno = EDEADLK;
			return -1;
		}

		for(int i=0; i<n_values; i++) {
			u8* ptr = rBuf + i*32;
			out_values[i].forward[0][0] = *(int32_t*)(ptr + 0);
			out_values[i].forward[0][1] = *(int32_t*)(ptr + 4);
			out_values[i].forward[1][0] = 0;
			out_values[i].forward[1][1] = 0;
			out_values[i].reverse[0][0] = *(int32_t*)(ptr + 8);
			out_values[i].reverse[0][1] = *(int32_t*)(ptr + 12);
			out_values[i].reverse[1][0] = *(int32_t*)(ptr + 16);
			out_values[i].reverse[1][1] = *(int32_t*)(ptr + 20);
			out_values[i].freqIndex = *(uint16_t*)(ptr + 24);
		}
		return n_values;
	}
	virtual ~xavna_default() {
		close(ttyFD);
	}
};

static int __init_xavna_default() {
	xavna_default_constructor = [](const char* dev){ return new xavna_default(dev); };
	return 0;
}

static int ghsfkghfjkgfs = __init_xavna_default();



extern "C" {
	void* xavna_open(const char* dev) {
		auto it = xavna_virtual_devices.find(dev);
		if(it != xavna_virtual_devices.end()) return (*it).second(dev);
		try {
			return xavna_default_constructor(dev);
		} catch(exception& ex) {
			return NULL;
		}
	}

	bool xavna_is_tr(void* dev) {
		return ((xavna_generic*)dev)->is_tr();
	}

	bool xavna_is_autosweep(void* dev) {
		return ((xavna_generic*)dev)->is_autosweep();
	}

	int xavna_set_params(void* dev, int freq_khz, int atten, int port, int nWait) {
		return ((xavna_generic*)dev)->set_params(freq_khz, atten, port, nWait);
	}

	int xavna_set_autosweep(void* dev, double sweepStartHz, double sweepStepHz, int sweepPoints, int nValues) {
		return ((xavna_generic*)dev)->set_autosweep(sweepStartHz, sweepStepHz, sweepPoints, nValues);
	}

	int xavna_read_values(void* dev, double* out_values, int n_samples) {
		return ((xavna_generic*)dev)->read_values(out_values, n_samples);
	}
	
	int xavna_read_values_raw(void* dev, double* out_values, int n_samples) {
		return ((xavna_generic*)dev)->read_values_raw(out_values, n_samples);
	}

	int xavna_read_autosweep(void* dev, autoSweepDataPoint* out_values, int n_values) {
		return ((xavna_generic*)dev)->read_autosweep(out_values, n_values);
	}
	
	int xavna_read_values_raw2(void* dev, double* out_values, int n_samples) {
		xavna_generic* tmp = (xavna_generic*)dev;
		return dynamic_cast<xavna_default*>(tmp)->read_values_raw2(out_values, n_samples);
	}

	void xavna_close(void* dev) {
		delete ((xavna_generic*)dev);
	}
}

