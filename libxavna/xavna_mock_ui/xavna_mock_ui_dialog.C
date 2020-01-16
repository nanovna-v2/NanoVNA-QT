#include "xavna_mock_ui_dialog.H"
#include "ui_xavna_mock_ui_dialog.h"
#include <string>
#include <stdio.h>
#include <xavna/workarounds.H>


using namespace std;
inline string ssprintf(int maxLen, const char* fmt, ...) {
    string tmp(maxLen, '\0');
    va_list args;
    va_start(args, fmt);
    vsnprintf((char*)tmp.data(), maxLen, fmt, args);
    va_end(args);
    return tmp;
}

xavna_mock_ui_dialog::xavna_mock_ui_dialog(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::xavna_mock_ui_dialog)
{
    ui->setupUi(this);

    for(auto& btn:ui->buttonGroup->buttons()) {
        connect(btn, SIGNAL(clicked()), SLOT(cb_changed()));
    }
    this->setWindowFlags(Qt::WindowStaysOnTopHint);
    cb = [](string,double,double){};
}

xavna_mock_ui_dialog::~xavna_mock_ui_dialog() {
    delete ui;
}

void xavna_mock_ui_dialog::cb_changed() {
    this->cb(ui->buttonGroup->checkedButton()->text().toStdString(), ui->slider1->value(), ui->slider2->value());
}

void set_text(QLabel* l, string text) {
    QString tmp = QString::fromStdString(text);
    l->setText(tmp);
}
void xavna_mock_ui_dialog::on_slider1_valueChanged(int value) {
    set_text(ui->l_slider1, to_string(value));
    cb_changed();
}
void xavna_mock_ui_dialog::on_slider2_valueChanged(int value) {
    set_text(ui->l_slider2, to_string(value));
    cb_changed();
}

