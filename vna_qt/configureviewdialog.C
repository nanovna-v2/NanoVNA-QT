#include "configureviewdialog.H"
#include "ui_configureviewdialog.h"

ConfigureViewDialog::ConfigureViewDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::ConfigureViewDialog)
{
    ui->setupUi(this);
}

ConfigureViewDialog::~ConfigureViewDialog()
{
    delete ui;
}
