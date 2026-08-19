#include "HtmlActivitiesChart.h"
#include "Context.h"
#include "HtmlActivitiesBridge.h"
#include "Colors.h"
#include <QWebChannel>

HtmlActivitiesChart::HtmlActivitiesChart(Context *context) : HtmlChart(context)
{
}

HtmlActivitiesChart::~HtmlActivitiesChart()
{
}

void HtmlActivitiesChart::setupBridges(QWebChannel *channel)
{
    HtmlActivitiesBridge *bridge = new HtmlActivitiesBridge(context, this);
    channel->registerObject("gc", bridge);
}

int HtmlActivitiesChart::backgroundColorIndex() const
{
    return CRIDEPLOTBACKGROUND;
}

QString HtmlActivitiesChart::defaultHtml() const
{
    return R"HTML(<!DOCTYPE html>
<html>
<head>
    <meta charset="utf-8">
    <title>Activity Chart</title>
    <script src="qrc:///qtwebchannel/qwebchannel.js"></script>
    <style>body{font-family:sans-serif;padding:12px;color:#333}</style>
</head>
<body>
    <h2>GC Activity Data</h2>
    <div id="summary">Loading metrics...</div>
    <div id="data">Loading series...</div>
    <script>
        var gc = null;
        function loadChartData() {
            if (!gc) return;
            gc.activityMetrics(function(response) {
                var metrics = JSON.parse(response);
                document.getElementById('summary').innerText = 
                    "Date: " + (metrics.date || "N/A") + " | TSS: " + (metrics.TSS || 0).toFixed(1);
            });
            gc.series("watts", function(response) {
                var watts = JSON.parse(response);
                document.getElementById('data').innerText = 
                    "Loaded " + (watts ? watts.length : 0) + " power samples.";
            });
        }
        if (typeof QWebChannel !== 'undefined') {
            new QWebChannel(qt.webChannelTransport, function(channel) {
                gc = channel.objects.gc;
                loadChartData();
                if (gc.activityChanged) {
                    gc.activityChanged.connect(function() {
                        loadChartData();
                    });
                }
            });
        }
    </script>
</body>
</html>)HTML";
}
