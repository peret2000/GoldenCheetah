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


#include "ColorButton.h"
#include "EquipmentOverviewItems.h"
#include "EquipmentCache.h"

// supports tile move & clone
#include "OverviewWindows.h"
#include "Perspective.h"

bool
EquipmentOverviewItemConfig::registerItems()
{
    // get the factory
    ChartSpaceItemRegistry &registry = ChartSpaceItemRegistry::instance();

    // Register      TYPE                           SHORT                           DESCRIPTION                         SCOPE                        CREATOR
    registry.addItem(OverviewItemType::EQ_ITEM,     QObject::tr("Equipment"),       QObject::tr("Equipment Item"),      OverviewScope::EQUIPMENT,    EquipmentItem::create);
    registry.addItem(OverviewItemType::EQ_SUMMARY,  QObject::tr("Eq Link Summary"), QObject::tr("Equipment Summary"),   OverviewScope::EQUIPMENT,    EquipmentSummary::create);
    registry.addItem(OverviewItemType::EQ_HISTORY,  QObject::tr("Eq Link History"), QObject::tr("Equipment History"),   OverviewScope::EQUIPMENT,    EquipmentHistory::create);
    registry.addItem(OverviewItemType::EQ_NOTES,    QObject::tr("Eq Link Notes"),   QObject::tr("Equipment Notes"),     OverviewScope::EQUIPMENT,    EquipmentNotes::create);

    return true;
}

//
// ------------------------------------ EquipmentOverviewItemConfig --------------------------------------
//

EquipmentOverviewItemConfig::EquipmentOverviewItemConfig(ChartSpaceItem* item, Context* context) :
    OverviewItemConfig(item), eqLinkCompleter(nullptr)
{
    // Insert the fields between the default title & background colour button
    int insertRow = 1;

    // Create EquipmentLink field completer
    foreach(FieldDefinition field, GlobalContext::context()->rideMetadata->getFields()) {
        if (field.name == "EquipmentLink") {

            eqLinkCompleter = field.getCompleter(this, context->athlete->rideCache);
            break;
        }
    }

    if (item->type == OverviewItemType::EQ_ITEM) {

        // prevent negative values
        QDoubleValidator* eqValidator = new QDoubleValidator();
        eqValidator->setBottom(0);
        eqValidator->setTop(999999);
        eqValidator->setDecimals(EQ_DECIMAL_PRECISION);
        eqValidator->setNotation(QDoubleValidator::StandardNotation);

        displaytotal = new QComboBox(this);
        displaytotal->addItem(tr("Elevation"));
        displaytotal->addItem(tr("Distance"));
        connect(displaytotal, &QComboBox::currentIndexChanged, this, &EquipmentOverviewItemConfig::dataChanged);
        layout->insertRow(insertRow++, tr("Display Total"), displaytotal);

        nonGCDistance = new QLineEdit();
        nonGCDistance->setValidator(eqValidator);
        connect(nonGCDistance, &QLineEdit::textChanged, this, &EquipmentOverviewItemConfig::dataChanged);
        layout->insertRow(insertRow++, tr("Manual dst"), nonGCDistance);

        nonGCElevation = new QLineEdit();
        nonGCElevation->setValidator(eqValidator);
        connect(nonGCElevation, &QLineEdit::textChanged, this, &EquipmentOverviewItemConfig::dataChanged);
        layout->insertRow(insertRow++, tr("Manual elev"), nonGCElevation);

        eqTimeWindows = new QTableWidget(0, 5);
        QStringList headers = (QStringList() << tr("EquipmentLink") << tr("Start") << tr("Start Date") << tr("End") << tr("End Date"));
        eqTimeWindows->setHorizontalHeaderLabels(headers);
        eqTimeWindows->setColumnWidth(1, 40 * dpiXFactor);
        eqTimeWindows->setColumnWidth(2, 90 * dpiXFactor);
        eqTimeWindows->setColumnWidth(3, 40 * dpiXFactor);
        eqTimeWindows->setColumnWidth(4, 90 * dpiXFactor);
        eqTimeWindows->setMinimumWidth(400 * dpiXFactor);
        eqTimeWindows->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Stretch);
        eqTimeWindows->setSelectionBehavior(QAbstractItemView::SelectRows);
        eqTimeWindows->setSelectionMode(QAbstractItemView::SingleSelection);
        QPalette palette = eqTimeWindows->palette();
        palette.setBrush(QPalette::Highlight, palette.brush(QPalette::Base));
        palette.setBrush(QPalette::HighlightedText, palette.brush(QPalette::Text));
        eqTimeWindows->setPalette(palette);
        eqTimeWindows->verticalHeader()->setVisible(false);
        connect(eqTimeWindows, &QTableWidget::cellClicked, this, &EquipmentOverviewItemConfig::tableCellClicked);
        layout->insertRow(insertRow++, "History", eqTimeWindows);

        QHBoxLayout* buttonRow = new QHBoxLayout();
        QPushButton *addEqLink = new QPushButton("Add EquipmentLink");
        QPushButton *removeEqLink = new QPushButton("Remove EquipmentLink");
        buttonRow->addWidget(addEqLink);
        connect(addEqLink, &QPushButton::clicked, this, &EquipmentOverviewItemConfig::addEqLinkRow);
        buttonRow->addWidget(removeEqLink);
        connect(removeEqLink, &QPushButton::clicked, this, &EquipmentOverviewItemConfig::removeEqLinkRow);
        layout->insertRow(insertRow++, "", buttonRow);

        replaceDistance = new QLineEdit();
        replaceDistance->setValidator(eqValidator);
        connect(replaceDistance, &QLineEdit::textChanged, this, &EquipmentOverviewItemConfig::dataChanged);
        layout->insertRow(insertRow++, tr("Replacement dst"), replaceDistance);

        replaceElevation = new QLineEdit();
        replaceElevation->setValidator(eqValidator);
        connect(replaceElevation, &QLineEdit::textChanged, this, &EquipmentOverviewItemConfig::dataChanged);
        layout->insertRow(insertRow++, tr("Replacement elev"), replaceElevation);

        QHBoxLayout *replaceLayout = new QHBoxLayout();
        replaceDateSet = new QPushButton();
        replaceDateSet->setMaximumWidth(60 * dpiXFactor);
        connect(replaceDateSet, &QPushButton::clicked, this, &EquipmentOverviewItemConfig::repDateSetClicked);
        replaceDate = new QDateEdit();
        replaceDate->setCalendarPopup(true);
        replaceDate->setStyleSheet(QString("QDateEdit { border: 0px; } "));
        QSizePolicy sp_retain = replaceDate->sizePolicy();
        sp_retain.setRetainSizeWhenHidden(true);
        replaceDate->setSizePolicy(sp_retain);
        connect(replaceDate, &QDateEdit::dateChanged, this, &EquipmentOverviewItemConfig::dataChanged);
        replaceLayout->addWidget(replaceDateSet);
        replaceLayout->addWidget(replaceDate);
        layout->insertRow(insertRow++, tr("Replacement date"), replaceLayout);

        notes = new QPlainTextEdit();
        connect(notes, &QPlainTextEdit::textChanged, this, &EquipmentOverviewItemConfig::dataChanged);
        layout->insertRow(insertRow++, tr("Notes"), notes);
    }

    if (item->type == OverviewItemType::EQ_SUMMARY) {

        eqLinkName = new QLineEdit();
        if (eqLinkCompleter) eqLinkName->setCompleter(eqLinkCompleter);
        connect(eqLinkName, &QLineEdit::textChanged, this, &EquipmentOverviewItemConfig::dataChanged);
        layout->insertRow(insertRow++, tr("EquipmentLink"), eqLinkName);

        eqCheckBox = new QCheckBox();
        connect(eqCheckBox, &QCheckBox::checkStateChanged, this, &EquipmentOverviewItemConfig::dataChanged);
        layout->insertRow(insertRow++, tr("Show Athlete's Activities"), eqCheckBox);
    }

    if (item->type == OverviewItemType::EQ_HISTORY) {

        eqCheckBox = new QCheckBox();
        connect(eqCheckBox, &QCheckBox::checkStateChanged, this, &EquipmentOverviewItemConfig::dataChanged);
        layout->insertRow(insertRow++, tr("Most Recent First"), eqCheckBox);

        eqTimeWindows = new QTableWidget(0, 2);
        QStringList headers = (QStringList() << tr("Date") << tr("Description"));
        eqTimeWindows->setHorizontalHeaderLabels(headers);
        eqTimeWindows->setColumnWidth(0, 90 * dpiXFactor);
        eqTimeWindows->setColumnWidth(1, 410 * dpiXFactor);
        eqTimeWindows->setMinimumWidth(400 * dpiXFactor);
        eqTimeWindows->horizontalHeader()->setSectionResizeMode(1, QHeaderView::Stretch);
        eqTimeWindows->setSelectionBehavior(QAbstractItemView::SelectRows);
        eqTimeWindows->setSelectionMode(QAbstractItemView::SingleSelection);
        QPalette palette = eqTimeWindows->palette();
        palette.setBrush(QPalette::Highlight, palette.brush(QPalette::Base));
        palette.setBrush(QPalette::HighlightedText, palette.brush(QPalette::Text));
        eqTimeWindows->setPalette(palette);
        eqTimeWindows->verticalHeader()->setVisible(false);
        connect(eqTimeWindows, &QTableWidget::cellClicked, this, &EquipmentOverviewItemConfig::dataChanged);
        layout->insertRow(insertRow++, "History", eqTimeWindows);

        QHBoxLayout* buttonRow = new QHBoxLayout();
        QPushButton *addHistory = new QPushButton("Add History");
        QPushButton *removeHistory = new QPushButton("Remove History");
        buttonRow->addWidget(addHistory);
        connect(addHistory, &QPushButton::clicked, this, &EquipmentOverviewItemConfig::addHistoryRow);
        buttonRow->addWidget(removeHistory);
        connect(removeHistory, &QPushButton::clicked, this, &EquipmentOverviewItemConfig::removeHistoryRow);
        layout->insertRow(insertRow++, "", buttonRow);
    }

    if (item->type == OverviewItemType::EQ_NOTES) {

        notes = new QPlainTextEdit();
        connect(notes, &QPlainTextEdit::textChanged, this, &EquipmentOverviewItemConfig::dataChanged);
        layout->insertRow(insertRow++, tr("Notes"), notes);
    }

    setWidgets();
}

