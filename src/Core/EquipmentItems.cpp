/*
 * Copyright (c) 2025 Paul Johnson (paulj49457@gmail.com)
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

#include "EquipmentItems.h"
#include "RideMetadata.h"
#include "Units.h"
#include "Utils.h"

#include <QJsonArray>
#include <QJsonValue>

// to allow integral type atomics (c++11) to be used and to get them to hold values to 3 decimal places the following
// factors are used to scale the values, this is sufficient for equipment overview usage. When c++23 is available
// this can changed to use atomic<double> and the scaling can be removed.
static const uint64_t EQ_REAL_TO_SCALED = 10000;
static const double EQ_SCALED_TO_REAL = 0.0001;

//
// ------------------------------------------ EqTimeWindow --------------------------------------------
//

EqTimeWindow::EqTimeWindow() :
     eqLinkIsCompleterVal_(false), startSet_(false), endSet_(false)
{
}

EqTimeWindow::EqTimeWindow(const QString& eqLinkName) :
    eqLinkName_(eqLinkName), startSet_(false), endSet_(false)
{
    eqLinkIsCompleterVal_ = FieldDefinition::isCompleterValue("EquipmentLink", eqLinkName_);
}

EqTimeWindow::EqTimeWindow(const QString& eqLinkName, bool startSet, const QDate& startDate, bool endSet, const QDate& endDate) :
    eqLinkName_(eqLinkName), startSet_(startSet), startDate_(startDate), endSet_(endSet), endDate_(endDate)
{
    eqLinkIsCompleterVal_ = FieldDefinition::isCompleterValue("EquipmentLink", eqLinkName_);
}

void
EqTimeWindow::reset()
{
    eqLinkName_ = "";
    eqLinkIsCompleterVal_ = false;
    startSet_ = false;
    startDate_ = QDate();
    endSet_ = false;
    endDate_ = QDate();
}

bool
EqTimeWindow::isWithin(const QDate& actDate) const
{
    if (!startSet_)
        if (!endSet_)
            return true; // no range set
        else
            return (actDate <= endDate_); // end set but not beginning !
    else
        if (!endSet_)
            return (startDate_ <= actDate);
        else
            return (startDate_ <= actDate) && (actDate <= endDate_);
}

bool
EqTimeWindow::rangeIsValid() const
{
    return (startSet_ && endSet_) ? endDate_ >= startDate_ : true;
}

//
// ------------------------------------- Abstract Equipment Item -----------------------------------------
//

AbstractEqItem::AbstractEqItem(const QUuid& equipmentRef, const QString& xmlChartName, const QString& xmlTileName, EqItemType equipmentType) :
    equipmentRef_(equipmentRef), xmlChartName_(xmlChartName), xmlTileName_(xmlTileName), equipmentType_(equipmentType)
{
}

void
AbstractEqItem::writeXml(uint32_t version, QTextStream& xmlOut) const
{
    if (version == 1) {

        xmlOut << "eqref=\"" << Utils::xmlprotect(getEquipmentRef().toString(QUuid::WithoutBraces)) << "\">\n";

        // the chart & tile names are stored and loaded from the equipment-perspectives.xml file,
        // they are only exported in the equipment-data.xml for tile identification purposes.
        xmlOut << "\t\t\t<eqchart>" << Utils::xmlprotect(xmlChartName_) << "</eqchart>\n";
        xmlOut << "\t\t\t<eqtile>" << Utils::xmlprotect(xmlTileName_) << "</eqtile>\n";
    }
}

//
// ---------------------------------------- Equipment Item --------------------------------------------
//

EqItem::EqItem(const QUuid& equipmentRef, const QString& xmlChartName, const QString& xmlTileName)
    : AbstractEqItem(equipmentRef, xmlChartName, xmlTileName, EqItemType::EQ_ITEM),
    displayTotalDistance_(true), nonGCDistance_(0), nonGCElevation_(0),
    repDistance_(0), repElevation_(0), repDateSet_(false),
    xmlScalerKmMile_(1.0), xmlScalerMtrFoot_(1.0)
{
    resetDerived();
}

EqItem::EqItem(const EqItem& toCopy)
    : AbstractEqItem(QUuid::createUuid(), toCopy.xmlChartName_, toCopy.xmlTileName_, EqItemType::EQ_ITEM)
{
    displayTotalDistance_ = toCopy.displayTotalDistance_;

    eqLinkUseList_ = toCopy.eqLinkUseList_;

    xmlScalerKmMile_ = toCopy.xmlScalerKmMile_;
    xmlScalerMtrFoot_ = toCopy.xmlScalerMtrFoot_;

    nonGCDistance_ = toCopy.nonGCDistance_;
    nonGCElevation_ = toCopy.nonGCElevation_;
    repDistance_ = toCopy.repDistance_;
    repElevation_ = toCopy.repElevation_;

    repDateSet_ = toCopy.repDateSet_;
    repDate_ = toCopy.repDate_;
    notes_ = toCopy.notes_;

    resetDerived();
}

void
EqItem::resetDerived()
{
    activities_ = 0;
    activityTimeInSecs_ = 0;
    gcDistance_ = 0;
    totalDistance_ = nonGCDistance_;
    gcElevation_ = 0;
    totalElevation_ = nonGCElevation_;
}

void
EqItem::sortEqLinkUseWindows()
{
    std::sort(eqLinkUseList_.begin(), eqLinkUseList_.end(), [](const EqTimeWindow& a, const EqTimeWindow& b)
    {
        return a.startDate_ > b.startDate_;
    });
}

void
EqItem::unitsChanged()
{
    // Need to rescale for the units change for the user entered data.
    bool metric = GlobalContext::context()->useMetricUnits;

    nonGCDistance_ = (metric) ? round(nonGCDistance_ * KM_PER_MILE) : round (nonGCDistance_ * MILES_PER_KM);
    repDistance_ = (metric) ? round(repDistance_ * KM_PER_MILE) : round(repDistance_ * MILES_PER_KM);
    nonGCElevation_ = (metric) ? round(nonGCElevation_ * METERS_PER_FOOT) : round(nonGCElevation_ * FEET_PER_METER);
    repElevation_ = (metric) ? round (repElevation_ * METERS_PER_FOOT) : round (repElevation_ * FEET_PER_METER);
}

void
EqItem::setNonGCDistance(double nonGCDistance)
{
    // using integral type atomics (c++11) but to retain accuracy multiply by EQ_REAL_TO_SCALED, see overviewItems.h
    nonGCDistance_ = nonGCDistance;
    totalDistance_ = gcDistance_ + nonGCDistance_;
}

void
EqItem::setNonGCElevation(double nonGCElevation)
{
    // using integral type atomics (c++11) but to retain accuracy multiply by EQ_REAL_TO_SCALED, see overviewItems.h
    nonGCElevation_ = nonGCElevation;
    totalElevation_ = gcElevation_ + nonGCElevation_;
}

void
EqItem::startOfCalculation()
{
    calc_activities_ = 0;
    calc_activityTimeInSecs_ = 0;
    calc_gcDistanceScaled_ = 0;
    calc_totalDistanceScaled_ = nonGCDistance_*EQ_REAL_TO_SCALED;
    calc_gcElevationScaled_ = 0;
    calc_totalElevationScaled_ = nonGCElevation_*EQ_REAL_TO_SCALED;
}

void
EqItem::addActivity(const QStringList& rideEqLinkNameList, const QDate& actDate,
                    double rideDistance, double rideElevation, uint64_t rideTimeInSecs, const QString& /* athleteName */ )
{
    if (isWithin(rideEqLinkNameList, actDate)) {

        uint64_t rideDistanceScaled = rideDistance*EQ_REAL_TO_SCALED;
        uint64_t rideElevationScaled = rideElevation*EQ_REAL_TO_SCALED;

        // Atomic safe additions
        calc_activities_ += 1;
        calc_activityTimeInSecs_ += rideTimeInSecs;
        calc_gcDistanceScaled_ += rideDistanceScaled;
        calc_totalDistanceScaled_ += rideDistanceScaled;
        calc_gcElevationScaled_ += rideElevationScaled;
        calc_totalElevationScaled_ += rideElevationScaled;
    }
}

