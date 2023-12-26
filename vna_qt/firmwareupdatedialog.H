#ifndef FIRMWAREUPDATEDIALOG_H
#define FIRMWAREUPDATEDIALOG_H

#include <QDialog>
#include <string>
#include <stdio.h>

using namespace std;
namespace Ui {
class FirmwareUpdateDialog;
}
class FirmwareUpdater;
class FirmwareUpdateDialog : public QDialog
{
    Q_OBJECT

public:
    explicit FirmwareUpdateDialog(QWidget *parent = nullptr);
    virtual ~FirmwareUpdateDialog();

    void beginUploadFirmware(string dev, string file);

public slots:
    void updateProgress(int progress);

private:
    Ui::FirmwareUpdateDialog *ui;
    FirmwareUpdater* updater = nullptr;
    //int _fd = -1;
    FILE* filePtr = nullptr;
};

#endif // FIRMWAREUPDATEDIALOG_H
