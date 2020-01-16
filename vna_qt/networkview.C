#include "networkview.H"
#include "polarview.H"
#include "markerslider.H"
#include "utility.H"
#include "ui_markerslider.h"
#include "graphpanel.H"
#include <xavna/common.H>
#include <xavna/workarounds.H>
#include <QLineSeries>
#include <QScatterSeries>
#include <QValueAxis>
#include <QChart>
#include <QPushButton>
#include <QComboBox>

using namespace std;
using namespace xaxaxa;

const vector<array<double,3> > NetworkView::defaultGraphLimits = {
    {-1000,-999, 12},
    {-80, 30, 11},      //TYPE_MAG=1
    {-180, 180, 12},    //TYPE_PHASE
    {0, 50, 10},        //TYPE_GRPDELAY
    {1, 11, 10},        //TYPE_SWR
    {0, 200, 10},       //TYPE_Z_RE
    {-200, 200, 10},    //TYPE_Z_IM
    {0, 200, 10},       //TYPE_Z_MAG
    {0, 1000, 10},      //TYPE_Z_CAP
    {0, 1000, 10},      //TYPE_Z_IND
    {0, 1000, 10},      //TYPE_Y_RE
    {-1000, 1000, 10},  //TYPE_Y_IM
    {0, 1000, 10},      //TYPE_Y_MAG
    {0, 1000, 10},      //TYPE_Y_CAP
    {0, 1000, 10},      //TYPE_Y_IND
    {-1000,-999, 10}    //TYPE_COMPLEX
};

NetworkView::NetworkView() {
    xAxisValueStr = [](double val) {
        return to_string(val);
    };
    graphLimits = NetworkView::defaultGraphLimits;
}

void NetworkView::init(QLayout *sliderContainer) {
    this->sliderContainer = sliderContainer;
}

void NetworkView::clear() {
    for(auto marker: markers) {
        delete marker.ms;
    }
    views.clear();
    xAxis.clear();
    markers.clear();
    values.clear();
}

GraphPanel* NetworkView::createGraphView(bool freqDomain, bool tr) {
    vector<string> graphTraces;
    vector<SParamViewSource> graphSources;
    for(int i=SParamViewSource::UNDEFINED+1;i<SParamViewSource::_LAST;i++) {
        string name;
        switch(i) {
        case SParamViewSource::TYPE_COMPLEX: continue;
        case SParamViewSource::TYPE_GRPDELAY: name = "GroupDelay"; break;
        case SParamViewSource::TYPE_MAG: name = "mag"; break;
        case SParamViewSource::TYPE_PHASE: name = "arg"; break;
        case SParamViewSource::TYPE_SWR: name = "swr"; break;
        case SParamViewSource::TYPE_Z_RE: name = "Z_re"; break;
        case SParamViewSource::TYPE_Z_IM: name = "Z_im"; break;
        case SParamViewSource::TYPE_Z_MAG: name = "|Z|"; break;
        case SParamViewSource::TYPE_Z_CAP: name = "Cseries"; break;
        case SParamViewSource::TYPE_Z_IND: name = "Lseries"; break;
        case SParamViewSource::TYPE_Y_RE: name = "Y_re"; break;
        case SParamViewSource::TYPE_Y_IM: name = "Y_im"; break;
        case SParamViewSource::TYPE_Y_MAG: name = "|Y|"; break;
        case SParamViewSource::TYPE_Y_CAP: name = "Cparallel"; break;
        case SParamViewSource::TYPE_Y_IND: name = "Lparallel"; break;
        }
        for(int row=0;row<2;row++)
            for(int col=0;col<2;col++) {
                if(tr && col==1) continue;

                // group delay only makes sense in frequency domain view
                if(!freqDomain && i==SParamViewSource::TYPE_GRPDELAY) continue;

                // impedance parameters only make sense for Snn, not Snm,
                // and also only in frequency domain.
                if(i >= SParamViewSource::TYPE_Z_RE && i <= SParamViewSource::TYPE_Y_IND) {
                    if(row != col) continue;
                    if(!freqDomain) continue;
                }
                string desc = name + "(S" + to_string(row+1) + to_string(col+1) + ")";
                graphTraces.push_back(desc);
                graphSources.push_back({row, col, SParamViewSource::Types(i)});
            }
    }
    GraphPanel* gp = new GraphPanel();
    gp->populateComboBox(0, graphTraces);
    gp->populateComboBox(1, graphTraces);
    xAxis.push_back(gp->axisX);

    vector<int> curViews;
    for(int i=0;i<2;i++) {
        // set up markers
        auto fn = [gp, i, this](SParamView& view){
            QScatterSeries* markerSeries = new QScatterSeries();
            gp->chart->addSeries(markerSeries);
            markerSeries->setPointsVisible(true);
            markerSeries->setPen(QPen(QColor(0,200,0),2.));
            markerSeries->setBrush(Qt::transparent);
            markerSeries->setMarkerSize(6.);
            markerSeries->attachAxis(gp->axisX);
            markerSeries->attachAxis(gp->axisY[i]);
            markerSeries->setPointLabelsVisible(true);
            view.markerViews.push_back(markerSeries);
        };
        views.push_back({
                            graphSources[0],
                            gp->series[i], gp->axisY[i],
                            {}, fn
                        });
        curViews.push_back(views.size()-1);
    }
    connect(gp,&GraphPanel::comboBoxSelectionChanged, [this, curViews, graphSources](int index, int sel) {
        views.at(curViews.at(index)).src = graphSources.at(sel);
        updateView(curViews.at(index));
        updateYAxis(curViews.at(index));
        updateMarkerViews();
    });
    gp->comboBox(0)->setCurrentIndex(tr?1:2);
    gp->comboBox(1)->setCurrentIndex(0);

    return gp;
}