EquipmentOverviewItemConfig::~EquipmentOverviewItemConfig()
{
    if (eqLinkCompleter) delete eqLinkCompleter;
}

void
EquipmentOverviewItemConfig::setWidgets()
{
    block = true;

    // ensure bkgd color is initialised.
    bgcolor->setColor(item->color().name());

    // get base info
    name->setText(item->name);

    bool showEqItemElevationWids = static_cast<EquipmentOverviewWindow*>(item->parent->window)->isShowElevation();
    bool showEqItemNotesWids = static_cast<EquipmentOverviewWindow*>(item->parent->window)->isShowNotes();

    // set the widget values from the item
    switch (item->type) {

    case OverviewItemType::EQ_ITEM:
    {
        EqItem* mi = static_cast<EqItem*>(static_cast<EquipmentItem*>(item)->getAbsEqItem());

        displaytotal->setCurrentIndex(mi->displayTotalDistance_ ? 1 : 0);
        nonGCDistance->setText(QString::number(mi->getNonGCDistance(), 'f', EQ_DECIMAL_PRECISION));

        nonGCElevation->setText(QString::number(mi->getNonGCElevation(), 'f', EQ_DECIMAL_PRECISION));
        replaceElevation->setText(QString::number(mi->repElevation_, 'f', EQ_DECIMAL_PRECISION));
        layout->setRowVisible(nonGCElevation, showEqItemElevationWids);
        layout->setRowVisible(replaceElevation, showEqItemElevationWids);
        replaceDistance->setText(QString::number(mi->repDistance_, 'f', EQ_DECIMAL_PRECISION));
        replaceDateSet->setText((mi->repDateSet_) ? "reset" : "set");
        replaceDate->setVisible(mi->repDateSet_);
        if (mi->repDateSet_) replaceDate->setDate(mi->repDate_);

        // clear the table
        eqTimeWindows->setRowCount(0);

        for (int tableRow = 0; tableRow < mi->eqLinkUseList_.size(); tableRow++) {

            eqTimeWindows->insertRow(tableRow);
            const EqTimeWindow& eqUse = mi->eqLinkUseList_[tableRow];
            setEqLinkRowWidgets(tableRow, &eqUse);

            static_cast<QLineEdit*>(eqTimeWindows->cellWidget(tableRow, 0))->setText(eqUse.eqLinkName());

            static_cast<QLabel*>(eqTimeWindows->cellWidget(tableRow, 1))->setText((eqUse.startSet_) ? "reset" : "set");
            if (eqUse.startSet_) {
                static_cast<QDateEdit*>(eqTimeWindows->cellWidget(tableRow, 2))->setDate(eqUse.startDate_);
            }

            static_cast<QLabel*>(eqTimeWindows->cellWidget(tableRow, 3))->setText((eqUse.endSet_) ? "reset" : "set");
            if (eqUse.endSet_) {
                static_cast<QDateEdit*>(eqTimeWindows->cellWidget(tableRow, 4))->setDate(eqUse.endDate_);
            }
        }
        notes->setPlainText(mi->notes_);
        layout->setRowVisible(notes, showEqItemNotesWids);
    }
    break;

    case OverviewItemType::EQ_SUMMARY:
    {
        EqSummary* mi = static_cast<EqSummary*>(static_cast<EquipmentSummary*>(item)->getAbsEqItem());
        eqLinkName->setText(mi->getEqLinkName());
        eqCheckBox->setChecked(mi->showActivitiesPerAthlete_);
    }
    break;

    case OverviewItemType::EQ_HISTORY:
    {
        EqHistory* mi = static_cast<EqHistory*>(static_cast<EquipmentHistory*>(item)->getAbsEqItem());
        eqCheckBox->setChecked(mi->sortMostRecentFirst_);

        // clear the table
        eqTimeWindows->setRowCount(0);

        int tableRow = 0;
        for (const auto& eqHistory : mi->eqHistoryList_) {

            eqTimeWindows->insertRow(tableRow);
            setEqHistoryEntryRowWidgets(tableRow);

            static_cast<QDateEdit*>(eqTimeWindows->cellWidget(tableRow, 0))->setDate(eqHistory.date_);
            static_cast<QLineEdit*>(eqTimeWindows->cellWidget(tableRow, 1))->setText(eqHistory.text_);
            tableRow++;
        }
    }
    break;

    case OverviewItemType::EQ_NOTES:
    {
        EqNotes* mi = static_cast<EqNotes*>(static_cast<EquipmentNotes*>(item)->getAbsEqItem());
        notes->setPlainText(mi->notes_);
    }
    break;
    }
    block = false;
}

