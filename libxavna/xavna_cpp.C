#include "include/xavna_cpp.H"
#include "include/xavna.h"
#include "include/platform_abstraction.H"
#include "include/workarounds.H"
#include <pthread.h>
#include <array>

using namespace std;

namespace xaxaxa {
    static void* _mainThread_(void* param);
	bool _noscan = false;

	VNADevice::VNADevice() {
        _cb_ = &_cb;
        frequencyCompletedCallback2_ = [](int freqIndex, const vector<array<complex<double>, 4> >& values) {};
	}
	VNADevice::~VNADevice() {
		
	}
	vector<string> VNADevice::findDevices() {
		return xavna_find_devices();
	}
	
	void* VNADevice::device() {
		return _dev;
	}
	
	void VNADevice::open(string dev) {
		if(_dev) close();
		if(dev == "") {
			auto tmp = findDevices();
			if(tmp.size() == 0) throw runtime_error("no vna device found");
			dev = tmp[0];
		}
		_dev = xavna_open(dev.c_str());
		if(!_dev) throw runtime_error(strerror(errno));
		
		if(isAutoSweep() != _lastDeviceIsAutosweep) {
			if(isAutoSweep()) {
				nValues = 2;
				nWait = -1;
			} else {
				nValues = 30;
				nWait = 20;
			}
		}
	}
	bool VNADevice::isTR() {
		if(!_dev) return true;
		//return true;
		return xavna_is_tr(_dev);
	}
	bool VNADevice::isAutoSweep() {
		if(!_dev) return false;
		return xavna_is_autosweep(_dev);
	}
	bool VNADevice::isTRMode() {
		return isTR() || forceTR;
	}
	void VNADevice::startScan() {
		if(!_dev) throw logic_error("VNADevice: you must call open() before calling startScan()");
		if(_threadRunning) return;
		_threadRunning = true;
		pthread_create(&_pth, NULL, &_mainThread_, this);
	}
	void VNADevice::stopScan() {
		if(!_threadRunning) return;
		_shouldExit = true;
		pthread_cancel(_pth);
		pthread_join(_pth, NULL);
		_shouldExit = false;
		_threadRunning = false;
	}
	bool VNADevice::isScanning() {
		return _threadRunning;
	}
	void VNADevice::close() {
		if(_threadRunning) stopScan();
		if(_dev != NULL) {
			xavna_close(_dev);
			_dev = NULL;
		}
	}
	
	void VNADevice::takeMeasurement(function<void(const vector<VNARawValue>& vals)> cb) {
		if(!_threadRunning) throw logic_error("takeMeasurement: vna scan thread must be started");
		_cb = cb;
		__sync_synchronize();
		__sync_add_and_fetch(&_measurementCnt, 1);
	}
	
