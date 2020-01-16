#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <assert.h>
#include "common_types.h"
#include <string>
#include <stdarg.h>
#include <termios.h>
#include <fftw3.h>
#include <string.h>
#include <vector>
#include <array>
#include <functional>
#include <xavna/xavna.h>
#include <xavna/calibration.H>
#include "vna_ui_core.H"

using namespace std;
//using namespace adf4350Board;



#define FFTWFUNC(x) fftw_ ## x


// settings
int nPoints=100;
int startFreq=137500;
int freqStep=25000;
int attenuation=20;

// currently loaded calibration references
array<vector<complex2>, 4> calibrationReferences;

bool newBoard = true;

int nValues=100;	//number of data points to integrate over
int nValuesExtended=nValues;



void* xavna_dev=NULL;

double Z0=50.;
bool use_cal=false;
bool refreshThreadShouldExit=false;

// currently active calibration coefficients, calculated from measured calibration references
vector<array<complex<double>,3> > cal_coeffs;	// the 3 calibration terms
vector<complex<double> > cal_thru;				// the raw value for the thru reference
vector<complex<double> > cal_thru_leak;			// leakage from port 1 forward to 2
vector<complex<double> > cal_thru_leak_r;		// leakage from port 1 reflected to 2



extern "C" int xavna_read_values_raw(void* dev, double* out_values, int n_samples);





double avgRe=0,avgIm=0;


// increment this variable to request the thread take an extended measurement (for when more accuracy is required)
volatile int requestedMeasurements=0;
// function to be called from the main thread when a requested measurement is complete
function<void(vector<complex2>)>* volatile measurementCallback;

void* refreshThread(void* v) {
	int requestedMeasurementsPrev=requestedMeasurements;
	
	complex<double>* reflArray = (complex<double>*)FFTWFUNC(malloc)(timePoints()*sizeof(complex<double>));
	complex<double>* thruArray = (complex<double>*)FFTWFUNC(malloc)(timePoints()*sizeof(complex<double>));
	//double* reflTD = (double*)FFTWFUNC(malloc)(timePoints()*2*sizeof(double));
	//double* thruTD = (double*)FFTWFUNC(malloc)(timePoints()*2*sizeof(double));
	complex<double>* reflTD = (complex<double>*)FFTWFUNC(malloc)(timePoints()*sizeof(complex<double>));
	complex<double>* thruTD = (complex<double>*)FFTWFUNC(malloc)(timePoints()*sizeof(complex<double>));
	
	FFTWFUNC(plan) p1, p2;
	p1 = fftw_plan_dft_1d(timePoints(), (fftw_complex*)reflArray, (fftw_complex*)reflTD, FFTW_BACKWARD, FFTW_ESTIMATE);
	p2 = fftw_plan_dft_1d(timePoints(), (fftw_complex*)thruArray, (fftw_complex*)thruTD, FFTW_BACKWARD, FFTW_ESTIMATE);
	//p1 = fftw_plan_dft_c2r_1d(timePoints()*2, (fftw_complex*)reflArray, reflTD, 0);
	//p2 = fftw_plan_dft_c2r_1d(timePoints()*2, (fftw_complex*)thruArray, thruTD, 0);
	
	complex2 values;
	while(true) {
		memset(reflArray, 0, timePoints()*sizeof(complex<double>));
		memset(thruArray, 0, timePoints()*sizeof(complex<double>));
		
		for(int i=0;i<nPoints;i++) {
			int freq_kHz=startFreq + i*freqStep;
			printf("%d\n",freq_kHz);
			xavna_set_params(xavna_dev, freq_kHz, attenuation, -1);
			xavna_read_values(xavna_dev, (double*)&values, nValues);
			
			complex<double> reflValue=values[reflIndex];
			complex<double> thruValue=values[thruIndex];
			printf("%7.2f %20lf %10lf %20lf %10lf\n",freq_kHz/1000.,
				abs(reflValue),arg(reflValue),abs(thruValue),arg(thruValue));
			auto refl = reflValue;
			auto thru = thruValue;
			if(use_cal) {
				//auto refl=(cal_X[i]*cal_Y[i]-value*cal_Z[i])/(value-cal_X[i]);
				//refl=(cal_X[i]*cal_Y[i]-reflValue)/(reflValue*cal_Z[i]-cal_X[i]);
				refl = SOL_compute_reflection(cal_coeffs[i], reflValue);
				
				thru = thruValue - (cal_thru_leak[i] + reflValue*cal_thru_leak_r[i]);
				
				auto refThru = cal_thru[i] - (cal_thru_leak[i] + reflValue*cal_thru_leak_r[i]);
				
				thru /= refThru;
				
				//thru=(thruValue-cal_thru_leak[i])/(cal_thru[i]-cal_thru_leak[i]);
			}
			
			reflArray[i] = refl * gauss(double(i)/nPoints, 0, 0.7);
			thruArray[i] = thru * gauss(double(i)/nPoints, 0, 0.7);
			
			updateGraph(i, {refl, thru});
			
			//if(requestedMeasurements>requestedMeasurementsPrev)
			//	if(i<(nPoints-50)) i = nPoints-50;
			if(refreshThreadShouldExit) return NULL;
		}

		// compute time domain values
		FFTWFUNC(execute)(p1);
		FFTWFUNC(execute)(p2);
		int tPoints = timePoints();
		double scale=1./nPoints;
		for(int i=0;i<tPoints;i++) {
			auto refl = reflTD[i]*scale;
			auto thru = thruTD[i]*scale;
			updateTimeGraph(i, {refl, thru});
			printf("%d %.10lf\n",i,reflTD[i]);
		}
		
		sweepCompleted();
		
		
		// take extended measurement
		if(requestedMeasurements>requestedMeasurementsPrev) {
			requestedMeasurementsPrev = requestedMeasurements;
			__sync_synchronize();
			
			printf("taking extended measurement...\n");
			vector<complex2> results;
			
			
			for(int i=0;i<nPoints;i++) {
				int freq_kHz=startFreq + i*freqStep;
				xavna_set_params(xavna_dev, freq_kHz, attenuation, -1);
				xavna_read_values(xavna_dev, (double*)&values, nValuesExtended);
				
				results.push_back(values);
				
				printf("%7.2f %20lf %10lf %20lf %10lf\n",freq_kHz/1000.,
					abs(values[0]),arg(values[0]),abs(values[1]),arg(values[1]));
				
				updateGraph(i, values);
			}
			printf("done.\n");
			(*measurementCallback)(results);
			delete measurementCallback;
			measurementCallback = NULL;
		}
	}
}


