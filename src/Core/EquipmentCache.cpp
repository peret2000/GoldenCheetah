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

#include <QMessageBox>


EquipmentCache::EquipmentCache() :
    eqXMLDataFile_(QDir(gcroot).absolutePath()+"/equipment-data.xml")
{
    qInfo() << "EquipmentCache created";

    if (eqXMLDataFile_.exists() && eqXMLDataFile_.isFile()) {

        // trying reading the latest format of the equipment-data.xml file
        if (readXml() == false) {

            // try reading legacy v1 format of the equipment-data.xml file (uses legacy QXmlDefaultHandler)
            readXmlv1(); 
        }

        // create list of QUuids read from the file, used to garbage collect orphaned items in writeXml().
        for (auto itr = allEqItems_.keyValueBegin(); itr != allEqItems_.keyValueEnd(); ++itr) {
            garbageItems_.push_back(itr->first);
        }

    } else {
        qDebug() << "EquipmentCalculator - " << eqXMLDataFile_.absoluteFilePath().toStdString().c_str()
                << " doesn't exist, assuming content in legacy json or baked in equipment perspective";
    }
}

EquipmentCache::~EquipmentCache()
{
    qInfo() << "EquipmentCache destroyed";
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
            qDebug() << "EquipmentCache::createEquipment - Unsupported equipment type: "
                    << static_cast<int>(equipmentType) << " in Equipment Cache";
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
                qDebug() << "EquipmentCache::cloneEquipment - Unsupported equipment type: "
                        << static_cast<int>(itr.value()->getEquipmentType()) << " in Equipment Cache";
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
EquipmentCache::writeXml() const
{
    // the xml file version to be written.
    uint32_t version = 2;

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

    xmlOut.setEncoding(QStringConverter::Utf8);

    xmlOut.setRealNumberNotation(QTextStream::FixedNotation);
    xmlOut.setRealNumberPrecision(EQ_DECIMAL_PRECISION);

    qInfo("EquipmentCache - Writing v%d xml file: %s", version, eqXMLDataFile_.absoluteFilePath().toStdString().c_str());

    // begin document
    xmlOut << "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n\n";
    xmlOut << "<!-- This file holds the equipment data information which cannot be derived from calculation -->\n";
    xmlOut << "<equipmentdata>\n";

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
            qWarning() << "EquipmentCache::writeXml - Discarding unused item:" << itr->first.toString();
        } else {
            itr->second->writeXml(version, xmlOut);
        }
    }

    xmlOut << "\t</equipment>\n";
    xmlOut << "</equipmentdata>\n";

    // close file
    file.close();
}

bool
EquipmentCache::readXml()
{
    QFile metadataFile(eqXMLDataFile_.absoluteFilePath());

    if (metadataFile.open(QFile::ReadOnly)) {

        QXmlStreamReader reader(metadataFile.readAll());
        metadataFile.close();

        uint32_t loadingVersion_{0};
        bool loadingAsMetric_{false};
        AbstractEqItem* itemToLoad_{nullptr};

        while (!reader.atEnd() && !reader.hasError()) {
            QXmlStreamReader::TokenType token = reader.readNext();

            if (token == QXmlStreamReader::StartElement) {

                // totalitems and garbageitems are only in the equipment-data.xml for tile health purposes.

                if (reader.name() == "version") {
                    loadingVersion_ = reader.readElementText().toUInt();
                    if (loadingVersion_ == 1) return false;
                    qInfo("EquipmentCache reading v%d xml file: %s", loadingVersion_, eqXMLDataFile_.absoluteFilePath().toStdString().c_str());
                } else if (reader.name() == "uom") {
                    loadingAsMetric_ = (Utils::unprotect(reader.readElementText()) == "metric");
                } else if (reader.name() == "equipmentitem") {
                    itemToLoad_ = new EqItem(QUuid::fromString(Utils::unprotect(reader.attributes().value("eqref").toString().trimmed())));
                } else if (reader.name() == "equipmentsummary") {
                    itemToLoad_ = new EqSummary(QUuid::fromString(Utils::unprotect(reader.attributes().value("eqref").toString().trimmed())));
                } else if (reader.name() == "equipmenthistory") {
                    itemToLoad_ = new EqHistory(QUuid::fromString(Utils::unprotect(reader.attributes().value("eqref").toString().trimmed())));
                } else if (reader.name() == "equipmentnotes") {
                    itemToLoad_ = new EqNotes(QUuid::fromString(Utils::unprotect(reader.attributes().value("eqref").toString().trimmed())));
                }

                if (itemToLoad_) {
                    itemToLoad_->xmlUoM(loadingAsMetric_);
                    itemToLoad_->parseXml(loadingVersion_, reader);
                    allEqItems_[itemToLoad_->getEquipmentRef()] = itemToLoad_;
                    itemToLoad_ = nullptr;
                }
            }
        }
    } else {
        QMessageBox::warning(nullptr, tr("Warning"), tr("Failed to open equipment-data.xml file!"));
    }
    return true;
}

void
EquipmentCache::readXmlv1()
{
    qInfo() << "EquipmentCache reading v1 xml file: " << eqXMLDataFile_.absoluteFilePath().toStdString().c_str();

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
    if (qName == "version") {
        loadingVersion_ = Utils::unprotect(buffer.trimmed()).toUInt();
        if (loadingVersion_ != 1) return false;
    }
    else if (qName == "uom") loadingAsMetric_ = (Utils::unprotect(buffer.trimmed()) == "metric");

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

        itemToLoad_->parseXmlv1(qName, Utils::unprotect(buffer.trimmed()));

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

