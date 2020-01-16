#ifndef DTFWINDOW_H
#define DTFWINDOW_H

#include <QMainWindow>
#include <fftw3.h>
#include "networkview.H"

namespace Ui {
class DTFWindow;
}

class DTFWindow : public QMainWindow
{
    Q_OBJECT

public:
    NetworkView nv;
    fftw_plan p;
    complex<double>* fft_in = nullptr;
    complex<double>* fft_out = nullptr;
    complex<double>* fft_window = nullptr;

    double timeScale = 1e9;
    explicit DTFWindow(QWidget *parent = 0);
    ~DTFWindow();

    void initFFT(int sz);
    void deinitFFT();
    void updateSweepParams(double stepFreqHz, int nPoints);
    void updateValues(const vector<VNACalibratedValue>& freqDomainValues);
signals:
    void hidden();
protected:
    void closeEvent(QCloseEvent *event);
private:
    Ui::DTFWindow *ui;
};

#endif // DTFWINDOW_H