void
EqItem::endOfCalculation()
{
    activities_ = calc_activities_.load();
    activityTimeInSecs_ = calc_activityTimeInSecs_.load();
    gcDistance_ = calc_gcDistanceScaled_.load()*EQ_SCALED_TO_REAL;
    totalDistance_ = calc_totalDistanceScaled_.load()*EQ_SCALED_TO_REAL;
    gcElevation_ = calc_gcElevationScaled_.load()*EQ_SCALED_TO_REAL;
    totalElevation_ = calc_totalElevationScaled_.load()*EQ_SCALED_TO_REAL;
}

bool
EqItem::isWithin(const QStringList& rideEqLinkNameList, const QDate& actDate) const
{
    for (const auto& eqUse : eqLinkUseList_) {

        if (rideEqLinkNameList.contains(eqUse.eqLinkName()) && eqUse.isWithin(actDate)) return true;
    }
    return false;
}

bool
EqItem::isWithin(const QDate& actDate) const
{
    for (const auto& eqUse : eqLinkUseList_) {

        if (eqUse.isWithin(actDate)) return true;
    }
    return false;
}

bool
EqItem::rangeIsValid() const
{
    bool rangeIsValid = true;
    for (const auto& eqUse : eqLinkUseList_) {
        rangeIsValid &= (eqUse.startSet_ && eqUse.endSet_) ? eqUse.endDate_ >= eqUse.startDate_ : true;
    }
    return rangeIsValid;
}

