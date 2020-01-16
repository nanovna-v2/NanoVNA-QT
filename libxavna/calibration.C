#include "include/calibration.H"
#include "include/workarounds.H"
#include <iostream>
#include <string>

using namespace std;
typedef Matrix<complex<double>,5,1> Vector5cd;
typedef Matrix<complex<double>,6,1> Vector6cd;

namespace xaxaxa {
    
MatrixXcd scalar(complex<double> value) {
    return MatrixXcd::Constant(1,1,value);
}

// T/R SOLT calibration and one port SOL calibration
class SOLTCalibrationTR: public VNACalibration {
public:
    bool noThru;
    SOLTCalibrationTR(bool noThru=false): noThru(noThru) {
        
    }
    // return the name of this calibration type
    virtual string name() const override {
        return noThru?"sol_1":"solt_tr";
    }
    string description() const override {
        return noThru?"SOL (1 port)":"SOLT (T/R)";
    }
    string helpText() const override {
        if(noThru)
            return "5-term two port calibration using 3 fully known standards Short, Open, and Load. Removes port-to-port leakage.";
        return "6-term two port calibration using Short, Open, Load, and zero-length thru. Removes port-to-port leakage and normalizes S21 using thru standard.";
    }
    // get a list of calibration standards required
    vector<array<string, 2> > getRequiredStandards() const override {
        if(noThru) return {{"short1","Short"},{"open1","Open"},{"load1","Load"}};
        else return {{"short1","Short"},{"open1","Open"},{"load1","Load"},{"thru","Thru"}};
    }
    Matrix2cd combineValues(Matrix2cd p1, Matrix2cd p2) {
        Matrix2cd tmp;
        tmp << p1(0,0), 0.,
                0., p2(1,1);
        return tmp;
    }
    
