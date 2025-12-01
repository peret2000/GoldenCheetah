/*
 * Copyright (c) 2024 GoldenCheetah Contributors
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the Free
 * Software Foundation; either version 2 of the License, or (at your option)
 * any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc., 51
 * Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 */

#ifndef _GC_StreetView3DWindow_h
#define _GC_StreetView3DWindow_h
#include "GoldenCheetah.h"

#include <QWidget>
#include <QDialog>
#include <QFormLayout>
#include <string>
#include <QSslSocket>
#include <QWebEnginePage>
#include <QWebEngineView>

#include "RideFile.h"
#include "IntervalItem.h"
#include "Context.h"
#include "ErgFile.h"
#include "RealtimeData.h"


class QMouseEvent;
class RideItem;
class Context;
class QColor;
class QVBoxLayout;
class QTabWidget;
class IntervalSummaryWindow;
class SmallPlot;

class StreetView3DWindow : public GcChartWindow
{
    Q_OBJECT
    G_OBJECT

    // properties can be saved/restored/set by the layout manager
    Q_PROPERTY(int tilt READ tilt WRITE setTilt USER true)
    Q_PROPERTY(int zoom READ zoom WRITE setZoom USER true)

    public:
        StreetView3DWindow(Context *);
        ~StreetView3DWindow();
        bool markerIsVisible;
        double plotLon = 0;
        double plotLat = 0;
        QString currentPage;
        QString routeLatLngs;

        // Current route data
        double currentBearing = 0;
        double lastLat = 0;
        double lastLon = 0;

        // set/get properties
        int tilt() const { return customTilt->value(); }
        void setTilt(int x) { customTilt->setValue(x); }
        int zoom() const { return customZoom->value(); }
        void setZoom(int x) { customZoom->setValue(x); }

    public slots:
        void configChanged(qint32);
        void ergFileSelected(ErgFile*);
        void applySettings();

    private:
        Context *context;
        QVBoxLayout *layout;

        QWebEngineView *view;
        QWebEnginePage* webPage;
        StreetView3DWindow();  // default ctor

        // setting dialog
        QLabel* customTiltLabel;
        QLabel* customZoomLabel;
        QSpinBox* customTilt;
        QSpinBox* customZoom;
        QPushButton* applyButton;

        void createHtml(double startLat, double startLon, double bearing);
        void drawRoute(ErgFile* f);
        double calculateBearing(double lat1, double lon1, double lat2, double lon2);

    private slots:
        void telemetryUpdate(RealtimeData rtd);
        void stop();

    protected:

};

#endif
