#ifndef CALIBRATIONFINETUNEDIALOG_H
#define CALIBRATIONFINETUNEDIALOG_H

#include <QDialog>
#include <QAbstractButton>
#include <vector>
#include <functional>
#include <xavna/common.H>
#include <xavna/calibration.H>
#include "calkitsettings.H"
using namespace std;
using namespace xaxaxa;

namespace Ui {
class CalibrationFineTuneDialog;
}

class CalibrationFineTuneDialog : public QDialog
{
    Q_OBJECT

public:
    // holds the original models supplied to the init() function.
    vector<vector<VNACalibratedValue> > origModels;

    // holds the current adjusted models when modelsChanged() is called or after dialog is closed.
    vector<vector<VNACalibratedValue> > newModels;

    // called when the user makes an adjustment.
    function<void()> modelsChanged;

    explicit CalibrationFineTuneDialog(QWidget *parent = nullptr);
    ~CalibrationFineTuneDialog();


    // calStdModels: indexed by [freqIndex][modelIndex],
    // where modelIndex is the index of the calibration standard corresponding to cal->getRequiredStandards()
    void init(const VNACalibration* cal, const vector<vector<VNACalibratedValue> >& calStdModels, double startFreqHz, double stepFreqHz);

    // update cal kit settings with adjusted parameters
    void toSettings(CalKitSettings& cks);
    void saveModel(CalKitSettings& cks, int modelIndex);

private slots:
    void on_s_short_valueChanged(int value);
    void on_s_open_valueChanged(int value);

    void on_b_r_short_clicked();

    void on_b_r_open_clicked();

    void on_s_load_valueChanged(int value);

    void on_b_r_load_clicked();

private:
    Ui::CalibrationFineTuneDialog *ui;
    const VNACalibration* cal;
    double startFreqHz;
    double stepFreqHz;

    // list of all indices of origModels that are short, open, and load
    vector<int> calStdShortIndex, calStdOpenIndex, calStdLoadIndex;

    vector<array<string, 2> > calStds;

    void updateModels();
    void addLengthOffset(int modelIndex, double offset);
    // parasitic is the imaginary component of S11 @ 1GHz
    void addParasitic(int modelIndex, double parasitic);
};

#endif // CALIBRATIONFINETUNEDIALOG_H
