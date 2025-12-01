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

#include "StreetView3DWindow.h"

#include "MainWindow.h"
#include "RideItem.h"
#include "RideFile.h"
#include "RideImportWizard.h"
#include "IntervalItem.h"
#include "IntervalTreeView.h"
#include "SmallPlot.h"
#include "Context.h"
#include "Athlete.h"
#include "Settings.h"
#include "Colors.h"
#include "Units.h"
#include "TimeUtils.h"
#include "HelpWhatsThis.h"
#include "Library.h"
#include "ErgFile.h"
#include "LocationInterpolation.h"

// overlay helper
#include "AbstractView.h"
#include "GcOverlayWidget.h"
#include "IntervalSummaryWindow.h"
#include "HelpWhatsThis.h"

#include <cmath>

// declared in main, we only want to use it to get QStyle
extern QApplication *application;

// Helper to convert degrees to radians
static double toRadians(double degrees) {
    return degrees * M_PI / 180.0;
}

// Helper to convert radians to degrees
static double toDegrees(double radians) {
    return radians * 180.0 / M_PI;
}

StreetView3DWindow::StreetView3DWindow(Context *context) : GcChartWindow(context), context(context)
{
    HelpWhatsThis *helpContents = new HelpWhatsThis(this);
    this->setWhatsThis(helpContents->getWhatsThisText(HelpWhatsThis::ChartTrain_LiveMap));

    // Connect signal to receive updates on lat/lon for plotting on map.
    connect(context, SIGNAL(telemetryUpdate(RealtimeData)), this, SLOT(telemetryUpdate(RealtimeData)));
    connect(context, SIGNAL(stop()), this, SLOT(stop()));
    connect(context, SIGNAL(ergFileSelected(ErgFile*)), this, SLOT(ergFileSelected(ErgFile*)));

    // Chart settings
    QWidget * settingsWidget = new QWidget(this);
    HelpWhatsThis *helpConfig = new HelpWhatsThis(settingsWidget);
    settingsWidget->setWhatsThis(helpConfig->getWhatsThisText(HelpWhatsThis::ChartTrain_LiveMap));
    settingsWidget->setContentsMargins(0,0,0,0);
    setProperty("color", GColor(CTRAINPLOTBACKGROUND));

    QFormLayout* commonLayout = new QFormLayout(settingsWidget);

    customTiltLabel = new QLabel(tr("Camera Tilt (degrees)"));
    customTilt = new QSpinBox(this);
    customTilt->setFixedWidth(60);
    customTilt->setRange(0, 85);
    customTilt->setValue(60); // Default tilt for 3D perspective

    customZoomLabel = new QLabel(tr("Zoom Level"));
    customZoom = new QSpinBox(this);
    customZoom->setFixedWidth(60);
    customZoom->setRange(15, 20);
    customZoom->setValue(18); // Default zoom for street-level view

    commonLayout->addRow(customTiltLabel, customTilt);
    commonLayout->addRow(customZoomLabel, customZoom);
    
    applyButton = new QPushButton(application->style()->standardIcon(QStyle::SP_ArrowRight), tr("Apply changes"), this);
    commonLayout->addRow(applyButton);

    // Connect signal to update map
    connect(applyButton, SIGNAL(clicked(bool)), this, SLOT(applySettings()));

    setControls(settingsWidget);
    setContentsMargins(0, 0, 0, 0);
    layout = new QVBoxLayout();
    layout->setSpacing(0);
    layout->setContentsMargins(2, 2, 2, 2);
    setChartLayout(layout);

    // set webview for map
    view = new QWebEngineView(this);
    webPage = new QWebEnginePage(context->webEngineProfile);
    view->setPage(webPage);

    view->setContentsMargins(0,10,0,0);
    view->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    view->setAcceptDrops(false);
    layout->addWidget(view);

    configChanged(CONFIG_APPEARANCE);

    // Finish initialization using current settings
    ergFileSelected(context->currentErgFile());
}

void StreetView3DWindow::applySettings()
{
    // Refresh the view with new settings
    ergFileSelected(context->currentErgFile());
}

StreetView3DWindow::~StreetView3DWindow()
{
    // webPage is managed by view's setPage, so just delete the view's page
    // The webPage pointer is the same as view->page() after setPage was called
    if (view) {
        delete view->page();
        view = nullptr;
    }
}

double StreetView3DWindow::calculateBearing(double lat1, double lon1, double lat2, double lon2)
{
    // Calculate bearing between two points
    double dLon = toRadians(lon2 - lon1);
    double lat1Rad = toRadians(lat1);
    double lat2Rad = toRadians(lat2);
    
    double y = sin(dLon) * cos(lat2Rad);
    double x = cos(lat1Rad) * sin(lat2Rad) - sin(lat1Rad) * cos(lat2Rad) * cos(dLon);
    
    double bearing = toDegrees(atan2(y, x));
    return fmod((bearing + 360.0), 360.0);
}

