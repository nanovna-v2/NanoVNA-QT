#ifndef __XAVNA_CPP_H
#define __XAVNA_CPP_H
#include <pthread.h>
#include <complex>
#include <vector>
#include <array>
#include <stdexcept>
#include "common.H"

using namespace std;

namespace xaxaxa {
	class VNADevice {
	public:
		// frequency sweep parameters; do NOT change while background thread is running
		double startFreqHz=200e6;	// start frequency in Hz
		double stepFreqHz=25e6;		// step frequency in Hz
		int nPoints=50;				// how many frequency points
		int nValues=30;	 			// how many values to average over
		int nWait=20;				// how many values to skip after changing frequency
		bool disableReference = false;	// if true, do not divide by reference value
		bool forceTR = false;		// if device is full two port vna, force T/R mode
		bool swapPorts = false;		// only supported on full two port vna

		// rf parameters
		int attenuation1=25, attenuation2=25;
		
		// called by background thread when a single frequency measurement is done
        function<void(int freqIndex, VNARawValue val)> frequencyCompletedCallback;
        
		
		// called by background thread when a complete sweep of all frequencies is done
		function<void(const vector<VNARawValue>& vals)> sweepCompletedCallback;
		
		// called by background thread when an error occurs
		function<void(const exception& exc)> backgroundErrorCallback;
		
		
		VNADevice();
		~VNADevice();
		
		// find all xaVNA devices present
		static vector<string> findDevices();
		
		// returns the device handle as returned by xavna_open(), or NULL
		void* device();
		
		// open a vna device
		// dev: path to the serial device; if empty, it will be selected automatically
		void open(string dev);
		
		// returns whether the device is a T/R vna
		bool isTR();
		
		// returns whether the device uses the autosweep protocol
		bool isAutoSweep();
		
		// returns true if either device is a T/R vna or forceTR is true
		bool isTRMode();
		
		// start the frequency sweep background thread, which repeatedly performs
		// scans until it is stopped using stopScan()
		void startScan();
		
		// stop the background thread
		void stopScan();
		
		// whether the background thread is running
		bool isScanning();
		
		// close the vna device
		void close();
		
		// wait for one full measurement, and call cb with results
		void takeMeasurement(function<void(const vector<VNARawValue>& vals)> cb);
		
		// return the frequency in Hz at an array index
		double freqAt(int i) {
			return startFreqHz+i*stepFreqHz;
		}
		
		// return highest power output in dBm
		int maxPower() const {
			return 10;
		}
		
		
		// diagnostics
		
		// called by background thread after frequencyCompletedCallback(); provides signal values
        // directly from hardware
        function<void(int freqIndex, const vector<array<complex<double>, 4> >& values)> frequencyCompletedCallback2_;
		
		
		// internal variables
		void* _dev=NULL;
		pthread_t _pth;
		bool _threadRunning=false;
		bool _shouldExit=false;
		bool _lastDeviceIsAutosweep=false;
		volatile uint32_t _measurementCnt=0;
		function<void(const vector<VNARawValue>& vals)> _cb;
		volatile function<void(const vector<VNARawValue>& vals)>* _cb_;
		
		// internal methods
		void* _mainThread();
		void* _runAutoSweep();
	};
}
#endif
