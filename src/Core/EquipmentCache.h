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

#ifndef _GC_EquipmentCache_h
#define _GC_EquipmentCache_h 1

#include "EquipmentItems.h"

#include <QMap>
#include <QUuid>
#include <QXmlDefaultHandler>
#include <QXmlStreamReader>
#include <QFileInfo>
#include <QVector>

class EquipmentCache : public QObject
{
    public:

        // Singleton pattern
        static EquipmentCache& getInstance() {
            static EquipmentCache instance;
            return instance;
        }
        ~EquipmentCache();

        EquipmentCache(EquipmentCache const&) = delete;
        void operator=(EquipmentCache const&) = delete;

        AbstractEqItem* getEquipment(const QUuid& equipmentRef);
        AbstractEqItem* createEquipment(const QUuid& equipmentRef, const QString& xmlChartName, const QString& xmlTileName, EqItemType equipmentType);
        AbstractEqItem* cloneEquipment(const QUuid& equipmentRef);
        bool deleteEquipment(const QUuid& equipmentRef);

        void writeXml() const;

        const QMap<QUuid, AbstractEqItem*>& getMap() { return allEqItems_; }

    private:
        EquipmentCache();

        void readXmlv1();
        bool readXml();

        const QFileInfo eqXMLDataFile_;
        QVector<QUuid> garbageItems_;
        QMap<QUuid, AbstractEqItem*> allEqItems_;
};

class EquipmentXMLParser : public QXmlDefaultHandler
{
    public:
        EquipmentXMLParser(QMap<QUuid, AbstractEqItem*>& allEqItems);

        bool startDocument() { return true; }
        bool endDocument();
        bool endElement( const QString&, const QString&, const QString &qName );
        bool startElement( const QString&, const QString&, const QString &name, const QXmlAttributes &attrs );
        bool characters( const QString& str );

    private:
        QString buffer;

        // whilst parsing elements are stored here
        QString xmlChartName_;
        QString xmlTileName_;
        EqItemType typeToLoad_;
        bool loadingAsMetric_;
        uint32_t loadingVersion_;

        // the chart items to be loaded with the xml data
        AbstractEqItem* itemToLoad_;
        QMap<QUuid, AbstractEqItem*>& allEqItems_;
};

#endif // _GC_EquipmentCache_h