void
EquipmentOverviewItemConfig::repDateSetClicked()
{
    if (replaceDateSet->text() == "reset") {

        replaceDateSet->setText("set");
        replaceDate->setVisible(false);
    }
    else
    {
        replaceDateSet->setText("reset");
        replaceDate->setVisible(true);
    }
    dataChanged();
}

void
EquipmentOverviewItemConfig::tableCellClicked(int row, int column)
{
    // only handle cell clicks on the checkboxs
    if (column == 1 || column == 3) {

        if (static_cast<QLabel*>(eqTimeWindows->cellWidget(row, column))->text() == "reset") {

            static_cast<QLabel*>(eqTimeWindows->cellWidget(row, column))->setText("set");

            eqTimeWindows->setCellWidget(row, column + 1, nullptr);
        }
        else
        {
            static_cast<QLabel*>(eqTimeWindows->cellWidget(row, column))->setText("reset");

            QDateEdit* date = new QDateEdit();
            date->setCalendarPopup(true);
            date->setStyleSheet(QString("QDateEdit { border: 0px; } "));
            connect(date, &QDateEdit::dateChanged, this, &EquipmentOverviewItemConfig::dataChanged);
            eqTimeWindows->setCellWidget(row, column + 1, date);
        }
        dataChanged();
    }

    if (column == 2 || column == 4) {

        if (eqTimeWindows->cellWidget(row, column) == nullptr) {

            QDateEdit* date = new QDateEdit();
            date->setCalendarPopup(true);
            date->setStyleSheet(QString("QDateEdit { border: 0px; } "));
            connect(date, &QDateEdit::dateChanged, this, &EquipmentOverviewItemConfig::dataChanged);
            eqTimeWindows->setCellWidget(row, column, date);

            static_cast<QLabel*>(eqTimeWindows->cellWidget(row, column - 1))->setText("reset");
        }
        else
        {
            // cellClicked signal is captured by the QDateEdit so never received here
        }
        dataChanged();
    }
}

void
EquipmentOverviewItemConfig::setEqLinkRowWidgets(int tableRow, const EqTimeWindow* eqUse)
{
    QLineEdit* eqLink = new QLineEdit();
    eqLink->setFrame(false);
    if (eqLinkCompleter) eqLink->setCompleter(eqLinkCompleter);
    connect(eqLink, &QLineEdit::textChanged, this, &EquipmentOverviewItemConfig::dataChanged);
    eqTimeWindows->setCellWidget(tableRow, 0, eqLink);

    QLabel *startSet = new QLabel();
    startSet->setAlignment(Qt::AlignHCenter);
    startSet->setText((eqUse && eqUse->startSet_) ? "reset" : "set");
    eqTimeWindows->setCellWidget(tableRow, 1, startSet);

    if (eqUse && eqUse->startSet_) {
        QDateEdit* startDate = new QDateEdit();
        startDate->setCalendarPopup(true);
        startDate->setStyleSheet(QString("QDateEdit { border: 0px; } "));
        connect(startDate, &QDateEdit::dateChanged, this, &EquipmentOverviewItemConfig::dataChanged);
        eqTimeWindows->setCellWidget(tableRow, 2, startDate);
    }

    QLabel *endSet = new QLabel();
    endSet->setAlignment(Qt::AlignHCenter);
    endSet->setText((eqUse && eqUse->endSet_) ? "reset" : "set");
    eqTimeWindows->setCellWidget(tableRow, 3, endSet);

    if (eqUse && eqUse->endSet_) {
        QDateEdit* endDate = new QDateEdit();
        endDate->setCalendarPopup(true);
        endDate->setStyleSheet(QString("QDateEdit { border: 0px; } "));
        connect(endDate, &QDateEdit::dateChanged, this, &EquipmentOverviewItemConfig::dataChanged);
        eqTimeWindows->setCellWidget(tableRow, 4, endDate);
    }
}

void
EquipmentOverviewItemConfig::addEqLinkRow()
{
    block = true;

    eqTimeWindows->insertRow(0);
    setEqLinkRowWidgets(0, nullptr);

    block = false;
    dataChanged();
}

void
EquipmentOverviewItemConfig::removeEqLinkRow()
{
    eqTimeWindows->removeRow(eqTimeWindows->currentRow());
    dataChanged();
}

void
EquipmentOverviewItemConfig::setEqHistoryEntryRowWidgets(int tableRow)
{
    QDateEdit* historyDate = new QDateEdit();
    historyDate->setCalendarPopup(true);
    historyDate->setStyleSheet(QString("QDateEdit { border: 0px; } "));
    connect(historyDate, &QDateEdit::dateChanged, this, &EquipmentOverviewItemConfig::dataChanged);
    eqTimeWindows->setCellWidget(tableRow, 0, historyDate);

    QLineEdit* historyText = new QLineEdit();
    historyText->setFrame(false);
    connect(historyText, &QLineEdit::textChanged, this, &EquipmentOverviewItemConfig::dataChanged);
    eqTimeWindows->setCellWidget(tableRow, 1, historyText);
}

void
EquipmentOverviewItemConfig::addHistoryRow()
{
    block = true;

    eqTimeWindows->insertRow(0);
    setEqHistoryEntryRowWidgets(0);
    static_cast<QDateEdit*>(eqTimeWindows->cellWidget(0, 0))->setDate(QDate::currentDate());

    block = false;
    dataChanged();
}

void
EquipmentOverviewItemConfig::removeHistoryRow()
{
    eqTimeWindows->removeRow(eqTimeWindows->currentRow());
    dataChanged();
}

