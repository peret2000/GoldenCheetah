/*
 * Copyright (c) 2013 Mark Liversedge (liversedge@gmail.com)
<<<<<<< HEAD
 * LTMSidebarView Copyright (c) 2025 Paul Johnson (paulj49457@gmail.com)
=======
 * EquipmentView Copyright (c) 2025 Paul Johnson (paulj49457@gmail.com)
>>>>>>> 93139adf7 (XML Equipment Management Feature)
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

#ifndef _GC_Views_h
#define _GC_Views_h 1

#include "AbstractView.h"

class TrainSidebar;
class AnalysisSidebar;
class LTMSidebar;
class IntervalSidebar;
class QDialog;
class RideNavigator;
class TrainBottom;

// the LTMSidebarView class manages the sharing of the Long Term Metrics (LTM) sidebar
// between the trends and plan views, each LTMSidebar instance is shared between the
// views for the same context/athlete.
class LTMSidebarView : public AbstractView
{
    Q_OBJECT

    public:

        static void selectDateRange(Context *sbContext, DateRange dr);

    signals:
        void dateChanged(DateRange);

    protected slots:

        void justSelected();
        void dateRangeChanged(DateRange);

    protected:

        LTMSidebarView(Context *context, int type, const QString& view, const QString& heading);
        virtual ~LTMSidebarView();

        void showEvent(QShowEvent*) override;
        void hideEvent(QHideEvent*) override;

        LTMSidebar* getLTMSidebar(Context *sbContext);
        void removeLTMSidebar(Context *sbContext);

    private:

        int type_; // view type

        // each athlete has their own LTMSidebar shared by the plan & trends views.
        static QMap<Context*, LTMSidebar*> LTMSidebars_;
};

class AnalysisViewParser : public ViewParser {

    public:
        AnalysisViewParser(Context* context, bool useDefault) : ViewParser(context, useDefault) {}

    protected:
        Perspective* getViewParsersPerspective(const QString& name) const override;
};

class AnalysisView : public AbstractView
{
    Q_OBJECT

    public:

        AnalysisView(Context *context, QStackedWidget *controls);
        virtual ~AnalysisView();
        void close() override;
        void setRide(RideItem*ride) override;
        void addIntervals();

        int viewType() const override { return VIEW_ANALYSIS; }

        RideNavigator *rideNavigator();
        AnalysisSidebar *analSidebar;

    public slots:

        bool isBlank() override;
        void compareChanged(bool);

    protected:
        Perspective* getViewsPerspective(const QString& name) const override;
        ViewParser* getViewParser(Context* context, bool useDefault) const override;

        void notifyViewSidebarChanged() override;
        int getViewSpecificPerspective() override;
        void notifyViewSplitterMoved() override;

    private:

        int findRidesPerspective(RideItem* ride);
};

class PlanViewParser : public ViewParser {

    public:
        PlanViewParser(Context* context, bool useDefault) : ViewParser(context, useDefault) {}

    protected:
        Perspective* getViewParsersPerspective(const QString& name) const override;
};

class PlanView : public LTMSidebarView
{
    Q_OBJECT

    public:

        PlanView(Context *context, QStackedWidget *controls);
        virtual ~PlanView();

        int viewType() const override { return VIEW_PLAN; }

    public slots:

        bool isBlank() override;

    protected:

        Perspective* getViewsPerspective(const QString& name) const override;
        ViewParser* getViewParser(Context* context, bool useDefault) const override;
};

class TrainViewParser : public ViewParser {

    public:
        TrainViewParser(Context* context, bool useDefault) : ViewParser(context, useDefault) {}

    protected:
        Perspective* getViewParsersPerspective(const QString& name) const override;
};

class TrainView : public AbstractView
{
    Q_OBJECT

    public:

        TrainView(Context *context, QStackedWidget *controls);
        virtual ~TrainView();
        void close() override;

        int viewType() const override { return VIEW_TRAIN; }

    public slots:

        bool isBlank() override;
        void onSelectionChanged();

    protected:

        void notifyViewPerspectiveAdded(Perspective* page) override;
        Perspective* getViewsPerspective(const QString& name) const override;
        ViewParser* getViewParser(Context* context, bool useDefault) const override;

    private:

        TrainSidebar *trainTool;
        TrainBottom *trainBottom;

    private slots:

        void onAutoHideChanged(bool enabled);
};

class TrendsViewParser : public ViewParser {

    public:
        TrendsViewParser(Context* context, bool useDefault) : ViewParser(context, useDefault) {}

    protected:
        Perspective* getViewParsersPerspective(const QString& name) const override;
};

class TrendsView : public LTMSidebarView
{
    Q_OBJECT

    public:

        TrendsView(Context *context, QStackedWidget *controls);
        virtual ~TrendsView();

        int countActivities(Perspective *, DateRange dr);
        int viewType() const override { return VIEW_TRENDS; }

    public slots:

        bool isBlank() override;
        void compareChanged(bool);

    protected:

        Perspective* getViewsPerspective(const QString& name) const override;
        ViewParser* getViewParser(Context* context, bool useDefault) const override;

};

class EquipmentViewParser : public ViewParser {

    public:
        EquipmentViewParser(Context* context, bool useDefault) : ViewParser(context, useDefault) {}

    protected:
        Perspective* getViewParsersPerspective(const QString& name) const override;
};

class EquipmentView : public AbstractView
{  
    Q_OBJECT

    public:

        EquipmentView(Context *context, QStackedWidget *controls);
        ~EquipmentView();

        int viewType() const override { return VIEW_EQUIPMENT; }

        // Don't want the base class behaviour for this...
        virtual void setRide(RideItem*) override {}

        // Need to modify the behaviour
        virtual void selectionChanged() override;

        ChartSettings* chartsettings;

    public slots:

        bool isBlank() override;
        void addChart(GcWinID id) override;

    protected:

        Perspective* getViewsPerspective(const QString& name) const override;
        ViewParser* getViewParser(Context* context, bool useDefault) const override;

    private:
};

#endif // _GC_Views_h
