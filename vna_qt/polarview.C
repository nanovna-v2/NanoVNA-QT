#define _USE_MATH_DEFINES
#include "polarview.H"
#include <cmath>
#include <QPaintEvent>
#include <QPainter>
PolarView::PolarView(QWidget *parent) : QWidget(parent)
{

}

double PolarView::radius() {
    double r=this->width()/2;
    if(this->height()/2. <r) r=this->height()/2.;
    r -= margin;
    return r;
}

QPointF PolarView::val_to_point(QPointF center, double r, complex<double> val) {
    return QPointF(center.x()+val.real()/scale*r,
                   center.y()-val.imag()/scale*r);
}

void PolarView::draw_grid(QPainter &painter) {
    QPointF center(this->width()/2,this->height()/2);
    double r=radius();

    // draw outer circle
    painter.setPen(QPen(Qt::black, 2.0));
    painter.drawEllipse(center,r,r);

    painter.setPen(QPen(QColor(50,50,50), 1.0));

    // draw constant resistance circles
    for(int i=1;i<4;i++) {
        painter.drawEllipse(center+QPointF(r-r/4*i,0),r/4*i,r/4*i);
    }

    // draw constant reactance circles
    painter.save();
    QPainterPath path;
    path.addEllipse(center,r,r);
    painter.setClipPath(path);
    int n=1;
    for(int i=1;i<4;i++) {
        painter.drawEllipse(center+QPointF(r,r/2*n), r/2*n, r/2*n);
        painter.drawEllipse(center+QPointF(r,-r/2*n), r/2*n, r/2*n);
        n*=2;
    }
    painter.restore();

    // draw center line
    painter.drawLine(center-QPointF(r,0), center+QPointF(r,0));
}

void PolarView::draw_chart(QPainter &painter) {
    if(points.size()==0) return;
    QPointF center(this->width()/2,this->height()/2);
    double r=radius();
    for(int i=1;i<(int)points.size();i++) {
        if(std::isnan(points[i-1].real()) || std::isnan(points[i].real())) continue;
        painter.drawLine(val_to_point(center,r,points[i-1]),
                         val_to_point(center,r,points[i]));
    }
}

void PolarView::draw_full(QPainter &painter) {
    painter.setRenderHints(QPainter::Antialiasing);
    painter.fillRect(rect(),Qt::white);
    draw_grid(painter);
    if(persistence) {
        painter.drawImage(0,0,image);
    } else {
        painter.setPen(QPen(Qt::blue, 2.0));
        draw_chart(painter);
    }
    for(Marker marker:markers) {
        if(marker.index<0) continue;
        painter.setPen(QPen(QColor(marker.color), 2.0));
        draw_point(painter,points.at(marker.index),3);
    }
}

void PolarView::draw_point(QPainter &painter, complex<double> pt, double size) {
    QPointF center(this->width()/2,this->height()/2);
    double r=radius();
    painter.drawEllipse(val_to_point(center,r,pt), size, size);
}

void PolarView::clearPersistence() {
    image=QImage(this->size(),QImage::Format_ARGB32);
}

void PolarView::commitTrace() {

}

void PolarView::paintEvent(QPaintEvent *) {
    QPainter painter(this);
    draw_full(painter);
}
