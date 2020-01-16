#include <stdint.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <math.h>
#include <assert.h>
#include <pthread.h>
#include <stdlib.h>

#include <complex>
#include <tuple>
#include <vector>
#include <array>
#include <iostream>
#include <map>

#include <eigen3/Eigen/Dense>

#include "../common_types.h"
#include "../include/xavna.h"
#include "../include/xavna_generic.H"
#include "xavna_mock_ui.H"

/*
WARNING: THIS MOCK IS FOR SOFTWARE TESTING PURPOSES ONLY
AND SHOULD NOT BE TAKEN TO IMPLY ANYTHING ABOUT THE
PERFORMANCE OF THE ACTUAL HARDWARE
*/

using namespace std;
using namespace Eigen;

typedef Matrix<complex<double>,8,8> Matrix8cd;
typedef Matrix<complex<double>,8,1> Vector8cd;


template<int N> array<complex<double>, N> operator*(const array<complex<double>, N>& a, double b) {
	array<complex<double>, N> res;
	for(int i=0;i<N;i++)
		res[i] = a[i]*b;
	return res;
}

// solve for the nodes in a signal flow graph, given excitation applied to each node
VectorXcd solveSFG(MatrixXcd sfg, VectorXcd excitation) {
	assert(sfg.rows() == sfg.cols());
	assert(sfg.rows() == excitation.size());
	
	VectorXcd rhs = -excitation;
	MatrixXcd A = sfg - MatrixXcd::Identity(sfg.rows(), sfg.rows());
	auto tmp = A.colPivHouseholderQr();
	assert(tmp.rank() == sfg.rows());
	return tmp.solve(rhs);
}
// solve for the nodes in a signal flow graph, but allow floating input nodes (allowed to take on any value)
// and allow for forced nodes (nodes that are constrained to a value, but still obeying edge equations)
// for example this can be used to solve for the inputs to a signal flow graph given the outputs
// note that the number of floating (free) nodes must equal the number of forced nodes for the system to
// have a unique solution
VectorXcd solveSFGImplicit(MatrixXcd sfg, VectorXcd excitation,
							vector<int> freeNodes, vector<int> forcedNodes, vector<complex<double> > forcedNodeValues) {
	assert(freeNodes.size() == forcedNodes.size());
	assert(forcedNodes.size() == forcedNodeValues.size());
	
	for(int i=0;i<(int)freeNodes.size();i++) {
		// create a self-loop of gain 1 on the free node
		// the self-loop allows the free node to take on any value with no incoming edge
		sfg(freeNodes[i], freeNodes[i]) = 1.;
		
		// create an edge from the forced node to the free node with value -1
		sfg(freeNodes[i], forcedNodes[i]) = -1.;
		// add an excitation on the free node with the value of the forced node value
		excitation(forcedNodes[i]) = forcedNodeValues[i];
		
		// the self-loop of gain 1 on the free node enforces all the incoming edges must sum to 0,
		// because otherwise the feedback will cause the value to blow up;
		// the effect of this constraint is that the forced node must in the end have a value
		// equal to the forced value
	}
	return solveSFG(sfg, excitation);
}


struct complex5 {
	complex<double> val[5];
};

struct scopeLock {
	pthread_mutex_t* mut;
	scopeLock(pthread_mutex_t& m): mut(&m) {
		pthread_mutex_lock(mut);
	}
	~scopeLock() {
		pthread_mutex_unlock(mut);
	}
};

struct xavna_virtual {
	double startFreq,freqStep; //Hz
	vector<Matrix2cd> dut; // S parameters
	// error adaptor S parameters; ports 0,1 are reflectometer side
	// and ports 2,3 are DUT side
	vector<Matrix4cd> errorParams;
    xavna_mock_ui* ui;
	pthread_mutex_t mutex;
	
	double curFreq;
	Vector2cd excitations;
	double phaseOffset;
	
    xavna_virtual() {
        ui = new xavna_mock_ui();
        ui->set_cb([this](string dut_name, double cableLen1, double cableLen2) {
            ui_changed_cb(dut_name,cableLen1,cableLen2);
        });
    }
    ~xavna_virtual() {
		delete ui;
	}
	double freqAt(int i) {
		return startFreq+freqStep*i;
	}
	
