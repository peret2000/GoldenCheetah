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

#ifndef _GC_EquipmentOverviewItem_h
#define _GC_EquipmentOverviewItem_h 1

#include <QMap>
#include <memory> // Atomics
#include "OverviewItems.h"
#include "EquipmentItems.h"

class ColorButton;

//
// Configuration widget for ALL Equipment Overview Items
//
class EquipmentOverviewItemConfig : public OverviewItemConfig
{
    Q_OBJECT

    public:

        EquipmentOverviewItemConfig(ChartSpaceItem *, Context*);
        virtual ~EquipmentOverviewItemConfig();

        static bool registerItems();

        // set the config widgets to reflect current config
        virtual void setWidgets() override;

    protected slots:

        // retrieve values when user edits them (if they're valid)
        virtual void dataChanged() override;

        void addEqLinkRow();
        void removeEqLinkRow();
        void addHistoryRow();
        void removeHistoryRow();
        void tableCellClicked(int row, int column);
        void repDateSetClicked();

    protected:

        // before show, lets make sure the background color and widgets are set correctly
        void showEvent(QShowEvent *) override { setWidgets(); }

    private:

        void setEqLinkRowWidgets(int tableRow, const EqTimeWindow* eqUse);
        void setEqHistoryEntryRowWidgets(int tableRow);

        // Equipment items
        QLineEdit *eqLinkName;
        QCompleter* eqLinkCompleter; // EquipmentLink completer
        QLineEdit *nonGCDistance, *nonGCElevation;
        QLineEdit *replaceDistance, *replaceElevation;
        QComboBox* displaytotal;
        QPushButton* replaceDateSet;
        QDateEdit* replaceDate;
        QCheckBox *eqCheckBox;
        QPlainTextEdit *notes;
        QTableWidget *eqTimeWindows;

};

class CommonEqItem : public ChartSpaceItem
{
    Q_OBJECT

    public:

        virtual ~CommonEqItem() {}

        void configChanged(qint32) override;

        // The following don't apply to most Equipment Items.
        void setData(RideItem*) override {}
        void setDateRange(DateRange) override {}
        void itemGeometryChanged() override {}
        void itemPaint(QPainter*, const QStyleOptionGraphicsItem*, QWidget*) override {}

        void showEvent(QShowEvent* event) override;
        void displayTileEditMenu(const QPoint& pos) override;

        void chartTitleChanged(const QString& title);

        QWidget* config() override { return configwidget_; }

        const QUuid& getEquipmentRef() const { return equipmentRef_; }
        AbstractEqItem* getAbsEqItem() const { return absEqItem_; }

        double tileDisplayHeight_;
        EquipmentOverviewItemConfig* configwidget_ = nullptr;

    protected slots:

        void popupAction(QAction*);

    protected:

        QFont eqbigfont, eqmidfont, eqsmallfont;
        QColor inactiveColor_, textColor_, alertColor_;

        // Hide constructors to create an Abstract class
        CommonEqItem(ChartSpace* parent, const QString& name);
        CommonEqItem(ChartSpace* parent, const QString& name, const QUuid& equipmentRef);

        QMap<int, QString> scrollableDisplayText_;
        int setupScrollableText(const QFontMetrics& fm, const QString& tileText, QMap<int, QString>& rowTextMap,
                                int rowOffset = 0, int protectOffset = -1);

        QUuid equipmentRef_; // somtimes set in derived class, constructor dependent
        AbstractEqItem* absEqItem_; // always set in derived class constructor
};

class EquipmentItem : public CommonEqItem
{
    Q_OBJECT

    public:

        // create using existing Uuid
        EquipmentItem(ChartSpace* parent, const QString& name, const QUuid& equipmentRef);

        // clone the equipment item, with new Uuid
        EquipmentItem(const EquipmentItem& toCopy);

        virtual ~EquipmentItem() {}

        void itemPaint(QPainter *painter, const QStyleOptionGraphicsItem *, QWidget *) override;
        void configChanged(qint32) override;
        void itemGeometryChanged() override;
        void setData(RideItem*) override;

        bool isWithin(const QDate& actDate) const;
        bool isWithin(const QStringList& rideEqLinkNameList, const QDate& actDate) const;
        bool rangeIsValid() const;
        bool allEqLinkNamesCompleterVals() const;

        // create and config using new Uuid
        static ChartSpaceItem* create(ChartSpace* parent) {
            return new EquipmentItem(parent, tr("Equipment Item"), QUuid::createUuid()); }
};

class EquipmentSummary : public CommonEqItem
{
    Q_OBJECT

    public:

        // create using existing Uuid
        EquipmentSummary(ChartSpace* parent, const QString& name, const QUuid& equipmentRef);

        // clone the equipment summary, with new Uuid
        EquipmentSummary(const EquipmentSummary& toCopy);

        virtual ~EquipmentSummary() {}

        void itemPaint(QPainter* painter, const QStyleOptionGraphicsItem*, QWidget*) override;

        // create and config using new Uuid
        static ChartSpaceItem* create(ChartSpace* parent) {
            return new EquipmentSummary(parent, tr("Summary Item"), QUuid::createUuid()); }
};

class EquipmentHistory : public CommonEqItem
{
    Q_OBJECT

    public:

        // create using existing Uuid
        EquipmentHistory(ChartSpace* parent, const QString& name, const QUuid& equipmentRef);

        // clone the equipment history, with new Uuid
        EquipmentHistory(const EquipmentHistory& toCopy);

        virtual ~EquipmentHistory() {}

        void itemPaint(QPainter* painter, const QStyleOptionGraphicsItem*, QWidget*) override;
        void itemGeometryChanged() override;
        void wheelEvent(QGraphicsSceneWheelEvent* event) override;

        // create and config using new Uuid
        static ChartSpaceItem* create(ChartSpace* parent) {
            return new EquipmentHistory(parent, tr("History Item"), QUuid::createUuid()); }

    private:

        int scrollPosn_;
        VScrollBar* scrollbar_;
};

class EquipmentNotes : public CommonEqItem
{
    Q_OBJECT

    public:

        // create using existing Uuid
        EquipmentNotes(ChartSpace* parent, const QString& name, const QUuid& equipmentRef);

        // clone the equipment notes, with new Uuid
        EquipmentNotes(const EquipmentNotes& toCopy);

        virtual ~EquipmentNotes() {}

        void itemPaint(QPainter* painter, const QStyleOptionGraphicsItem*, QWidget*) override;
        void itemGeometryChanged() override;
        void wheelEvent(QGraphicsSceneWheelEvent* event) override;

        // create and config using new Uuid
        static ChartSpaceItem* create(ChartSpace* parent) {
            return new EquipmentNotes(parent, tr("Notes Item"), QUuid::createUuid()); }

    private:

        int scrollPosn_;
        VScrollBar* scrollbar_;
};

#endif // _GC_EquipmentOverviewItem_h
