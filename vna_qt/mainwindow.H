#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <vector>
#include <exception>
#include <functional>
#include <map>
#include <string>
#include <xavna/common.H>
#include "calkitsettings.H"
#include "networkview.H"
#include "dtfwindow.H"
namespace Ui {
class MainWindow;
}
class QWidget;
class QPushButton;
class PolarView;
class ImpedanceDisplay;
class GraphPanel;

namespace QtCharts {
class QChartView;
class QChart;
class QValueAxis;
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


struct CalibrationInfo {
    int nPoints;
    double startFreqHz,stepFreqHz;
    int attenuation1,attenuation2;
    string calName;
    map<string, vector<VNARawValue> > measurements;
    CalKitSettings cks;
};
class MainWindow : public QMainWindow
{
    Q_OBJECT

private:
    Ui::MainWindow *ui;

public:
    const char* calButtonDoneStyle = "background-color: #9999ff";

    VNADevice* vna=NULL;
    ImpedanceDisplay* impdisp=NULL;
    QTimer* timer=NULL;
    PolarView* polarView=NULL;
    NetworkView nv;
    DTFWindow dtf;
    vector<GraphPanel*> graphs;
    bool viewIsTR = true;

    // array of values directly from the vna, updated every time a frequency point arrives
    vector<VNARawValue> rawValues;

    // for each reference standard, map from name to the measure button
    map<string, QWidget*> calButtons;
    // for each reference standard, map from name to measured values
    map<string, vector<VNARawValue> > calMeasurements;
    // current active calibration
    const VNACalibration* curCal = NULL;
    // current calibration coefficients
    vector<MatrixXcd> curCalCoeffs;
    // a copy of calMeasurements from when curCal was updated
    map<string, vector<VNARawValue> > curCalMeasurements;
    // the current global cal kit settings
    CalKitSettings cks;
    // the current length extension adjustment
    double portExt1Seconds = 0., portExt2Seconds = 0.;

    vector<complex<double> > tmp_s11;
    vector<VNACalibratedValue> tmp_sn1,tmp_sn2,tmp_sparams;
    QString tmp_fileName;

    QStringList recentFiles;
    int maxRecentFiles=10;
    vector<QAction*> recentFileActions;

    QByteArray layoutState;
    QByteArray unmaximizedLayoutState;
    QWidget* maximizedPane = nullptr;
    QWidget* maximizedPaneParent = nullptr;
    int maximizedPaneIndex = -1;

    time_t lastDTFUpdate=0;

    struct {
        bool enabled;
        bool calEnabled;
    } uiState {true, false};

    double freqScale = 1e-6;

    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();

    void loadSettings();

    void populateCalTypes();
    // setup the line graph and smith chart widgets, and populate this->views
    void setupViews();
    void destroyViews();
    // set the callbacks of this->vna
    void setCallbacks();
    // find devices and populate the devices menu
    void populateDevicesMenu();

    // try to use the device dev
    void openDevice(string dev);

    // call after changing frequency sweep parameters of vna
    void updateSweepParams();
    // update the impedance displays
    void updateValueDisplays();

    void updateUIState();
    void enableUI(bool enable);
    bool maximizePane(QWidget* w);
    QWidget* createTopFloat(QWidget* w);

    void setPortExtension(double t1_seconds, double t2_seconds);

    string serializeCalibration(const CalibrationInfo& cal);
    CalibrationInfo deserializeCalibration(QTextStream &inp);

    void saveFile(QString path, const string& data);
    void saveCalibration(QString path);
    void loadCalibration(QString path);

    void addRecentFile(QString path);
    void refreshRecentFiles();

    void captureSParam(vector<VNACalibratedValue>* var);

    QString fileDialogSave(QString title, QString filter, QString defaultSuffix, QString defaultFile="");


    string freqStr(double freqHz);

protected:
    void hideEvent(QHideEvent* event);
    void showEvent(QShowEvent* event);

private slots:
    void on_d_caltype_currentIndexChanged(int index);
    void btn_measure_click(QPushButton* btn);
    void on_actionOther_triggered();
    void calMeasurementCompleted(string calName);
    void on_b_clear_m_clicked();
    void on_b_apply_clicked();
    void on_b_clear_clicked();
    void on_actionSweep_params_triggered();

    // update a single point on all views in this->views
    void updateViews(int freqIndex);

    // called when the vna background thread encounters an error
    void handleBackgroundError(QString msg);

    // called after measurement complete when the user clicks "export s1p"
    void s11MeasurementCompleted(QString fileName);
    // called after measurement complete when the user clicks "export s2p"
    void sparamsMeasurementCompleted(QString fileName);
    void sMeasurementCompleted();

    void on_actionLoad_triggered();
    void on_actionSave_triggered();
    void export_s1p(int port);
    void on_actionExport_s1p_triggered();
    void on_actionExport_s1p_port_2_triggered();
    void on_actionCapture_S_1_triggered();
    void on_actionCapture_S_2_triggered();
    void on_actionExport_s2p_triggered();
    void on_actionImpedance_pane_toggled(bool arg1);
    void on_actionCalibration_pane_toggled(bool arg1);
    void on_dock_cal_visibilityChanged(bool visible);
    void on_dock_impedance_visibilityChanged(bool visible);
    void on_actionRefresh_triggered();
    void on_actionKit_settings_triggered();
    void on_actionMock_device_triggered();
    void on_menuDevice_aboutToShow();


    void on_actionTime_to_fault_toggled(bool arg1);
    void on_actionGraph_limits_triggered();
    void on_actionPort_length_extension_toggled(bool arg1);
    void on_slider_ext1_valueChanged(int value);
    void on_slider_ext2_valueChanged(int value);
    void on_dock_ext_visibilityChanged(bool visible);
    void on_b_reset_ext_clicked();
    void on_t_ext1_returnPressed();
    void on_t_ext2_returnPressed();
    void on_actionT_R_mode_toggled(bool checked);
    void on_actionSwap_ports_toggled(bool checked);
    void on_b_cal_help_clicked();
    void on_actionExport_csv_triggered();
};

#endif // MAINWINDOW_H