bool
EqItem::allEqLinkNamesCompleterVals() const
{
    bool allCompleterVals = true;
    for (const auto& eqUse : eqLinkUseList_) {
        if (!eqUse.eqLinkIsCompleterVal()) return false;
    }
    return allCompleterVals;
}

void
EqItem::xmlUoM(bool loadingAsMetric)
{
    // Due to lazy loading of perspectives, saved distances might need converting 
    // as the units might have changed before the equipment perspective is loaded.
    if (loadingAsMetric && !GlobalContext::context()->useMetricUnits) {
        xmlScalerKmMile_ = KM_PER_MILE; xmlScalerMtrFoot_ = METERS_PER_FOOT;
    } else if (!loadingAsMetric && GlobalContext::context()->useMetricUnits) {
        xmlScalerKmMile_ = MILES_PER_KM; xmlScalerMtrFoot_ = FEET_PER_METER;
    } else {
        xmlScalerKmMile_ = xmlScalerMtrFoot_ = 1.0;
    }
}

bool
EqItem::parseXml(uint32_t version, QXmlStreamReader& reader)
{
    if (version == 1) {

        EqTimeWindow windowToLoad;
        bool done = false;

        while (!reader.atEnd() && !reader.hasError() && !done) {
            QXmlStreamReader::TokenType token = reader.readNext();

            if (token == QXmlStreamReader::StartElement) {

                // the chart & tile names are stored and loaded from the equipment-perspectives.xml file,
                // they are not imported from the equipment-data.xml.

                if (reader.name() == "displaytotal") displayTotalDistance_ = (reader.readElementText() != "elevation");
                else if (reader.name() == "nongcdistance") setNonGCDistance(reader.readElementText().toDouble() * xmlScalerKmMile_);
                else if (reader.name() == "nongcelevation") setNonGCElevation(reader.readElementText().toDouble() * xmlScalerMtrFoot_);
                else if (reader.name() == "repdistance") repDistance_ = reader.readElementText().toDouble() * xmlScalerKmMile_;
                else if (reader.name() == "repelevation") repElevation_ = reader.readElementText().toDouble() * xmlScalerMtrFoot_;
                else if (reader.name() == "repdate") {
                    QString date(Utils::unprotect(reader.readElementText()));
                    repDateSet_ = (date != "");
                    repDate_ = repDateSet_ ? QDate::fromString(date) : QDate();
                }
                else if (reader.name() == "equipmentuse") windowToLoad = EqTimeWindow(Utils::unprotect(reader.attributes().value("eqlink").toString()));
                else if (reader.name() == "startdate") {
                    QString date(Utils::unprotect(reader.readElementText()));
                    windowToLoad.startSet_ = (date != "");
                    windowToLoad.startDate_ = windowToLoad.startSet_ ? QDate::fromString(date) : QDate();
                }
                else if (reader.name() == "enddate") {
                    QString date(Utils::unprotect(reader.readElementText()));
                    windowToLoad.endSet_ = (date != "");
                    windowToLoad.endDate_ = windowToLoad.endSet_ ? QDate::fromString(date) : QDate();
                }
                else if (reader.name() == "notes") notes_ = Utils::unprotect(reader.readElementText());

            } else if (token == QXmlStreamReader::EndElement) {

                if (reader.name() == "equipmentuse") eqLinkUseList_.push_back(windowToLoad);
                else if (reader.name() == "equipmentitem") done = true;
            }
        }
        return true;
    }

    qDebug() << "EqItem::parseXml - invalid file version:" << version;
    return false;
}

