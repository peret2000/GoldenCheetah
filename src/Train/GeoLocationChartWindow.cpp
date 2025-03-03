#include "GeoLocationChartWindow.h"

#include <QObject>
#include <QFrame>
#include <QColor>
#include <QFormLayout>
#include <QVBoxLayout>
#include <QLabel>
#include <QSpinBox>
#include <QTimer>
#include <QPropertyAnimation>
#include <QWidget>

#include "Context.h"
#include "HelpWhatsThis.h"
#include "ScalingLabel.h"
#include "Colors.h"

#include <QGeoAddress>
#include <QGeoCodingManager>
#include <QGeoCoordinate>
#include <QGeoLocation>
#include <QGeoServiceProvider>
#include <QGeoCodeReply>
#include <QLocale>

class AnimationFrame : public QFrame {
    Q_OBJECT
    Q_PROPERTY(QColor backgroundColor READ backgroundColor WRITE setBackgroundColor)
public:
    AnimationFrame(QWidget *parent = nullptr) : QFrame(parent) {};
    QColor backgroundColor() const { return m_backgroundColor; };
    void setBackgroundColor(const QColor &color) {
        m_backgroundColor = color;
        setStyleSheet(QString("AnimationFrame { background-color: %1; }").arg(color.name()));
    };
private:
    QColor m_backgroundColor;
};

#include "GeoLocationChartWindow.moc"   // Avoids creation .cpp and .h files for AnimationFrame


GeoLocationChartWindow::GeoLocationChartWindow(Context *context) :
	GcChartWindow(context), m_lastAddress("")
{

    HelpWhatsThis *helpContents = new HelpWhatsThis(this);
    this->setWhatsThis(helpContents->getWhatsThisText(HelpWhatsThis::ChartTrain_GeoLocation));

    QWidget *settingsWidget = new QWidget(this);
    HelpWhatsThis *helpConfig = new HelpWhatsThis(settingsWidget);
    settingsWidget->setWhatsThis(helpConfig->getWhatsThisText(HelpWhatsThis::ChartTrain_GeoLocation));
    settingsWidget->setContentsMargins(0,0,0,0);
    setControls(settingsWidget);
    QFormLayout* commonLayout = new QFormLayout(settingsWidget);

    customUpdateIntervalLabel = new QLabel(tr("Update Interval (s)"));
    customUpdateInterval = new QSpinBox(this);
    customUpdateInterval->setFixedWidth(60);
    if (customUpdateInterval->text().trimmed().isEmpty()) customUpdateInterval->setValue(60);
    customUpdateInterval->setRange(10, 600); // 10s to 10min
    customUpdateInterval->setSingleStep(10);
    customUpdateInterval->setAccelerated(true);
    customUpdateInterval->setSuffix(" s");
    customUpdateInterval->setToolTip(tr("Set the interval in seconds for updating the location from GPS coordinates"));

    commonLayout->addRow(customUpdateIntervalLabel, customUpdateInterval);

    setProperty("color", GColor(CTRAINPLOTBACKGROUND));

    // Data shown in the chart

    QVBoxLayout *mainLayout = new QVBoxLayout;
    mainLayout->setSpacing(0);
    mainLayout->setContentsMargins(3,3,3,3);

    valueLabel = new ScalingLabel(1, this);
    valueLabel->setStrategy(appsettings->value(this, TRAIN_TELEMETRY_FONT_SCALING, 0).toInt() == 0 ? ScalingLabelStrategy::HeightOnly : ScalingLabelStrategy::Linear);
    QFont vlFont = valueLabel->font();
    vlFont.setWeight(QFont::Bold);
    valueLabel->setFont(vlFont);
    valueLabel->setAlignment(Qt::AlignCenter | Qt::AlignVCenter);
    // To allow the background color to be animated, we need to set the autoFillBackground property to false
    valueLabel->setAutoFillBackground(false);
    QString textStyle = QString("QLabel { color: %1; background: transparent; }")
                        .arg(GColor(CPOWER).name());
    valueLabel->setStyleSheet(textStyle);
    
    AnimationFrame *frame = new AnimationFrame(this);
    QVBoxLayout *layout = new QVBoxLayout(frame);
    layout->setContentsMargins(0,0,0,0);
    layout->addWidget(valueLabel);
    

    // Setup the animation
    backgroundAnimation = new QPropertyAnimation(frame, "backgroundColor");
    backgroundAnimation->setDuration(3000);
    backgroundAnimation->setStartValue(GColor(CPOWER).name());
    backgroundAnimation->setEndValue(GColor(CTRAINPLOTBACKGROUND));

    mainLayout->addWidget(frame);

    setChartLayout(mainLayout);

    // Create and setup timer
    updateTimer = new QTimer(this);
    updateTimer->setSingleShot(true);

    // Geoprovider
    pQGeoProvider = new QGeoServiceProvider("osm");
    if (!pQGeoProvider) {
        qDebug() << tr("ERROR: Geolocation Widget: GeoServiceProvider not available!");
        valueLabel->setText(tr("GeoServiceProvider not available!"));
        return;
    }
    else {
        // QVariantMap parameters;
        // parameters["mapbox.access_token"] = "xxxxxxx";
        // pQGeoProvider->setParameters(parameters);
        if (!pQGeoProvider->geocodingManager()) {
            qDebug() << tr("ERROR: Geolocation Widget: GeoCodingManager not available!");
            valueLabel->setText(tr("GeoCodingManager not available!"));
            return;
        }
        else {
            QLocale qLocaleC(QLocale::Spanish, QLocale::Spain);
            pQGeoProvider->geocodingManager()->setLocale(qLocaleC);
        }
    }


    connect(updateTimer, SIGNAL(timeout()), this, SLOT(onTimerTimeout()));
    connect(context, SIGNAL(stop()), this, SLOT(stop()));
    connect(context, SIGNAL(start()), this, SLOT(start()));
    connect(context, SIGNAL(pause()), this, SLOT(stopTimer()));
    connect(context, SIGNAL(unpause()), this, SLOT(unpause()));
    connect(context, SIGNAL(telemetryUpdate(RealtimeData)), this, SLOT(telemetryUpdate(RealtimeData)));

}




