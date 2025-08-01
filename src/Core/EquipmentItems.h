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

#ifndef _GC_EqItems_h
#define _GC_EqItems_h 1

#include <QDate>
#include <QUuid>
#include <QMutex>
#include <QMap>
#include <QTextStream>
#include <QJsonObject>
#include <memory> // Atomics

static const uint64_t EQ_DECIMAL_PRECISION = 1;

class EqTimeWindow
{
    public:
        EqTimeWindow();
        EqTimeWindow(const QString& eqLinkName);
        EqTimeWindow(const QString& eqLinkName, bool startSet, const QDate& startDate, bool endSet, const QDate& endDate);

        QString eqLinkName() const { return eqLinkName_; }

        bool startSet_, endSet_;
        QDate startDate_, endDate_;

        void reset();
        bool isWithin(const QDate& actDate) const;
        bool rangeIsValid() const;
        bool eqLinkIsCompleterVal() const { return eqLinkIsCompleterVal_; }

    private:

        QString eqLinkName_;
        bool eqLinkIsCompleterVal_;
};

enum class EqItemType { EQ_NOT_SET, EQ_ITEM, EQ_SUMMARY, EQ_NOTES, EQ_HISTORY };

class AbstractEqItem : public QObject
{
    public:

        virtual ~AbstractEqItem() {}

        virtual void startOfCalculation() {}
        virtual void endOfCalculation() {}
        virtual void unitsChanged() {}

        EqItemType getEquipmentType() const { return equipmentType_; }
        const QUuid& getEquipmentRef() const { return equipmentRef_; }

        virtual void addActivity(const QStringList& /* rideEqLinkNameList */, const QDate& /* activityDate */,
                                double /* rideDistance */, double /* rideElevation */, uint64_t /* rideTimeInSecs */, const QString& /* athleteName */ ) {}

        virtual void xmlUoM(bool /* loadingAsMetric */) {}
        virtual void parseXML(uint32_t version, const QString& qName, const QString& buffer) = 0;
        virtual void writeXml(uint32_t version, QTextStream& xmlOut) const;
        virtual void parseLegacyJSON(const QJsonObject& obj) = 0;

        QString xmlChartName_;
        QString xmlTileName_;

    protected:

        // Hide constructor to create an Abstract class
        AbstractEqItem(const QUuid& equipmentRef, const QString& xmlChartName, const QString& xmlTileName, EqItemType equipmentType);

        const EqItemType equipmentType_;
        const QUuid equipmentRef_;
};

class EqItem : public AbstractEqItem
{
    public:

        // create using existing Uuid
        EqItem(const QUuid& equipmentRef, const QString& xmlChartName = QString(""), const QString& xmlTileName = QString(""));

        // clone EqItem, with new Uuid
        EqItem(const EqItem& toCopy);

        virtual ~EqItem() {}

        bool isWithin(const QDate& actDate) const;
        bool isWithin(const QStringList& rideEqLinkNameList, const QDate& actDate) const;
        bool rangeIsValid() const;
        bool allEqLinkNamesCompleterVals() const;

        void unitsChanged() override;
        void sortEqLinkUseWindows();

        void startOfCalculation() override;
        void endOfCalculation() override;
        void addActivity(const QStringList& rideEqLinkNameList, const QDate& actDate,
                        double rideDistance, double rideElevation, uint64_t rideTimeInSecs, const QString& athleteName) override;

        // set and get primary state
        void setNonGCDistance(double nonGCDistance);
        double getNonGCDistance() const { return nonGCDistance_; }
        void setNonGCElevation(double nonGCElevation);
        double getNonGCElevation() const { return nonGCElevation_; }

        // get derived state
        double getGCDistance() const { return gcDistance_; }
        double getTotalDistance() const { return totalDistance_; }
        double getGCElevation() const { return gcElevation_; }
        double getTotalElevation() const { return totalElevation_; }
        uint64_t getNumActivities() const { return activities_; }
        uint64_t getActivityTimeInSecs() const { return activityTimeInSecs_; }

        void xmlUoM(bool loadingAsMetric) override;
        void parseXML(uint32_t version, const QString& qName, const QString& buffer);
        void writeXml(uint32_t version, QTextStream& xmlOut) const override;
        void parseLegacyJSON(const QJsonObject& obj) override;

        // primary state
        bool displayTotalDistance_;
        QVector<EqTimeWindow> eqLinkUseList_;
        double repDistance_;
        double repElevation_;
        bool repDateSet_;
        QDate repDate_;
        QString notes_;

    private:

        void resetDerived();

        // primary state
        double nonGCDistance_;
        double nonGCElevation_;
        double xmlScalerKmMile_;
        double xmlScalerMtrFoot_;

        // derived state
        uint64_t activities_;
        uint64_t activityTimeInSecs_;
        double gcDistance_;
        double totalDistance_;
        double gcElevation_;
        double totalElevation_;

        // calculation temporary values
        // using integral type atomics (c++11)
        std::atomic<uint64_t> calc_activities_;
        std::atomic<uint64_t> calc_activityTimeInSecs_;

