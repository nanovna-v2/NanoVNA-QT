#include "utility.H"
#include "graphpanel.H"
#include "ui_graphpanel.h"
#include <QGraphicsLayout>
#include <QtCharts/QChart>
#include <QtCharts/QChartView>
#include <QtCharts/QLineSeries>
#include <QtCharts/QValueAxis>
#include <QtCharts/QScatterSeries>

using namespace QtCharts;
GraphPanel::GraphPanel(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::GraphPanel)
{
    ui->setupUi(this);

    chart = new QChart();
    chartView = new QChartView();
    this->layout()->addWidget(chartView);

    chartView->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);
    chartView->setRenderHint(QPainter::Antialiasing);
    chartView->setChart(chart);
    chartView->setStyleSheet("background-color:#000");



    axisX = new QValueAxis;
    axisX->setTickCount(10);
    axisX->setLabelsAngle(-90);
    chart->addAxis(axisX, Qt::AlignBottom);



    series.resize(2);
    axisY.resize(2);
    for(int i=0;i<(int)series.size();i++) {
        axisY[i] = new QValueAxis;
        axisY[i]->setTickCount(12);
        axisY[i]->setLinePenColor(i==0?Qt::red:Qt::blue);
        axisY[i]->setMin(-80);
        axisY[i]->setMax(30);
        chart->addAxis(axisY[i], i==0?Qt::AlignLeft:Qt::AlignRight);

        series[i] = new QLineSeries();
        series[i]->setPen(QPen(axisY[i]->linePenColor(), 2.));
        chart->addSeries(series[i]);
        chart->legend()->hide();
        chart->layout()->setContentsMargins(0, 0, 0, 0);
        chart->setBackgroundRoundness(0);
        //chart->setBackgroundBrush(Qt::red);
        //chart->setMargins(QMargins(0,0,0,0));
        series[i]->attachAxis(axisX);
        series[i]->attachAxis(axisY[i]);
    }
}

GraphPanel::~GraphPanel()
{
    delete ui;
}

void GraphPanel::populateComboBox(int index, const vector<string> &items) {
    QComboBox* combo = comboBox(index);
    combo->clear();
    for(string item:items) {
        combo->addItem(qs(item));
    }
}

QComboBox *GraphPanel::comboBox(int index) {
    if(index == 0) return ui->d1;
    return ui->d2;
}

QPushButton *GraphPanel::maximizeButton() {
    return ui->b_maximize;
}

void GraphPanel::on_d1_currentIndexChanged(int index) {
    emit comboBoxSelectionChanged(0,index);
}

void GraphPanel::on_d2_currentIndexChanged(int index) {
    emit comboBoxSelectionChanged(1,index);
}