    // given the measurements for each of the calibration standards, compute the coefficients
    MatrixXcd computeCoefficients(const vector<VNARawValue>& measurements,
                                  const vector<VNACalibratedValue>& calStdModels) const override {                    
        CalibrationEngine ce(1);
        ce.addFullEquation(scalar(calStdModels.at(0)(0,0)), measurements.at(0));
        ce.addFullEquation(scalar(calStdModels.at(1)(0,0)), measurements.at(1));
        ce.addFullEquation(scalar(calStdModels.at(2)(0,0)), measurements.at(2));
        ce.addNormalizingEquation();
        MatrixXcd tmp = ce.computeCoefficients();
        
        
        auto x1 = measurements[2](0,0),
            y1 = measurements[2](1,0),
            x2 = measurements[1](0,0),
            y2 = measurements[1](1,0);
        complex<double> cal_thru_leak_r = (y1-y2)/(x1-x2);
        complex<double> cal_thru_leak = y2-cal_thru_leak_r*x2;
        
        complex<double> thru = 1.;
        if(!noThru) thru = measurements.at(3)(1,0);
        
        tmp.conservativeResize(4, 2);
        tmp(2,0) = cal_thru_leak;
        tmp(2,1) = cal_thru_leak_r;
        tmp(3,0) = thru;
        tmp(3,1) = 0.;
        return tmp;
    }
    // given cal coefficients and a raw value, compute S parameters
    VNACalibratedValue computeValue(MatrixXcd coeffs, VNARawValue val) const override {
        VNACalibratedValue ret;
        ret(0,0) = CalibrationEngine::computeSParams(coeffs.topRows(2), scalar(val(0,0)))(0,0);
        
        complex<double> cal_thru_leak_r = coeffs(2,1);
        complex<double> cal_thru_leak = coeffs(2,0);
        
        complex<double> thru = val(1,0) - (cal_thru_leak + val(0,0)*cal_thru_leak_r);
        complex<double> refThru = coeffs(3,0) - (cal_thru_leak + val(0,0)*cal_thru_leak_r);
        
        ret(1,0) = thru/refThru;
        ret(0,1) = 0.;
        ret(1,1) = 0.;
        return ret;
    }
};

// solve for the nodes in a signal flow graph, given excitation applied to each node
static VectorXcd solveSFG(MatrixXcd sfg, VectorXcd excitation) {
	assert(sfg.rows() == sfg.cols());
	assert(sfg.rows() == excitation.size());
	
	VectorXcd rhs = -excitation;
	MatrixXcd A = sfg - MatrixXcd::Identity(sfg.rows(), sfg.rows());
	auto tmp = A.colPivHouseholderQr();
	assert(tmp.rank() == sfg.rows());
	return tmp.solve(rhs);
}

class SOLTCalibration: public VNACalibration {
public:
    bool matchedThru;
    SOLTCalibration(bool matchedThru=true): matchedThru(matchedThru) {}
    // return the name of this calibration type
    virtual string name() const override {
        return matchedThru?"solt":"solr";
    }
    string description() const override {
        return matchedThru?"SOLT (modified UXYZ)":"SOLT (UXYZ)";
    }
    string helpText() const override {
        if(matchedThru)
            return "Unknown thru calibration using 3 fully known one port standards"
                    " and a reciprocal thru standard. Thru standard must have "
                    "an electrical delay between -90 and 90 degrees, should be "
                    "well matched (or as short as possible), and have less than 10dB of loss. "
                    "Matching error of the two instrument ports is fully removed.";
        return "Unknown thru calibration using 3 fully known one port standards"
                    " and a reciprocal thru standard. Thru standard must have "
                    "an electrical delay between -90 and 90 degrees and have "
                    "less than 10dB of loss. It is recommended to use the modified UXYZ"
                    " calibration unless the thru standard is known to be poorly matched.";
    }
    // get a list of calibration standards required
    vector<array<string, 2> > getRequiredStandards() const override {
        return {{"short1","Short (port 1)"}, {"short2","Short (port 2)"},
                {"open1","Open (port 1)"}, {"open2","Open (port 2)"},
                {"load1","Load (port 1)"}, {"load2","Load (port 2)"},
                {"thru","Thru"}};
    }
    // given the measurements for each of the calibration standards, compute the coefficients
    MatrixXcd computeCoefficients(const vector<VNARawValue>& measurements,
                                  const vector<VNACalibratedValue>& calStdModels) const override {
        // perform two SOL calibrations, one for each port
        vector<VNARawValue> m1 {measurements[0], measurements[2], measurements[4]};
        vector<VNACalibratedValue> s1 {calStdModels[0], calStdModels[2], calStdModels[4]};
        vector<VNARawValue> m2 {mirror(measurements[1]), mirror(measurements[3]), mirror(measurements[5])};
        vector<VNACalibratedValue> s2 {calStdModels[1], calStdModels[3], calStdModels[5]};
        
        MatrixXcd out1 = getSOLCoeffs(m1, s1);
        MatrixXcd out2 = getSOLCoeffs(m2, s2);
        
        // since both SOL calibration error models are normalized, we are missing one term
        // which is the ratio factor between the two error parameters.
        
        // calculate ratio factor using unknown thru standard
        VNACalibratedValue thru = applySOL(out1, out2, measurements[6]);
        complex<double> c = sqrt(thru(0,1)/thru(1,0));
        
        // there is a 180 degree phase uncertainty; c can be positive or negative.
        // choose the value that makes -90 degrees < arg(S21) < 90 degrees
        if(abs(arg(applySOL(out1*c, out2, measurements[6])(1,0))) > M_PI/2)
            c = -c;
        out1 = out1*c;
        
        if(matchedThru) {
            // assume that the thru standard is well-matched, measure S11
            // of the two ports
            VNACalibratedValue thru_sparams = applySOL(out1, out2, measurements[6]);
            complex<double> delay = thru_sparams(1,0)*thru_sparams(0,1);
            complex<double> refl2 = thru_sparams(0,0)/delay;
            complex<double> refl1 = thru_sparams(1,1)/delay;
            MatrixXcd out(5, 2);
            out << out1,
                    out2,
                    refl2, refl1;
            return out;
        } else {
            MatrixXcd out(4, 2);
            out << out1,
                    out2;
            return out;
        }
    }
    // given cal coefficients and a raw value, compute S parameters
    VNACalibratedValue computeValue(MatrixXcd coeffs, VNARawValue val) const override {
        assert(coeffs.rows() == 4 || coeffs.rows() == 5);
        assert(coeffs.cols() == 2);
        VNACalibratedValue tmp = applySOL(coeffs.topRows(2), coeffs.block(2,0,2,2), val);
        
        if(matchedThru) {
            assert(coeffs.rows() == 5);
            // calibrate out the match error of the vna ports;
            // since we know the refl coeff of the two ports from measuring
            // the thru standard, we can add an opposing edge to the sfg
            // of the DUT S parameters and re-calculate S11 and S22.
            MatrixXcd sfg = MatrixXcd::Zero(4, 4);
            sfg(0,1) = tmp(0,0);
            sfg(0,2) = tmp(0,1);
            sfg(3,1) = tmp(1,0);
            sfg(3,2) = tmp(1,1);
            sfg(2,3) = -coeffs(4,0);
            complex<double> s11 = solveSFG(sfg, Vector4cd{0.,1.,0.,0.})(0);
            sfg(2,3) = 0.;
            sfg(1,0) = -coeffs(4,1);
            complex<double> s22 = solveSFG(sfg, Vector4cd{0.,0.,1.,0.})(3);
            tmp(0,0) = s11;
            tmp(1,1) = s22;
        }
        return tmp;
    }
    
