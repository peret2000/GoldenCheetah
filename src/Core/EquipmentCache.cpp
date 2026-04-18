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
            qDebug() << "EquipmentCache - Unsupported format in " << eqXMLDataFile_.absoluteFilePath().toStdString().c_str();
        }

        // create list of QUuids read from the file, used to garbage collect orphaned items in writeXml().
        for (auto itr = allEqItems_.keyValueBegin(); itr != allEqItems_.keyValueEnd(); ++itr) {
            garbageItems_.push_back(itr->first);
        }

    } else {
        qDebug() << "EquipmentCache - " << eqXMLDataFile_.absoluteFilePath().toStdString().c_str()
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
    // always write latest xml file version.
    uint32_t version = 1;

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
    xmlOut << "<!-- This file holds the equipment information that cannot be derived from calculation -->\n";
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
                    qInfo("EquipmentCache reading v%d xml file: %s", loadingVersion_, eqXMLDataFile_.absoluteFilePath().toStdString().c_str());

                } else if (loadingVersion_ == 1) {
                    if (reader.name() == "uom") {
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
                }

                if (itemToLoad_) {
                    itemToLoad_->xmlUoM(loadingAsMetric_);
                    if (itemToLoad_->parseXml(loadingVersion_, reader) == false) return false;
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