void
EqItem::writeXml(uint32_t version, QTextStream& xmlOut) const
{
    if (version == 1) {

        xmlOut << "\t\t<equipmentitem ";

        AbstractEqItem::writeXml(version, xmlOut);

        xmlOut << "\t\t\t<displaytotal>" << (displayTotalDistance_ ? "distance" : "elevation") << "</displaytotal>\n";
        xmlOut << "\t\t\t<nongcdistance>" << nonGCDistance_ << "</nongcdistance>\n";
        xmlOut << "\t\t\t<nongcelevation>" << nonGCElevation_ << "</nongcelevation>\n";
        xmlOut << "\t\t\t<repdistance>" << repDistance_ << "</repdistance>\n";
        xmlOut << "\t\t\t<repelevation>" << repElevation_ << "</repelevation>\n";
        xmlOut << "\t\t\t<repdate>" << (repDateSet_ ? Utils::xmlprotect(repDate_.toString()) : "") << "</repdate>\n";

        for (const auto& eqUse : eqLinkUseList_) {
            xmlOut << "\t\t\t<equipmentuse eqlink=\"" << Utils::xmlprotect(eqUse.eqLinkName()) << "\">\n";
            xmlOut << "\t\t\t\t<startdate>" << (eqUse.startSet_ ? Utils::xmlprotect(eqUse.startDate_.toString()) : "") << "</startdate>\n";
            xmlOut << "\t\t\t\t<enddate>" << (eqUse.endSet_ ? Utils::xmlprotect(eqUse.endDate_.toString()) : "") << "</enddate>\n";
            xmlOut << "\t\t\t</equipmentuse>\n";
        }

        xmlOut << "\t\t\t<notes>" << Utils::xmlprotect(notes_) << "</notes>\n";
        xmlOut << "\t\t</equipmentitem>\n";
    }
}

//
// ---------------------------------------- Equipment Summary --------------------------------------------
//

EqSummary::EqSummary(const QUuid& equipmentRef, const QString& xmlChartName, const QString& xmlTileName)
    : AbstractEqItem(equipmentRef, xmlChartName, xmlTileName, EqItemType::EQ_SUMMARY),
        eqLinkIsCompleterVal_(false), showActivitiesPerAthlete_(false)
{
    resetDerived();
}

EqSummary::EqSummary(const EqSummary& toCopy)
    : AbstractEqItem(QUuid::createUuid(), toCopy.xmlChartName_, toCopy.xmlTileName_, EqItemType::EQ_SUMMARY)
{
    eqLinkName_ = toCopy.eqLinkName_;
    eqLinkIsCompleterVal_ = toCopy.eqLinkIsCompleterVal_;
    showActivitiesPerAthlete_ = toCopy.showActivitiesPerAthlete_;

    resetDerived();
}

void
EqSummary::setEqLinkName(const QString& eqLinkName)
{
    eqLinkName_ = eqLinkName;
    eqLinkIsCompleterVal_ = FieldDefinition::isCompleterValue("EquipmentLink", eqLinkName_);
}

void
EqSummary::resetDerived()
{
    athleteActivityMap_.clear();
    eqLinkNumActivities_ = 0;
    eqLinkTotalDistance_ = 0;
    eqLinkTotalElevation_ = 0;
    eqLinkTotalTimeInSecs_ = 0;
    eqLinkEarliestDate_ = QDate(1900, 01, 01);
    eqLinkLatestDate_ = eqLinkEarliestDate_;
}

void
EqSummary::startOfCalculation()
{
    calc_athleteActivityMap_.clear();
    calc_eqLinkNumActivities_ = 0;
    calc_eqLinkTotalDistanceScaled_ = 0;
    calc_eqLinkTotalElevationScaled_ = 0;
    calc_eqLinkTotalTimeInSecs_ = 0;
    calc_eqLinkEarliestDate_ = QDate(1900, 01, 01);
    calc_eqLinkLatestDate_ = eqLinkEarliestDate_;
}

