#include "firmwareupdatedialog.H"
#include "ui_firmwareupdatedialog.h"
#include "firmwareupdater.H"
#include "utility.H"
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <QMessageBox>

FirmwareUpdateDialog::FirmwareUpdateDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::FirmwareUpdateDialog)
{
    ui->setupUi(this);
}

FirmwareUpdateDialog::~FirmwareUpdateDialog()
{
    delete ui;
}

void FirmwareUpdateDialog::beginUploadFirmware(string dev, string file) {
    ui->buttonBox->setDisabled(true);
    _fd = ::open(file.c_str(), O_RDONLY);
    if(_fd < 0)
        throw runtime_error(strerror(errno));

    updater = new FirmwareUpdater();
    updater->open(dev.c_str());
    updater->beginUploadFirmware(0x08004000, [this](uint8_t* buf, int len) {
        return int(::read(_fd, buf, size_t(len)));
    },
    [this](int progress){
        QMetaObject::invokeMethod(this, "updateProgress", Qt::QueuedConnection, Q_ARG(int, progress));
    });
}

void FirmwareUpdateDialog::updateProgress(int progress) {
    if(progress == -1) {
        auto ex = updater->endUploadFirmware();
        if(ex == nullptr)
            updater->reboot();

        updater->close();
        delete updater;
        updater = nullptr;

        if(ex != nullptr) {
            QString msg = "An error occurred during firmware update:\n\n";
            msg += ex->what();
            msg += "\n\nPlease retry the update.\nYou can re-enter DFU mode by holding down the JOG LEFT button and power cycling the device.";
            QMessageBox::critical(this, "Error", msg);
            this->accept();
        } else {
            ui->l_progress->setText("Done");
            ui->buttonBox->setDisabled(false);
        }
        return;
    }
    ui->l_progress->setText(qs(ssprintf(128, "%d KiB", progress)));
}