        // calculation temporary values
        // using integral type atomics (c++11) but to retain accuracy, need c++ 23 for
        // double atomics all distance and elevation attributes are scaled.
        std::atomic<uint64_t> calc_gcDistanceScaled_;
        std::atomic<uint64_t> calc_totalDistanceScaled_;
        std::atomic<uint64_t> calc_gcElevationScaled_;
        std::atomic<uint64_t> calc_totalElevationScaled_;
};

class EqSummary : public AbstractEqItem
{
    public:

        // create using existing Uuid
        EqSummary(const QUuid& equipmentRef, const QString& xmlChartName = QString(""), const QString& xmlTileName = QString(""));

        // clone EqSummary, with new Uuid
        EqSummary(const EqSummary& toCopy);

        virtual ~EqSummary() {}

        void startOfCalculation() override;
        void endOfCalculation() override;
        void addActivity(const QStringList& rideEqLinkNameList, const QDate& actDate,
                        double rideDistance, double rideElevation, uint64_t rideTimeInSecs, const QString& athleteName) override;

        void setEqLinkName(const QString& eqLinkName);
        QString getEqLinkName() const { return eqLinkName_; }
        bool eqLinkIsCompleterVal() const { return eqLinkIsCompleterVal_; }

        uint64_t getEqLinkTotalTimeInSecs() const { return eqLinkTotalTimeInSecs_; }
        uint64_t getEqLinkNumActivities() const { return eqLinkNumActivities_; }
        double getEqLinkTotalDistance() const { return eqLinkTotalDistance_; }
        double getEqLinkTotalElevation() const { return eqLinkTotalElevation_; }

        const QDate& getEqLinkEarliestDate() const { return eqLinkEarliestDate_; }
        const QDate& getEqLinkLatestDate() const { return eqLinkLatestDate_; }
        const QMap<QString, uint32_t>& getAthleteActivityMap() const { return athleteActivityMap_; }

        void parseXML(uint32_t version, const QString& qName, const QString& buffer);
        void writeXml(uint32_t version, QTextStream& xmlOut) const override;
        void parseLegacyJSON(const QJsonObject& obj) override;

        // primary state
        bool showActivitiesPerAthlete_;

    private:

        void resetDerived();

        // primary state
        QString eqLinkName_;
        bool eqLinkIsCompleterVal_;

        // derived state
        uint64_t eqLinkTotalTimeInSecs_;
        uint64_t eqLinkNumActivities_;
        double eqLinkTotalDistance_;
        double eqLinkTotalElevation_;

        QMutex activityMutex_;
        QDate eqLinkEarliestDate_, eqLinkLatestDate_;
        QMap<QString, uint32_t> athleteActivityMap_;

        // calculation temporary values
        // using integral type atomics (c++11) but to retain accuracy, need c++ 23 for
        // double atomics all distance and elevation attributes are scaled.
        std::atomic<uint64_t> calc_eqLinkTotalTimeInSecs_;
        std::atomic<uint64_t> calc_eqLinkNumActivities_;
        std::atomic<uint64_t> calc_eqLinkTotalDistanceScaled_;
        std::atomic<uint64_t> calc_eqLinkTotalElevationScaled_;

        // calculation temporary values
        QDate calc_eqLinkEarliestDate_, calc_eqLinkLatestDate_;
        QMap<QString, uint32_t> calc_athleteActivityMap_;
};

class EqHistoryEntry
{
    public:
        EqHistoryEntry();
        EqHistoryEntry(const QDate& date, const QString& text);
        EqHistoryEntry(const EqHistoryEntry& toCopy);

        void reset();

        QDate date_;
        QString text_;
};

class EqHistory : public AbstractEqItem
{
    public:

        // create using existing Uuid
        EqHistory(const QUuid& equipmentRef, const QString& xmlChartName = QString(""), const QString& xmlTileName = QString(""));

        // clone EqHistory, with new Uuid
        EqHistory(const EqHistory& toCopy);

        void sortHistoryEntries();

        virtual ~EqHistory() {}

        void parseXML(uint32_t version, const QString& qName, const QString& buffer);
        void writeXml(uint32_t version, QTextStream& xmlOut) const override;
        void parseLegacyJSON(const QJsonObject& obj) override;

        bool sortMostRecentFirst_;
        QVector<EqHistoryEntry> eqHistoryList_;

    private:

};

class EqNotes : public AbstractEqItem
{
    public:

        // create using existing Uuid
        EqNotes(const QUuid& equipmentRef, const QString& xmlChartName = QString(""), const QString& xmlTileName = QString(""));

        // clone EqNotes, with new Uuid
        EqNotes(const EqNotes& toCopy);

        void parseXML(uint32_t version, const QString& qName, const QString& buffer);
        void writeXml(uint32_t version, QTextStream& xmlOut) const override;
        void parseLegacyJSON(const QJsonObject& obj) override;

        virtual ~EqNotes() {}

        QString notes_;

    private:

};

#endif // _GC_EqItems_h
