#ifndef MARKERSLIDER_H
#define MARKERSLIDER_H

#include <QWidget>
#include <vector>

namespace Ui {
class MarkerSlider;
}
class QLabel;

using namespace std;

class MarkerSlider : public QWidget
{
    Q_OBJECT

public:
    int id = -1;
    vector<QLabel*> labels;
    explicit MarkerSlider(QWidget *parent = 0);
    ~MarkerSlider();

    void setLabelText(int index, string text);

public:
    Ui::MarkerSlider *ui;
};

#endif // MARKERSLIDER_H
