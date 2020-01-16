#include "markerslider.H"
#include "ui_markerslider.h"

MarkerSlider::MarkerSlider(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::MarkerSlider)
{
    ui->setupUi(this);
    labels = {ui->l1, ui->l2, ui->l3, ui->l4};
}

MarkerSlider::~MarkerSlider()
{
    delete ui;
}

void MarkerSlider::setLabelText(int index, string text) {
    labels.at(index)->setText(QString::fromStdString(text));
    labels.at(index)->setVisible(text!="");
}