void
EquipmentOverviewItemConfig::dataChanged()
{
    // user edited or programmatically the data was changed
    // so lets update the item to reflect those changes
    // if they are valid. But block set when the widgets
    // are being initialised
    if (block) return;

    // Update base info
    item->name = name->text();
    item->bgcolor = bgcolor->getColor().name();

    // set the widget values from the item
    switch (item->type) {

    case OverviewItemType::EQ_ITEM:
    {
        EqItem* mi = static_cast<EqItem*>(static_cast<EquipmentItem*>(item)->getAbsEqItem());
        mi->xmlChartName_ = item->parent->window->title();
        mi->xmlTileName_ = name->text();

        mi->displayTotalDistance_ = displaytotal->currentIndex() ? true : false;
        mi->setNonGCDistance(nonGCDistance->text().toDouble());
        mi->setNonGCElevation(nonGCElevation->text().toDouble());

        QVector<EqTimeWindow> eqLinkUse;
        for (int tableRow = 0; tableRow < eqTimeWindows->rowCount(); tableRow++) {

            QString eqLinkName = static_cast<QLineEdit*>(eqTimeWindows->cellWidget(tableRow, 0))->text().simplified().remove(' ');

            if (eqLinkName != "") { // don't accept time windows without any eqLinkName text

                EqTimeWindow eqWindow(eqLinkName);

                QDate startDate;
                bool startSet = static_cast<QLabel*>(eqTimeWindows->cellWidget(tableRow, 1))->text() == "reset";
                if (startSet) {
                    startDate = static_cast<QDateEdit*>(eqTimeWindows->cellWidget(tableRow, 2))->date();
                }

                QDate endDate;
                bool endSet = static_cast<QLabel*>(eqTimeWindows->cellWidget(tableRow, 3))->text() == "reset";
                if (endSet) {
                    endDate = static_cast<QDateEdit*>(eqTimeWindows->cellWidget(tableRow, 4))->date();
                }

                eqLinkUse.push_back(EqTimeWindow(eqLinkName, startSet, startDate, endSet, endDate));
            }
        }
        mi->eqLinkUseList_ = eqLinkUse;

        mi->repDistance_ = replaceDistance->text().toDouble();
        mi->repElevation_ = replaceElevation->text().toDouble();

        mi->repDateSet_ = replaceDateSet->text() == "reset";
        if (mi->repDateSet_) mi->repDate_ = replaceDate->date();

        mi->notes_ = notes->toPlainText();
    }
    break;

    case OverviewItemType::EQ_SUMMARY:
    {
        EqSummary* mi = static_cast<EqSummary*>(static_cast<EquipmentSummary*>(item)->getAbsEqItem());
        mi->xmlChartName_ = item->parent->window->title();
        mi->xmlTileName_ = name->text();
        mi->setEqLinkName(eqLinkName->text());
        mi->showActivitiesPerAthlete_ = eqCheckBox->isChecked();
    }
    break;

    case OverviewItemType::EQ_HISTORY:
    {
        EqHistory* mi = static_cast<EqHistory*>(static_cast<EquipmentHistory*>(item)->getAbsEqItem());
        mi->xmlChartName_ = item->parent->window->title();
        mi->xmlTileName_ = name->text();
        mi->sortMostRecentFirst_ = eqCheckBox->isChecked();

        QVector<EqHistoryEntry> eqHistory;
        for (int tableRow = 0; tableRow < eqTimeWindows->rowCount(); tableRow++) {

            eqHistory.push_back(EqHistoryEntry(static_cast<QDateEdit*>(eqTimeWindows->cellWidget(tableRow, 0))->date(),
                                          static_cast<QLineEdit*>(eqTimeWindows->cellWidget(tableRow, 1))->text()));
        }
        mi->eqHistoryList_ = eqHistory;
        mi->sortHistoryEntries();
    }
    break;

    case OverviewItemType::EQ_NOTES:
    {
        EqNotes* mi = static_cast<EqNotes*>(static_cast<EquipmentNotes*>(item)->getAbsEqItem());
        mi->xmlChartName_ = item->parent->window->title();
        mi->xmlTileName_ = name->text();
        mi->notes_ = notes->toPlainText();
    }
    break;
    }
}

//
// ------------------------------------- Common Equipment Item -----------------------------------------
//

CommonEqItem::CommonEqItem(ChartSpace* parent, const QString& name)
    :   ChartSpaceItem(parent, name), tileDisplayHeight_(ROWHEIGHT * 5), absEqItem_(nullptr)
{
    setShowEdit(true);
}

CommonEqItem::CommonEqItem(ChartSpace* parent, const QString& name, const QUuid& equipmentRef)
    :   CommonEqItem(parent, name)
{
    equipmentRef_ = equipmentRef;
}

void
CommonEqItem::showEvent(QShowEvent*)
{
    // wait for tile geometry to be defined
    itemGeometryChanged();
}

void
CommonEqItem::chartTitleChanged(const QString& title)
{
    if (absEqItem_) absEqItem_->xmlChartName_ = title;
}

void
CommonEqItem::displayTileEditMenu(const QPoint& pos)
{
    QMenu popMenu;

    for (const GcChartWindow* chart : parent->window->getPerspective()->getCharts()) {

        // add the clone tile option at the top first
        if (chart == parent->window) {
            QAction* metaAction = new QAction("Clone");
            GcChartWindow* thisChart = const_cast<GcChartWindow*>(chart);
            QVariant varPtr(QVariant::fromValue(static_cast<void*>(thisChart)));
            metaAction->setData(varPtr);
            popMenu.addAction(metaAction);
        }
    }

    QAction* metaAction = new QAction(tr("Expand"));
    popMenu.addAction(metaAction);

    metaAction = new QAction(tr("Collapse"));
    popMenu.addAction(metaAction);

    metaAction = new QAction(tr("Expand All"));
    popMenu.addAction(metaAction);

    metaAction = new QAction(tr("Collapse All"));
    popMenu.addAction(metaAction);

    for (const GcChartWindow* chart : parent->window->getPerspective()->getCharts()) {

        // Add the move to chart options
        if (chart != parent->window) {
            QAction* metaAction = new QAction("-->" + chart->title());
            GcChartWindow* anotherChart = const_cast<GcChartWindow*>(chart);
            QVariant varPtr(QVariant::fromValue(static_cast<void*>(anotherChart)));
            metaAction->setData(varPtr);
            popMenu.addAction(metaAction);
        }
    }

    if (!popMenu.isEmpty()) {

        connect(&popMenu, &QMenu::triggered, this, &CommonEqItem::popupAction);
        popMenu.exec(pos);
    }
}

void CommonEqItem::popupAction(QAction* action)
{
    if (action->text() == tr("Expand")) {
        parent->adjustItemHeight(this, round(tileDisplayHeight_/ROWHEIGHT));
        return;
    }
    if (action->text() == tr("Collapse")) {
        parent->adjustItemHeight(this, 5);
        return;
    }
    if (action->text() == tr("Expand All")) {

        for (ChartSpaceItem* item : parent->allItems()) {

            parent->adjustItemHeight(item, round(static_cast<CommonEqItem*>(item)->tileDisplayHeight_ / ROWHEIGHT));
            updateGeometry();
        }
        return;
    }
    if (action->text() == tr("Collapse All")) {
        for (ChartSpaceItem* item : parent->allItems()) {

            parent->adjustItemHeight(item, 5);
            updateGeometry();
        }
        return;
    }

    GcChartWindow* toChart = static_cast<GcChartWindow*>(action->data().value<void*>());
    const ChartSpace* toChartSpace = static_cast<EquipmentOverviewWindow*>(toChart)->getSpace();

    if (parent == toChartSpace) {

        // clone me
        static_cast<EquipmentOverviewWindow*>(toChart)->cloneTile(this);
    }
    else
    {
        // move from existing to new chart
        parent->moveItem(this, const_cast<ChartSpace*>(toChartSpace));
    }
}