void NetworkView::updateXAxis(double start, double step, int cnt) {
    this->xAxisStart = start;
    this->xAxisStep = step;
    this->values.resize(cnt);
    for(int i=0;i<(int)views.size();i++) {
        SParamView tmp = views[i];
        switch(tmp.src.type) {
        case SParamViewSource::TYPE_MAG:
        case SParamViewSource::TYPE_SWR:
        case SParamViewSource::TYPE_PHASE:
        case SParamViewSource::TYPE_GRPDELAY:
        case SParamViewSource::TYPE_Z_RE:
        case SParamViewSource::TYPE_Z_IM:
        case SParamViewSource::TYPE_Z_MAG:
        case SParamViewSource::TYPE_Z_CAP:
        case SParamViewSource::TYPE_Z_IND:
        case SParamViewSource::TYPE_Y_RE:
        case SParamViewSource::TYPE_Y_IM:
        case SParamViewSource::TYPE_Y_MAG:
        case SParamViewSource::TYPE_Y_CAP:
        case SParamViewSource::TYPE_Y_IND:
        {
            auto* series = dynamic_cast<QLineSeries*>(tmp.view);
            series->clear();
            for(int j=0;j<cnt;j++) {
                series->append(xAxisAt(j), 0);
            }
            break;
        }
        case SParamViewSource::TYPE_COMPLEX:
        {
            auto* view = dynamic_cast<PolarView*>(tmp.view);
            view->points.resize(cnt);
            break;
        }
        default: assert(false);
        }
    }
    for(Marker& marker:markers) {
        if(marker.ms != NULL)
            marker.ms->ui->slider->setRange(0, cnt-1);
    }
    for(QValueAxis* axisX:xAxis) {
        axisX->setMin(xAxisStart);
        axisX->setMax(xAxisAt(cnt-1));
    }
}

void NetworkView::updateViews(int freqIndex) {
    if(freqIndex >= (int)values.size()) return;
    for(int i=0;i<(int)this->views.size();i++) {
        updateView(i, freqIndex);
    }
}

