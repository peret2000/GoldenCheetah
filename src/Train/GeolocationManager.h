#ifndef _GC_GeolocationManager_h
#define _GC_GeolocationManager_h 1

class QGeoServiceProvider;
class QGeoCodeReply;
class QTimer;


#include <QObject>


class GeolocationManager : public QObject
{
	Q_OBJECT

	public:
        GeolocationManager();
		virtual ~GeolocationManager();

		void setLatitude(double latitude) { lat = latitude;};
		void setLongitude(double longitude) { lon = longitude;};
		void setInterval(long interval) { updateInterval = interval; };
		QString getAddress() { return m_Address; };
        void enable(long interval = 0); // If not sent interval, it does not change
		void disable();

	private slots:
		void onTimerTimeout();
		void collectAddress(QGeoCodeReply *pQGeoCodeReply);

	private:
		QTimer *updateTimer;
		long updateInterval;	// In millisecs
		bool enabled;

		// Geolocation data (QtLocation)
		QGeoServiceProvider *pQGeoProvider;
		double lat, lon;
		QString m_Address;

};

#endif // _GC_GeolocationManager_h
