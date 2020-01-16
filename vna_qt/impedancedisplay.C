#include "utility.H"
#include "impedancedisplay.H"
#include "ui_impedancedisplay.h"


ImpedanceDisplay::ImpedanceDisplay(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::ImpedanceDisplay)
{
    ui->setupUi(this);
}

ImpedanceDisplay::~ImpedanceDisplay()
{
    delete ui;
}

void ImpedanceDisplay::setValue(complex<double> s11, double freqHz, double z0) {
    complex<double> Z = -z0*(s11+1.)/(s11-1.);
    complex<double> Y = -(s11-1.)/(z0*(s11+1.));

    ui->l_impedance->setText(qs(ssprintf(127, "  %.2f\n%s j%.2f", Z.real(), Z.imag()>=0 ? "+" : "-", fabs(Z.imag()))));
    ui->l_admittance->setText(qs(ssprintf(127, "  %.4f\n%s j%.4f", Y.real(), Y.imag()>=0 ? "+" : "-", fabs(Y.imag()))));
    ui->l_s_admittance->setText(qs(ssprintf(127, "  %.4f\n%s j%.4f", 1./Z.real(), Z.imag()>=0 ? "+" : "-", fabs(1./Z.imag()))));
    ui->l_p_impedance->setText(qs(ssprintf(127, "  %.2f\n|| j%.2f", 1./Y.real(), 1./Y.imag())));

    double value = capacitance_inductance(freqHz, Z.imag());
    ui->l_series->setText(qs(ssprintf(127, "%.2f Ω\n%.2f %s%s", Z.real(), fabs(si_scale(value)), si_unit(value), value>0?"H":"F")));

    value = capacitance_inductance_Y(freqHz, Y.imag());
    ui->l_parallel->setText(qs(ssprintf(127, "%.2f Ω\n%.2f %s%s", 1./Y.real(), fabs(si_scale(value)), si_unit(value), value>0?"H":"F")));
}

void ImpedanceDisplay::clearValue() {
    QString p = "-";
    ui->l_impedance->setText(p);
    ui->l_admittance->setText(p);
    ui->l_s_admittance->setText(p);
    ui->l_p_impedance->setText(p);
    ui->l_series->setText(p);
    ui->l_parallel->setText(p);
}