int
CommonEqItem::setupScrollableText(const QFontMetrics& fm, const QString& tileText,
                                        QMap<int, QString>& rowTextMap,
                                        int rowOffset /* = 0 */, int protectOffset /* = -1 */)
{
    // Using the QFontMetrics rectangle on the whole string doesn't always give the correct number
    // of rows as the painter will not render partial words, and there maybe newline characters,
    // this algorithm calculates the rows for whole words like the painter.

    QChar chr;
    int lastSpace = 0;
    int beginingOfRow = 0;
    int widthOfLineChars = 0;
    int numRowsInNotes = 0;
    int rowWidth = round(geometry().width() - (ROWHEIGHT * 2));

    // determine the number of rows and the scrollable text for each scrollbar position,
    // based upon only whole words per row and any newline characters.
    int i = 0;
    int textLen = tileText.length();

    while (i < textLen)
    {
        chr = tileText.at(i);
        widthOfLineChars += fm.horizontalAdvance(chr);
        if (chr == ' ' && (i > protectOffset)) lastSpace = i;

        if (chr == '\n') { 
            rowTextMap[numRowsInNotes + rowOffset] = tileText.mid(beginingOfRow, i - beginingOfRow);

            i++;
            lastSpace = beginingOfRow = i;
            numRowsInNotes++;
            widthOfLineChars = 0;
        }
        else if (widthOfLineChars > rowWidth) { // characters exceed row capacity

            if (chr.isSpace()) // the last character causing the overflow is a space
            {
                rowTextMap[numRowsInNotes + rowOffset] = tileText.mid(beginingOfRow, i - beginingOfRow);
                while (i < textLen) {
                    if (tileText.at(i) == ' ') i++; else break;
                }
                lastSpace = beginingOfRow = i;
            }
            else if (lastSpace > beginingOfRow) // space character exists in the row, so revert to last whole word
            {
                rowTextMap[numRowsInNotes + rowOffset] = tileText.mid(beginingOfRow, lastSpace - beginingOfRow);
                while (i < textLen) {
                    if (tileText.at(i) == ' ') i++; else break;
                }
                lastSpace = beginingOfRow = i = lastSpace+1;
            }
            else  // otherwise total string flow
            {
                rowTextMap[numRowsInNotes + rowOffset] = tileText.mid(beginingOfRow, i - beginingOfRow);
                lastSpace = beginingOfRow = i;
            }
            numRowsInNotes++;
            widthOfLineChars = 0;
        }
        else
        {
            i++; // advance to next character
        }
    }

    rowTextMap[numRowsInNotes + rowOffset] = tileText.mid(beginingOfRow, i - beginingOfRow);
    return numRowsInNotes + 1;
}

void
CommonEqItem::configChanged(qint32 cfg) {

    if (cfg & CONFIG_APPEARANCE) {
        inactiveColor_ = (GCColor::luminance(RGBColor(color())) < 127) ? QColor(QColor(100, 100, 100)) : QColor(QColor(170, 170, 170));
        textColor_ = (GCColor::luminance(RGBColor(color())) < 127) ? QColor(QColor(200, 200, 200)) : QColor(QColor(70, 70, 70));
        alertColor_ = QColor(QColor(255, 170, 0));
    }
}

//
// ---------------------------------------- Equipment Item --------------------------------------------
//

EquipmentItem::EquipmentItem(ChartSpace* parent, const QString& name, const QUuid& equipmentRef)
    : CommonEqItem(parent, name, equipmentRef)
{
    type = OverviewItemType::EQ_ITEM;

    // find cached equipment using the tile's reference from perspective file
    absEqItem_ = EquipmentCache::getInstance().getEquipment(equipmentRef_);
    
    if (absEqItem_ == nullptr) {

        // create a new cached equipment (user tile creation or chart import causing tile creation)
        absEqItem_ = EquipmentCache::getInstance().createEquipment(equipmentRef_, parent->window->title(), name, EqItemType::EQ_ITEM);
    }

    // setup xml reference names
    absEqItem_->xmlChartName_ = parent->window->title();
    absEqItem_->xmlTileName_ = name;

    configwidget_ = new EquipmentOverviewItemConfig(this, parent->context);
    configwidget_->hide();

    configChanged(CONFIG_APPEARANCE);
}

EquipmentItem::EquipmentItem(const EquipmentItem& toCopy)
    :   CommonEqItem(toCopy.parent, toCopy.name + " clone")
{
    type = OverviewItemType::EQ_ITEM;

    absEqItem_ = EquipmentCache::getInstance().cloneEquipment(toCopy.getEquipmentRef());
    // setup xml reference names
    absEqItem_->xmlChartName_ = parent->window->title();
    absEqItem_->xmlTileName_ = name;
    equipmentRef_ = absEqItem_->getEquipmentRef();

    configwidget_ = new EquipmentOverviewItemConfig(this, parent->context);
    configwidget_->hide();

    configChanged(CONFIG_APPEARANCE);
}

void
EquipmentItem::setData(RideItem*)
{
    // called when the item's config dialog is closed.
    static_cast<EqItem*>(absEqItem_)->sortEqLinkUseWindows();
}

void
EquipmentItem::itemGeometryChanged()
{
    scrollableDisplayText_.clear();
    QFontMetrics fm(parent->smallfont, parent->device());

    setupScrollableText(fm, static_cast<EqItem*>(absEqItem_)->notes_, scrollableDisplayText_);
}

bool
EquipmentItem::isWithin(const QStringList& rideEqLinkNameList, const QDate& actDate) const
{
    return static_cast<EqItem*>(absEqItem_)->isWithin(rideEqLinkNameList, actDate);
}

bool
EquipmentItem::isWithin(const QDate& actDate) const
{
    return static_cast<EqItem*>(absEqItem_)->isWithin(actDate);
}

bool
EquipmentItem::rangeIsValid() const
{
    return static_cast<EqItem*>(absEqItem_)->rangeIsValid();
}

bool
EquipmentItem::allEqLinkNamesCompleterVals() const
{
    return static_cast<EqItem*>(absEqItem_)->allEqLinkNamesCompleterVals();
}

void
EquipmentItem::configChanged(qint32 cfg) {

    if (cfg & CONFIG_APPEARANCE) {

        CommonEqItem::configChanged(cfg);

        eqsmallfont.setPixelSize(pixelSizeForFont(eqsmallfont, ROWHEIGHT *1.8f));
        eqsmallfont.setHintingPreference(QFont::HintingPreference::PreferNoHinting);

        eqmidfont.setPixelSize(pixelSizeForFont(eqmidfont, ROWHEIGHT *2.0f));
        eqmidfont.setHintingPreference(QFont::HintingPreference::PreferNoHinting);

        eqbigfont.setPixelSize(pixelSizeForFont(eqbigfont, ROWHEIGHT *2.5f));
        eqbigfont.setHintingPreference(QFont::HintingPreference::PreferNoHinting);
    }
}

