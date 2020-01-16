
#include "polar_view.H"
#include "graph_view.H"
#include <gtkmm/application.h>
#include <gtkmm/window.h>
#include <xavna/xavna.h>
#include "vna_ui_core.H"

//#include "adf4350_board.H"

using namespace std;
//using namespace adf4350Board;

#define GETWIDGET(x) builder->get_widget(TOKEN_TO_STRING(x), x)




// globals

// to be declared in the generated vna.glade.c file
extern unsigned char vna_glade[];
extern unsigned int vna_glade_len;


Glib::RefPtr<Gtk::Builder> builder;
xaxaxa::PolarView* polarView=NULL;
xaxaxa::GraphView* graphView=NULL;
xaxaxa::GraphView* timeGraphView=NULL;
pthread_t refreshThread_;


bool timeDomain = false;
bool showCursor = true;

void alert(string msg) {
	Gtk::Window* window1;
	GETWIDGET(window1);
	Gtk::MessageDialog dialog(*window1, msg,
		false /* use_markup */, Gtk::MESSAGE_WARNING,
		Gtk::BUTTONS_OK);
	dialog.run();
}

void updateGraph(int i, complex2 values) {
	complex<double> refl = values[reflIndex], thru = values[thruIndex];
	polarView->points[i]=refl;
	graphView->lines[0][i] = arg(refl);
	graphView->lines[1][i] = arg(thru);
	graphView->lines[2][i] = dB(norm(refl));
	graphView->lines[3][i] = dB(norm(thru));
}
void updateTimeGraph(int i, complex2 values) {
	timeGraphView->lines[0][i] = dB(norm(values[reflIndex]));
	timeGraphView->lines[1][i] = dB(norm(values[thruIndex]));
}
void sweepCompleted() {
	Glib::signal_idle().connect([]() {
		polarView->commitTrace();
		return false;
	});
}

void resizeVectors2() {
	Gtk::Scale *s_freq,*s_time;
	GETWIDGET(s_freq); GETWIDGET(s_time);
	s_freq->set_range(0, nPoints-1);
	s_time->set_range(0, timePoints()-1);
	polarView->points.resize(nPoints);
	for(int i=0;i<4;i++)
		graphView->lines[i].resize(nPoints);
	for(int i=0;i<2;i++)
		timeGraphView->lines[i].resize(timePoints());
}

void updateFreqButton() {
	Gtk::Button *b_freq;
	GETWIDGET(b_freq);
	b_freq->set_label(ssprintf(31, "%.2f MHz -\n%.2f MHz", freqAt(0), freqAt(nPoints-1)));
}


void measureCalReference(int index) {
	Gtk::Window *window1;
	GETWIDGET(window1);
	
	for(int i=0;i<nPoints;i++) polarView->points[i] = NAN;
	for(int i=0;i<nPoints;i++) graphView->lines[3][i] = NAN;
	window1->set_sensitive(false);
	takeMeasurement([window1,index](vector<complex2> values) {
		calibrationReferences[index] = values;
		
		Glib::signal_idle().connect([window1]() {
			window1->set_sensitive(true);
			return false;
		});
	});
}


