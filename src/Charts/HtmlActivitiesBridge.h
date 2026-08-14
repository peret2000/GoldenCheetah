/*
 * Copyright (c) 2026 GoldenCheetah
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

#ifndef _GC_HtmlActivitiesBridge_h
#define _GC_HtmlActivitiesBridge_h

#include <QObject>
#include <QString>

class Context;
class RideItem;

/**
 * HtmlActivitiesBridge - Qt/JavaScript bridge for HTML activities pages
 *
 * Exposes activity data to HTML pages via QtWebChannel.
 * Accessible in JavaScript as: window.gc.activity(), gc.series(), gc.athlete(), etc.
 */
class HtmlActivitiesBridge : public QObject
{
    Q_OBJECT

public:
    explicit HtmlActivitiesBridge(Context *context, QObject *parent = nullptr);
    ~HtmlActivitiesBridge();

    /// Get all time series for the current activity as a JSON object
    Q_INVOKABLE QString activity();

    /// Get summary metrics for the current activity as a JSON object
    Q_INVOKABLE QString activityMetrics();

    /// Get a specific time series array as a JSON string
    Q_INVOKABLE QString series(const QString &metric);

    /// Get a specific XData series array as a JSON string
    Q_INVOKABLE QString xdataSeries(const QString &name, const QString &series);

    /// Get athlete profile as a JSON object
    Q_INVOKABLE QString athlete();

    /// Get athlete zones as a JSON object of arrays (mimics python dataframe)
    Q_INVOKABLE QString athleteZones(const QString &dateStr = "", const QString &sport = "");

signals:
    /// Emitted when the selected activity changes
    void activityChanged();

private slots:
    void onRideSelected(RideItem *ride);

private:
    Context *m_context;
};

#endif
