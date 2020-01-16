#ifndef CONFIGUREVIEWDIALOG_H
#define CONFIGUREVIEWDIALOG_H

#include <QDialog>

namespace Ui {
class ConfigureViewDialog;
}

class ConfigureViewDialog : public QDialog
{
    Q_OBJECT

public:
    explicit ConfigureViewDialog(QWidget *parent = 0);
    ~ConfigureViewDialog();

private:
    Ui::ConfigureViewDialog *ui;
};

#endif // CONFIGUREVIEWDIALOG_H
