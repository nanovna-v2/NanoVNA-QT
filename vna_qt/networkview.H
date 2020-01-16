#ifndef NETWORKVIEW_H
#define NETWORKVIEW_H
#include <QLayout>
#include <functional>
#include <array>
#include <xavna/common.H>

namespace QtCharts {
class QChartView;
class QChart;
class QValueAxis;
}

class QTimer;
class MarkerSlider;
class QTextStream;
class GraphPanel;

using namespace QtCharts;
using namespace xaxaxa;
using namespace std;

struct SParamViewSource {
    int row,col;    // which S parameter are we viewing
    enum Types {
        UNDEFINED=0,
        // view must be QLineSeries:
        TYPE_MAG=1,     // display the magnitude of Snn
        TYPE_PHASE,     // display the phase of Snn
        TYPE_GRPDELAY,  // group delay calculated from change in phase
        TYPE_SWR,       // standing wave ratio
        TYPE_Z_RE,      // real part of series equivalent impedance
        TYPE_Z_IM,      // imaginary part of series equivalent impedance
        TYPE_Z_MAG,     // magnitude of series equivalent impedance
        TYPE_Z_CAP,     // capacitance (in pF), series equivalent
        TYPE_Z_IND,     // inductance (in nH), series equivalent
        TYPE_Y_RE,      // real part of parallel equivalent admittance, mS
        TYPE_Y_IM,      // imaginary part of parallel equivalent admittance, mS
        TYPE_Y_MAG,     // magnitude of parallel equivalent admittance, mS
        TYPE_Y_CAP,     // capacitance (in pF), parallel equivalent
        TYPE_Y_IND,     // inductance (in nH), parallel equivalent
        // view must be PolarView:
        TYPE_COMPLEX,
        _LAST
    } type;
};

// a UI view of one S parameter vs frequency
struct SParamView {
    SParamViewSource src;
    QObject* view;      // either PolarView or QLineSeries depending on src.type
    QValueAxis* yAxis;  // NULL if view is not QLineSeries

    // these are not applicable for PolarView
    vector<QObject*> markerViews; // a QScatterSeries for each marker if view is a QLineSeries
    function<void(SParamView&)> addMarker;   // this function must add a new object to markerViews
};

struct Marker {
    int freqIndex;
    MarkerSlider* ms;
    bool enabled;
};


/*
helper class for displaying data about a linear network;
this is not a widget, but a set of functions to update views from S parameter data.
views (graphs, polar views) are user defined by adding items to ->views;
includes slider/marker management.

usage:
    1. instantiate NetworkView
    2. call init()
    3. (optional) set graphLimits, xAxisValueStr, etc
    4. add views (graphs, polar views, etc)
    5. call addMarker(false); note: no new views may be added after a marker is added.
    6. call update*() as needed
*/
class NetworkView: public QObject
{
    Q_OBJECT
public:
    QLayout* sliderContainer;
    vector<VNACalibratedValue> values;
    double xAxisStart = 0., xAxisStep = 1.;
    // called to convert a x value to a display string on the slider widget
    function<string(double)> xAxisValueStr;
    // array of UI views of the data; each view has a source (describing what data to display) and a widget
    // (where to display the data on
    vector<SParamView> views;
    // array of axis to update when frequency sweep params are changed
    vector<QValueAxis*> xAxis;
    // user-added markers
    vector<Marker> markers;
    // min, max, and division count of the y axis of the graph, indexed by SParamViewSource::Types
    vector<array<double,3> > graphLimits;

    static const vector<array<double,3> > defaultGraphLimits;


    NetworkView();

    void init(QLayout* sliderContainer);
    void clear();

    double xAxisAt(int i) {
        return i*xAxisStep + xAxisStart;
    }
    GraphPanel* createGraphView(bool freqDomain=true, bool tr=true);
    void updateXAxis(double start, double step, int cnt);
    // update a single point on all views in this->views
    void updateViews(int freqIndex=-1);
    // update a single point on a view, or all points on a view (if freqIndex is -1)
    void updateView(int viewIndex, int freqIndex=-1);
    void updateMarkerViews(int marker=-1);
    void updateBottomLabels(int marker=-1);
    void updateYAxis(int viewIndex=-1);
    int addMarker(bool removable);
signals:
    void markerChanged(int marker, int newIndex);
};

#endif // NETWORKVIEW_H
