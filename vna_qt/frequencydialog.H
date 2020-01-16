#ifndef FREQUENCYDIALOG_H
#define FREQUENCYDIALOG_H

#include <QDialog>
#include <QMessageBox>

namespace Ui {
class FrequencyDialog;
}
namespace xaxaxa {
class VNADevice;
}
using namespace xaxaxa;
class FrequencyDialog : public QDialog
{
    Q_OBJECT

public:
    explicit FrequencyDialog(QWidget *parent = 0);
    ~FrequencyDialog();

    // populate the UI with parameters from dev
    void fromVNA(const VNADevice& dev);

    // update dev with parameters from the UI
    // returns true if frequency parameters were changed, false otherwise
    bool toVNA(VNADevice& dev);

    void updateLabels();

private slots:
    void on_slider_power_valueChanged(int value);
    void on_t_start_valueChanged(const QString &arg1);
    void on_t_stop_valueChanged(const QString &arg1);
    void on_t_points_valueChanged(const QString &arg1);

    void on_c_advanced_stateChanged(int);

private:
    Ui::FrequencyDialog *ui;
};

#endif // FREQUENCYDIALOG_H
