/*
 * polarView.C
 * 
 * Copyright 2012  <xaxaxa@xaxaxa-mac>
 * 
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
 * MA 02110-1301, USA.
 * 
 * 
 */
#include <gtkmm.h>
#include <gtkmm/drawingarea.h>
#include <gdk/gdk.h>
#include <iostream>
#include <math.h>
#include <complex>

using namespace std;
using namespace Gdk;
using namespace Gtk;
//using namespace ::Cairo;
namespace xaxaxa
{
	class GraphView: public Gtk::DrawingArea
	{
	public:
		vector<vector<double> > lines;
		vector<uint32_t> colors;		// colors must have the same length as lines
		double minValue=0.;
		double maxValue=1.;
		double hgridMin=0.;
		double hgridSpacing=10.;
		
		double w,h;
		vector<int> selectedPoints;
		
		double ptX(int index, int totalPoints) {
			return index*(w-1)/(totalPoints-1);
		}
		double ptY(double val) {
			return (maxValue-val)*(h-1)/(maxValue-minValue);
		}
		void do_draw(const ::Cairo::RefPtr< ::Cairo::Context>& gc)
		{
			w=get_allocation().get_width();
			h=get_allocation().get_height();
			
			gc->set_source_rgb(0.5, 0.5, 0.5);
			for(double y=hgridMin; y<maxValue; y+=hgridSpacing) {
				gc->move_to(0, ptY(y));
				gc->line_to(w, ptY(y));
				gc->stroke();
			}
			
			for(int i=0;i<(int)lines.size();i++) {
				const vector<double>& points = lines[i];
				if(points.size() == 0) continue;
				int pts=(int)points.size();
				
				uint32_t color = colors[i];
				gc->set_source_rgb(double((color>>16)&0xFF)/255,
					double((color>>8)&0xFF)/255,double((color)&0xFF)/255);
				
				gc->move_to(ptX(0, pts), ptY(points[0]));
				for(int j=1;j<pts;j++) {
					if(isnan(points[j])) continue;
					if(isnan(points[j-1])) {
						gc->move_to(ptX(j, pts), ptY(points[j]));
						continue;
					}
					gc->line_to(ptX(j, pts), ptY(points[j]));
				}
				gc->stroke();
			}
			
			gc->set_source_rgb(0.5, 0.5, 0.5);
			for(int i=0;i<(int)selectedPoints.size();i++)
				draw_point(gc,ptX(selectedPoints[i], (int)lines[i].size()),ptY(lines[i][selectedPoints[i]]),3);
		}
		void draw_point(const ::Cairo::RefPtr< ::Cairo::Context>& gc, double x, double y, double size) {
			gc->arc(x, y, size, 0.0, 2 * M_PI);
			gc->stroke();
		}
		virtual bool on_motion_notify_event(GdkEventMotion* event)
		{
			Gtk::DrawingArea::on_motion_notify_event(event);
			
			return false;
		}
		virtual bool on_button_press_event(GdkEventButton* event)
		{
			Gtk::DrawingArea::on_button_press_event(event);
			return false;
		}
		virtual bool on_button_release_event(GdkEventButton* event)
		{
			Gtk::DrawingArea::on_button_release_event(event);
			return false;
		}
		void do_draw(GdkEventExpose* evt=NULL)
		{
			Glib::RefPtr<Gdk::Window> window = get_window();
			if(window)
			{
				::Cairo::RefPtr< ::Cairo::Context> ctx = window->create_cairo_context();
				if(evt)
				{
					ctx->rectangle(evt->area.x, evt->area.y, evt->area.width, evt->area.height);
					ctx->clip();
				}
				do_draw(ctx);
			}
		}
		virtual bool on_expose_event(GdkEventExpose* evt)
		{
			printf("on_expose_event\n");
			do_draw(evt);
			return false;
		}
		virtual bool on_draw(const ::Cairo::RefPtr<::Cairo::Context>& cr) {
			do_draw(cr);
			return false;
		}
		GraphView(): Gtk::DrawingArea()
		{
			set_app_paintable(true);
			set_double_buffered(true);
			set_redraw_on_allocate(true);
			set_events(get_events()|EXPOSURE_MASK|POINTER_MOTION_MASK|BUTTON_MOTION_MASK|BUTTON_PRESS_MASK|BUTTON_RELEASE_MASK);
		}
	};
}

