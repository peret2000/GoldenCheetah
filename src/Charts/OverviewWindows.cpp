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

#include "OverviewWindows.h"
#include "EquipmentCalculator.h"
#include "EquipmentOverviewItems.h"
#include "EquipmentCache.h"

AnalysisOverviewWindow::AnalysisOverviewWindow(Context* context, bool blank) :
    OverviewWindow(context, OverviewScope::ANALYSIS, blank)
{
    space->setWhatsThis(help->getWhatsThisText(HelpWhatsThis::ChartRides_Overview));

    // tell space when a ride is selected
    connect(this, &GcWindow::rideItemChanged, space, &ChartSpace::rideSelected);
}

PlanOverviewWindow::PlanOverviewWindow(Context* context, bool blank) :
    OverviewWindow(context, OverviewScope::PLAN, blank) {

    space->setWhatsThis(help->getWhatsThisText(HelpWhatsThis::Chart_Overview));

    connect(this, &GcWindow::dateRangeChanged, space, &ChartSpace::dateRangeChanged);
    connect(context, &Context::filterChanged, space, &ChartSpace::filterChanged);
    connect(context, &Context::homeFilterChanged, space, &ChartSpace::filterChanged);
    connect(this, &GcWindow::perspectiveFilterChanged, space, &ChartSpace::filterChanged);
}

TrendsOverviewWindow::TrendsOverviewWindow(Context* context, bool blank) :
    OverviewWindow(context, OverviewScope::TRENDS, blank) {

    space->setWhatsThis(help->getWhatsThisText(HelpWhatsThis::Chart_Overview));

    connect(this, &GcWindow::dateRangeChanged, space, &ChartSpace::dateRangeChanged);
    connect(context, &Context::filterChanged, space, &ChartSpace::filterChanged);
    connect(context, &Context::homeFilterChanged, space, &ChartSpace::filterChanged);
    connect(this, &GcWindow::perspectiveFilterChanged, space, &ChartSpace::filterChanged);
}

EquipmentOverviewWindow::EquipmentOverviewWindow(Context* context, bool blank) :
    OverviewWindow(context, OverviewScope::EQUIPMENT, blank), eqWindowVisible_(false),
    chartExportInProgress_(false)
{
    space->setWhatsThis(help->getWhatsThisText(HelpWhatsThis::ChartEquip_Overview));

    space->configIcon = ":images/tile-edit.png";
    space->editIcon = ":images/equipment-popup.png";
    space->configChanged(CONFIG_APPEARANCE);

    formlayout->addRow(new QLabel(" "));
    formlayout->addRow(new QLabel(tr("Equipment Tiles:")));
    formlayout->addRow(new QLabel(" "));
   
    textSize = new QComboBox(this);
    textSize->addItem(tr("Small"));
    textSize->addItem(tr("Medium"));
    textSize->addItem(tr("Large"));
    formlayout->addRow(new QLabel(tr("Text Size")), textSize);
    textSize->setCurrentIndex(eqTextSizeType::MEDIUM);

    showActivities = new QCheckBox(tr(""), this);
    showActivities->setCheckState(Qt::Checked);
    formlayout->addRow(new QLabel(tr("Activities Field")), showActivities);

    showTime = new QCheckBox(tr(""), this);
    showTime->setCheckState(Qt::Checked);
    formlayout->addRow(new QLabel(tr("Time Field")), showTime);

    showElevation = new QCheckBox(tr(""), this);
    showElevation->setCheckState(Qt::Unchecked);
    formlayout->addRow(new QLabel(tr("Elevation Field")), showElevation);

    showNotes = new QCheckBox(tr(""), this);
    showNotes->setCheckState(Qt::Unchecked);
    formlayout->addRow(new QLabel(tr("Notes Field")), showNotes);

    // cannot use athlete specific signals, as there is only one equipment view.
    connect(GlobalContext::context(), &GlobalContext::configChanged, this, &EquipmentOverviewWindow::configChanged);
    connect(GlobalContext::context(), &GlobalContext::eqRecalculationComplete, this, &EquipmentOverviewWindow::eqRecalculationComplete);

    connect(textSize, &QComboBox::currentIndexChanged, this, &EquipmentOverviewWindow::setTextSize);
    connect(showActivities, &QCheckBox::checkStateChanged, this, &EquipmentOverviewWindow::setShowActivities);
    connect(showTime, &QCheckBox::checkStateChanged, this, &EquipmentOverviewWindow::setShowTime);
    connect(showElevation, &QCheckBox::checkStateChanged, this, &EquipmentOverviewWindow::setShowElevation);
    connect(showNotes, &QCheckBox::checkStateChanged, this, &EquipmentOverviewWindow::setShowNotes);

    // connect GcWindow titleChanged() to this object's slot
    connect(this, &GcWindow::titleChanged, this, &EquipmentOverviewWindow::titleChanged);
}

