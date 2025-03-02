#ifndef _GC_GeoLocationChartWindow_h
#define _GC_GeoLocationChartWindow_h 1

#include <QObject>

#include "GoldenCheetah.h"
#include "RealtimeData.h"

class QGeoServiceProvider;
class QGeoCodeReply;

class Context;
class ScalingLabel;
class QTimer;
class QLabel;
class QSpinBox;
class QPropertyAnimation;

class GeoLocationChartWindow : public GcChartWindow
{
	Q_OBJECT
    G_OBJECT

    // properties can be saved/restored/set by the layout manager
    Q_PROPERTY(int updateInterval READ updateInterval WRITE setUpdateInterval USER true)


	public:
		GeoLocationChartWindow(Context *context);
	    virtual ~GeoLocationChartWindow();

        // set/get properties
        int updateInterval() const { return customUpdateInterval->value(); }
        void setUpdateInterval(int x) { customUpdateInterval->setValue(x); }

	public slots:
		void telemetryUpdate(const RealtimeData &rtData);
        void start();
        void stop();
		void stopTimer();

	private slots:
		void onTimerTimeout();
		void collectAddress(QGeoCodeReply *pQGeoCodeReply);

	private:
		// Settings data
        QLabel* customUpdateIntervalLabel;
        QSpinBox* customUpdateInterval;

		// display
		ScalingLabel *valueLabel;
		QPropertyAnimation* backgroundAnimation;
		//void setupBackgroundAnimation(QLabel *label);

		RealtimeData m_rtData;
		QTimer *updateTimer;

		// Geolocation data (QtLocation)
		QGeoServiceProvider *pQGeoProvider;
};

#endif // _GC_GeoLocationChartWindow_h