	static inline void swap(VNARawValue& a, VNARawValue& b) {
		VNARawValue tmp = a;
		a = b;
		b = tmp;
	}
	void* VNADevice::_mainThread() {
		if(xavna_is_autosweep(_dev)) {
			return _runAutoSweep();
		}
		bool tr = isTRMode();
		uint32_t last_measurementCnt = _measurementCnt;
		int cnt=0;
		while(!_shouldExit) {
			vector<VNARawValue> results(nPoints);
			for(int i=0;i<nPoints;i++) {
				fflush(stdout);
				int ports = tr?1:2;
				
				// values is indexed by excitation #, then wave #
				// e.g. values[0][1] is wave 1 measured with excitation on port 0
				vector<array<complex<double>, 4> > values(ports);
				for(int port=0; port<ports; port++) {
					int p = swapPorts?(1-port):port;
					if(!_noscan) {
						if(xavna_set_params(_dev, (int)round(freqAt(i)/1000.),
											(p==0?attenuation1:attenuation2), p, nWait) < 0) {
							backgroundErrorCallback(runtime_error("xavna_set_params failed: " + string(strerror(errno))));
							return NULL;
						}
					}
                    if(xavna_read_values_raw(_dev, (double*)&values[port], nValues)<0) {
						backgroundErrorCallback(runtime_error("xavna_read_values_raw failed: " + string(strerror(errno))));
						return NULL;
					}
				}
				if(swapPorts) {
					for(int port=0;port<ports;port++) {
						swap(values[port][0], values[port][2]);
						swap(values[port][1], values[port][3]);
					}
				}
				
				VNARawValue tmp;
				if(tr) {
					if(disableReference)
						tmp << values[0][1], 0,
						        values[0][3], 0;
					else
						tmp << values[0][1]/values[0][0], 0,
						        values[0][3]/values[0][0], 0;
				} else {
					complex<double> a0,b0,a3,b3;
					complex<double> a0p,b0p,a3p,b3p;
					a0 = values[0][0];
					b0 = values[0][1];
					a3 = values[0][2];
					b3 = values[0][3];
					a0p = values[1][0];
					b0p = values[1][1];
					a3p = values[1][2];
					b3p = values[1][3];
					
					// solving for two port S parameters given two sets of
					// observed waves (port 1 in, port 1 out, port 2 in, port 2 out)
					
					complex<double> d = 1. - (a3*a0p)/(a0*a3p);
					
					// S11M
					tmp(0,0) = ((b0/a0) - (b0p*a3)/(a3p*a0))/d;
					// S21M
					tmp(1,0) = ((b3/a0) - (b3p*a3)/(a3p*a0))/d;
					// S12M
					tmp(0,1) = ((b0p/a3p) - (b0*a0p)/(a3p*a0))/d;
					// S22M
					tmp(1,1) = ((b3p/a3p) - (b3*a0p)/(a3p*a0))/d;
				}
				//tmp(0,0) = tmp(1,0)*6.;
				/*if(abs(tmp(0,0)) > 0.5 && (arg(tmp(0,0))*180) < -90) {
					static int cnt = 0;
					if((cnt++) > 10) {
						_noscan = true;
					}
				}*/
				
                frequencyCompletedCallback(i, tmp);
                frequencyCompletedCallback2_(i, values);
                
				results[i]=tmp;
				if(_shouldExit) return NULL;
			}
			sweepCompletedCallback(results);
			
			if(_measurementCnt != last_measurementCnt) {
				__sync_synchronize();
				if(cnt == 1) {
					function<void(const vector<VNARawValue>& vals)> func
						= *(function<void(const vector<VNARawValue>& vals)>*)_cb_;
					func(results);
					cnt = 0;
					last_measurementCnt = _measurementCnt;
				} else cnt++;
			}
			
		}
		return NULL;
	}
	static complex<double> cx(const double* v) {
		return {v[0], v[1]};
	}
	void* VNADevice::_runAutoSweep() {
		uint32_t last_measurementCnt = _measurementCnt;
		int lastFreqIndex = -1;
		int measurementEndPoint = -1;
		int chunkPoints = 8;
		int nValues = this->nValues;
		int collectDataState = 0;
		int cnt = 0;
		int currValueIndex = 0;
		vector<VNARawValue> results(nPoints);
		vector<array<complex<double>, 4> > rawValues(1);
		rawValues[0] = {0., 0., 0., 0.};
		
		xavna_set_autosweep(_dev, startFreqHz, stepFreqHz, nPoints, nValues);
		while(!_shouldExit) {
			fflush(stdout);
			int chunkValues = chunkPoints * nValues;
			autoSweepDataPoint values[chunkValues];

			// read a chunk of values
			if(xavna_read_autosweep(_dev, values, chunkValues)<0) {
				backgroundErrorCallback(runtime_error("xavna_read_autosweep failed: " + string(strerror(errno))));
				return NULL;
			}
			
			// process chunk
			for(int i=0; i<chunkValues; i++) {
				auto& value = values[i];
				array<complex<double>, 4> currRawValue =
						{cx(value.forward[0]), cx(value.reverse[0]),
						cx(value.forward[1]), cx(value.reverse[1])};

				for(int j=0; j<4; j++)
					rawValues[0][j] += currRawValue[j];

				if(value.freqIndex >= nPoints) {
					fprintf(stderr, "warning: hw returned freqIndex (%d) >= nPoints (%d)\n", value.freqIndex, nPoints);
					continue;
				}

				// keep track of how many values so far for this frequency
				if(value.freqIndex != lastFreqIndex) {
					currValueIndex = 0;
					lastFreqIndex = value.freqIndex;
				} else currValueIndex++;

				// last value for this frequency point
				if(currValueIndex == nValues - 1) {
					VNARawValue tmp;
					if(disableReference)
						tmp << rawValues[0][1], 0,
						        rawValues[0][3], 0;
					else
						tmp << rawValues[0][1]/rawValues[0][0], 0,
						        rawValues[0][3]/rawValues[0][0], 0;
					frequencyCompletedCallback(value.freqIndex, tmp);
					frequencyCompletedCallback2_(value.freqIndex, rawValues);
					results[value.freqIndex] = tmp;
					if(value.freqIndex == nPoints - 1)
						sweepCompletedCallback(results);
					rawValues[0] = {0., 0., 0., 0.};
				}
				if(_shouldExit) return NULL;
				if(collectDataState == 1 && value.freqIndex == 0) {
					collectDataState = 2;
				} else if(collectDataState == 2 && value.freqIndex == nPoints - 1) {
					collectDataState = 3;
					cnt = 0;
				} else if(collectDataState == 3) {
					if(currValueIndex == nValues - 1) {
						cnt++;
						if(cnt >= 5) {
							collectDataState = 0;
							nValues = this->nValues;
							xavna_set_autosweep(_dev, startFreqHz, stepFreqHz, nPoints, nValues);

							function<void(const vector<VNARawValue>& vals)> func
									= *(function<void(const vector<VNARawValue>& vals)>*)_cb_;
							func(results);
						}
					}
				}
			}
			
			if(_measurementCnt != last_measurementCnt) {
				// collect measurement requested
				last_measurementCnt = _measurementCnt;
				// when collecting measurements, use double the averaging factor
				nValues = this->nValues * 2;
				xavna_set_autosweep(_dev, startFreqHz, stepFreqHz, nPoints, nValues);
				collectDataState = 1;
			}
		}
		return NULL;
	}
	static void* _mainThread_(void* param) {
		return ((VNADevice*)param)->_mainThread();
	}
}
