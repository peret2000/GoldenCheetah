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

#ifndef _GC_OverviewWindows_h
#define _GC_OverviewWindows_h 1

#include "Overview.h"
#include "ChartSpace.h"
#include "EquipmentOverviewItems.h"

namespace eqTextSizeType {
    enum eqTxtType { SMALL = 0, MEDIUM, LARGE };
};

class AnalysisOverviewWindow : public OverviewWindow {

    public:
        AnalysisOverviewWindow(Context* context, bool blank = false);

        void configItem(ChartSpaceItem* item, QPoint pos) override;

    protected:
        void tileAddedNotication(ChartSpaceItem* added) override { if (added->parent->currentRideItem) added->setData(added->parent->currentRideItem); }
        void importChartNotification(ChartSpaceItem* add) override { if (space->currentRideItem) add->setData(space->currentRideItem); }
        QString getChartSource() const override { return ":charts/overview-analysis.gchart"; }
        virtual GcWindowTypes::gcwinid getWindowType() const override { return GcWindowTypes::UserAnalysis; }
        AddTileWizard* getTileWizard(ChartSpaceItem* &added) const override { return new AddTileWizard(context, space, OverviewScope::ANALYSIS, added); }
};

class PlanOverviewWindow : public OverviewWindow {

    public:
        PlanOverviewWindow(Context* context, bool blank = false);

        void configItem(ChartSpaceItem* item, QPoint pos) override;

    protected:
        void tileAddedNotication(ChartSpaceItem* added) override { added->setDateRange(added->parent->currentDateRange); }
        void importChartNotification(ChartSpaceItem* add) override { add->setDateRange(space->currentDateRange); }
        QString getChartSource() const override { return ":charts/overview-plan.gchart"; }
        virtual GcWindowTypes::gcwinid getWindowType() const override { return GcWindowTypes::UserPlan; }
        AddTileWizard* getTileWizard(ChartSpaceItem* &added) const override { return new AddTileWizard(context, space, OverviewScope::PLAN, added); }
};

class TrendsOverviewWindow : public OverviewWindow {

    public:
        TrendsOverviewWindow(Context* context, bool blank = false);

        void configItem(ChartSpaceItem* item, QPoint pos) override;

    protected:
        void tileAddedNotication(ChartSpaceItem* added) override { added->setDateRange(added->parent->currentDateRange); }
        void importChartNotification(ChartSpaceItem* add) override { add->setDateRange(space->currentDateRange); }
        QString getChartSource() const override { return ":charts/overview-trends.gchart"; }
        virtual GcWindowTypes::gcwinid getWindowType() const override { return GcWindowTypes::UserTrends; }
        AddTileWizard* getTileWizard(ChartSpaceItem* &added) const override { return new AddTileWizard(context, space, OverviewScope::TRENDS, added); }
};

class EquipmentOverviewWindow : public OverviewWindow
{
    Q_OBJECT
    G_OBJECT

    // the display properties are used by the layout manager to save and restore
    // the parameters, this is so we can have multiple windows open at once with
    // different settings managed by the user.
    Q_PROPERTY(int textSize READ isTextSize WRITE setTextSize USER true)
    Q_PROPERTY(int showActivities READ isShowActivities WRITE setShowActivities USER true)
    Q_PROPERTY(int showTime READ isShowTime WRITE setShowTime USER true)
    Q_PROPERTY(int showElevation READ isShowElevation WRITE setShowElevation USER true)
    Q_PROPERTY(int showNotes READ isShowNotes WRITE setShowNotes USER true)

    public:

        EquipmentOverviewWindow(Context* context, bool blank = false);
        virtual ~EquipmentOverviewWindow();

        void showChart(bool visible) override;

        int isTextSize() const { return textSize->currentIndex(); }
        int isShowActivities() const { return showActivities->checkState(); }
        int isShowTime() const { return showTime->checkState(); }
        int isShowElevation() const { return showElevation->checkState(); }
        int isShowNotes() const { return showNotes->checkState(); }

    public slots:

        ChartSpaceItem* addTile() override;
        void configItem(ChartSpaceItem*, QPoint) override;
        void cloneTile(ChartSpaceItem*);

        void setTextSize(int value) { textSize->setCurrentIndex(value); }
        void setShowActivities(int value) { showActivities->setChecked(value); }
        void setShowTime(int value) { showTime->setChecked(value); }
        void setShowElevation(int value) { showElevation->setChecked(value); }
        void setShowNotes(int value) { showNotes->setChecked(value); }

        void titleChanged(QString title);
        void configChanged(qint32 cfg);

        void saveChart() override;
#ifdef GC_HAS_CLOUD_DB
        void exportChartToCloudDB() override;
#endif

    protected slots:

        void eqRecalculationComplete();

    protected:

        QString getChartSource() const override { return ":charts/overview-equipment.gchart"; }
        virtual GcWindowTypes::gcwinid getWindowType() const override { return GcWindowTypes::EquipmentOverview; }
        AddTileWizard* getTileWizard(ChartSpaceItem* &added) const override { return new AddTileWizard(context, space, OverviewScope::EQUIPMENT, added); }

        void getTileConfig(ChartSpaceItem* item, QString& config) const override;
        void setTileConfig(const QJsonObject& obj, int type, const QString& name,
                           const QString& datafilter, int order, int column,
                           int span, int deep, ChartSpaceItem* add) const override;

    private:
        bool eqWindowVisible_;
        bool chartExportInProgress_;
        QComboBox* textSize;
        QCheckBox* showActivities, *showTime, *showElevation;
        QCheckBox* showNotes;
};

class AnalysisOverviewConfigDialog : public OverviewConfigDialog {

    public:
        AnalysisOverviewConfigDialog(ChartSpaceItem* item, QPoint pos);

    protected:
        void updateItemNotification() override { if (item->parent->currentRideItem) item->setData(item->parent->currentRideItem); }

        QString getViewForExport() const override { return "analysis"; }
        int getTypeForExport() const override { return GcWindowTypes::UserAnalysis; }
};

class PlanOverviewConfigDialog : public OverviewConfigDialog {

    public:
        PlanOverviewConfigDialog(ChartSpaceItem* item, QPoint pos);

    protected:
        void updateItemNotification() override { item->setDateRange(item->parent->currentDateRange); }

        QString getViewForExport() const override { return "plan"; }
        int getTypeForExport() const override { return GcWindowTypes::UserPlan; }
};

class TrendsOverviewConfigDialog : public OverviewConfigDialog {

    public:
        TrendsOverviewConfigDialog(ChartSpaceItem* item, QPoint pos);

    protected:
        void updateItemNotification() override { item->setDateRange(item->parent->currentDateRange); }

        QString getViewForExport() const override { return "home"; }
        int getTypeForExport() const override { return GcWindowTypes::UserTrends; }
};

class EquipmentOverviewConfigDialog : public OverviewConfigDialog
{

    public:
        EquipmentOverviewConfigDialog(ChartSpaceItem* item, QPoint pos);

        void removeItem() override;

    protected:
        QString getViewForExport() const override { return "equipment"; }
        int getTypeForExport() const override { return GcWindowTypes::EquipmentOverview; }
};

#endif // _GC_OverviewWindows_h
