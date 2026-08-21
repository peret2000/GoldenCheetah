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

#include "HtmlActivitiesBridge.h"
#include "Context.h"
#include "RideFile.h"
#include "Athlete.h"
#include "Settings.h"
#include "RideItem.h"
#include "Specification.h"
#include "RideMetadata.h"
#include "RideMetric.h"
#include "SpecialFields.h"
#include "Colors.h"
#include "PaceZones.h"
#include "Zones.h"
#include "HrZones.h"

#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QMap>
#include <QDate>

HtmlActivitiesBridge::HtmlActivitiesBridge(Context *context, QObject *parent)
    : QObject(parent), m_context(context)
{
    Q_ASSERT(m_context != nullptr);

    if (m_context) {
        connect(m_context, &Context::rideSelected, this, &HtmlActivitiesBridge::onRideSelected);
        connect(m_context, &Context::rideChanged, this, &HtmlActivitiesBridge::onRideSelected);
    }
}

HtmlActivitiesBridge::~HtmlActivitiesBridge()
{
}

void HtmlActivitiesBridge::onRideSelected(RideItem *)
{
    emit activityChanged();
}

QString HtmlActivitiesBridge::activity()
{
    if (!m_context || !m_context->rideItem()) return "{}";
    RideFile *f = m_context->rideItem()->ride();
    if (!f) return "{}";

    QJsonObject rd;
    QMap<RideFile::SeriesType, QJsonArray> seriesArrays;

    for (int type = 0; type < (int)RideFile::none; ++type) {
        auto st = static_cast<RideFile::SeriesType>(type);
        if (f->isDataPresent(st)) {
            seriesArrays[st] = QJsonArray();
        }
    }

    RideFileIterator it(f, Specification());
    while (it.hasNext()) {
        struct RideFilePoint *point = it.next();
        for (auto st = seriesArrays.keyBegin(); st != seriesArrays.keyEnd(); ++st) {
            seriesArrays[*st].append(point->value(*st));
        }
    }

    for (auto st = seriesArrays.keyBegin(); st != seriesArrays.keyEnd(); ++st) {
        QString name = RideFile::seriesName(*st, true);
        rd[name] = seriesArrays[*st];
    }

    // Add XData
    for (const QString &name : f->xdata().keys()) {
        XDataSeries *xd = f->xdata(name);
        if (xd) {
            QJsonArray secsArr;
            QJsonArray kmArr;
            for (int i = 0; i < xd->datapoints.count(); ++i) {
                secsArr.append(xd->datapoints[i]->secs);
                kmArr.append(xd->datapoints[i]->km);
            }
            rd[QString("%1_secs").arg(name)] = secsArr;
            rd[QString("%1_km").arg(name)] = kmArr;

            for (const QString &serie : xd->valuename) {
                int sidx = xd->valuename.indexOf(serie);
                if (sidx >= 0) {
                    QJsonArray arr;
                    for (int i = 0; i < xd->datapoints.count(); ++i) {
                        arr.append(xd->datapoints[i]->number[sidx]);
                    }
                    rd[QString("%1_%2").arg(name).arg(serie)] = arr;
                }
            }
        }
    }

    return QString::fromUtf8(QJsonDocument(rd).toJson(QJsonDocument::Compact));
}

QString HtmlActivitiesBridge::activityMetrics()
{
    if (!m_context || !m_context->rideItem()) return "{}";
    RideItem *item = m_context->rideItem();

    QJsonObject rd;

    // Date and time
    rd["date"] = item->dateTime.date().toString(Qt::ISODate);
    rd["time"] = item->dateTime.time().toString(Qt::ISODate);

    // Metrics
    const RideMetricFactory &factory = RideMetricFactory::instance();
    for(int i=0; i<factory.metricCount();i++) {
        QString symbol = factory.metricName(i);
        const RideMetric *metric = factory.rideMetric(symbol);
        QString name = SpecialFields::getInstance().internalName(factory.rideMetric(symbol)->name());
        name = name.replace(" ","_");
        name = name.replace("'","_");

        bool useMetricUnits = GlobalContext::context()->useMetricUnits;
        double value = item->metrics()[i] * (useMetricUnits ? 1.0f : metric->conversion()) + (useMetricUnits ? 0.0f : metric->conversionSum());

        rd[name] = value;
    }

    // Meta
    foreach(FieldDefinition field, GlobalContext::context()->rideMetadata->getFields()) {
        if (field.name == "" || field.tab == "" || SpecialFields::getInstance().isMetric(field.name)) continue;
        rd[field.name.replace(" ", "_")] = item->getText(field.name, "");
    }

    // Color
    QString color;
    if (item->color == QColor(1,1,1,1)) {
        QColor col = GCColor::invertColor(GColor(CPLOTBACKGROUND));
        if (col == QColor(Qt::white)) col = QColor(127,127,127);
        color = col.name();
    } else {
        color = item->color.name();
    }
    rd["color"] = color;

    return QString::fromUtf8(QJsonDocument(rd).toJson(QJsonDocument::Compact));
}