    MatrixXcd getSOLCoeffs(const vector<VNARawValue>& measurements,
                                  const vector<VNACalibratedValue>& calStdModels) const {                    
        CalibrationEngine ce(1);
        ce.addFullEquation(scalar(calStdModels.at(0)(0,0)), measurements.at(0));
        ce.addFullEquation(scalar(calStdModels.at(1)(0,0)), measurements.at(1));
        ce.addFullEquation(scalar(calStdModels.at(2)(0,0)), measurements.at(2));
        ce.addNormalizingEquation();
        return ce.computeCoefficients();
    }
    // given two one-port SOL cal coefficients, compute S parameters
    VNACalibratedValue applySOL(const MatrixXcd& coeffs1, const MatrixXcd& coeffs2, VNARawValue val) const {
        // place the two 2-port T parameters into the 4-port T parameters
        // to go from two 1-port error adapters to one 2-port error adapter.
        // see page 22, 36, and 37 of
        // Network_Analyzer_Error_Models_and_Calibration_Methods.pdf for more info
        MatrixXcd tmp = MatrixXcd::Zero(4, 4);
        tmp(0,0) = coeffs1(0,0);
        tmp(0,2) = coeffs1(0,1);
        tmp(2,0) = coeffs1(1,0);
        tmp(2,2) = coeffs1(1,1);
        
        tmp(1,1) = coeffs2(0,0);
        tmp(1,3) = coeffs2(0,1);
        tmp(3,1) = coeffs2(1,0);
        tmp(3,3) = coeffs2(1,1);
        
        return CalibrationEngine::computeSParams(tmp, val);
    }
};

vector<const VNACalibration*> initCalTypes() {
    return {new SOLTCalibrationTR(true), new SOLTCalibrationTR(false),
        new SOLTCalibration(true), new SOLTCalibration(false)};
}


vector<const VNACalibration*> calibrationTypes = initCalTypes();
map<string, VNACalibratedValue> idealCalStds;


int initCalStds() {
    idealCalStds["short1"] << -1., 0,
                                0, -1.;
    idealCalStds["open1"] << 1., 0,
                            0, 1.;
    idealCalStds["load1"] << 0, 0,
                            0, 0;
    idealCalStds["thru"] << 0, 1.,
                            1., 0;
    idealCalStds["short2"] = idealCalStds["short1"];
    idealCalStds["open2"] = idealCalStds["open1"];
    idealCalStds["load2"] = idealCalStds["load1"];
    return 0;
}
int tmp = initCalStds();


// CalibrationEngine


CalibrationEngine::CalibrationEngine(int nPorts) {
    _nPorts = nPorts;
    _nEquations = 0;
    _equations = MatrixXcd::Zero(nCoeffs(), nCoeffs());
    _rhs = VectorXcd::Zero(nCoeffs());
}

int CalibrationEngine::nEquations() {
    return _nEquations;
}

int CalibrationEngine::nCoeffs() {
    return _nPorts*_nPorts*4;
}

int CalibrationEngine::nEquationsRequired() {
    return nCoeffs();
}

void CalibrationEngine::clearEquations() {
    _nEquations = 0;
}

#define T1(ROW, COL) equation(ROW*_nPorts + COL)
#define T2(ROW, COL) equation(TSize + ROW*_nPorts + COL)
#define T3(ROW, COL) equation(TSize*2 + ROW*_nPorts + COL)
#define T4(ROW, COL) equation(TSize*3 + ROW*_nPorts + COL)

void CalibrationEngine::addFullEquation(const MatrixXcd &actualSParams, const MatrixXcd &measuredSParams) {
    int TSize = _nPorts*_nPorts;
    const MatrixXcd& S = actualSParams;
    const MatrixXcd& M = measuredSParams;
    

    // T1*S + T2 - M*T3*S - M*T4 = 0
    // T1, T2, T3, T4 are unknowns
    // the rhs is a size nPort*nPort zero matrix

    // iterate through the entries of the rhs,
    // and add one equation for each entry
    for(int row=0;row<_nPorts;row++)
        for(int col=0;col<_nPorts;col++) {
            if(_nEquations >= nCoeffs()) throw logic_error("calibration engine: too many equations; required: " + to_string(nCoeffs()));
            VectorXcd equation = VectorXcd::Zero(nCoeffs());

            // + T1*S
            for(int i=0;i<_nPorts;i++) {
                T1(row, i) = S(i, col);
            }

            // + T2
            T2(row, col) = 1.;

            // - M*T3*S
            for(int i=0;i<_nPorts;i++)
                for(int j=0;j<_nPorts;j++) {
                    /* to derive these coefficients, play around in sympy:
                    from sympy import *
                    M = MatrixSymbol('M', 2, 2)
                    T = MatrixSymbol('T', 2, 2)
                    S = MatrixSymbol('S', 2, 2)
                    rhs = Matrix(M)*Matrix(T)*Matrix(S)

                    print expand(rhs[0,0])
                    print expand(rhs[0,1])
                    print expand(rhs[1,1])
                    */
                    T3(i, j) = -M(row,i)*S(j,col);
                }

            // - M*T4
            for(int i=0;i<_nPorts;i++) {
                T4(i,col) = -M(row,i);
            }

            _equations.row(_nEquations) = equation;
            _rhs(_nEquations) = 0.;
            _nEquations++;
        }
}

void CalibrationEngine::addOnePortEquation(const MatrixXcd &actualSParams, const MatrixXcd &measuredSParams, int n) {
    int TSize = _nPorts*_nPorts;
    const MatrixXcd& S = actualSParams;
    const MatrixXcd& M = measuredSParams;
    int col = n;
    for(int row=0;row<_nPorts;row++) {
        if(_nEquations >= nCoeffs()) throw logic_error("calibration engine: too many equations; required: " + to_string(nCoeffs()));
        VectorXcd equation = VectorXcd::Zero(nCoeffs());

        // + T1*S
        for(int i=0;i<_nPorts;i++) {
            T1(row, i) = S(i, col);
        }

        // + T2
        T2(row, col) = 1.;

        // - M*T3*S
        for(int i=0;i<_nPorts;i++)
            for(int j=0;j<_nPorts;j++) {
                T3(i, j) = -M(row,i)*S(j,col);
            }

        // - M*T4
        for(int i=0;i<_nPorts;i++) {
            T4(i,col) = -M(row,i);
        }

        _equations.row(_nEquations) = equation;
        _rhs(_nEquations) = 0.;
        _nEquations++;
    }
}

void CalibrationEngine::addNormalizingEquation() {
    int TSize = _nPorts*_nPorts;
    if(_nEquations >= nCoeffs()) throw logic_error("calibration engine: too many equations; required: " + to_string(nCoeffs()));
    VectorXcd equation = VectorXcd::Zero(nCoeffs());
    T4(0,0) = 1.;
    _equations.row(_nEquations) = equation;
    _rhs(_nEquations) = 1.;
    _nEquations++;
}

#undef T1
#undef T2
#undef T3
#undef T4

MatrixXcd CalibrationEngine::computeCoefficients() {
    //cout << _equations << endl;
    auto tmp = _equations.colPivHouseholderQr();
    if(tmp.rank() != nCoeffs()) throw runtime_error("matrix rank is not full! should be " + to_string(nCoeffs()) + ", is " + to_string(tmp.rank()));

    int TSize = _nPorts*_nPorts;

    VectorXcd res = tmp.solve(_rhs);
    MatrixXcd T = MatrixXcd::Zero(_nPorts*2, _nPorts*2);
    for(int i=0;i<4;i++)
        for(int row=0;row<_nPorts;row++)
            for(int col=0;col<_nPorts;col++) {
                int colOffset = int(i%2) * _nPorts;
                int rowOffset = int(i/2) * _nPorts;
                T(row+rowOffset, col+colOffset) = res(TSize*i + row*_nPorts + col);
            }
    return T;
}

MatrixXcd CalibrationEngine::computeSParams(const MatrixXcd &coeffs, const MatrixXcd &measuredSParams) {
    int _nPorts = measuredSParams.rows();
    MatrixXcd T1 = coeffs.topLeftCorner(_nPorts, _nPorts);
    MatrixXcd T2 = coeffs.topRightCorner(_nPorts, _nPorts);
    MatrixXcd T3 = coeffs.bottomLeftCorner(_nPorts, _nPorts);
    MatrixXcd T4 = coeffs.bottomRightCorner(_nPorts, _nPorts);
    MatrixXcd A = T1 - measuredSParams*T3;
    MatrixXcd rhs = measuredSParams*T4 - T2;
    return A.colPivHouseholderQr().solve(rhs);
}



}
