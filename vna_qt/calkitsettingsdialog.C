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
        auto& inf = info[name];
        if(calStdDesc.find(name) != calStdDesc.end())
            desc = calStdDesc[name];

        Ui::CalKitSettingsWidget& ui1 = inf.ui;
        QWidget* w = new QWidget();
        ui1.setupUi(w);
        layout->addWidget(w);

        ui1.l_desc->setText(qs(desc));

        auto it = settings.calKitModels.find(name);
        if(it != settings.calKitModels.end() && (*it).second.values.size() != 0) {
            ui1.r_s_param->setChecked(true);
            const SParamSeries& series = (*it).second;
            inf.data = series;
            inf.useIdeal = false;
            auto it2 = settings.calKitNames.find(name);
            if(it2 != settings.calKitNames.end())
                inf.fileName = (*it2).second;
            ui1.l_status->setText(generateLabel(inf));
        } else {
            inf.useIdeal = true;
            ui1.l_status->setText("");
        }

        connect(ui1.r_ideal, &QRadioButton::clicked, [this, name](){
            auto& inf = info[name];
            inf.useIdeal = true;
            inf.ui.l_status->setText("");
        });

        connect(ui1.r_s_param, &QRadioButton::clicked, [this, name](){
            auto& inf = info[name];
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
                        inf.useIdeal = false;
                        inf.data = series;
                        inf.fileName = fileName.toStdString();
                        inf.ui.l_status->setText(generateLabel(inf));
                    } catch(exception& ex) {
                        QMessageBox::warning(this, tr("Error parsing S parameter file"), ex.what());
                        goto fail;
                    }
                }
            }
            return;
        fail:
            // revert radiobutton state
            inf.ui.r_ideal->setChecked(inf.useIdeal);
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
            settings.calKitNames[name] = (*it).second.fileName;
        }
    }
}

QString CalKitSettingsDialog::generateLabel(const CalKitSettingsDialog::calKitInfo &inf) {
    QString status;
    status = qs(inf.fileName);
    if(status == "")
        status = "<no filename>";

    status = status.toHtmlEscaped();

    double startFreqHz = inf.data.values.begin()->first;
    double stopFreqHz = inf.data.values.rbegin()->first;
    status = "<pre>" + status
            + qs(ssprintf(256, "\n   <b>%8.3f</b> MHz - <b>%8.3f</b> MHz, <b>%d</b> points</pre>",
                               startFreqHz*1e-6, stopFreqHz*1e-6, (int)inf.data.values.size()));
    return status;
}