void StreetView3DWindow::ergFileSelected(ErgFile* f)
{
    // rename window to workout name and draw route if data exists
    if (f && f->filename() != "" )
    {
        setIsBlank(false);
        
        // Get starting coordinates with extended precision
        double startingLat = ((f->Points)[0]).lat;
        double startingLon = ((f->Points)[0]).lon;
        
        if (startingLat == 0 && startingLon == 0)
        {
            markerIsVisible = false;
            setIsBlank(true);
        }
        else
        {
            // Calculate initial bearing if we have at least 2 points
            double initialBearing = 0;
            if (f->Points.size() > 1) {
                // Find first valid pair of points for bearing calculation
                for (int i = 0; i < f->Points.size() - 1; i++) {
                    geolocation geoloc1(f->Points[i].lat, f->Points[i].lon, f->Points[i].y);
                    geolocation geoloc2(f->Points[i+1].lat, f->Points[i+1].lon, f->Points[i+1].y);
                    if (geoloc1.IsReasonableGeoLocation() && geoloc2.IsReasonableGeoLocation()) {
                        initialBearing = calculateBearing(
                            f->Points[i].lat, f->Points[i].lon,
                            f->Points[i+1].lat, f->Points[i+1].lon
                        );
                        break;
                    }
                }
            }
            
            // Build route coordinates for the polyline using helper method
            buildRouteLatLngs(f);
            
            // Store initial values
            lastLat = startingLat;
            lastLon = startingLon;
            currentBearing = initialBearing;
            
            createHtml(startingLat, startingLon, initialBearing);
            view->page()->setHtml(currentPage);
        }
    }
    else
    {
        markerIsVisible = false;
        setIsBlank(true);
    }
}

// Helper method to build route coordinates from ErgFile
void StreetView3DWindow::buildRouteLatLngs(ErgFile* f) {
    routeLatLngs = "[";
    for (int pt = 0; pt < f->Points.size(); pt++) {
        geolocation geoloc(f->Points[pt].lat, f->Points[pt].lon, f->Points[pt].y);
        if (geoloc.IsReasonableGeoLocation()) {
            if (routeLatLngs != "[") { routeLatLngs += ","; }
            routeLatLngs += "[";
            routeLatLngs += QVariant(f->Points[pt].lat).toString();
            routeLatLngs += ",";
            routeLatLngs += QVariant(f->Points[pt].lon).toString();
            routeLatLngs += "]";
        }
    }
    routeLatLngs += "]";
}

// Reset map to preferred View when the activity is stopped.
void StreetView3DWindow::stop()
{
    markerIsVisible = false;
}

void StreetView3DWindow::configChanged(qint32)
{
    // tinted palette for headings etc
    QPalette palette;
    palette.setBrush(QPalette::Window, QBrush(GColor(CPLOTBACKGROUND)));
    palette.setColor(QPalette::WindowText, GColor(CPLOTMARKER));
    palette.setColor(QPalette::Text, GColor(CPLOTMARKER));
    palette.setColor(QPalette::Base, GCColor::alternateColor(GColor(CPLOTBACKGROUND)));
    setPalette(palette);
}

// Update position on the map when telemetry changes.
void StreetView3DWindow::telemetryUpdate(RealtimeData rtd)
{
    if (!isVisible())
        return;

    QString code = "";
    geolocation geoloc(rtd.getLatitude(), rtd.getLongitude(), rtd.getAltitude());
    if (geoloc.IsReasonableGeoLocation()) {
        double newLat = rtd.getLatitude();
        double newLon = rtd.getLongitude();
        QString sLat = QVariant(newLat).toString();
        QString sLon = QVariant(newLon).toString();
        
        // Calculate new bearing if we've moved using Haversine distance approximation
        if (lastLat != 0 && lastLon != 0) {
            // Use cosine of latitude for a rough but more accurate distance estimate
            double latDiff = toRadians(newLat - lastLat);
            double lonDiff = toRadians(newLon - lastLon) * cos(toRadians((newLat + lastLat) / 2.0));
            double distance = sqrt(latDiff * latDiff + lonDiff * lonDiff);
            if (distance > 0.0000001) { // Only update bearing if we've moved significantly (~11m at equator)
                currentBearing = calculateBearing(lastLat, lastLon, newLat, newLon);
            }
        }
        
        QString sBearing = QString::number(currentBearing);
        
        if (!markerIsVisible)
        {
            code += QString("centerMapWithBearing(%1, %2, %3);").arg(sLat).arg(sLon).arg(sBearing);
            code += QString("showMyMarker(%1, %2);").arg(sLat).arg(sLon);
            markerIsVisible = true;
        }
        else
        {
            code += QString("updateView(%1, %2, %3);").arg(sLat).arg(sLon).arg(sBearing);
        }
        
        // Update last known position
        lastLat = newLat;
        lastLon = newLon;
        
        view->page()->runJavaScript(code);
    }
}