void NetworkView::updateView(int viewIndex, int freqIndex) {
    double period = 1./xAxisStep;
    double grpDelayScale = period/(2*M_PI) * 1e3;

    if(freqIndex < 0) {
        for(int i=0;i<(int)values.size();i++) {
            updateView(viewIndex,i);
        }
        return;
    }
    double freqHz = xAxisAt(freqIndex)*1e6; // only meaningful in frequency domain
    SParamView tmp = this->views.at(viewIndex);

    VNACalibratedValue val = this->values.at(freqIndex);
    complex<double> entry = val(tmp.src.row,tmp.src.col);
    double z0 = 50;
    complex<double> Z = -z0*(entry+1.)/(entry-1.);
    complex<double> Y = -(entry-1.)/(z0*(entry+1.));

    if(tmp.src.type == SParamViewSource::TYPE_COMPLEX) {
        auto* view = dynamic_cast<PolarView*>(tmp.view);
        view->points.at(freqIndex) = entry;
    } else {
        auto* series = dynamic_cast<QLineSeries*>(tmp.view);
        double y = 0;
        switch(tmp.src.type) {
        case SParamViewSource::TYPE_MAG:
            y = dB(norm(entry));
            break;
        case SParamViewSource::TYPE_SWR:
            y = swr(norm(entry));
            break;
        case SParamViewSource::TYPE_PHASE:
            y = arg(entry)*180./M_PI;
            break;
        case SParamViewSource::TYPE_Z_RE:
            y = Z.real();
            break;
        case SParamViewSource::TYPE_Z_IM:
            y = Z.imag();
            break;
        case SParamViewSource::TYPE_Z_MAG:
            y = abs(Z);
            break;
        case SParamViewSource::TYPE_Z_CAP: // capacitance in pF
            y = -capacitance_inductance(freqHz, Z.imag()) * 1e12;
            break;
        case SParamViewSource::TYPE_Z_IND: // inductance in nH
            y = capacitance_inductance(freqHz, Z.imag()) * 1e9;
            break;
        case SParamViewSource::TYPE_Y_RE:
            y = Y.real() * 1000;
            break;
        case SParamViewSource::TYPE_Y_IM:
            y = Y.imag() * 1000;
            break;
        case SParamViewSource::TYPE_Y_MAG:
            y = abs(Y) * 1000;
            break;
        case SParamViewSource::TYPE_Y_CAP: // capacitance in pF
            y = -capacitance_inductance_Y(freqHz, Y.imag()) * 1e12;
            break;
        case SParamViewSource::TYPE_Y_IND: // inductance in nH
            y = capacitance_inductance_Y(freqHz, Y.imag()) * 1e9;
            break;
        case SParamViewSource::TYPE_GRPDELAY:
        {
            if(freqIndex>0) {
                VNACalibratedValue prevVal = this->values.at(freqIndex-1);
                complex<double> prevEntry = prevVal(tmp.src.row,tmp.src.col);
                y = arg(prevEntry) - arg(entry);
                if(y>=M_PI) y-=2*M_PI;
                if(y<-M_PI) y+=2*M_PI;
            } else y=0;
            y *= grpDelayScale;
            break;
        }
        default: assert(false);
        }

        // clamp values to avoid enlarging labels
        switch(tmp.src.type) {
            case SParamViewSource::TYPE_Z_CAP:
            case SParamViewSource::TYPE_Z_IND:
            case SParamViewSource::TYPE_Y_CAP:
            case SParamViewSource::TYPE_Y_IND:
                if(y < 0) y = 0;
                if(y > 999) y = 999;
                break;
            case SParamViewSource::TYPE_Z_RE:
            case SParamViewSource::TYPE_Z_IM:
            case SParamViewSource::TYPE_Z_MAG:
                if(y < -999) y = -999;
                if(y > 999) y = 999;
                break;
            default: break;
        }

        series->replace(freqIndex,series->at(freqIndex).x(), y);
    }
}

void NetworkView::updateMarkerViews(int marker) {
    for(SParamView view:views) {
        if(auto* series = dynamic_cast<QLineSeries*>(view.view)) {
            for(int i=0;i<(int)markers.size();i++) {
                if(marker>=0 && marker!=i) continue;
                auto* ss = dynamic_cast<QScatterSeries*>(view.markerViews.at(i));
                int freqIndex = markers[i].freqIndex;
                if(markers[i].ms == NULL || !markers[i].enabled) {
                    ss->replace(0, -100, 0);
                    continue;
                }
                ss->replace(0,series->at(freqIndex));
                //printf("sss %f %f\n",series->at(freqIndex).x(), series->at(freqIndex).y());
                fflush(stdout);
            }
        }
        if(auto* pv = dynamic_cast<PolarView*>(view.view)) {
            for(int i=0;i<(int)markers.size();i++) {
                if(marker>=0 && marker!=i) continue;
                if(markers[i].ms == NULL || !markers[i].enabled)
                    pv->markers.at(i).index = -1;
                else pv->markers.at(i).index = markers[i].freqIndex;
            }
            pv->repaint();
        }
    }
}

