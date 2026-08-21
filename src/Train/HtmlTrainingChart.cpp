#include "HtmlTrainingChart.h"
#include "Context.h"
#include "HtmlTrainingBridge.h"
#include "Colors.h"
#include <QWebChannel>

HtmlTrainingChart::HtmlTrainingChart(Context *context) : HtmlChart(context)
{
    initHtmlChart();
    connect(context, SIGNAL(ergFileSelected(ErgFile*)), this, SLOT(applyHtml()));
}

HtmlTrainingChart::~HtmlTrainingChart()
{
}

void HtmlTrainingChart::setupBridges(QWebChannel *channel)
{
    HtmlTrainingBridge *bridge = context->getHtmlTrainingBridge();
    if (bridge) {
        channel->registerObject("gc", bridge);
    }
}

int HtmlTrainingChart::backgroundColorIndex() const
{
    return CTRAINPLOTBACKGROUND;
}

QString HtmlTrainingChart::defaultHtml() const
{
    return R"HTML(<!DOCTYPE html>
<html>
<head>
    <meta charset="utf-8">
    <script src="qrc:///qtwebchannel/qwebchannel.js"></script>
    <style>body{font-family:sans-serif;padding:12px;color:#333}</style>
</head>
<body>
    <h2>GC WebChannel</h2>
    <div>Speed: <b id="speed">0.0</b> km/h</div>
    <div>Power: <b id="power">0</b> W</div>
    <div>HR: <b id="hr">0</b> bpm</div>
    <div>State: <b id="state">Stopped</b></div>
    <script>
        if (typeof QWebChannel !== 'undefined') {
            new QWebChannel(qt.webChannelTransport, function(channel) {
                window.gc = channel.objects.gc;
                if (window.gc.telemetry) {
                    window.gc.telemetry.connect(function(payload) {
                        var d = typeof payload === 'string' ? JSON.parse(payload) : payload;
                        document.getElementById("speed").innerText = (d.speed_kmh || 0).toFixed(1);
                        document.getElementById("power").innerText = Math.round(d.power_w || 0);
                        document.getElementById("hr").innerText = Math.round(d.hr_bpm || 0);
                    });
                }
                if (window.gc.stateChanged) {
                    window.gc.stateChanged.connect(function(state) {
                        document.getElementById("state").innerText = state.toUpperCase();
                    });
                }
            });
        }
    </script>
</body>
</html>)HTML";
}
