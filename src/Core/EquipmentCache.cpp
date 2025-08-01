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

#include <EquipmentCache.h>

#include "Context.h"
#include "MainWindow.h" // for gcroot
#include "Utils.h"
#include "Units.h"

#include <QMessageBox>

#define DBG 0

EquipmentCache::EquipmentCache() :
    eqXMLDataFile_(QDir(gcroot).absolutePath()+"/equipment-data.xml")
{
    if(DBG) printf("EquipmentCache created\n");

    if (eqXMLDataFile_.exists() && eqXMLDataFile_.isFile()) {

        readXML();

        // create list of QUuids read from the file, used to garbage collect orphaned items in writeXML().
        for (auto itr = allEqItems_.keyValueBegin(); itr != allEqItems_.keyValueEnd(); ++itr) {
            garbageItems_.push_back(itr->first);
        }

    } else {
        if(DBG) printf("EquipmentCalculator - %s doesn't exist, assuming content in legacy json or baked in equipment perspective\n",
                  eqXMLDataFile_.absoluteFilePath().toStdString().c_str());
    }
}

EquipmentCache::~EquipmentCache()
{
    if(DBG) printf("EquipmentCache destroyed\n");
}

AbstractEqItem*
EquipmentCache::getEquipment(const QUuid& equipmentRef)
{
    QMap<QUuid, AbstractEqItem*>::const_iterator itr = allEqItems_.find(equipmentRef);

    if (itr != allEqItems_.end()) {
        garbageItems_.removeOne(itr.key());
        return itr.value();
    }
    return nullptr;
}


AbstractEqItem*
EquipmentCache::createEquipment(const QUuid& equipmentRef, const QString& xmlChartName, const QString& xmlTileName, EqItemType equipmentType)
{
    AbstractEqItem* eqItem = nullptr;
    switch (equipmentType) {
        case EqItemType::EQ_ITEM: eqItem = new EqItem(equipmentRef, xmlChartName, xmlTileName); break;
        case EqItemType::EQ_SUMMARY: eqItem = new EqSummary(equipmentRef, xmlChartName, xmlTileName); break;
        case EqItemType::EQ_HISTORY: eqItem = new EqHistory(equipmentRef, xmlChartName, xmlTileName); break;
        case EqItemType::EQ_NOTES: eqItem = new EqNotes(equipmentRef, xmlChartName, xmlTileName); break;
        default: {
            if(DBG) printf("EquipmentCache::createEquipment - Unsupported equipment type: %d in Equipment Cache\n",
                      static_cast<int>(equipmentType));
        } break;
    }

    if (eqItem) allEqItems_[equipmentRef] = eqItem;
    return eqItem;
}

AbstractEqItem*
EquipmentCache::cloneEquipment(const QUuid& equipmentRef)
{
    QMap<QUuid, AbstractEqItem*>::const_iterator itr = allEqItems_.find(equipmentRef);

    if (itr != allEqItems_.end()) {

        AbstractEqItem* eqItem = nullptr;
        switch (itr.value()->getEquipmentType()) {
            case EqItemType::EQ_ITEM: eqItem = new EqItem(*(static_cast<EqItem*>(itr.value()))); break;
            case EqItemType::EQ_SUMMARY: eqItem = new EqSummary(*(static_cast<EqSummary*>(itr.value()))); break;
            case EqItemType::EQ_HISTORY: eqItem = new EqHistory(*(static_cast<EqHistory*>(itr.value()))); break;
            case EqItemType::EQ_NOTES: eqItem = new EqNotes(*(static_cast<EqNotes*>(itr.value()))); break;
            default: {
                if(DBG) printf("EquipmentCache::cloneEquipment - Unsupported equipment type: %d in Equipment Cache\n",
                          static_cast<int>(itr.value()->getEquipmentType()));
            } break;
        }

        if (eqItem) {
            allEqItems_[eqItem->getEquipmentRef()] = eqItem;
            return eqItem;
        }
    }
    return nullptr;
}

bool
EquipmentCache::deleteEquipment(const QUuid& equipmentRef)
{
    QMap<QUuid, AbstractEqItem*>::const_iterator itr = allEqItems_.find(equipmentRef);

    if (itr != allEqItems_.end()) {
        delete itr.value();
        garbageItems_.removeOne(itr.key());
        allEqItems_.remove(itr.key());
        return true;
    }
    return false;
}