void NetworkView::updateBottomLabels(int marker) {
    SParamView* lineViews[4];
    int lineViewCount=0;
    for(auto& view:views) {
        if(dynamic_cast<QLineSeries*>(view.view)) {
            lineViews[lineViewCount] = &view;
            lineViewCount++;
            if(lineViewCount>=4) break;
        }
    }
    for(int i=0;i<(int)markers.size();i++) {
        if(marker>=0 && marker!=i) continue;
        auto& marker = markers[i];
        if(marker.ms == NULL) continue;
        for(int j=0;j<4;j++) {
            if(j>=lineViewCount) marker.ms->setLabelText(j, "");
            else {
                auto* series = dynamic_cast<QLineSeries*>(lineViews[j]->view);
                const char* fmt = "";
                switch(lineViews[j]->src.type) {
                case SParamViewSource::TYPE_MAG: fmt = "%.1lf dB"; break;
                case SParamViewSource::TYPE_SWR: fmt = "%.2lf:1"; break;
                case SParamViewSource::TYPE_PHASE: fmt = "%.1lf °"; break;
                case SParamViewSource::TYPE_GRPDELAY: fmt = "%.2lf ns"; break;
                case SParamViewSource::TYPE_Z_RE: fmt = "%.1lf Ω"; break;
                case SParamViewSource::TYPE_Z_IM: fmt = "%.1lf Ω"; break;
                case SParamViewSource::TYPE_Z_MAG: fmt = "%.1lf Ω"; break;
                case SParamViewSource::TYPE_Z_CAP: fmt = "%.1lf pF"; break;
                case SParamViewSource::TYPE_Z_IND: fmt = "%.2lf nH"; break;
                case SParamViewSource::TYPE_Y_RE: fmt = "%.1lf mS"; break;
                case SParamViewSource::TYPE_Y_IM: fmt = "%.1lf mS"; break;
                case SParamViewSource::TYPE_Y_MAG: fmt = "%.1lf mS"; break;
                case SParamViewSource::TYPE_Y_CAP: fmt = "%.1lf pF"; break;
                case SParamViewSource::TYPE_Y_IND: fmt = "%.2lf nH"; break;
                default: fmt = "%.2lf";
                }
                marker.ms->setLabelText(j, ssprintf(32, fmt, series->at(marker.freqIndex).y()));
            }
        }
    }
}

void NetworkView::updateYAxis(int viewIndex) {
    if(viewIndex<0) {
        for(int i=0;i<(int)views.size();i++) updateYAxis(i);
        return;
    }
    auto& view = views.at(viewIndex);
    int typeIndex = view.src.type;
    if(view.yAxis == nullptr) return;
    view.yAxis->setRange(graphLimits.at(typeIndex)[0], graphLimits.at(typeIndex)[1]);
    view.yAxis->setTickCount(graphLimits.at(typeIndex)[2] + 1);
}

int NetworkView::addMarker(bool removable) {
    int newId=-1;
    for(int i=0;i<(int)markers.size();i++)
        if(markers[i].ms == NULL) {
            newId=i;
            break;
        }
    if(newId<0) {
        newId = (int)markers.size();
        markers.push_back({0, NULL, true});
        for(SParamView& view:views) {
            if(dynamic_cast<QLineSeries*>(view.view)) {
                view.addMarker(view);
                auto* series = dynamic_cast<QScatterSeries*>(view.markerViews.at(view.markerViews.size()-1));
                series->append({-100, 0});
                series->setPointLabelsFormat(qs(to_string(newId)));
            }
            if(auto* pv = dynamic_cast<PolarView*>(view.view)) {
                pv->markers.push_back({0xff0000,0});
            }
        }
    }
    MarkerSlider* ms = new MarkerSlider();
    ms->id = newId;
    ms->ui->l_id->setText(qs(to_string(newId)));
    ms->ui->slider->setRange(0, ((int)values.size())-1);
    markers[newId].ms = ms;
    markers[newId].freqIndex = 0;
    updateMarkerViews(newId);

    connect(ms->ui->slider,&QSlider::valueChanged, [this,ms,newId](int value) {
        this->markers[newId].freqIndex = value;
        ms->ui->b_freq->setText(qs(xAxisValueStr(xAxisAt(value))));
        updateMarkerViews(newId);
        updateBottomLabels(newId);
        emit markerChanged(newId,value);
    });
    connect(ms->ui->b_freq,&QPushButton::toggled, [this,newId](bool checked){
        markers.at(newId).enabled = checked;
        updateMarkerViews(newId);
    });
    ms->ui->slider->valueChanged(0);
    if(removable) {
        ms->ui->b_add_del->setIcon(QIcon(":/icons/close"));
        connect(ms->ui->b_add_del, &QPushButton::clicked, [this,ms,newId](){
            this->markers[newId].ms = NULL;
            updateMarkerViews(newId);
            ms->deleteLater();
        });
    } else {
        ms->ui->b_add_del->setIcon(QIcon(":/icons/add"));
        connect(ms->ui->b_add_del, &QPushButton::clicked, [this](){
            addMarker(true);
        });
    }
    auto* layout = dynamic_cast<QBoxLayout*>(this->sliderContainer);
    layout->insertWidget(0, ms);
    return newId;
}