EquipmentOverviewWindow::~EquipmentOverviewWindow()
{
    // need to delete items here, as they cannot be deleted by the tile item destructor, otherwise
    // equipment cache data is removed before it is written out to the xml file during shutdown.
    foreach(ChartSpaceItem *item, space->allItems()) {

        // remove the chart space item's equipment cache entry
        EquipmentCache::getInstance().deleteEquipment(static_cast<CommonEqItem*>(item)->getEquipmentRef());
    }
}

void
EquipmentOverviewWindow::showChart(bool visible)
{
    eqWindowVisible_ = visible;
    GcChartWindow::showChart(visible);
}

ChartSpaceItem*
EquipmentOverviewWindow::addTile()
{
    ChartSpaceItem* item = OverviewWindow::addTile();

    // need to recalculate the equipment cache after a user tile creation.
    if (item) GlobalContext::context()->requestEqItemRecalculation(static_cast<CommonEqItem*>(item)->getEquipmentRef(), "Item added");

    return item;
}

void
EquipmentOverviewWindow::cloneTile(ChartSpaceItem* item)
{
    CommonEqItem* clonedItem = nullptr;

    switch (item->type) {

    case OverviewItemType::EQ_ITEM: {
        clonedItem = new EquipmentItem(*(static_cast<EquipmentItem*>(item)));
    } break;

    case OverviewItemType::EQ_SUMMARY: {
        clonedItem = new EquipmentSummary(*(static_cast<EquipmentSummary*>(item)));
    } break;

    case OverviewItemType::EQ_HISTORY: {
        clonedItem = new EquipmentHistory(*(static_cast<EquipmentHistory*>(item)));
    } break;

    case OverviewItemType::EQ_NOTES: {
        clonedItem = new EquipmentNotes(*(static_cast<EquipmentNotes*>(item)));
    } break;
    }

    if (clonedItem) {

        clonedItem->bgcolor = item->bgcolor;
        space->addItem(item->order, item->column, item->span, item->deep, clonedItem);

        // update geometry
        space->updateGeometry();
        space->updateView();

        // need to recalculate the equipment cache after a user clones a tile.
        GlobalContext::context()->requestEqItemRecalculation(static_cast<CommonEqItem*>(clonedItem)->getEquipmentRef(), "Item cloned");
    }
}

void
AnalysisOverviewWindow::configItem(ChartSpaceItem *item, QPoint pos)
{
    OverviewConfigDialog *p = new AnalysisOverviewConfigDialog(item, pos);
    p->exec(); // no mem leak as delete on close
}

void
PlanOverviewWindow::configItem(ChartSpaceItem* item, QPoint pos)
{
    OverviewConfigDialog* p = new PlanOverviewConfigDialog(item, pos);
    p->exec(); // no mem leak as delete on close
}

void
TrendsOverviewWindow::configItem(ChartSpaceItem* item, QPoint pos)
{
    OverviewConfigDialog* p = new TrendsOverviewConfigDialog(item, pos);
    p->exec(); // no mem leak as delete on close
}

void
EquipmentOverviewWindow::configItem(ChartSpaceItem* item, QPoint pos)
{
    OverviewConfigDialog* p = new EquipmentOverviewConfigDialog(item, pos);
    p->exec(); // no mem leak as delete on close

    // Called for both item updates and deletion, if the item is still in the chart space list
    // then its an update, otherwise it's a tile deletion which requires no recaculation.
    if (space->allItems().indexOf(item) != -1) {

        // need to recalculate the equipment cache after a user update.
        GlobalContext::context()->requestEqItemRecalculation(static_cast<CommonEqItem*>(item)->getEquipmentRef(), "Item changed");
    }
}

void
EquipmentOverviewWindow::configChanged(qint32 cfg)
{
    if (cfg & CONFIG_APPEARANCE) {

        for (ChartSpaceItem* item : space->allItems()) {
            item->configChanged(CONFIG_APPEARANCE);
        }
    }
}

void
EquipmentOverviewWindow::getTileConfig(ChartSpaceItem* item, QString& config) const
{
    // now the actual card settings
    switch (item->type) {

    case OverviewItemType::EQ_ITEM:
    case OverviewItemType::EQ_SUMMARY:
    case OverviewItemType::EQ_HISTORY:
    case OverviewItemType::EQ_NOTES:
    {
        // Due to lazy loading of perspectives, save the units in case
        // they are changed before the equipment perspective is loaded.
        if (chartExportInProgress_) {
            // for special case of exported charts, provide a default equipment id.
            config += "\"equipmentRef\":\"" + QUuid().toString() + "\",";
        } else {
            config += "\"equipmentRef\":\"" + static_cast<CommonEqItem*>(item)->getEquipmentRef().toString(QUuid::WithoutBraces) + "\",";
        }
    }
    break;

    default:
    {
        OverviewWindow::getTileConfig(item, config);
    }
    break;
    }
}