void
EquipmentItem::itemPaint(QPainter* painter, const QStyleOptionGraphicsItem*, QWidget*) {
    
    // mid is slightly higher to account for space around title, move mid up
    static double mid = ROWHEIGHT * 3.0f;

    bool showElevationWids(static_cast<EquipmentOverviewWindow*>(parent->window)->isShowElevation());

    EqItem* eqItem = static_cast<EqItem*>(absEqItem_);

    bool overDistance = (eqItem->repDistance_ !=0) && (eqItem->getTotalDistance() > eqItem->repDistance_);
    bool overElevation = showElevationWids && (eqItem->repElevation_ !=0) && (eqItem->getTotalElevation() > eqItem->repElevation_);
    bool overDate = (eqItem->repDateSet_) && (QDate::currentDate() > eqItem->repDate_);

    // we align centre and mid
    QFont selected;
    switch (static_cast<EquipmentOverviewWindow*>(parent->window)->isTextSize()) {
    case eqTextSizeType::SMALL: {
        selected = eqsmallfont;
        painter->setFont(eqsmallfont);
    } break;
    case eqTextSizeType::MEDIUM: {
        selected = eqmidfont;
        painter->setFont(eqmidfont);
    }break;
    case eqTextSizeType::LARGE: {
        selected = eqbigfont;
        painter->setFont(eqbigfont);
    }break;
    }
    QFontMetrics fm(selected);

    double mainDisplayValue = eqItem->displayTotalDistance_ ? eqItem->getTotalDistance() : eqItem->getTotalElevation();
    QString totalValue(QString("%L1").arg(mainDisplayValue, 0, 'f', 0));
    QRectF rect = QFontMetrics(selected, parent->device()).boundingRect(totalValue);

    if ((!rangeIsValid()) || (!allEqLinkNamesCompleterVals()) || overDistance || overElevation || overDate) {
        painter->setPen(alertColor_);
    } else if (isWithin(QDate::currentDate())) {
        painter->setPen(GColor(CPLOTMARKER));
    } else {
        painter->setPen(inactiveColor_);
    }

    // display the main text
    painter->drawText(QPointF((geometry().width() - rect.width()) / 2.0f,
                        mid + (fm.ascent() / 3.0f)), totalValue); // divided by 3 to account for "gap" at top of font

    painter->setPen(QColor(100, 100, 100));
    painter->setFont(parent->smallfont);
    double addy = QFontMetrics(parent->smallfont).height();

    QString distUnits = GlobalContext::context()->useMetricUnits ? tr(" km") : tr(" miles");
    QString elevUnits = GlobalContext::context()->useMetricUnits ? tr(" meters") : tr(" feet");
    QString totalUnits = eqItem->displayTotalDistance_ ? distUnits : elevUnits;

    painter->drawText(QPointF((geometry().width() - QFontMetrics(parent->smallfont).horizontalAdvance(totalUnits)) / 2.0f,
        mid + (fm.ascent() / 3.6f) + addy), totalUnits); // divided by 3 to account for "gap" at top of font

    double rowY(ROWHEIGHT * 5);
    double rowWidth(geometry().width() - (ROWHEIGHT * 2));
    double rowHeight(geometry().height() - (ROWHEIGHT * 4));

    for (const auto& eqUse : eqItem->eqLinkUseList_) {

        // display the date in either active, inactive or out of range colours
        if ((!eqUse.rangeIsValid()) || (!eqUse.eqLinkIsCompleterVal())) {
            painter->setPen(alertColor_);
        }
        else if (eqUse.isWithin(QDate::currentDate())) {
            painter->setPen(GColor(CPLOTMARKER));
        }
        else {
            painter->setPen(inactiveColor_);
        }

        QString dateString(eqUse.eqLinkName() + ": ");

        // Format date field
        if (!eqUse.startSet_) {
            if (!eqUse.endSet_) {
                dateString += tr("All Dates");
            }
            else {
                dateString += eqUse.endDate_.toString("->d MMM yy");
            }
        }
        else {
            if (!eqUse.endSet_) {
                dateString += eqUse.startDate_.toString("d MMM yy ->");
            }
            else {
                dateString += (eqUse.startDate_.toString("d MMM yy->") + eqUse.endDate_.toString("d MMM yy"));
            }
        }

        rect = QFontMetrics(parent->smallfont, parent->device()).boundingRect(dateString);
        painter->drawText(QRectF(ROWHEIGHT, rowY, rowWidth, rowHeight), dateString);

        rowY += (ROWHEIGHT * ceil(rect.width() / rowWidth));
    }

    painter->setPen(textColor_);

    if (static_cast<EquipmentOverviewWindow*>(parent->window)->isShowActivities()) {
        painter->drawText(QRectF(ROWHEIGHT, rowY, rowWidth,rowHeight),
            QString(tr("Activities: ")) + QString("%L1").arg(eqItem->getNumActivities()));
        rowY += (ROWHEIGHT * 1.2);
    } else {
        rowY += (ROWHEIGHT * 0.2);
    }

    painter->drawText(QRectF(ROWHEIGHT, rowY, rowWidth, rowHeight),
        QString(tr("Distance: ")) + QString("%L1").arg(eqItem->getGCDistance(), 0, 'f', 0) + distUnits);

    if (showElevationWids) {
        rowY += ROWHEIGHT;
        painter->drawText(QRectF(ROWHEIGHT, rowY, rowWidth, rowHeight),
            QString(tr("Elevation: ")) + QString("%L1").arg(eqItem->getGCElevation(), 0, 'f', 0) + elevUnits);
    }

    if (static_cast<EquipmentOverviewWindow*>(parent->window)->isShowTime()) {
        rowY += ROWHEIGHT;
        painter->drawText(QRectF(ROWHEIGHT, rowY, rowWidth, rowHeight),
            QString(tr("Time: ")) + QString("%L1").arg(eqItem->getActivityTimeInSecs() * HOURS_PER_SECOND, 0, 'f', 0) + " hrs");
    }

    bool addNotesOffset = false;
    rowY += (ROWHEIGHT * 0.2);

    if (eqItem->getNonGCDistance() != 0) {
        rowY += (ROWHEIGHT);
        addNotesOffset = true;
        painter->drawText(QRectF(ROWHEIGHT, rowY, rowWidth, rowHeight),
            QString(tr("Manual dst: ")) + QString("%L1").arg(eqItem->getNonGCDistance(), 0, 'f', 0) + distUnits);
    }

    if (showElevationWids && (eqItem->getNonGCElevation() != 0)) {
        rowY += ROWHEIGHT;
        addNotesOffset = true;
        painter->drawText(QRectF(ROWHEIGHT, rowY, rowWidth, rowHeight),
            QString(tr("Manual elev: ")) + QString("%L1").arg(eqItem->getNonGCElevation(), 0, 'f', 0) + elevUnits);
    }

    if (eqItem->repDistance_ != 0) {
        if (overDistance) painter->setPen(alertColor_);
        rowY += ROWHEIGHT;
        addNotesOffset = true;
        painter->drawText(QRectF(ROWHEIGHT, rowY, rowWidth, rowHeight),
            QString(tr("Replace dist: ")) + QString("%L1").arg(eqItem->repDistance_, 0, 'f', 0) + distUnits);
        if (overDistance) painter->setPen(textColor_);
    }

    if (showElevationWids && (eqItem->repElevation_ != 0)) {
        if (overElevation) painter->setPen(alertColor_);
        rowY += ROWHEIGHT;
        addNotesOffset = true;
        painter->drawText(QRectF(ROWHEIGHT, rowY, rowWidth, rowHeight),
            QString(tr("Replace elev: ")) + QString("%L1").arg(eqItem->repElevation_, 0, 'f', 0) + elevUnits);
        if (overElevation) painter->setPen(textColor_);
    }

    if (eqItem->repDateSet_) {
        if (overDate) painter->setPen(alertColor_);
        rowY += ROWHEIGHT;
        addNotesOffset = true;
        painter->drawText(QRectF(ROWHEIGHT, rowY, rowWidth, rowHeight),
            QString(tr("Replace date: ")) + eqItem->repDate_.toString("->d MMM yy"));
        if (overDate) painter->setPen(textColor_);
    }

    if (static_cast<EquipmentOverviewWindow*>(parent->window)->isShowNotes() && (!scrollableDisplayText_.isEmpty())) {
        rowY +=  (addNotesOffset) ? ROWHEIGHT*1.3 : ROWHEIGHT;
        painter->drawText(QRectF(ROWHEIGHT, rowY, rowWidth, rowHeight), "Notes:");
        rowY += ROWHEIGHT;

        for (const auto& eqNotes : scrollableDisplayText_) {
            painter->drawText(QRectF(ROWHEIGHT, rowY, rowWidth, ROWHEIGHT), eqNotes);
            rowY += ROWHEIGHT;
        }
    }

    tileDisplayHeight_ = rowY + (ROWHEIGHT * 1.5);
}