void
EqSummary::addActivity(const QStringList& rideEqLinkNameList, const QDate& actDate,
                    double rideDistance, double rideElevation, uint64_t rideTimeInSecs, const QString& athleteName)
{
    // if eqLinkName is not set then include all activities
    if ((eqLinkName_ == "") || (rideEqLinkNameList.contains(eqLinkName_))) {

        activityMutex_.lock();

        calc_athleteActivityMap_[athleteName] += 1;

        if (calc_eqLinkNumActivities_++) {
            if (actDate < calc_eqLinkEarliestDate_) calc_eqLinkEarliestDate_ = actDate;
            if (actDate > calc_eqLinkLatestDate_) calc_eqLinkLatestDate_ = actDate;
        }
        else {
            calc_eqLinkEarliestDate_ = actDate;
            calc_eqLinkLatestDate_ = actDate;
        }

        activityMutex_.unlock();

        uint64_t rideDistanceScaled = rideDistance*EQ_REAL_TO_SCALED;
        uint64_t rideElevationScaled = rideElevation*EQ_REAL_TO_SCALED;

        calc_eqLinkTotalDistanceScaled_ += rideDistanceScaled;
        calc_eqLinkTotalElevationScaled_ += rideElevationScaled;
        calc_eqLinkTotalTimeInSecs_ += rideTimeInSecs;
    }
}

void
EqSummary::endOfCalculation()
{
    athleteActivityMap_ = calc_athleteActivityMap_;
    eqLinkNumActivities_ = calc_eqLinkNumActivities_.load();
    eqLinkTotalDistance_ = calc_eqLinkTotalDistanceScaled_.load()*EQ_SCALED_TO_REAL;
    eqLinkTotalElevation_ = calc_eqLinkTotalElevationScaled_.load()*EQ_SCALED_TO_REAL;
    eqLinkTotalTimeInSecs_ = calc_eqLinkTotalTimeInSecs_.load();
    eqLinkEarliestDate_ = calc_eqLinkEarliestDate_;
    eqLinkLatestDate_ = calc_eqLinkLatestDate_;
}

bool
EqSummary::parseXml(uint32_t version, QXmlStreamReader& reader)
{
    if (version == 1) {

        bool done = false;
        while (!reader.atEnd() && !reader.hasError() && !done) {
            QXmlStreamReader::TokenType token = reader.readNext();

            if (token == QXmlStreamReader::StartElement) {

                // the chart & tile names are stored and loaded from the equipment-perspectives.xml file,
                // they are not imported from the equipment-data.xml.

                if (reader.name() == "eqlink") setEqLinkName(Utils::unprotect(reader.readElementText()));
                else if (reader.name() == "showathleteactivities") showActivitiesPerAthlete_ = (reader.readElementText() == "true");

            } else if (token == QXmlStreamReader::EndElement) {

                if (reader.name() == "equipmentsummary") done = true;
            }
        }
        return true;
    }

    qDebug() << "EqSummary::parseXml - invalid file version:" << version;
    return false;
}

void
EqSummary::writeXml(uint32_t version, QTextStream& xmlOut) const
{
    if (version == 1) {

        xmlOut << "\t\t<equipmentsummary ";

        AbstractEqItem::writeXml(version, xmlOut);

        xmlOut << "\t\t\t<eqlink>" << Utils::xmlprotect(getEqLinkName()) << "</eqlink>\n";
        xmlOut << "\t\t\t<showathleteactivities>" << (showActivitiesPerAthlete_ ? "true" : "false") << "</showathleteactivities>\n";

        xmlOut << "\t\t</equipmentsummary>\n";
    }
}

//
// ------------------------------------------ EqTimeWindow --------------------------------------------
//

EqHistoryEntry::EqHistoryEntry()
{
}

EqHistoryEntry::EqHistoryEntry(const QDate& date, const QString& text) :
    date_(date), text_(text)
{
}

EqHistoryEntry::EqHistoryEntry(const EqHistoryEntry& toCopy) :
    date_(toCopy.date_), text_(toCopy.text_)
{
}

void
EqHistoryEntry::reset()
{
    date_ = QDate();
    text_.clear();
}

//
// ---------------------------------------- Equipment History --------------------------------------------
//

EqHistory::EqHistory(const QUuid& equipmentRef, const QString& xmlChartName, const QString& xmlTileName) :
    AbstractEqItem(equipmentRef, xmlChartName, xmlTileName, EqItemType::EQ_HISTORY), sortMostRecentFirst_(true)
{
}

EqHistory::EqHistory(const EqHistory& toCopy)
    : AbstractEqItem(QUuid::createUuid(), toCopy.xmlChartName_, toCopy.xmlTileName_, EqItemType::EQ_HISTORY)
{
    eqHistoryList_ = toCopy.eqHistoryList_;
    sortMostRecentFirst_ = toCopy.sortMostRecentFirst_;
}

