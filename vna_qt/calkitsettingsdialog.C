#include <map>
#include <QLayout>
#include <QFileDialog>
#include <QMessageBox>
#include <QTextStream>
#include <xavna/calibration.H>
#include "calkitsettingsdialog.H"
#include "ui_calkitsettingsdialog.h"
#include "ui_calkitsettingswidget.h"
#include "utility.H"
#include "touchstone.H"

using namespace xaxaxa;
using namespace std;

CalKitSettingsDialog::CalKitSettingsDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::CalKitSettingsDialog)
{
    ui->setupUi(this);
}

CalKitSettingsDialog::~CalKitSettingsDialog()
{
    delete ui;
}

void CalKitSettingsDialog::fromSettings(const CalKitSettings &settings) {
    info.clear();
    map<string, string> calStdDesc;
    for(const VNACalibration* cal: calibrationTypes) {
        for(auto tmp:cal->getRequiredStandards()) {
            calStdDesc[tmp[0]] = tmp[1];
        }
    }

    QBoxLayout* layout = new QBoxLayout(QBoxLayout::TopToBottom);
    //layout->setMargin(0);
    delete ui->w_content->layout();
    ui->w_content->setLayout(layout);

    for(auto& item:idealCalStds) {
        string name = item.first;
        string desc = name;
        if(calStdDesc.find(name) != calStdDesc.end())
            desc = calStdDesc[name];

        Ui::CalKitSettingsWidget& ui1 = info[name].ui;
        QWidget* w = new QWidget();
        ui1.setupUi(w);
        layout->addWidget(w);

        ui1.l_desc->setText(qs(desc));

        auto it = settings.calKitModels.find(name);
        if(it != settings.calKitModels.end()) {
            ui1.r_s_param->setChecked(true);
            info[name].data = (*it).second;
            info[name].useIdeal = false;
            auto it2 = settings.calKitNames.find(name);
            if(it2 != settings.calKitNames.end())
                ui1.l_status->setText(qs((*it2).second));
        } else info[name].useIdeal = true;

        connect(ui1.r_ideal, &QRadioButton::clicked, [this, name](){
            info[name].useIdeal = true;
            info[name].ui.l_status->setText("");
        });

        connect(ui1.r_s_param, &QRadioButton::clicked, [this, name](){
            QString fileName = QFileDialog::getOpenFileName(this,
                    tr("Open S parameters file"), "",
                    tr("S parameters (*.s1p *.s2p);;All Files (*)"));
            if (fileName.isEmpty()) goto fail;
            {
                QFile file(fileName);
                if (!file.open(QIODevice::ReadOnly)) {
                    QMessageBox::warning(this, tr("Unable to open file"), file.errorString());
                    goto fail;
                }
                {
                    QTextStream stream(&file);
                    string data = stream.readAll().toStdString();

                    SParamSeries series;
                    int nPorts;
                    try {
                        QFileInfo fileInfo(fileName);
                        parseTouchstone(data,nPorts,series.values);
                        info[name].useIdeal = false;
                        info[name].data = series;
                        info[name].ui.l_status->setText(fileInfo.fileName());
                    } catch(exception& ex) {
                        QMessageBox::warning(this, tr("Error parsing S parameter file"), ex.what());
                        goto fail;
                    }
                }
            }
            return;
        fail:
            // revert radiobutton state
            info[name].ui.r_ideal->setChecked(info[name].useIdeal);
        });
    }
}

void CalKitSettingsDialog::toSettings(CalKitSettings &settings) {
    settings.calKitModels.clear();
    settings.calKitNames.clear();
    for(auto& item:idealCalStds) {
        string name = item.first;
        auto it = info.find(name);
        if(it == info.end()) continue;
        if(!(*it).second.useIdeal) {
            settings.calKitModels[name] = (*it).second.data;
            settings.calKitNames[name] = (*it).second.ui.l_status->text().toStdString();
        }
    }
}
