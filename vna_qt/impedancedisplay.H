#ifndef IMPEDANCEDISPLAY_H
#define IMPEDANCEDISPLAY_H

#include <QWidget>
#include <complex>

using namespace std;
namespace Ui {
class ImpedanceDisplay;
}

class ImpedanceDisplay : public QWidget
{
    Q_OBJECT

public:
    explicit ImpedanceDisplay(QWidget *parent = 0);
    ~ImpedanceDisplay();

    void setValue(complex<double> s11, double freqHz, double z0=50.);
    void clearValue();

private:
    Ui::ImpedanceDisplay *ui;
};

#endif // IMPEDANCEDISPLAY_H