QString HtmlActivitiesBridge::series(const QString &metric)
{
    if (!m_context || !m_context->rideItem()) return "[]";
    RideFile *f = m_context->rideItem()->ride();
    if (!f) return "[]";

    RideFile::SeriesType st = RideFile::none;
    for (int type = 0; type < (int)RideFile::none; ++type) {
        auto t = static_cast<RideFile::SeriesType>(type);
        if (RideFile::seriesName(t, true).compare(metric, Qt::CaseInsensitive) == 0 ||
            RideFile::seriesName(t, false).compare(metric, Qt::CaseInsensitive) == 0) {
            st = t;
            break;
        }
    }

    if (st != RideFile::none && f->isDataPresent(st)) {
        QJsonArray arr;
        RideFileIterator it(f, Specification());
        while (it.hasNext()) {
            struct RideFilePoint *point = it.next();
            arr.append(point->value(st));
        }
        return QString::fromUtf8(QJsonDocument(arr).toJson(QJsonDocument::Compact));
    }

    return "[]";
}

QString HtmlActivitiesBridge::xdataSeries(const QString &name, const QString &series)
{
    if (!m_context || !m_context->rideItem()) return "[]";
    RideFile *f = m_context->rideItem()->ride();
    if (!f) return "[]";

    XDataSeries *xd = f->xdata(name);
    if (xd) {
        int sidx = xd->valuename.indexOf(series);
        if (sidx >= 0) {
            QJsonArray arr;
            for (int i = 0; i < xd->datapoints.count(); ++i) {
                arr.append(xd->datapoints[i]->number[sidx]);
            }
            return QString::fromUtf8(QJsonDocument(arr).toJson(QJsonDocument::Compact));
        } else if (series == "secs") {
            QJsonArray arr;
            for (int i = 0; i < xd->datapoints.count(); ++i) {
                arr.append(xd->datapoints[i]->secs);
            }
            return QString::fromUtf8(QJsonDocument(arr).toJson(QJsonDocument::Compact));
        } else if (series == "km") {
            QJsonArray arr;
            for (int i = 0; i < xd->datapoints.count(); ++i) {
                arr.append(xd->datapoints[i]->km);
            }
            return QString::fromUtf8(QJsonDocument(arr).toJson(QJsonDocument::Compact));
        }
    }
    return "[]";
}

namespace {
    struct gcZoneConfig {
        gcZoneConfig(QString sport) : sport(sport), date(QDate(1,1,1)), cp(0), wprime(0), pmax(0), aetp(0), ftp(0), lthr(0), aethr(0), rhr(0), hrmax(0), cv(0), aetv(0) {}
        bool operator<(const gcZoneConfig& rhs) const { return date < rhs.date; }

        QString sport;
        QDate date;
        QList<int> zoneslow;
        QList<int> hrzoneslow;
        QList<double> pacezoneslow;
        int cp, wprime, pmax, aetp, ftp, lthr, aethr, rhr, hrmax;
        double cv, aetv;
    };
}