	void init() {
		excitations << 1.,0.;
		phaseOffset = (rand() / (RAND_MAX + 1.0))*2*M_PI;
		pthread_mutex_init(&mutex, NULL);
		startFreq = 1e6;
		freqStep = 1e6;
		curFreq = startFreq;
		dut.resize(3000);
		errorParams.resize(3000);

		for(int i=0;i<3000;i++) {
            dut[i] << -1., 0.,
                      0., -1.;
					 
			errorParams[i] <<
			//   0    1    2    3
			  0.2, 0.1, 0.8, 0.0,
			  0.1, 0.2, 0.2, 0.8,
			  0.8, 0.1, 0.2, 0.1,
			  0.0, 0.8, 0.0, 0.2;
			
			double f = double(i)/3000.;
			complex<double> freqFactor = polar(f, -f);
			complex<double> freqFactor1 = polar(f/2, -f*1.5);
			complex<double> freqFactor2 = polar(f/2, -f*2.);
			
			complex<double> freqFactor3 = polar(1.-f/2, -f*2.+1.3);
			
			errorParams[i](0,0) *= freqFactor;
			errorParams[i](1,1) *= freqFactor;
			errorParams[i](2,2) *= freqFactor*polar(1.,0.2);
			errorParams[i](3,3) *= freqFactor*polar(1.,0.2);
			
			errorParams[i](0,1) *= freqFactor1;
			errorParams[i](1,0) *= freqFactor1;
			errorParams[i](0,3) *= freqFactor2;
			errorParams[i](3,0) *= freqFactor2;
			
			errorParams[i](2,3) *= freqFactor1;
			errorParams[i](3,2) *= freqFactor1;
			errorParams[i](1,2) *= freqFactor2*polar(1.,0.5);
			errorParams[i](2,1) *= freqFactor2*polar(1.,0.5);
			
			errorParams[i](0,2) *= freqFactor3;
			errorParams[i](2,0) *= freqFactor3;
			errorParams[i](1,3) *= freqFactor3;
			errorParams[i](3,1) *= freqFactor3;
		}
	}

    void ui_changed_cb(string dut_name, double cableLen1, double cableLen2) {
        printf("%s %f %f\n",dut_name.c_str(), cableLen1,cableLen2);
        scopeLock _l(mutex);
        if(dut_name == "short") {
			for(int i=0;i<(int)dut.size();i++) {
				dut[i] << -1., 0.,
						  0., -1.;
			}
		} else if(dut_name == "open") {
			for(int i=0;i<(int)dut.size();i++) {
				dut[i] << 1., 0.,
						  0., 1.;
			}
		} else if(dut_name == "load") {
			for(int i=0;i<(int)dut.size();i++) {
				dut[i] << 0., 0.,
						  0., 0.;
			}
		} else if(dut_name == "thru-1cm") {
			_set_dut_thru(10e-3/160e6);
		} else if(dut_name == "thru-5cm") {
			_set_dut_thru(50e-3/160e6);
		} else if(dut_name == "stub-5cm") {
			_set_dut_stub(50e-3/160e6);
		} else if(dut_name == "stub-12cm") {
			_set_dut_stub(120e-3/160e6);
		}
		double del1 = cableLen1/1000./160e6, del2 = cableLen2/1000./160e6;
		for(int i=0;i<(int)dut.size();i++) {
			complex<double> a1 = polar(1.,-del1*freqAt(i)*2*M_PI);
			complex<double> a2 = polar(1.,-del2*freqAt(i)*2*M_PI);
			complex<double> a3 = a1*a2;
			dut[i](0,0) *= a1*a1;
			dut[i](0,1) *= a3;
			dut[i](1,0) *= a3;
			dut[i](1,1) *= a2*a2;
		}
    }
    void _set_dut_thru(double delay) {
		for(int i=0;i<(int)dut.size();i++) {
			double phase=freqAt(i)*delay*2*M_PI;
			complex<double> val = polar(1.,-phase);
			dut[i] << 0., val,
					  val, 0.;
		}
	}
	void _set_dut_stub(double delay) {
		for(int i=0;i<(int)dut.size();i++) {
			double phase=freqAt(i)*delay*2*M_PI;
			complex<double> val = polar(1.,-phase);
			dut[i] << val, 0.,
					  0., val;
		}
	}
	
	// return the true S parameters of the emulated DUT at freq_hz
	Matrix2cd getDUTSParam(double freq_hz) {
		scopeLock _l(mutex);
		double index=(freq_hz-startFreq)/freqStep;
		if(index < 0) return dut[0];
		if(index >= (dut.size()-1)) return dut[dut.size()-1];
		
		// linear interpolate
		int i1 = (int)floor(index);
		int i2 = i1 + 1;
		return dut[i1]*(i2-index) + dut[i2]*(index-i1);
	}
	
