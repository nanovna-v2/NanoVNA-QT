#ifndef POLARVIEW_H
#define POLARVIEW_H

#include <QWidget>
#include <QImage>
#include <vector>
#include <complex>
using namespace std;


class QPainter;

class PolarView : public QWidget
{
    Q_OBJECT
public:
    struct Marker {
        uint32_t color; // rgb
        int index;
    };
    vector<complex<double> > points;
    vector<Marker> markers;
    double scale=1.;
    double margin=10;	// pixels
    bool persistence = false;

    explicit PolarView(QWidget *parent = 0);

    double radius();
    QPointF val_to_point(QPointF center, double r, complex<double> val);
    void draw_grid(QPainter& painter);
    void draw_chart(QPainter& painter);
    void draw_full(QPainter& painter);
    void draw_point(QPainter& painter, complex<double> pt, double size);

    void clearPersistence();
    void commitTrace();

protected:
    QImage image;
    void paintEvent(QPaintEvent *event) override;
signals:

public slots:
};

#endif // POLARVIEW_H