// Build HTML code with all the javascript functions for 3D perspective view
void StreetView3DWindow::createHtml(double startLat, double startLon, double bearing)
{
    currentPage = "";
    
    int tiltValue = customTilt->value();
    int zoomValue = customZoom->value();
    QString sLat = QString::number(startLat, 'g', 10);
    QString sLon = QString::number(startLon, 'g', 10);
    QString sBearing = QString::number(bearing);
    QString sZoom = QString::number(zoomValue);
    QString sTilt = QString::number(tiltValue);

    currentPage = QString(
        "<!DOCTYPE html>\n"
        "<html><head>\n"
        "<meta name=\"viewport\" content=\"initial-scale=1.0, user-scalable=yes\"/> \n"
        "<meta http-equiv=\"content-type\" content=\"text/html; charset=UTF-8\"/>\n"
        "<title>GoldenCheetah 3D Street View</title>\n"
        
        // Leaflet CSS and JS (using 1.6.0 for consistency with LiveMapWebPageWindow)
        "<link rel=\"stylesheet\" href=\"https://unpkg.com/leaflet@1.6.0/dist/leaflet.css\"\n"
        "integrity=\"sha512-xwE/Az9zrjBIphAcBb3F6JVqxf46+CDLwfLMHloNu6KEQCAWi6HcDUbeOfBIptF7tcCzusKFjFw2yuvEpDL9wQ==\" crossorigin=\"\"/>\n"
        "<script src=\"https://unpkg.com/leaflet@1.6.0/dist/leaflet.js\"\n"
        "integrity=\"sha512-gZwIG9x3wUXg2hdXF6+rVkLF/0Vi9U8D2Ntg4Ga5I5BZpVkVxlJWbSQtXPSiUTtC0TjtGOmxa1AJPuV0CPthew==\" crossorigin=\"\"></script>\n"
        
        // Custom CSS for 3D effect
        "<style>\n"
        "html, body { height: 100%; margin: 0; padding: 0; overflow: hidden; }\n"
        "#mapid { height: 100%; width: 100%; }\n"
        ".leaflet-container {\n"
        "    perspective: 1000px;\n"
        "    perspective-origin: 50% 100%;\n"
        "}\n"
        ".leaflet-map-pane {\n"
        "    transform-style: preserve-3d;\n"
        "}\n"
        "#map-wrapper {\n"
        "    width: 100%;\n"
        "    height: 100%;\n"
        "    position: relative;\n"
        "    overflow: hidden;\n"
        "}\n"
        "#mapid {\n"
        "    transform-origin: center bottom;\n"
        "    transform: perspective(1000px) rotateX(" + sTilt + "deg);\n"
        "    height: 150%;\n"  // Extend height to fill view when tilted
        "    margin-top: -25%;\n"
        "}\n"
        // Compass overlay
        ".compass {\n"
        "    position: absolute;\n"
        "    top: 20px;\n"
        "    right: 20px;\n"
        "    width: 80px;\n"
        "    height: 80px;\n"
        "    z-index: 1000;\n"
        "    pointer-events: none;\n"
        "}\n"
        ".compass svg {\n"
        "    width: 100%;\n"
        "    height: 100%;\n"
        "}\n"
        // Speed/info overlay
        ".info-overlay {\n"
        "    position: absolute;\n"
        "    bottom: 20px;\n"
        "    left: 50%;\n"
        "    transform: translateX(-50%);\n"
        "    z-index: 1000;\n"
        "    background: rgba(0,0,0,0.6);\n"
        "    padding: 10px 20px;\n"
        "    border-radius: 10px;\n"
        "    color: white;\n"
        "    font-family: Arial, sans-serif;\n"
        "    text-align: center;\n"
        "}\n"
        "</style></head>\n"
        
        "<body>\n"
        "<div id=\"map-wrapper\">\n"
        "    <div id=\"mapid\"></div>\n"
        "    <div class=\"compass\" id=\"compass\">\n"
        "        <svg viewBox=\"0 0 100 100\">\n"
        "            <circle cx=\"50\" cy=\"50\" r=\"45\" fill=\"rgba(255,255,255,0.9)\" stroke=\"#333\" stroke-width=\"2\"/>\n"
        "            <polygon id=\"compass-needle\" points=\"50,10 45,50 50,45 55,50\" fill=\"red\" transform-origin=\"50 50\"/>\n"
        "            <polygon points=\"50,90 45,50 50,55 55,50\" fill=\"#333\" transform-origin=\"50 50\"/>\n"
        "            <text x=\"50\" y=\"25\" text-anchor=\"middle\" font-size=\"12\" font-weight=\"bold\">N</text>\n"
        "            <text x=\"50\" y=\"82\" text-anchor=\"middle\" font-size=\"10\">S</text>\n"
        "            <text x=\"18\" y=\"54\" text-anchor=\"middle\" font-size=\"10\">W</text>\n"
        "            <text x=\"82\" y=\"54\" text-anchor=\"middle\" font-size=\"10\">E</text>\n"
        "        </svg>\n"
        "    </div>\n"
        "</div>\n"
        
        "<script type=\"text/javascript\">\n"
        "var mymap, mylayer, mymarker, routepolyline, currentBearing = " + sBearing + ";\n"
        
        // Initialize map
        "function initMap(myLat, myLon, myZoom, bearing) {\n"
        "    var mapOptions = {\n"
        "        center: [myLat, myLon],\n"
        "        zoom: myZoom,\n"
        "        zoomControl: false,\n"
        "        scrollWheelZoom: false,\n"
        "        dragging: false,\n"
        "        doubleClickZoom: false,\n"
        "        attributionControl: false\n"
        "    };\n"
        "    mymap = L.map('mapid', mapOptions);\n"
        "    mylayer = new L.tileLayer('https://{s}.tile.openstreetmap.org/{z}/{x}/{y}.png', {\n"
        "        maxZoom: 20\n"
        "    });\n"
        "    mymap.addLayer(mylayer);\n"
        "    updateCompass(bearing);\n"
        "}\n"
        
        // Show position marker
        "function showMyMarker(myLat, myLon) {\n"
        "    if (mymarker) {\n"
        "        mymap.removeLayer(mymarker);\n"
        "    }\n"
        // Custom arrow marker pointing in direction of travel
        "    var arrowIcon = L.divIcon({\n"
        "        className: 'arrow-marker',\n"
        "        html: '<div style=\"transform: rotate(' + currentBearing + 'deg); transform-origin: center center;\">' +\n"
        "              '<svg width=\"40\" height=\"40\" viewBox=\"0 0 40 40\">' +\n"
        "              '<polygon points=\"20,0 10,35 20,28 30,35\" fill=\"#4285f4\" stroke=\"white\" stroke-width=\"2\"/>' +\n"
        "              '</svg></div>',\n"
        "        iconSize: [40, 40],\n"
        "        iconAnchor: [20, 20]\n"
        "    });\n"
        "    mymarker = L.marker([myLat, myLon], { icon: arrowIcon }).addTo(mymap);\n"
        "}\n"
        
        // Move marker and update view
        "function moveMarker(myLat, myLon) {\n"
        "    if (mymarker) {\n"
        "        mymarker.setLatLng([myLat, myLon]);\n"
        "    }\n"
        "    mymap.panTo([myLat, myLon]);\n"
        "}\n"
        
        // Center map with bearing
        "function centerMapWithBearing(myLat, myLon, bearing) {\n"
        "    currentBearing = bearing;\n"
        "    mymap.setView([myLat, myLon], " + sZoom + ");\n"
        "    updateCompass(bearing);\n"
        "}\n"
        
        // Update view with new position and bearing
        "function updateView(myLat, myLon, bearing) {\n"
        "    currentBearing = bearing;\n"
        "    if (mymarker) {\n"
        "        mymap.removeLayer(mymarker);\n"
        "    }\n"
        "    showMyMarker(myLat, myLon);\n"
        "    mymap.panTo([myLat, myLon]);\n"
        "    updateCompass(bearing);\n"
        "}\n"
        
        // Update compass rotation
        "function updateCompass(bearing) {\n"
        "    var needle = document.getElementById('compass-needle');\n"
        "    if (needle) {\n"
        "        needle.setAttribute('transform', 'rotate(' + bearing + ' 50 50)');\n"
        "    }\n"
        "}\n"
        
        // Show route polyline
        "function showRoute(myRouteLatlngs) {\n"
        "    if (routepolyline) {\n"
        "        mymap.removeLayer(routepolyline);\n"
        "    }\n"
        "    routepolyline = L.polyline(myRouteLatlngs, { \n"
        "        color: '#FF4444',\n"
        "        weight: 6,\n"
        "        opacity: 0.8\n"
        "    }).addTo(mymap);\n"
        "}\n"
        
        // Initialize on load
        "initMap(" + sLat + ", " + sLon + ", " + sZoom + ", " + sBearing + ");\n"
        "showRoute(" + routeLatLngs + ");\n"
        
        "</script>\n"
        "</body></html>\n"
    );
}