//
// ---------------------------------------- Equipment Summary --------------------------------------------
//

EquipmentSummary::EquipmentSummary(ChartSpace* parent, const QString& name, const QUuid& equipmentRef)
    : CommonEqItem(parent, name, equipmentRef)
{
    // equipment summary item
    type = OverviewItemType::EQ_SUMMARY;

    // find cached equipment using the tile's reference from perspective file
    absEqItem_ = EquipmentCache::getInstance().getEquipment(equipmentRef_);

    if (absEqItem_ == nullptr) {

        // create a new cached equipment (user tile creation or chart import causing tile creation)
        absEqItem_ = EquipmentCache::getInstance().createEquipment(equipmentRef_, parent->window->title(), name, EqItemType::EQ_SUMMARY);
    }

    // setup xml reference names
    absEqItem_->xmlChartName_ = parent->window->title();
    absEqItem_->xmlTileName_ = name;

    configwidget_ = new EquipmentOverviewItemConfig(this, parent->context);
    configwidget_->hide();

    configChanged(CONFIG_APPEARANCE);
}

EquipmentSummary::EquipmentSummary(const EquipmentSummary& toCopy)
    :   CommonEqItem(toCopy.parent, toCopy.name + " clone")
{
    type = OverviewItemType::EQ_SUMMARY;

    absEqItem_ = EquipmentCache::getInstance().cloneEquipment(toCopy.getEquipmentRef());
    // setup xml reference names
    absEqItem_->xmlChartName_ = parent->window->title();
    absEqItem_->xmlTileName_ = name;
    equipmentRef_ = absEqItem_->getEquipmentRef();

    configwidget_ = new EquipmentOverviewItemConfig(this, parent->context);
    configwidget_->hide();

    configChanged(CONFIG_APPEARANCE);
}

void
EquipmentSummary::itemPaint(QPainter* painter, const QStyleOptionGraphicsItem*, QWidget*) {

    double rowY(ROWHEIGHT * 2.5);
    double rowWidth(geometry().width() - (ROWHEIGHT * 2));
    double rowHeight(geometry().height() - (ROWHEIGHT * 4));

    painter->setFont(parent->smallfont);

    EqSummary* eqSummary = static_cast<EqSummary*>(absEqItem_);

    if (eqSummary->eqLinkIsCompleterVal()) {
        painter->setPen(GColor(CPLOTMARKER));
    } else {
        painter->setPen(alertColor_);
    }

    QString eqLinkName = (eqSummary->getEqLinkName() != "") ? eqSummary->getEqLinkName() + ": All Dates" : "All Activities";
    painter->drawText(QRectF(ROWHEIGHT, rowY, rowWidth, rowHeight), eqLinkName);

    painter->setPen(GColor(CPLOTMARKER));

    rowY += (ROWHEIGHT * 1.3);
    painter->setPen(textColor_);
    painter->drawText(QRectF(ROWHEIGHT, rowY, rowWidth, rowHeight),
        QString(tr("Activities: ")) + QString("%L1").arg(eqSummary->getEqLinkNumActivities()));

    rowY += ROWHEIGHT;
    if (eqSummary->showActivitiesPerAthlete_) {
        for (auto itr = eqSummary->getAthleteActivityMap().keyValueBegin(); itr != eqSummary->getAthleteActivityMap().keyValueEnd(); ++itr) {

            painter->drawText(QRectF(ROWHEIGHT, rowY, rowWidth, rowHeight),
                QString("   ") + itr->first + ": " + QString("%L1").arg(itr->second));

            rowY += ROWHEIGHT;
        }
    }

    rowY += (ROWHEIGHT * 0.25);
    QString eqLinkEarliestDate = (eqSummary->getEqLinkNumActivities() != 0) ? eqSummary->getEqLinkEarliestDate().toString("d MMM yyyy") : " --";
    painter->drawText(QRectF(ROWHEIGHT, rowY, rowWidth, rowHeight),
        QString(tr("Earliest Activity: ")) + eqLinkEarliestDate);

    rowY += (ROWHEIGHT);
    QString eqLinkLatestDate = (eqSummary->getEqLinkNumActivities() != 0) ? eqSummary->getEqLinkLatestDate().toString("d MMM yyyy") : " --";
    painter->drawText(QRectF(ROWHEIGHT, rowY, rowWidth, rowHeight),
        QString(tr("Latest Activity: ")) + eqLinkLatestDate);

    rowY += (ROWHEIGHT * 1.25);
    QString distUnits = GlobalContext::context()->useMetricUnits ? tr(" km") : tr(" miles");
    painter->drawText(QRectF(ROWHEIGHT, rowY, rowWidth, rowHeight),
        QString(tr("Distance: ")) + QString("%L1").arg(eqSummary->getEqLinkTotalDistance(), 0, 'f', 0) + distUnits);

    rowY += (ROWHEIGHT);
    QString elevUnits = GlobalContext::context()->useMetricUnits ? tr(" meters") : tr(" feet");
    painter->drawText(QRectF(ROWHEIGHT, rowY, rowWidth, rowHeight),
        QString(tr("Elevation: ")) + QString("%L1").arg(eqSummary->getEqLinkTotalElevation(), 0, 'f', 0) + elevUnits);

    rowY += ROWHEIGHT;
    painter->drawText(QRectF(ROWHEIGHT, rowY, rowWidth, rowHeight),
        QString(tr("Time: ")) + QString("%L1").arg(eqSummary->getEqLinkTotalTimeInSecs()*HOURS_PER_SECOND, 0, 'f', 0) + " hrs");

    tileDisplayHeight_ = rowY + (ROWHEIGHT * 1.5);
}

//
// ---------------------------------------- Equipment History --------------------------------------------
//

EquipmentHistory::EquipmentHistory(ChartSpace* parent, const QString& name, const QUuid& equipmentRef) :
    CommonEqItem(parent, name, equipmentRef)
{
    // equipment summary item
    type = OverviewItemType::EQ_HISTORY;

    // find cached equipment using the tile's reference from perspective file
    absEqItem_ = EquipmentCache::getInstance().getEquipment(equipmentRef_);

    if (absEqItem_ == nullptr) {

        // create a new cached equipment (user tile creation or chart import causing tile creation)
        absEqItem_ = EquipmentCache::getInstance().createEquipment(equipmentRef_, parent->window->title(), name, EqItemType::EQ_HISTORY);
    }

    // setup xml reference names
    absEqItem_->xmlChartName_ = parent->window->title();
    absEqItem_->xmlTileName_ = name;

    configwidget_ = new EquipmentOverviewItemConfig(this, parent->context);
    configwidget_->hide();

    scrollPosn_ = 0;
    scrollbar_ = new VScrollBar(this, parent);
    scrollbar_->show();

    configChanged(CONFIG_APPEARANCE);
}