void addButtonHandlers() {
	// controls
	Gtk::Window *window1, *window3;
	Gtk::Button *b_oc, *b_sc, *b_t, *b_thru, *b_apply, *b_clear, *b_load, *b_save, *b_freq;
	Gtk::ToggleButton *c_persistence, *c_freq, *c_ttf;
	
	// get controls
	GETWIDGET(window1); GETWIDGET(window3);
	GETWIDGET(b_oc); GETWIDGET(b_sc); GETWIDGET(b_t); GETWIDGET(b_thru);
	GETWIDGET(b_apply); GETWIDGET(b_clear); 
	GETWIDGET(b_load); GETWIDGET(b_save);
	GETWIDGET(b_freq); GETWIDGET(c_persistence);
	GETWIDGET(c_freq); GETWIDGET(c_ttf);
	b_oc->signal_clicked().connect([]() {
		measureCalReference(CAL_OPEN);
	});
	b_sc->signal_clicked().connect([window1]() {
		measureCalReference(CAL_SHORT);
	});
	b_t->signal_clicked().connect([window1]() {
		measureCalReference(CAL_LOAD);
	});
	b_thru->signal_clicked().connect([window1]() {
		measureCalReference(CAL_THRU);
	});
	b_apply->signal_clicked().connect([]() {
		applySOLT();
	});
	b_clear->signal_clicked().connect([]() {
		clearCalibration();
	});
	
	b_save->signal_clicked().connect([window1]() {
		Gtk::FileChooserDialog d(*window1, "Save calibration file...", FILE_CHOOSER_ACTION_SAVE);
		Gtk::Button* b = d.add_button(Stock::SAVE, RESPONSE_OK);
		if(d.run() == RESPONSE_OK)
		{
			string data=saveCalibration();
			string etag;
			d.get_file()->replace_contents(data, "", etag);
		}
	});
	b_load->signal_clicked().connect([window1]() {
		Gtk::FileChooserDialog d(*window1, "Open calibration file...", FILE_CHOOSER_ACTION_OPEN);
		Gtk::Button* b = d.add_button(Stock::OPEN, RESPONSE_OK);
		if(d.run() == RESPONSE_OK)
		{
			char* fileData = NULL;
			gsize sz = 0;
			d.get_file()->load_contents(fileData, sz);
			loadCalibration(fileData, sz);
			free(fileData);
		}
	});
	b_freq->signal_clicked().connect([window1]() {
		Gtk::Dialog* dialog_freq;
		Gtk::Entry *d_t_start, *d_t_step, *d_t_span;
		GETWIDGET(dialog_freq);
		GETWIDGET(d_t_start); GETWIDGET(d_t_step); GETWIDGET(d_t_span);
		
		d_t_start->set_text(ssprintf(20, "%.2f", double(startFreq)*freqMultiplier));
		d_t_step->set_text(ssprintf(20, "%.2f", double(freqStep)*freqMultiplier));
		d_t_span->set_text(ssprintf(20, "%d", nPoints));
		
		dialog_freq->set_transient_for(*window1);
		if(dialog_freq->run() == RESPONSE_OK) {
			refreshThreadShouldExit = true;
			pthread_join(refreshThread_, NULL);
			refreshThreadShouldExit = false;
			
			Gtk::Entry *d_t_start, *d_t_step, *d_t_span;
			GETWIDGET(d_t_start); GETWIDGET(d_t_step); GETWIDGET(d_t_span);
			auto start = d_t_start->get_text();
			auto step = d_t_step->get_text();
			auto span = d_t_span->get_text();
			startFreq = atof(start.c_str())/freqMultiplier;
			freqStep = atof(step.c_str())/freqMultiplier;
			
			resizeVectors(atoi(span.c_str()));
			resizeVectors2();
			updateFreqButton();
			clearCalibration();
			
			if(pthread_create(&refreshThread_, NULL, &refreshThread,NULL)<0) {
				perror("pthread_create");
				exit(1);
			}
		}
		dialog_freq->hide();
	});
	c_persistence->signal_toggled().connect([c_persistence]() {
		polarView->persistence = c_persistence->get_active();
		if(polarView->persistence)
			polarView->clearPersistence();
	});
	c_freq->signal_toggled().connect([c_freq]() {
		showCursor = c_freq->get_active();
		if(showCursor) {
			polarView->cursorColor = 0xffffff00;
		} else {
			polarView->cursorColor = 0x00000000;
		}
		polarView->queue_draw();
	});
	c_ttf->signal_toggled().connect([c_ttf, window3]() {
		if(c_ttf->get_active())
			window3->show();
		else window3->hide();
	});
}

string GetDirFromPath(const string path)
{
	int i = path.rfind("/");
	if (i < 0) return string();
	return path.substr(0, i + 1);
}
string GetProgramPath()
{
	char buf[256];
	int i = readlink("/proc/self/exe", buf, sizeof(buf));
	if (i < 0) {
		perror("readlink");
		return "";
	}
	return string(buf, i);
}