void takeMeasurement(function<void(vector<complex2>)> cb) {
	function<void(vector<complex2>)>* cbNew = new function<void(vector<complex2>)>();
	*cbNew = cb;
	__sync_synchronize();
	measurementCallback = cbNew;
	__sync_synchronize();
	__sync_add_and_fetch(&requestedMeasurements, 1);
}
vector<complex<double> > extract(vector<complex2> in, int i) {
	vector<complex<double> > tmp;
	for(int j=0;j<in.size();j++)
		tmp.push_back(in[j][i]);
	return tmp;
}
void applySOLT() {
	for(int i=0;i<nPoints;i++) {
		cal_coeffs[i] = SOL_compute_coefficients(
							calibrationReferences[CAL_SHORT][i][0],
							calibrationReferences[CAL_OPEN][i][0],
							calibrationReferences[CAL_LOAD][i][0]);
		if(calibrationReferences[CAL_THRU].size() != 0)
			cal_thru[i] = calibrationReferences[CAL_THRU][i][1];
		else cal_thru[i] = 1.;
		
		auto x1 = calibrationReferences[CAL_LOAD][i][0],
			y1 = calibrationReferences[CAL_LOAD][i][1],
			x2 = calibrationReferences[CAL_OPEN][i][0],
			y2 = calibrationReferences[CAL_OPEN][i][1];
		
		cal_thru_leak_r[i] = (y1-y2)/(x1-x2);
		cal_thru_leak[i] = y2-cal_thru_leak_r[i]*x2;
	}
	use_cal=true;
}
void clearCalibration() {
	use_cal = false;
}


string saveCalibration() {
	string tmp;
	tmp += '\x05';		// file format version
	tmp.append((char*)&nPoints, sizeof(nPoints));
	tmp.append((char*)&startFreq, sizeof(startFreq));
	tmp.append((char*)&freqStep, sizeof(freqStep));

	for(int i=0;i<nCal;i++) {
		if(calibrationReferences[i].size() == 0) continue;
		uint32_t calType = i;
		tmp.append((char*)&calType, sizeof(calType));
		tmp.append((char*)calibrationReferences[i].data(), sizeof(complex2) * nPoints);
	}
	return tmp;
}
bool loadCalibration(char* data, int size) {
	int calSize=sizeof(complex2) * nPoints;
	if(size<=0) {
		alert("invalid/corrupt calibration file; file is empty");
		return false;
	}
	if(data[0] != 5) {
		alert("incorrect calibration file version; should be 5, is " + to_string((int)data[0]));
		return false;
	}
	if(size < 13) {
		alert("file corrupt; length too short");
		return false;
	}
	int nPoints1, startFreq1, freqStep1;
	nPoints1 = *(int*)(data+1);
	startFreq1 = *(int*)(data+5);
	freqStep1 = *(int*)(data+9);
	if(nPoints1 != nPoints || startFreq1 != startFreq || freqStep1 != freqStep) {
		alert(ssprintf(128, "calibration file has different parameters: %d points, start %d kHz, step %d kHz", nPoints1, startFreq1, freqStep1));
		return false;
	}
	data+=13;
	size-=13;
	
	for(int i=0;i<nCal;i++)
		calibrationReferences[i].resize(0);

	while(size>=(sizeof(uint32_t)+calSize)) {
		uint32_t calType = *(uint32_t*)data;
		if(calType >= nCal) {
			alert(ssprintf(128, "calibration file has an entry of calType %u which is not recognized", calType));
			return false;
		}
		calibrationReferences[calType].resize(nPoints);
		memcpy(calibrationReferences[calType].data(), data+4, calSize);
		data += (4 + calSize);
		size -= (4 + calSize);
	}
	
	applySOLT();
	return true;
}


void resizeVectors(int n) {
	nPoints = n;
	
	for(int i=0;i<nCal;i++)
		calibrationReferences[i].resize(0);
	cal_thru = vector<complex<double> >(nPoints, 1);
	cal_thru_leak = vector<complex<double> >(nPoints, 0);
	cal_thru_leak_r = vector<complex<double> >(nPoints, 0);
	cal_coeffs.resize(nPoints);
}