EquipmentHistory::EquipmentHistory(const EquipmentHistory& toCopy)
    :   CommonEqItem(toCopy.parent, toCopy.name + " clone")
{
    type = OverviewItemType::EQ_HISTORY;

    absEqItem_ = EquipmentCache::getInstance().cloneEquipment(toCopy.getEquipmentRef());
    // setup xml reference names
    absEqItem_->xmlChartName_ = parent->window->title();
    absEqItem_->xmlTileName_ = name;
    equipmentRef_ = absEqItem_->getEquipmentRef();

    configwidget_ = new EquipmentOverviewItemConfig(this, parent->context);
    configwidget_->hide();

    scrollPosn_ = 0;
    scrollbar_ = new VScrollBar(this, parent);
    scrollbar_->show();

    configChanged(CONFIG_APPEARANCE);
}

void
EquipmentHistory::itemGeometryChanged()
{
    QFontMetrics fm(parent->smallfont, parent->device());

    int numHistoryRows = 0;
    scrollableDisplayText_.clear();

    for (const auto& eqHistory : static_cast<EqHistory*>(absEqItem_)->eqHistoryList_) {

        QString historyEntryStr(eqHistory.date_.toString("dd MMM yyyy") + ": " + eqHistory.text_);
        numHistoryRows += setupScrollableText(fm, historyEntryStr, scrollableDisplayText_, numHistoryRows, 13);
    }

    double scrollwidth = fm.boundingRect("X").width();

    if ((geometry().height() - 40)  < ((numHistoryRows + 2.5) * ROWHEIGHT ))
    {
        // set the scrollbar to the rhs
        scrollbar_->show();
        scrollbar_->setGeometry(geometry().width() - scrollwidth, ROWHEIGHT * 2.5, scrollwidth, geometry().height() - (ROWHEIGHT * 2.5) - 40);
        scrollbar_->setAreaHeight(numHistoryRows * ROWHEIGHT);
    }
    else
    {
        scrollbar_->hide();
    }
}

void
EquipmentHistory::wheelEvent(QGraphicsSceneWheelEvent* w)
{
    if (scrollbar_->canscroll) {

        scrollbar_->movePos(w->delta());
        w->accept();
    }
}

void
EquipmentHistory::itemPaint(QPainter* painter, const QStyleOptionGraphicsItem*, QWidget*) {

    double rowY(ROWHEIGHT * 2.5);
    double rowWidth(geometry().width() - (ROWHEIGHT * 2));
    double rowHeight(geometry().height() - rowY);

    painter->setFont(parent->smallfont);
    painter->setPen(textColor_);

    // Scale scrollbarPosn based on ratio of displayed rows and eqHistoryList_ entries
    scrollPosn_ = (scrollbar_->pos() + (ROWHEIGHT/2)) / ROWHEIGHT;

    // don't paint on the edges
    painter->setClipRect(40, 40, geometry().width() - 80, geometry().height() - 80);

    int listPosn = 0;
    for (const auto& eqHistory : scrollableDisplayText_) {

        if (listPosn >= scrollPosn_) {

            painter->drawText(QRectF(ROWHEIGHT, rowY, rowWidth, rowHeight), eqHistory);

            if ((eqHistory.size() > 11) && (eqHistory.at(11) == ':')) {
                painter->setPen(GColor(CPLOTMARKER));
                painter->drawText(QRectF(ROWHEIGHT, rowY, rowWidth, rowHeight), eqHistory.mid(0, 12));
            }
            painter->setPen(textColor_);

            rowY += ROWHEIGHT;
        }
        listPosn++;
    }

    tileDisplayHeight_ = rowY;
}

//
// ---------------------------------------- Equipment Notes --------------------------------------------
//

EquipmentNotes::EquipmentNotes(ChartSpace* parent, const QString& name, const QUuid& equipmentRef) :
    CommonEqItem(parent, name, equipmentRef)
{
    // equipment summary item
    type = OverviewItemType::EQ_NOTES;

    // find cached equipment using the tile's reference from perspective file
    absEqItem_ = EquipmentCache::getInstance().getEquipment(equipmentRef_);

    if (absEqItem_ == nullptr) {

        // create a new cached equipment (user tile creation or chart import causing tile creation)
        absEqItem_ = EquipmentCache::getInstance().createEquipment(equipmentRef_, parent->window->title(), name, EqItemType::EQ_NOTES);
    }

    // setup xml reference names
    absEqItem_->xmlChartName_ = parent->window->title();
    absEqItem_->xmlTileName_ = name;

    configwidget_ = new EquipmentOverviewItemConfig(this, parent->context);
    configwidget_->hide();

    scrollPosn_ = 0;
    scrollbar_ = new VScrollBar(this, parent);
    scrollbar_->show();

    configChanged(CONFIG_APPEARANCE);
}

EquipmentNotes::EquipmentNotes(const EquipmentNotes& toCopy)
    :   CommonEqItem(toCopy.parent, toCopy.name + " clone")
{
    type = OverviewItemType::EQ_NOTES;

    absEqItem_ = EquipmentCache::getInstance().cloneEquipment(toCopy.getEquipmentRef());
    // setup xml reference names
    absEqItem_->xmlChartName_ = parent->window->title();
    absEqItem_->xmlTileName_ = name;
    equipmentRef_ = absEqItem_->getEquipmentRef();

    configwidget_ = new EquipmentOverviewItemConfig(this, parent->context);
    configwidget_->hide();

    scrollPosn_ = 0;
    scrollbar_ = new VScrollBar(this, parent);
    scrollbar_->show();

    configChanged(CONFIG_APPEARANCE);
}

void
EquipmentNotes::itemGeometryChanged()
{
    QFontMetrics fm(parent->smallfont, parent->device());

    scrollableDisplayText_.clear();
    int numRowsInNotes = setupScrollableText(fm, static_cast<EqNotes*>(absEqItem_)->notes_, scrollableDisplayText_);

    // set the scrollbar width
    double charWidth = fm.boundingRect("X").width();

    if ((geometry().height() - 40) < ((numRowsInNotes+2) * ROWHEIGHT))
    {
        // set the scrollbar to the rhs
        scrollbar_->show();
        scrollbar_->setGeometry(geometry().width() - charWidth, ROWHEIGHT * 2.5, charWidth, geometry().height() - (3 * ROWHEIGHT));
        scrollbar_->setAreaHeight(numRowsInNotes * ROWHEIGHT);
    }
    else
    {
        scrollbar_->hide();
    }
}

void
EquipmentNotes::wheelEvent(QGraphicsSceneWheelEvent* w)
{
    if (scrollbar_->canscroll) {

        scrollbar_->movePos(w->delta());
        w->accept();
    }
}

void
EquipmentNotes::itemPaint(QPainter* painter, const QStyleOptionGraphicsItem*, QWidget*) {

    painter->setPen(textColor_);
    painter->setFont(parent->smallfont);

    double rowY(ROWHEIGHT * 2.5);
    double rowWidth(geometry().width() - (ROWHEIGHT * 2));
    double rowHeight(geometry().height() - rowY);

    scrollPosn_ = (scrollbar_->pos() + (ROWHEIGHT/2)) / ROWHEIGHT;

    // don't paint on the edges
    painter->setClipRect(40, 40, geometry().width() - 80, geometry().height() - 80);

    int listPosn = 0;
    for (const auto& eqNotes : scrollableDisplayText_) {

        if (listPosn >= scrollPosn_) {
            painter->drawText(QRectF(ROWHEIGHT, rowY, rowWidth, rowHeight), eqNotes);
            rowY += ROWHEIGHT;
        }
        listPosn++;
    }

    tileDisplayHeight_ = rowY;
}

