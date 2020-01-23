#include "calibrationfinetunedialog.H"
#include "ui_calibrationfinetunedialog.h"
#include "utility.H"

CalibrationFineTuneDialog::CalibrationFineTuneDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::CalibrationFineTuneDialog)
{
    ui->setupUi(this);
}

CalibrationFineTuneDialog::~CalibrationFineTuneDialog()
{
    delete ui;
}

void CalibrationFineTuneDialog::init(const VNACalibration *cal, const vector<vector<VNACalibratedValue> > &calStdModels, double startFreqHz, double stepFreqHz) {
    this->cal = cal;
    this->origModels = calStdModels;
    this->startFreqHz = startFreqHz;
    this->stepFreqHz = stepFreqHz;
    calStds = cal->getRequiredStandards();
    calStdShortIndex.clear();
    calStdOpenIndex.clear();
    calStdLoadIndex.clear();
    int i = 0;
    for(auto calstd: cal->getRequiredStandards()) {
        string name = calstd[0];
        if(name.size() > 5 && name.substr(0, 5) == "short")
            calStdShortIndex.push_back(i);
        if(name.size() > 4 && name.substr(0, 4) == "open")
            calStdOpenIndex.push_back(i);
        if(name.size() > 4 && name.substr(0, 4) == "load")
            calStdLoadIndex.push_back(i);
        i++;
    }
}

void CalibrationFineTuneDialog::toSettings(CalKitSettings &cks) {
    for(auto i: calStdLoadIndex)
        saveModel(cks, i);
    for(auto i: calStdOpenIndex)
        saveModel(cks, i);
    for(auto i: calStdShortIndex)
        saveModel(cks, i);
}

void CalibrationFineTuneDialog::saveModel(CalKitSettings &cks, int modelIndex) {
    int nPoints = (int) newModels.size();
    string name = calStds.at(modelIndex)[0];

    cks.calKitNames[name]; // create the entry if it doesn't exist

    auto& values = cks.calKitModels[name].values;
    values.clear();
    for(int i=0; i<nPoints; i++) {
        double freqHz = startFreqHz + i*stepFreqHz;
        values.insert({freqHz, newModels[i].at(modelIndex)});
    }
}

void CalibrationFineTuneDialog::on_s_short_valueChanged(int value) {
    updateModels();
}

void CalibrationFineTuneDialog::on_s_open_valueChanged(int value) {
    updateModels();
}

void CalibrationFineTuneDialog::on_s_load_valueChanged(int value) {
    updateModels();
}

void CalibrationFineTuneDialog::updateModels() {
    newModels = origModels;
    double offsShort = double(ui->s_short->value()) * 1e-14;
    double offsOpen = double(ui->s_open->value()) * 1e-14;

    // imaginary component of S11 @ 1GHz
    double parasitic = double(ui->s_load->value()) * 1e-5;
    complex<double> s11Load = {0., parasitic};
    complex<double> ZLoad = -50.*(s11Load+1.)/(s11Load-1.);
    complex<double> YLoad = 1./ZLoad;
    // series inductance
    double ind = capacitance_inductance(1e9, ZLoad.imag());
    // shunt capacitance
    double cap = capacitance_inductance_Y(1e9, YLoad.imag());

    ui->t_short->setText(qs(ssprintf(32, "%.2fps", offsShort*1e12)));
    ui->t_open->setText(qs(ssprintf(32, "%.2fps", offsOpen*1e12)));
    if(parasitic > 0)
        ui->t_load->setText(qs(ssprintf(32, "%.0fpH", ind*1e12)));
    else
        ui->t_load->setText(qs(ssprintf(32, "%.0ffF", -cap*1e15)));

    for(int i: calStdShortIndex)
        addLengthOffset(i, offsShort);
    for(int i: calStdOpenIndex)
        addLengthOffset(i, offsOpen);
    for(int i: calStdLoadIndex)
        addParasitic(i, parasitic);
    modelsChanged();
}

void CalibrationFineTuneDialog::addLengthOffset(int modelIndex, double offset) {
    for(size_t i=0; i<newModels.size(); i++) {
        double freqHz = startFreqHz + i * stepFreqHz;
        newModels[i].at(modelIndex)(0, 0) *= polar(1., -4*M_PI*freqHz*offset);
    }
}

void CalibrationFineTuneDialog::addParasitic(int modelIndex, double parasitic) {
    for(size_t i=0; i<newModels.size(); i++) {
        double freqHz = startFreqHz + i * stepFreqHz;
        auto& tmp = newModels[i].at(modelIndex)(0, 0);
        tmp = {tmp.real(), tmp.imag() + parasitic * freqHz * 1e-9};
    }
}


void CalibrationFineTuneDialog::on_b_r_short_clicked() {
    ui->s_short->setValue(0);
}
void CalibrationFineTuneDialog::on_b_r_open_clicked() {
    ui->s_open->setValue(0);
}
void CalibrationFineTuneDialog::on_b_r_load_clicked() {
    ui->s_load->setValue(0);
}
