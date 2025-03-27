#include "GeolocationManager.h"

#include <QLoggingCategory>
#include <QTimer>

#include <QGeoAddress>
#include <QGeoCodingManager>
#include <QGeoCoordinate>
#include <QGeoLocation>
#include <QGeoServiceProvider>
#include <QGeoCodeReply>
#include <QLocale>

GeolocationManager::GeolocationManager() :
    m_Address(""), lat(0.0), lon(0.0), enabled(false), updateInterval(60000)
{
    // Create and setup timer
    updateTimer = new QTimer(this);
    updateTimer->setSingleShot(true);

    // Geoprovider
    pQGeoProvider = new QGeoServiceProvider("osm");
    if (!pQGeoProvider) {
        qDebug() << tr("ERROR: Geolocation Widget: GeoServiceProvider not available!");
        m_Address = tr("GeoServiceProvider not available!");
        return;
    }
    else {
        // QVariantMap parameters;
        // parameters["mapbox.access_token"] = "xxxxxxx";
        // pQGeoProvider->setParameters(parameters);
        if (!pQGeoProvider->geocodingManager()) {
            qDebug() << tr("ERROR: Geolocation Widget: GeoCodingManager not available!");
            m_Address = tr("GeoCodingManager not available!");
            return;
        }
        else {
            QLocale qLocaleC(QLocale::Spanish, QLocale::Spain);
            pQGeoProvider->geocodingManager()->setLocale(qLocaleC);
        }
    }

    connect(updateTimer, SIGNAL(timeout()), this, SLOT(onTimerTimeout()));
}

void GeolocationManager::enable(long interval)
{
    if (interval > 0)
        setInterval(interval);
	if (!enabled) {
		enabled = true;
		updateTimer->setInterval(updateInterval);
		updateTimer->start();
	}
}

void GeolocationManager::disable()
{
	if (enabled) {
		enabled = false;
		updateTimer->stop();
	}
}

void GeolocationManager::onTimerTimeout()
{
    updateTimer->setInterval(updateInterval);   // In case the user changed the interval
    updateTimer->start();

    if (!enabled) {
        return;
    }

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
        m_Address = "";
    }
}

void GeolocationManager::collectAddress(QGeoCodeReply *pQGeoCodeReply)
{
    QString address;
    if (pQGeoCodeReply->error() != QGeoCodeReply::NoError) {
        m_Address = tr("Error: %1").arg(pQGeoCodeReply->errorString());
    }
    else {
        QGeoLocation qGeoLocation = pQGeoCodeReply->locations().at(0);
        QGeoAddress qGeoAddress = qGeoLocation.address();
        m_Address = qGeoAddress.text();
    }
    pQGeoCodeReply->deleteLater();
}

GeolocationManager::~GeolocationManager()
{
	if (updateTimer) {
		updateTimer->stop();
		delete updateTimer;
	}

	if (pQGeoProvider) {
		delete pQGeoProvider;
	}
}