void
EqHistory::sortHistoryEntries()
{
    if (sortMostRecentFirst_)
        std::sort(eqHistoryList_.begin(), eqHistoryList_.end(), [](const EqHistoryEntry& a, const EqHistoryEntry& b) { return a.date_ > b.date_; });
    else
        std::sort(eqHistoryList_.begin(), eqHistoryList_.end(), [](const EqHistoryEntry& a, const EqHistoryEntry& b) { return a.date_ < b.date_; });
}

bool
EqHistory::parseXml(uint32_t version, QXmlStreamReader& reader)
{
    if (version == 1) {

        bool done = false;
        EqHistoryEntry eqHistoryEntry;

        while (!reader.atEnd() && !reader.hasError() && !done) {
            QXmlStreamReader::TokenType token = reader.readNext();

            if (token == QXmlStreamReader::StartElement) {

                // the chart & tile names are stored and loaded from the equipment-perspectives.xml file,
                // they are not imported from the equipment-data.xml.

                if (reader.name() == "sortmostrecentfirst") sortMostRecentFirst_ = (reader.readElementText() == "true");

                else if (reader.name() == "historyentry") {
                    eqHistoryEntry.reset();
                    eqHistoryEntry.date_ = QDate::fromString(Utils::unprotect(reader.attributes().value("date").toString()));
                }
                else if (reader.name() == "historytext") eqHistoryEntry.text_ = Utils::unprotect(reader.readElementText());

            } else if (token == QXmlStreamReader::EndElement) {

                if (reader.name() == "historyentry") eqHistoryList_.push_back(eqHistoryEntry);
                else if (reader.name() == "equipmenthistory") done = true;
            }
        }
        return true;
    }

    qDebug() << "EqHistory::parseXml - invalid file version:" << version;
    return false;
}

void
EqHistory::writeXml(uint32_t version, QTextStream& xmlOut) const
{
    if (version == 1) {

        xmlOut << "\t\t<equipmenthistory ";

        AbstractEqItem::writeXml(version, xmlOut);

        xmlOut << "\t\t\t<sortmostrecentfirst>" << (sortMostRecentFirst_ ? "true" : "false") << "</sortmostrecentfirst>\n";

        for (const auto& eqHistory : eqHistoryList_) {

            xmlOut << "\t\t\t<historyentry date=\"" << Utils::xmlprotect(eqHistory.date_.toString()) << "\">\n";
            xmlOut << "\t\t\t\t<historytext>" << Utils::xmlprotect(eqHistory.text_) << "</historytext>\n";
            xmlOut << "\t\t\t</historyentry>\n";
        }
        xmlOut << "\t\t</equipmenthistory>\n";
    }
}

//
// ---------------------------------------- Equipment Notes --------------------------------------------
//

EqNotes::EqNotes(const QUuid& equipmentRef, const QString& xmlChartName, const QString& xmlTileName)
    :   AbstractEqItem(equipmentRef, xmlChartName, xmlTileName, EqItemType::EQ_NOTES)
{
}

EqNotes::EqNotes(const EqNotes& toCopy)
    : AbstractEqItem(QUuid::createUuid(), toCopy.xmlChartName_, toCopy.xmlTileName_, EqItemType::EQ_NOTES)
{
    notes_ = toCopy.notes_;
}

bool
EqNotes::parseXml(uint32_t version, QXmlStreamReader& reader)
{
    if (version == 1) {

        bool done = false;
        while (!reader.atEnd() && !reader.hasError() && !done) {
            QXmlStreamReader::TokenType token = reader.readNext();

            if (token == QXmlStreamReader::StartElement) {

                // the chart & tile names are stored and loaded from the equipment-perspectives.xml file,
                // they are not imported from the equipment-data.xml.

                if (reader.name() == "notes") notes_ = Utils::unprotect(reader.readElementText());

            } else if (token == QXmlStreamReader::EndElement) {

                if (reader.name() == "equipmentnotes") done = true;
            }
        }
        return true;

    }

    qDebug() << "EqNotes::parseXml - invalid file version:" << version;
    return false;
}

void
EqNotes::writeXml(uint32_t version, QTextStream& xmlOut) const
{
    if (version == 1 ) {

        xmlOut << "\t\t<equipmentnotes ";

        AbstractEqItem::writeXml(version, xmlOut);

        xmlOut << "\t\t\t<notes>" << Utils::xmlprotect(notes_) << "</notes>\n";
        xmlOut << "\t\t</equipmentnotes>\n";
    }
}
