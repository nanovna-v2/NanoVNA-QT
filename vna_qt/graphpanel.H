#ifndef GRAPHPANEL_H
#define GRAPHPANEL_H

#include <QWidget>
#include <vector>

namespace Ui {
class GraphPanel;
}
namespace QtCharts {
class QChartView;
class QChart;
class QValueAxis;
class QLineSeries;
}
class QComboBox;
class QPushButton;

using namespace QtCharts;
using namespace std;
class GraphPanel : public QWidget
{
    Q_OBJECT

public:
    explicit GraphPanel(QWidget *parent = 0);
    ~GraphPanel();

    QChart* chart;
    QChartView* chartView;
    vector<QLineSeries*> series;
    QValueAxis* axisX;
    vector<QValueAxis*> axisY;

    // combo box 0 is the leftmost one; 1 is the rightmost one
    void populateComboBox(int index, const vector<string>& items);
    QComboBox* comboBox(int index);
    QPushButton* maximizeButton();

signals:
    void comboBoxSelectionChanged(int index, int selection);

private slots:
    void on_d1_currentIndexChanged(int index);
    void on_d2_currentIndexChanged(int index);

private:
    Ui::GraphPanel *ui;
};

#endif // GRAPHPANEL_H