QString HtmlActivitiesBridge::athleteZones(const QString &dateStr, const QString &sportFilter)
{
    if (!m_context || !m_context->athlete) return "{}";

    QList<gcZoneConfig> config;
    QDate forDate;

    if (!dateStr.isEmpty()) {
        forDate = QDate::fromString(dateStr, Qt::ISODate);
    }

    if (forDate.isValid()) {
        foreach (QString sp, GlobalContext::context()->rideMetadata->sports()) {
            if (!sportFilter.isEmpty() && sportFilter.compare(sp, Qt::CaseInsensitive) != 0) continue;

            if (m_context->athlete->zones(sp)) {
                int range = m_context->athlete->zones(sp)->whichRange(forDate);
                if (range >= 0) {
                    gcZoneConfig c(sp);
                    c.date = forDate;
                    c.cp = m_context->athlete->zones(sp)->getCP(range);
                    c.wprime = m_context->athlete->zones(sp)->getWprime(range);
                    c.pmax = m_context->athlete->zones(sp)->getPmax(range);
                    c.aetp = m_context->athlete->zones(sp)->getAeT(range);
                    c.ftp = m_context->athlete->zones(sp)->getFTP(range);
                    c.zoneslow = m_context->athlete->zones(sp)->getZoneLows(range);
                    config << c;
                }
            }

            if (m_context->athlete->hrZones(sp)) {
                int range = m_context->athlete->hrZones(sp)->whichRange(forDate);
                if (range >= 0) {
                    gcZoneConfig c(sp);
                    c.date = forDate;
                    c.lthr = m_context->athlete->hrZones(sp)->getLT(range);
                    c.aethr = m_context->athlete->hrZones(sp)->getAeT(range);
                    c.rhr = m_context->athlete->hrZones(sp)->getRestHr(range);
                    c.hrmax = m_context->athlete->hrZones(sp)->getMaxHr(range);
                    c.hrzoneslow = m_context->athlete->hrZones(sp)->getZoneLows(range);
                    config << c;
                }
            }

            if ((sp == "Run" || sp == "Swim") && m_context->athlete->paceZones(sp == "Swim")) {
                int range = m_context->athlete->paceZones(sp == "Swim")->whichRange(forDate);
                if (range >= 0) {
                    gcZoneConfig c(sp);
                    c.date = forDate;
                    c.cv = m_context->athlete->paceZones(sp == "Swim")->getCV(range);
                    c.aetv = m_context->athlete->paceZones(sp == "Swim")->getAeT(range);
                    c.pacezoneslow = m_context->athlete->paceZones(sp == "Swim")->getZoneLows(range);
                    config << c;
                }
            }
        }
    } else {
        foreach (QString sp, GlobalContext::context()->rideMetadata->sports()) {
            if (!sportFilter.isEmpty() && sportFilter.compare(sp, Qt::CaseInsensitive) != 0) continue;

            if (m_context->athlete->zones(sp)) {
                for (int range = 0; range < m_context->athlete->zones(sp)->getRangeSize(); range++) {
                    gcZoneConfig c(sp);
                    c.date = m_context->athlete->zones(sp)->getStartDate(range);
                    c.cp = m_context->athlete->zones(sp)->getCP(range);
                    c.wprime = m_context->athlete->zones(sp)->getWprime(range);
                    c.pmax = m_context->athlete->zones(sp)->getPmax(range);
                    c.aetp = m_context->athlete->zones(sp)->getAeT(range);
                    c.ftp = m_context->athlete->zones(sp)->getFTP(range);
                    c.zoneslow = m_context->athlete->zones(sp)->getZoneLows(range);
                    config << c;
                }
            }

            if (m_context->athlete->hrZones(sp)) {
                for (int range = 0; range < m_context->athlete->hrZones(sp)->getRangeSize(); range++) {
                    gcZoneConfig c(sp);
                    c.date = m_context->athlete->hrZones(sp)->getStartDate(range);
                    c.lthr = m_context->athlete->hrZones(sp)->getLT(range);
                    c.aethr = m_context->athlete->hrZones(sp)->getAeT(range);
                    c.rhr = m_context->athlete->hrZones(sp)->getRestHr(range);
                    c.hrmax = m_context->athlete->hrZones(sp)->getMaxHr(range);
                    c.hrzoneslow = m_context->athlete->hrZones(sp)->getZoneLows(range);
                    config << c;
                }
            }

            if ((sp == "Run" || sp == "Swim") && m_context->athlete->paceZones(sp == "Swim")) {
                for (int range = 0; range < m_context->athlete->paceZones(sp == "Swim")->getRangeSize(); range++) {
                    gcZoneConfig c(sp);
                    c.date = m_context->athlete->paceZones(sp == "Swim")->getStartDate(range);
                    c.cv = m_context->athlete->paceZones(sp == "Swim")->getCV(range);
                    c.aetv = m_context->athlete->paceZones(sp == "Swim")->getAeT(range);
                    c.pacezoneslow = m_context->athlete->paceZones(sp == "Swim")->getZoneLows(range);
                    config << c;
                }
            }
        }
    }

    if (config.isEmpty()) return "{}";

    // Compress by sport
    QList<gcZoneConfig> compressed;
    std::sort(config.begin(), config.end());

    foreach (QString sp, GlobalContext::context()->rideMetadata->sports()) {
        if (!sportFilter.isEmpty() && sportFilter.compare(sp, Qt::CaseInsensitive) != 0) continue;

        gcZoneConfig last(sp);
        foreach (const gcZoneConfig &x, config) {
            if (x.sport == sp) {
                if (x.date > last.date) {
                    if (last.date > QDate(1,1,1)) compressed << last;
                    last.date = x.date;
                }
                if (x.date == last.date) {
                    if (x.cp) last.cp = x.cp;
                    if (x.wprime) last.wprime = x.wprime;
                    if (x.pmax) last.pmax = x.pmax;
                    if (x.aetp) last.aetp = x.aetp;
                    if (x.ftp) last.ftp = x.ftp;
                    if (x.lthr) last.lthr = x.lthr;
                    if (x.aethr) last.aethr = x.aethr;
                    if (x.rhr) last.rhr = x.rhr;
                    if (x.hrmax) last.hrmax = x.hrmax;
                    if (x.cv) last.cv = x.cv;
                    if (x.aetv) last.aetv = x.aetv;
                    if (!x.zoneslow.isEmpty()) last.zoneslow = x.zoneslow;
                    if (!x.hrzoneslow.isEmpty()) last.hrzoneslow = x.hrzoneslow;
                    if (!x.pacezoneslow.isEmpty()) last.pacezoneslow = x.pacezoneslow;
                }
            }
        }
        if (last.date > QDate(1,1,1)) compressed << last;
    }

    config = compressed;
    std::sort(config.begin(), config.end());

    // Build JSON dictionary of arrays
    QJsonObject dict;
    QJsonArray dates, sports, cp, wprime, pmax, aetp, ftp, lthr, aethr, rhr, hrmax, cv, aetv;
    QJsonArray zoneslow, hrzoneslow, pacezoneslow;

    foreach (const gcZoneConfig &x, config) {
        dates.append(x.date.toString(Qt::ISODate));
        sports.append(x.sport);
        cp.append(x.cp);
        wprime.append(x.wprime);
        pmax.append(x.pmax);
        aetp.append(x.aetp);
        ftp.append(x.ftp);
        lthr.append(x.lthr);
        aethr.append(x.aethr);
        rhr.append(x.rhr);
        hrmax.append(x.hrmax);
        cv.append(x.cv);
        aetv.append(x.aetv);

        QJsonArray zl, hzl, pzl;
        for (int v : x.zoneslow) zl.append(v);
        for (int v : x.hrzoneslow) hzl.append(v);
        for (double v : x.pacezoneslow) pzl.append(v);
        zoneslow.append(zl);
        hrzoneslow.append(hzl);
        pacezoneslow.append(pzl);
    }

    dict["dates"] = dates;
    dict["sports"] = sports;
    dict["cp"] = cp;
    dict["wprime"] = wprime;
    dict["pmax"] = pmax;
    dict["aetp"] = aetp;
    dict["ftp"] = ftp;
    dict["lthr"] = lthr;
    dict["aethr"] = aethr;
    dict["rhr"] = rhr;
    dict["hrmax"] = hrmax;
    dict["cv"] = cv;
    dict["aetv"] = aetv;
    dict["zoneslow"] = zoneslow;
    dict["hrzoneslow"] = hrzoneslow;
    dict["pacezoneslow"] = pacezoneslow;

    return QString::fromUtf8(QJsonDocument(dict).toJson(QJsonDocument::Compact));
}

QString HtmlActivitiesBridge::athlete()
{
    if (!m_context || !m_context->athlete) return "{}";

    QJsonObject obj;
    obj["name"] = m_context->athlete->cyclist;
    obj["home"] = m_context->athlete->home->root().absolutePath();

    QDate dob = appsettings->cvalue(m_context->athlete->cyclist, GC_DOB).toDate();
    if (dob.isValid()) {
        obj["dob"] = dob.toString(Qt::ISODate);
    }

    obj["weight"] = appsettings->cvalue(m_context->athlete->cyclist, GC_WEIGHT).toDouble();
    obj["height"] = appsettings->cvalue(m_context->athlete->cyclist, GC_HEIGHT).toDouble();

    int isfemale = appsettings->cvalue(m_context->athlete->cyclist, GC_SEX).toInt();
    obj["gender"] = isfemale ? "female" : "male";

    return QString::fromUtf8(QJsonDocument(obj).toJson(QJsonDocument::Compact));
}
