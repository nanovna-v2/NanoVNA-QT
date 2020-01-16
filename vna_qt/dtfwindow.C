#define _USE_MATH_DEFINES
#include "dtfwindow.H"
#include "ui_dtfwindow.h"
#include "graphpanel.H"
#include "utility.H"
#include <math.h>
#include <QPushButton>
#include <QHideEvent>

DTFWindow::DTFWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::DTFWindow)
{
    ui->setupUi(this);
    nv.init(ui->w_bottom->layout());
    nv.xAxisValueStr = [](double val) {
        return ssprintf(32, "%.3lf ns", val);
    };

    GraphPanel* gp = nv.createGraphView(false, false);
    gp->maximizeButton()->hide();
    ui->w_graph->layout()->addWidget(gp);

    nv.addMarker(false);
}

DTFWindow::~DTFWindow()
{
    delete ui;
}

inline double gauss(double x, double m, double s) {
    static const double inv_sqrt_2pi = 0.3989422804014327;
    double a = (x - m) / s;
    return inv_sqrt_2pi / s * std::exp(-0.5 * a * a);
}

void DTFWindow::initFFT(int sz) {
    fft_in = (complex<double>*) fftw_malloc(sizeof(complex<double>) * sz);
    fft_out = (complex<double>*) fftw_malloc(sizeof(complex<double>) * sz);
    fft_window = (complex<double>*) fftw_malloc(sizeof(complex<double>) * sz);
    p = fftw_plan_dft_1d(sz, (fftw_complex*)fft_in, (fftw_complex*)fft_out, FFTW_BACKWARD, 0);

    /*double raisedCosineWidth = 0.1;
    for(int i=0; i<sz; i++) {
        double x = (double(i)+0.5)/sz;
        if(x > 0.5) x = 1. - x;
        if(x > raisedCosineWidth) fft_window[i] = 1.;
        else {
            double tmp = -cos(x*M_PI/raisedCosineWidth);
            fft_window[i] = (tmp + 1) * 0.5;
        }
    }*/
    for(int i=0;i<sz;i++) {
        double val = gauss(i-sz/2, 0, sz/5);
        fft_window[i] = val;
    }
/*
    memcpy(fft_in, fft_window, sz*sizeof(fftw_complex));
    fftw_execute(p);
    memcpy(fft_window, fft_out, sz*sizeof(fftw_complex));*/
}

void DTFWindow::deinitFFT() {
    if(fft_in != nullptr) {
        fftw_destroy_plan(p);
        fftw_free(fft_in);
        fftw_free(fft_out);
        fftw_free(fft_window);
        fft_in = fft_out = fft_window = nullptr;
    }
}

void DTFWindow::updateSweepParams(double stepFreqHz, int nPoints) {
    double stepTimeSec = 1./(stepFreqHz*nPoints);
    if(int(nv.values.size()) != nPoints) {
        deinitFFT();
        initFFT(nPoints);
    }
    nv.updateXAxis(0, stepTimeSec*timeScale, nPoints);
}

void DTFWindow::updateValues(const vector<VNACalibratedValue> &freqDomainValues) {
    int sz = (int)freqDomainValues.size();
    assert(sz == (int)nv.values.size());
    for(int row=0;row<2;row++)
        for(int col=0;col<2;col++) {
            for(int i=0;i<sz;i++)
                fft_in[i] = freqDomainValues.at(i)(row,col)*fft_window[i];
            fftw_execute(p);
            for(int i=0;i<sz;i++)
                nv.values.at(i)(row,col) = fft_out[i];
        }
    nv.updateViews(-1);
    nv.updateBottomLabels();
    nv.updateMarkerViews();
}

void DTFWindow::closeEvent(QCloseEvent *) {
    emit hidden();
}