void GeoLocationChartWindow::start()
{
    m_lastAddress = "";
    unpause();
}


void GeoLocationChartWindow::unpause()
{
    onTimerTimeout();
    // Not necessary, as the timer is started in onTimerTimeout
    //updateTimer->setInterval(customUpdateInterval->value()*1000);
    //updateTimer->start();
}

void GeoLocationChartWindow::stop()
{
    valueLabel->clear();
    stopTimer();
}
void GeoLocationChartWindow::stopTimer()
{
    updateTimer->stop();
}


void
GeoLocationChartWindow::telemetryUpdate(const RealtimeData &rtData)
{
    // Testing this is not worth
    // if (isHidden()) {
    //     return;
    // }

    m_rtData = rtData;
}

void GeoLocationChartWindow::onTimerTimeout()
{
    // Even if the window is hidden, we still want to start the timer, in case it is shown later
    updateTimer->setInterval(customUpdateInterval->value()*1000);   // In case the user changed the interval
    updateTimer->start();

    if (isHidden()) {
        return;
    }

    double lon = m_rtData.getLongitude();
    double lat = m_rtData.getLatitude();
    if (lon==0.0 && lat == 0.0) {
        return;
    }

    QGeoCoordinate qGeoCoord;

    qGeoCoord.setLatitude(lat);
    qGeoCoord.setLongitude(lon);
  
    QGeoCodeReply *pQGeoCodeReply
      = pQGeoProvider->geocodingManager()->reverseGeocode(qGeoCoord);
  
    if (pQGeoCodeReply) {
        QObject::connect(pQGeoCodeReply, &QGeoCodeReply::finished, this, [=]() {collectAddress(pQGeoCodeReply);});
    } else {
        valueLabel->clear();
    }
}

void GeoLocationChartWindow::collectAddress(QGeoCodeReply *pQGeoCodeReply)
{
    QString address;
    if (pQGeoCodeReply->error() != QGeoCodeReply::NoError) {
        address = tr("Error: %1").arg(pQGeoCodeReply->errorString());
    }
    else {
        QGeoLocation qGeoLocation = pQGeoCodeReply->locations().at(0);
        QGeoAddress qGeoAddress = qGeoLocation.address();
        address = qGeoAddress.text();
    }
    if (address != m_lastAddress) {
        m_lastAddress = address;
        address = address.replace(",", "\n");
        // Restart animation
        backgroundAnimation->stop();    // In case it is still running
        backgroundAnimation->start();
        valueLabel->setText(address);
    }
    pQGeoCodeReply->deleteLater();
}

GeoLocationChartWindow::~GeoLocationChartWindow()
{
    if (updateTimer) {
        updateTimer->stop();
        delete updateTimer;
    }
    if (valueLabel) {
        delete valueLabel;
    }
    if (backgroundAnimation) {
        backgroundAnimation->stop();
        delete backgroundAnimation;
    }
    if (pQGeoProvider) {
        delete pQGeoProvider;
    }
}
