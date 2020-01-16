#define _USE_MATH_DEFINES
#include "mainwindow.H"
#include "ui_mainwindow.h"
#include <iostream>

#include <QMessageBox>
#include <QGraphicsLayout>
#include <QtCharts/QChart>
#include <QtCharts/QChartView>
#include <QtCharts/QLineSeries>
#include <QtCharts/QValueAxis>
#include <QtCharts/QScatterSeries>

#include <xavna/xavna_cpp.H>
using namespace xaxaxa;
using namespace std;

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    vna = new VNADevice();
    vna->startFreqHz = 35e6;
    vna->nPoints = 100;
    rawValues.resize(vna->nPoints);
    series.resize(4);

    setCallbacks();

    //                      reference  refl   			   thru
    vector<QColor> colors {Qt::blue, Qt::green, Qt::yellow, Qt::red};



    chart = new QChart();
    chartView = new QChartView();
    ui->w_content->layout()->addWidget(chartView);

    chartView->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);
    chartView->setRenderHint(QPainter::Antialiasing);
    chartView->setChart(chart);
    chartView->setStyleSheet("background-color:#000");



    axisX = new QValueAxis;
    axisX->setTickCount(10);
    axisX->setLabelsAngle(-90);
    axisX->setMin(freqAt(0));
    axisX->setMax(freqAt(vna->nPoints-1));
    chart->addAxis(axisX, Qt::AlignBottom);


    axisY = new QValueAxis;
    axisY->setTickCount(12);
    //axisY->setLinePenColor(Qt::red);
    axisY->setMin(-80);
    axisY->setMax(30);
    chart->addAxis(axisY, Qt::AlignLeft);

    for(int i=0;i<(int)series.size();i++) {
        series[i] = new QLineSeries();
        series[i]->setPen(QPen(colors.at(i), 2.));
        //series[i]->setUseOpenGL(true);
        chart->addSeries(series[i]);
        chart->legend()->hide();
        chart->layout()->setContentsMargins(0, 0, 0, 0);
        chart->setBackgroundRoundness(0);
        //chart->setBackgroundBrush(Qt::red);
        //chart->setMargins(QMargins(0,0,0,0));
        series[i]->attachAxis(axisX);
        series[i]->attachAxis(axisY);
        for(int j=0;j<vna->nPoints;j++) {
            series[i]->append(freqAt(j), 0);
        }
    }
}

double MainWindow::freqAt(int i) {
    return vna->freqAt(i)/1e6;
}

void MainWindow::populateDevicesMenu() {
    ui->menuDevice->clear();

    vector<string> devices;
    try {
        devices = vna->findDevices();
        for(string dev:devices) {
            QAction* action = new QAction(qs("   " + dev));
            connect(action, &QAction::triggered, [this,dev](){
                this->openDevice(dev);
            });
            ui->menuDevice->addAction(action);
        }
    } catch(exception& ex) {
        cerr << ex.what() << endl;
    }
    if(devices.empty()) {
        QAction* action = new QAction("   No devices found; check dmesg or device manager");
        action->setEnabled(false);
        ui->menuDevice->addAction(action);
    }
}


void MainWindow::openDevice(string dev) {
    try {
        vna->open(dev);
        vna->startScan();
    } catch(exception& ex) {
        QMessageBox::critical(this,"Exception",ex.what());
    }
}

void MainWindow::setCallbacks() {
    vna->frequencyCompletedCallback = [this](int freqIndex, VNARawValue val) {
    };
    vna->sweepCompletedCallback = [this](const vector<VNARawValue>&) {
    };
    vna->frequencyCompletedCallback2_ = [this](int freqIndex, const vector<array<complex<double>, 4> >& values) {
        this->rawValues[freqIndex] = values;
        QMetaObject::invokeMethod(this, "updateViews", Qt::QueuedConnection, Q_ARG(int, freqIndex));
    };

    vna->backgroundErrorCallback = [this](const exception& exc) {
        fprintf(stderr,"background thread error: %s\n",exc.what());
        QString msg = exc.what();
        QMetaObject::invokeMethod(this, "handleBackgroundError", Qt::QueuedConnection, Q_ARG(QString, msg));
    };
}


void MainWindow::updateViews(int freqIndex) {

    fflush(stderr);
    if(freqIndex >= (int)rawValues.size()) return;
    
    int exIndex = excitation;

    double offset = 0; //-193;
    double y = dB(norm(rawValues.at(freqIndex)[exIndex][0])) + offset;
    series[0]->replace(freqIndex, series[0]->at(freqIndex).x(), y);
    printf("%f %f\n", series[0]->at(freqIndex).x(), y);

    y = dB(norm(rawValues.at(freqIndex)[exIndex][1])) + offset;
    series[1]->replace(freqIndex, series[0]->at(freqIndex).x(), y);
    
    y = dB(norm(rawValues.at(freqIndex)[exIndex][2])) + offset;
    series[2]->replace(freqIndex, series[0]->at(freqIndex).x(), y);

    y = dB(norm(rawValues.at(freqIndex)[exIndex][3])) + offset;
    series[3]->replace(freqIndex, series[0]->at(freqIndex).x(), y);
}

void MainWindow::handleBackgroundError(QString msg) {
    vna->close();
    QMessageBox::critical(this, "Error", msg);
}

void MainWindow::on_menuDevice_aboutToShow() {
    populateDevicesMenu();
}

MainWindow::~MainWindow()
{
    vna->stopScan();
    vna->close();
    delete vna;
    delete ui;
}

inline string ssprintf(int maxLen, const char* fmt, ...) {
    string tmp(maxLen, '\0');
    va_list args;
    va_start(args, fmt);
    int len = vsnprintf((char*)tmp.data(), maxLen, fmt, args);
    va_end(args);
    tmp.resize(len);
    return tmp;
}
void MainWindow::on_slider_valueChanged(int value) {
    vna->attenuation1 = vna->attenuation2 = vna->maxPower() - value;
    ui->label->setText(qs(ssprintf(32, "%d dBm", value)));
}

void MainWindow::on_r_port1_clicked() {
    excitation = 0;
}

void MainWindow::on_r_port2_clicked() {
    excitation = 1;
}
