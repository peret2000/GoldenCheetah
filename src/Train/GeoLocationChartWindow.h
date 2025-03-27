#ifndef _GC_GeoLocationChartWindow_h
#define _GC_GeoLocationChartWindow_h 1

#include <QObject>

#include "GoldenCheetah.h"

class RealtimeData;

class Context;
class ScalingLabel;
class QLabel;
class QSpinBox;
class QPropertyAnimation;

class GeolocationManager;
class GeoLocationChartWindow : public GcChartWindow
{
	Q_OBJECT
    G_OBJECT

    // properties can be saved/restored/set by the layout manager
    Q_PROPERTY(int updateInterval READ updateInterval WRITE setUpdateInterval USER true)
	Q_PROPERTY(int linesLength READ linesLength WRITE setLinesLength USER true)

	public:
		GeoLocationChartWindow(Context *context);
	    virtual ~GeoLocationChartWindow();

        // set/get properties
        int updateInterval() const { return customUpdateInterval->value(); }
        void setUpdateInterval(int x) { customUpdateInterval->setValue(x); }
        int linesLength() const { return customLinesLength->value(); }
        void setLinesLength(int x) { customLinesLength->setValue(x); }

	public slots:
		void telemetryUpdate(const RealtimeData &rtData);
        void start();
        void stop();
		void pause();
		void unpause();

    protected:
        void showEvent(QShowEvent *event) override;
        void hideEvent(QHideEvent *event) override;

	private:
		Context *context;

		// Settings data
        QLabel* customUpdateIntervalLabel;
        QSpinBox* customUpdateInterval;

        QLabel* customLinesLengthLabel;
        QSpinBox* customLinesLength;

		// display
		ScalingLabel *valueLabel;
		QPropertyAnimation* backgroundAnimation;
		//void setupBackgroundAnimation(QLabel *label);

		// Geolocation object
		GeolocationManager *m_geolocationManager;
		QString m_lastAddress;

};

#endif // _GC_GeoLocationChartWindow_h