void
EquipmentCache::writeXML() const
{
    if(DBG) printf("EquipmentCache - Writing xml file: %s \n", eqXMLDataFile_.absoluteFilePath().toStdString().c_str());

    // open file - truncate contents
    QFile file(eqXMLDataFile_.absoluteFilePath());
    if (!file.open(QFile::WriteOnly)) {
        QMessageBox msgBox;
        msgBox.setIcon(QMessageBox::Critical);
        msgBox.setText(tr("Problem Saving Equipment Data"));
        msgBox.setInformativeText(tr("File: %1 cannot be opened for 'Writing'. Please check file properties.").arg(eqXMLDataFile_.absoluteFilePath()));
        msgBox.exec();
        return;
    };

    file.resize(0);
    QTextStream xmlOut(&file);

#if QT_VERSION < 0x060000
    xmlOut.setCodec("UTF-8");
#else
    xmlOut.setEncoding(QStringConverter::Utf8);
#endif

    xmlOut.setRealNumberNotation(QTextStream::FixedNotation);
    xmlOut.setRealNumberPrecision(EQ_DECIMAL_PRECISION);

    // begin document
    xmlOut << "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n\n";
    xmlOut << "<!-- This file holds the equipment data information which cannot be derived from calculation -->\n";
    xmlOut << "<equipmentdata>\n";

    uint32_t version = 1;
    xmlOut << "\t<version>" << version << "</version>\n";
    xmlOut << "\t<uom>" << Utils::xmlprotect(GlobalContext::context()->useMetricUnits ? "metric" : "imperial") << "</uom>\n";

    // totalitems and garbageitems are only exported in the equipment-data.xml for tile health purposes.
    xmlOut << "\t<totalitems>" << allEqItems_.size() << "</totalitems>\n";
    xmlOut << "\t<garbageitems>" << garbageItems_.size() << "</garbageitems>\n";

    xmlOut << "\t<equipment>\n";

    // iterate over items, saving their information, unused items imported from the
    // xml file will be discarded on the next import.
    for (auto itr = allEqItems_.keyValueBegin(); itr != allEqItems_.keyValueEnd(); ++itr) {
        if (garbageItems_.contains(itr->first)) {
            qWarning() << "EquipmentCache::writeXML - Discarding unused item:" << itr->first.toString();
        } else {
            itr->second->writeXml(version, xmlOut);
        }
    }

    xmlOut << "\t</equipment>\n";
    xmlOut << "</equipmentdata>\n";

    // close file
    file.close();
}

void
EquipmentCache::readXML()
{
    if(DBG) printf("EquipmentCache reading xml file: %s \n", eqXMLDataFile_.absoluteFilePath().toStdString().c_str());

    QFile metadataFile(eqXMLDataFile_.absoluteFilePath());
    QXmlInputSource source( &metadataFile );
    QXmlSimpleReader xmlReader;
    EquipmentXMLParser handler(allEqItems_);

    xmlReader.setContentHandler(&handler);
    xmlReader.setErrorHandler(&handler);
    xmlReader.parse( source );
}

EquipmentXMLParser::EquipmentXMLParser(QMap<QUuid, AbstractEqItem*>& allEqItems)
    :   allEqItems_(allEqItems), typeToLoad_(EqItemType::EQ_NOT_SET),
        loadingVersion_(0), loadingAsMetric_(false), itemToLoad_(nullptr)
{
}

bool EquipmentXMLParser::endElement( const QString&, const QString&, const QString &qName)
{
    if (qName == "uom") loadingAsMetric_ = (Utils::unprotect(buffer.trimmed()) == "metric");
    else if (qName == "version") loadingVersion_ = Utils::unprotect(buffer.trimmed()).toUInt();

    // totalitems and garbageitems are only in the equipment-data.xml for tile health purposes.
    //else if (qName == "totalitems") totalItems_ = Utils::unprotect(buffer.trimmed());
    //else if (qName == "garbageitems") garbageItems_ = Utils::unprotect(buffer.trimmed());

    else if (qName == "eqreference") {
        QUuid eqXmlRef = QUuid::fromString(Utils::unprotect(buffer.trimmed()));

        switch (typeToLoad_) {
        case EqItemType::EQ_ITEM: itemToLoad_ = new EqItem(eqXmlRef); break;
        case EqItemType::EQ_SUMMARY: itemToLoad_ = new EqSummary(eqXmlRef); break;
        case EqItemType::EQ_HISTORY: itemToLoad_ = new EqHistory(eqXmlRef); break;
        case EqItemType::EQ_NOTES: itemToLoad_ = new EqNotes(eqXmlRef); break;
        default: {
            itemToLoad_ = nullptr;
            qDebug() << "Unsupported tile type:" << static_cast<int>(typeToLoad_) << " in Equipment XML Parser";
            return false;
        } break;
        }
        itemToLoad_->xmlUoM(loadingAsMetric_);
    }
    else if (itemToLoad_) {

        itemToLoad_->parseXML(loadingVersion_, qName, Utils::unprotect(buffer.trimmed()));

        if ((qName == "equipmentitem") || (qName == "equipmentsummary") ||
            (qName == "equipmenthistory") || (qName == "equipmentnotes")) {
            allEqItems_[itemToLoad_->getEquipmentRef()] = itemToLoad_;
        }
    }
    return true;
}

bool EquipmentXMLParser::startElement( const QString&, const QString&, const QString &name, const QXmlAttributes & )
{
    buffer.clear();

    if (name == "equipmentitem") {
        xmlChartName_.clear();
        xmlTileName_.clear();
        itemToLoad_ = nullptr;
        typeToLoad_ = EqItemType::EQ_ITEM;
    }
    else if (name == "equipmenthistory") {
        xmlChartName_.clear();
        xmlTileName_.clear();
        itemToLoad_ = nullptr;
        typeToLoad_ = EqItemType::EQ_HISTORY;
    }
    else if (name == "equipmentsummary") {
        xmlChartName_.clear();
        xmlTileName_.clear();
        itemToLoad_ = nullptr;
        typeToLoad_ = EqItemType::EQ_SUMMARY;
    }
    else if (name == "equipmentnotes") {
        xmlChartName_.clear();
        xmlTileName_.clear();
        itemToLoad_ = nullptr;
        typeToLoad_ = EqItemType::EQ_NOTES;
    }

    return true;
}

bool EquipmentXMLParser::characters( const QString& str )
{
    buffer += str;
    return true;
}

bool EquipmentXMLParser::endDocument()
{
    return true;
}