void updateLabels() {
	Gtk::Scale *s_freq;
	Gtk::Label *l_freq,*l_refl,*l_refl_phase,*l_through,*l_through_phase;
	Gtk::Label *l_impedance, *l_admittance, *l_s_admittance, *l_p_impedance, *l_series, *l_parallel;
	Gtk::ToggleButton *c_freq;
	GETWIDGET(s_freq); GETWIDGET(c_freq); GETWIDGET(l_refl); GETWIDGET(l_refl_phase);
	GETWIDGET(l_through); GETWIDGET(l_through_phase);
	GETWIDGET(l_impedance); GETWIDGET(l_admittance); GETWIDGET(l_s_admittance);
	GETWIDGET(l_p_impedance); GETWIDGET(l_series); GETWIDGET(l_parallel);
	
	// frequency label
	int freqIndex=(int)s_freq->get_value();
	double freq=freqAt(freqIndex);
	c_freq->set_label(ssprintf(20, "%.2f MHz", freq));
	
	
	complex<double> reflCoeff = polarView->points[freqIndex];
	l_refl->set_text(ssprintf(20, "%.1f dB", dB(norm(reflCoeff))));
	l_refl_phase->set_text(ssprintf(20, "%.1f °", arg(reflCoeff)*180/M_PI));
	
	l_through->set_text(ssprintf(20, "%.1f dB", graphView->lines[3][freqIndex]));
	l_through_phase->set_text(ssprintf(20, "%.1f °", graphView->lines[1][freqIndex]*180/M_PI));
	
	if(!use_cal) {
		l_impedance->set_text("");
		l_admittance->set_text("");
		l_s_admittance->set_text("");
		l_p_impedance->set_text("");
		l_series->set_text("");
		l_parallel->set_text("");
		return;
	}
	
	// impedance display panel (left side)
	
	complex<double> Z = -Z0*(reflCoeff+1.)/(reflCoeff-1.);
	complex<double> Y = -(reflCoeff-1.)/(Z0*(reflCoeff+1.));
	
	l_impedance->set_text(ssprintf(127, "  %.2f\n%s j%.2f", Z.real(), Z.imag()>=0 ? "+" : "-", fabs(Z.imag())));
	l_admittance->set_text(ssprintf(127, "  %.4f\n%s j%.4f", Y.real(), Y.imag()>=0 ? "+" : "-", fabs(Y.imag())));
	l_s_admittance->set_text(ssprintf(127, "  %.4f\n%s j%.4f", 1./Z.real(), Z.imag()>=0 ? "+" : "-", fabs(1./Z.imag())));
	l_p_impedance->set_text(ssprintf(127, "  %.2f\n|| j%.2f", 1./Y.real(), 1./Y.imag()));
	
	double value = capacitance_inductance(freq*1e6, Z.imag());
	l_series->set_text(ssprintf(127, "%.2f Ω\n%.2f %s%s", Z.real(), fabs(si_scale(value)), si_unit(value), value>0?"H":"F"));
	
	value = capacitance_inductance_Y(freq*1e6, Y.imag());
	l_parallel->set_text(ssprintf(127, "%.2f Ω\n%.2f %s%s", 1./Y.real(), fabs(si_scale(value)), si_unit(value), value>0?"H":"F"));
}
void updateLabels_ttf() {
	Gtk::Scale *s_time;
	Gtk::ToggleButton *c_time;
	Gtk::Label *l_refl1,*l_through1;
	GETWIDGET(s_time); GETWIDGET(c_time);
	GETWIDGET(l_refl1); GETWIDGET(l_through1);
	
	// time label
	int timeIndex=(int)s_time->get_value();
	double t=timeAt(timeIndex);
	c_time->set_label(ssprintf(20, "%.2f ns", t));
	
	// reflection and through labels
	double refl = timeGraphView->lines[0][timeIndex];
	double through = timeGraphView->lines[1][timeIndex];
	l_refl1->set_text(ssprintf(20, "%.1f dB", refl));
	l_through1->set_text(ssprintf(20, "%.1f dB", through));
	
}