	Matrix4cd getErrorSParam(double freq_hz) {
		//pthread_mutex_lock(&mutex);
		double index=(freq_hz-startFreq)/freqStep;
		if(index < 0) return errorParams[0];
		if(index >= (errorParams.size()-1)) return errorParams[errorParams.size()-1];
		
		// linear interpolate
		int i1 = (int)floor(index);
		int i2 = i1 + 1;
		return errorParams[i1]*(double(i2)-index) + errorParams[i2]*(index-double(i1));
		//pthread_mutex_unlock(&mutex);
	}
	
	Vector2cd getMeasuredValues(double freq_hz, Vector2cd excitations) {
		Matrix8cd sfg = Matrix8cd::Zero();
		Vector8cd exc = Vector8cd::Zero();
		sfg.bottomLeftCorner(4,4) = getErrorSParam(freq_hz);
		sfg.topRightCorner(4,4).bottomRightCorner(2,2) = getDUTSParam(freq_hz);
		exc.head(2) = excitations;
		
		//cout << sfg << endl;
		
		Vector8cd res = solveSFG(sfg, exc);
		
		return res.tail(4).head(2);
	}
};


class xavna_mock: public xavna_generic {
public:
    xavna_virtual virt;
    xavna_mock(const char*) {
        virt.init();
	}

    bool is_tr() {
        return false;
    }

	bool is_autosweep() {
		return false;
	}
	
    int set_params(int freq_khz, int atten, int port, int nWait) {
		if(atten == -1) atten=100;
		
        virt.curFreq = double(freq_khz)*1000;
        virt.excitations[0] = (port==0?polar(2.5*pow(10,-atten/10.), virt.phaseOffset + 1.23):0.);
        virt.excitations[1] = (port==1?polar(3.6*pow(10,-atten/10.), virt.phaseOffset + 2.5):0.);
		
		// simulate some switch leakage
        //complex<double> tmp = virt.excitations[0];
        //virt.excitations[0] += virt.excitations[1]*polar(0.002,1.2);
        //virt.excitations[1] += tmp*polar(0.002,5.3);
		
		return 0;
	}
	int set_autosweep(double sweepStartHz, double sweepStepHz, int sweepPoints, int nValues) {
		return -1;
	}
	int read_autosweep(autoSweepDataPoint* out_values, int n_values) {
		return -1;
	}

    int set_if_freq(int freq_khz) {
        return 0;
    }
	
	double noise(double amplitude) {
		return ((rand() / (RAND_MAX + 1.0))*2-1) * amplitude;
	}

	// out_values: array of size 4 holding the following values:
	//				reflection real, reflection imag,
	//				thru real, thru imag
	// n_samples: number of samples to average over; typical 50
	// returns: number of samples read, or -1 if failure
    int read_values(double* out_values, int n_samples) {
        Vector2cd res0 = virt.getMeasuredValues(virt.curFreq, {1.,0.});
        //Vector8cd res1 = virt.getMeasuredValues(virt.curFreq, {0.,1.});
		
		double a = 500e-6;
		
		out_values[0] = res0[0].real() + noise(a);
		out_values[1] = res0[0].imag() + noise(a);
		out_values[2] = res0[1].real() + noise(a);
		out_values[3] = res0[1].imag() + noise(a);
        usleep(5000);
		return n_samples;
	}
	
    int read_values_raw(double* out_values, int n_samples) {
        Vector2cd res0 = virt.getMeasuredValues(virt.curFreq, virt.excitations);
        //Vector2cd res1 = virt.getMeasuredValues(virt.curFreq, {0.01,3.6});
		
		double a = 5e-7;
		
        out_values[0] = virt.excitations[0].real() + noise(a);
        out_values[1] = virt.excitations[0].imag() + noise(a);
		out_values[2] = res0[0].real() + noise(a);
		out_values[3] = res0[0].imag() + noise(a);
        out_values[4] = virt.excitations[1].real() + noise(a);
        out_values[5] = virt.excitations[1].imag() + noise(a);
		out_values[6] = res0[1].real() + noise(a);
		out_values[7] = res0[1].imag() + noise(a);
        usleep(5000);
		return n_samples;
    }
};

extern map<string, xavna_constructor> xavna_virtual_devices;

extern "C" int __init_xavna_mock() {
    xavna_virtual_devices["mock"] = [](const char* dev){ return new xavna_mock(dev); };
    fprintf(stderr, "loaded xavna mock library\n");
    fflush(stderr);
    return 0;
}

static int ghsfkghfjkgfs = __init_xavna_mock();



