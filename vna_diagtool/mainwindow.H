#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <xavna/xavna_cpp.H>
#include <math.h>
namespace Ui {
class MainWindow;
}

namespace QtCharts {
class QChartView;
class QChart;
class QValueAxis;
class QLineSeries;
}
namespace xaxaxa {
class VNADevice;
class VNACalibration;
}
class QTimer;
class MarkerSlider;
class QTextStream;

using namespace QtCharts;
using namespace xaxaxa;
using namespace std;

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();

    void populateDevicesMenu();
    void openDevice(string dev);
    double freqAt(int i);
    void setCallbacks();
public slots:
    void updateViews(int freqIndex);
    void handleBackgroundError(QString msg);
    void on_menuDevice_aboutToShow();
private slots:
    void on_slider_valueChanged(int value);

    void on_r_port1_clicked();

    void on_r_port2_clicked();

private:
    Ui::MainWindow *ui;
    VNADevice* vna=NULL;
    int excitation=0;

    vector<vector<array<complex<double>, 4> > > rawValues;


    QChart* chart;
    QChartView* chartView;
    vector<QLineSeries*> series;
    QValueAxis* axisX;
    QValueAxis* axisY;
};

inline QString qs(const string& s) {
    return QString::fromStdString(s);
}

inline double dB(double power) {
    return log10(power)*10;
}

#endif // MAINWINDOW_H