int main(int argc, char** argv) {
	if(argc<2) {
		fprintf(stderr,"usage: %s /PATH/TO/TTY\n",argv[0]);
		return 1;
	}
	xavna_dev = xavna_open(argv[1]);
	if(xavna_dev == NULL) {
		perror("xavna_open");
		return 1;
	}
	
	
	// set up UI
	int argc1=1;
	auto app = Gtk::Application::create(argc1, argv, "org.gtkmm.example");
	string binDir=GetDirFromPath(GetProgramPath());
	if(chdir(binDir.c_str())<0)
		perror("chdir");

	builder = Gtk::Builder::create_from_string(string((char*)vna_glade, vna_glade_len));
	
	// controls
	Gtk::Window* window1;
	Gtk::Viewport *vp_main, *vp_graph, *vp_ttf;
	Gtk::Scale *s_freq, *s_time;
	
	// get controls
	GETWIDGET(window1); GETWIDGET(vp_main); GETWIDGET(vp_graph); GETWIDGET(vp_ttf); GETWIDGET(s_freq); GETWIDGET(s_time);
	addButtonHandlers();
	
	
	// polar view
	polarView = new xaxaxa::PolarView();
	vp_main->add(*polarView);
	polarView->show();
	
	// graph view
	graphView = new xaxaxa::GraphView();
	graphView->minValue = -80;
	graphView->maxValue = 50;
	graphView->hgridMin = -80;
	graphView->hgridSpacing = 10;
	graphView->selectedPoints = {0, 0, 0, 0};
	graphView->colors = {0x00aadd, 0xff8833, 0x0000ff, 0xff0000};
	graphView->lines.resize(4);
	vp_graph->add(*graphView);
	graphView->show();
	
	// time graph view
	timeGraphView = new xaxaxa::GraphView();
	timeGraphView->minValue = -60;
	timeGraphView->maxValue = 0;
	timeGraphView->hgridMin = -60;
	timeGraphView->hgridSpacing = 10;
	timeGraphView->selectedPoints = {0, 0};
	timeGraphView->colors = {0x0000ff, 0xff0000};
	timeGraphView->lines.resize(2);
	vp_ttf->add(*timeGraphView);
	timeGraphView->show();
	
	resizeVectors(nPoints);
	resizeVectors2();
	
	// controls
	s_freq->set_value(0);
	s_time->set_value(0);
	s_freq->set_increments(1, 10);
	s_time->set_increments(1, 10);
	updateLabels();
	updateLabels_ttf();
	s_freq->signal_value_changed().connect([s_freq](){
		polarView->selectedPoint = (int)s_freq->get_value();
		graphView->selectedPoints[0] = graphView->selectedPoints[1]
			= graphView->selectedPoints[2] = graphView->selectedPoints[3] = (int)s_freq->get_value();
		updateLabels();
		polarView->queue_draw();
		graphView->queue_draw();
	});
	s_time->signal_value_changed().connect([s_time](){
		timeGraphView->selectedPoints[0] = timeGraphView->selectedPoints[1] = (int)s_time->get_value();
		updateLabels_ttf();
		timeGraphView->queue_draw();
	});
	
	// frequency dialog
	Gtk::Dialog* dialog_freq;
	Gtk::Entry *d_t_start, *d_t_step, *d_t_span;
	Gtk::Label *d_l_end;
	GETWIDGET(dialog_freq); GETWIDGET(d_l_end);
	GETWIDGET(d_t_start); GETWIDGET(d_t_step); GETWIDGET(d_t_span);
	
	auto func = [d_t_start, d_t_step, d_t_span, d_l_end](){
		auto start = d_t_start->get_text();
		auto step = d_t_step->get_text();
		auto span = d_t_span->get_text();
		double endFreq = atof(start.c_str()) + atof(step.c_str()) * (atoi(span.c_str()) - 1);
		d_l_end->set_text(ssprintf(20, "%.2f", endFreq));
	};
	d_t_start->signal_changed().connect(func);
	d_t_step->signal_changed().connect(func);
	d_t_span->signal_changed().connect(func);
	updateFreqButton();
	
	// periodic refresh
	sigc::connection conn = Glib::signal_timeout().connect([](){
		updateLabels();
		updateLabels_ttf();
		polarView->queue_draw();
		graphView->queue_draw();
		timeGraphView->queue_draw();
		return true;
	}, 200);
	
	
	if(pthread_create(&refreshThread_, NULL, &refreshThread,NULL)<0) {
		perror("pthread_create");
		return 1;
	}

	return app->run(*window1);
}
