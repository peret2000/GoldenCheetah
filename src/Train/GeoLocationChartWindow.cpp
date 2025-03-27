#include "GeoLocationChartWindow.h"
#include "GeolocationManager.h"

#include <QObject>
#include <QFrame>
#include <QColor>
#include <QFormLayout>
#include <QVBoxLayout>
#include <QLabel>
#include <QSpinBox>
#include <QPropertyAnimation>
#include <QWidget>

#include "RealtimeData.h"
#include "Context.h"
#include "HelpWhatsThis.h"
#include "ScalingLabel.h"
#include "Colors.h"

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
	context(context), GcChartWindow(context), m_lastAddress("")
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

    customLinesLengthLabel = new QLabel(tr("Max. lines length"));
    customLinesLength = new QSpinBox(this);
    customLinesLength->setFixedWidth(60);
    if (customLinesLength->text().trimmed().isEmpty()) customLinesLength->setValue(60);
    customLinesLength->setRange(50, 120);
    customLinesLength->setSingleStep(5);
    customLinesLength->setAccelerated(true);
    customLinesLength->setToolTip(tr("Set number of characters per line in the address text"));
    commonLayout->addRow(customLinesLengthLabel, customLinesLength);

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

    m_geolocationManager = new GeolocationManager;

    connect(context, SIGNAL(stop()), this, SLOT(stop()));
    connect(context, SIGNAL(start()), this, SLOT(start()));
    connect(context, SIGNAL(pause()), this, SLOT(pause()));
    connect(context, SIGNAL(unpause()), this, SLOT(unpause()));
    connect(context, SIGNAL(telemetryUpdate(RealtimeData)), this, SLOT(telemetryUpdate(RealtimeData)));
}

void GeoLocationChartWindow::showEvent(QShowEvent *event)
{
    GcChartWindow::showEvent(event);
    if (context->isRunning)
        m_geolocationManager->enable(customUpdateInterval->value()*1000);
}

void GeoLocationChartWindow::hideEvent(QHideEvent *event)
{
    GcChartWindow::hideEvent(event);
    m_geolocationManager->disable();
}

void GeoLocationChartWindow::start()
{
    m_lastAddress = "";
    m_geolocationManager->enable(customUpdateInterval->value()*1000);
}

void GeoLocationChartWindow::stop()
{
    valueLabel->clear();
    m_geolocationManager->disable();
}
void GeoLocationChartWindow::pause()
{
    m_geolocationManager->disable();
}

void GeoLocationChartWindow::unpause()
{
    m_geolocationManager->enable(customUpdateInterval->value()*1000);
}

void
GeoLocationChartWindow::telemetryUpdate(const RealtimeData &rtData)
{
    if (!isVisible()) {
        m_geolocationManager->disable();
        return;
    }

    // In case interval has changed at any moment
    m_geolocationManager->setInterval(customUpdateInterval->value()*1000); // in millisecs

    QString address = m_geolocationManager->getAddress();
    if (address != m_lastAddress) {
        m_lastAddress = address;

        // Split address into lines with maximum length
        int maxlength = customLinesLength->value(); // Define maximum characters per line
        QString formattedAddress;

        for (int i = 0; i < address.length(); i += maxlength) {
            if (i > 0) formattedAddress += "\n";
            formattedAddress += address.mid(i, maxlength);
        }

        // Restart animation
        backgroundAnimation->stop();    // In case it is still running
        backgroundAnimation->start();
        valueLabel->setText(formattedAddress);
    }
        // For next update, used by m_geolocationManager when it needs it
    m_geolocationManager->setLatitude(rtData.getLatitude());
    m_geolocationManager->setLongitude(rtData.getLongitude());
}

GeoLocationChartWindow::~GeoLocationChartWindow()
{
    if (valueLabel) {
        delete valueLabel;
    }
    if (backgroundAnimation) {
        backgroundAnimation->stop();
        delete backgroundAnimation;
    }
    if (m_geolocationManager) {
        delete m_geolocationManager;
    }
}