void
EquipmentOverviewWindow::setTileConfig(const QJsonObject& obj, int type, const QString& name, const QString& datafilter,
                                       int order, int column, int span, int deep, ChartSpaceItem* add) const
{
    QUuid equipmentRef = QUuid::fromString(obj["equipmentRef"].toString());

    // when charts are imported the tiles equipmentRef will be all zeros,
    // so we need to create a unique id for the tiles.
    if (equipmentRef.isNull()) equipmentRef = QUuid::createUuid();

    // now the actual card settings
    switch (type) {

    case OverviewItemType::EQ_ITEM:
    {
        EquipmentItem* eqItem = new EquipmentItem(space, name, equipmentRef);
        add = eqItem;
        add->datafilter = datafilter;
        space->addItem(order, column, span, deep, add);
    }
    break;

    case OverviewItemType::EQ_SUMMARY:
    {
        EquipmentSummary* eqSummary = new EquipmentSummary(space, name, equipmentRef);
        add = eqSummary;
        add->datafilter = datafilter;
        space->addItem(order, column, span, deep, add);
    }
    break;

    case OverviewItemType::EQ_HISTORY:
    {
        EquipmentHistory* eqHistory = new EquipmentHistory(space, name, equipmentRef);
        add = eqHistory;
        add->datafilter = datafilter;
        space->addItem(order, column, span, deep, add);
    }
    break;

    case OverviewItemType::EQ_NOTES:
    {
        EquipmentNotes* eqNotes = new EquipmentNotes(space, name, equipmentRef);
        add = eqNotes;
        add->datafilter = datafilter;
        space->addItem(order, column, span, deep, add);
    }
    break;

    default:
    {
        OverviewWindow::setTileConfig(obj, type, name, datafilter, order, column, span, deep, add);
    }
    break;
    }
}

void
EquipmentOverviewWindow::eqRecalculationComplete()
{
    // ensure the displayed tiles are up to date now calculation has finished
    if (eqWindowVisible_) {

        foreach(ChartSpaceItem *item, space->allItems()) {
            item->update();
        }
    }
}

void
EquipmentOverviewWindow::titleChanged(QString title)
{
    foreach(ChartSpaceItem *item, space->allItems()) {
        static_cast<CommonEqItem*>(item)->chartTitleChanged(title);
    }
}

void
EquipmentOverviewWindow::saveChart()
{
    chartExportInProgress_ = true;

    // saving the chart calls getTileConfig() via the "config" property read, but
    // we must not export the unique tile ids, otherwise when the chart is imported
    // it leads to duplicate tiles, this allows getTileConfig() to know what's happening.
    GcChartWindow::saveChart();

    chartExportInProgress_ = false;
}

#ifdef GC_HAS_CLOUD_DB
void
EquipmentOverviewWindow::exportChartToCloudDB()
{
    chartExportInProgress_ = true;

    // saving the chart calls getTileConfig() via the "config" property read, but
    // we must not export the unique tile ids, otherwise when the chart is imported
    // it leads to duplicate tiles, this allows getTileConfig() to know what's happening.
    GcChartWindow::saveChart();

    chartExportInProgress_ = false;
}
#endif

//
// ------------------------------- Overview Config Dialogs ------------------------------
//

AnalysisOverviewConfigDialog::AnalysisOverviewConfigDialog(ChartSpaceItem* item, QPoint pos)
    : OverviewConfigDialog(item, pos)
{
    setWhatsThis(help->getWhatsThisText(HelpWhatsThis::ChartRides_Overview_Config).arg(itemDetail.quick, itemDetail.description));
}

PlanOverviewConfigDialog::PlanOverviewConfigDialog(ChartSpaceItem* item, QPoint pos)
    : OverviewConfigDialog(item, pos)
{
    setWhatsThis(help->getWhatsThisText(HelpWhatsThis::Chart_Overview_Config).arg(itemDetail.quick, itemDetail.description));
}

TrendsOverviewConfigDialog::TrendsOverviewConfigDialog(ChartSpaceItem* item, QPoint pos)
    : OverviewConfigDialog(item, pos)
{
    setWhatsThis(help->getWhatsThisText(HelpWhatsThis::Chart_Overview_Config).arg(itemDetail.quick, itemDetail.description));
}

EquipmentOverviewConfigDialog::EquipmentOverviewConfigDialog(ChartSpaceItem* item, QPoint pos)
    : OverviewConfigDialog(item, pos)
{
    setWhatsThis(help->getWhatsThisText(HelpWhatsThis::Chart_Overview_Config).arg(itemDetail.quick, itemDetail.description));
}

void
EquipmentOverviewConfigDialog::removeItem()
{
    // need to delete it here, as it cannot be deleted by the tile item destructor, otherwise
    // equipment cache data is removed before it is written out to the xml file during shutdown.
    QUuid itemToDelete = static_cast<CommonEqItem*>(item)->getEquipmentRef();
 
    // remove item from chart space and delete it
    OverviewConfigDialog::removeItem();

    // remove the items equipment cache entry
    if (!itemToDelete.isNull()) EquipmentCache::getInstance().deleteEquipment(itemToDelete);
}



