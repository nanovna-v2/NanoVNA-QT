#include "utility.H"
#include "frequencydialog.H"
#include "ui_frequencydialog.h"
#include <xavna/xavna_cpp.H>
#include <xavna/workarounds.H>

using namespace xaxaxa;
FrequencyDialog::FrequencyDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::FrequencyDialog)
{
    ui->setupUi(this);
    ui->w_advanced->setVisible(false);
    this->resize(this->width(),0);
    this->setWindowTitle("Sweep Parameters");
}

FrequencyDialog::~FrequencyDialog()
{
    delete ui;
}

void FrequencyDialog::fromVNA(const VNADevice &dev) {
    ui->t_start->setValue(dev.startFreqHz*1e-6);
    ui->t_stop->setValue(dev.startFreqHz*1e-6 + dev.stepFreqHz*1e-6 *(dev.nPoints - 1));
    ui->t_points->setValue(dev.nPoints);
    ui->slider_power->setRange(dev.maxPower()-40, dev.maxPower());
    ui->slider_power->setValue(dev.maxPower() - dev.attenuation1);
    ui->t_nValues->setText(qs(ssprintf(32, "%d", dev.nValues)));
    ui->t_nWait->setText(qs(ssprintf(32, "%d", dev.nWait)));
    emit on_slider_power_valueChanged(ui->slider_power->value());
}

bool FrequencyDialog::toVNA(VNADevice &dev) {
    double oldStartFreq = dev.startFreqHz;
    double oldStepFreq = dev.stepFreqHz;
    double oldNPoints = dev.nPoints;
    dev.startFreqHz = ui->t_start->value()*1e6;
    double stopFreqHz = ui->t_stop->value()*1e6;

    if(stopFreqHz <= dev.startFreqHz){ //sanity check
        stopFreqHz=dev.startFreqHz + 1e6;
        QMessageBox::critical(this, "Error","Invalid Stop Frequency");
    }

    dev.nPoints = ui->t_points->value();
    dev.stepFreqHz=(stopFreqHz - dev.startFreqHz)/(dev.nPoints - 1); //-1 to not skip the last step

    dev.attenuation1 = dev.attenuation2 = dev.maxPower() - ui->slider_power->value();
    dev.nValues = atoi(ui->t_nValues->text().toUtf8().data());
    dev.nWait = atoi(ui->t_nWait->text().toUtf8().data());
    return (dev.startFreqHz != oldStartFreq) || (dev.stepFreqHz != oldStepFreq) || (dev.nPoints != oldNPoints);
}

void FrequencyDialog::updateLabels() {
    double stepSize = ( ui->t_stop->value() - ui->t_start->value()) / ui->t_points->value();

    if(!std::isnan(stepSize) && stepSize>0)
        ui->l_end->setText(qs(ssprintf(32, "%.2f", stepSize)));
}

void FrequencyDialog::on_slider_power_valueChanged(int value) {
    ui->l_power->setText(qs(ssprintf(32, "%d dBm", value)));
}

void FrequencyDialog::on_t_start_valueChanged(const QString &) {
    updateLabels();
}

void FrequencyDialog::on_t_stop_valueChanged(const QString &) {
    updateLabels();
}

void FrequencyDialog::on_t_points_valueChanged(const QString &) {
    updateLabels();
}

void FrequencyDialog::on_c_advanced_stateChanged(int) {
    ui->w_advanced->setVisible(ui->c_advanced->isChecked());
}
