/*
 * Copyright (c) 2010 Mark Liversedge (liversedge@gmail.com)
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

#ifndef _GC_HomeWindow_h
#define _GC_HomeWindow_h 1
#include "GoldenCheetah.h"
#include "ChartSettings.h"
#include "GcWindowRegistry.h"
#include "GcWindowLayout.h"

#include <QtGui>
#include <QScrollArea>
#include <QFormLayout>
#include <QXmlDefaultHandler>
#include <QMessageBox>
#include <QLabel>
#include <QDialog>
#include <QTreeWidget>
#include <QTableWidget>
#include <QCheckBox>
#include <QStackedWidget>

#include "Context.h"
#include "RideItem.h"

class ChartBar;
class LTMSettings;
class AbstractView;
class ViewParser;
class PerspectiveDialog;
class QTextStream;
class DataFilter;
class SearchBox;
class TrendsView;
class UserChartWindow;

class Perspective : public GcWindow
{
    Q_OBJECT
    G_OBJECT

    friend ::AbstractView;
    friend ::TrendsView;
    friend ::ViewParser;
    friend ::PerspectiveDialog;

    public:

        ~Perspective();

        // am I relevant? (for switching when ride selected)
        virtual bool relevant(RideItem*) const { return true; }

        // the items I'd choose (for filtering on trends view, optionally refined by chart filter)
        virtual bool isFiltered() const override { return false; }
        QStringList filterlist(DateRange dr, bool isfiltered=false, QStringList files=QStringList());

        // get/set the expression (will compile df)
        const QString& expression() const { return expression_; }
        virtual bool setExpression(const QString& expr);

        // trainswitch
        enum switchenum { None=0, Erg=1, Slope=2, Video=3, Map=4 };
        int trainSwitch() const { return trainswitch; }
        void setTrainSwitch(int x) { trainswitch = (switchenum)x; }

        // import and export
        static Perspective *fromFile(ViewParser* handler, const QString& filename, int type);
        bool toFile(const QString& filename);
        void toXml(QTextStream &out);

        virtual int type() const = 0;
        const QString& title() const { return title_; }

        void resetLayout();
        void importChart(const QMap<QString,QString>& properties, bool select);

        void setStyle(int style) { styleChanged(style); }
        int currentStyle;

        int currentTab() { return currentStyle ? -1 : controlStack->currentIndex(); }
        GcChartWindow *currentChart() {
            return currentTab() >= 0 ? charts[currentTab()] : NULL;
        }

        const QList<GcChartWindow*>& getCharts() { return charts; }

    public slots:

        // GC signals
        void rideSelected();
        void dateRangeChanged(DateRange);
        void configChanged(qint32);
        void presetSelected(int n);

        // QT Widget events and signals
        void tabSelected(int id);
        void tabSelected(int id, bool forride);
        void tabMoved(int from, int to);
        void tabMenu(int index, int x);
        void dragEnterEvent(QDragEnterEvent *) override;
        void dropEvent(QDropEvent *) override;
        void resizeEvent(QResizeEvent *) override;
        void resize();
        void showEvent(QShowEvent *) override;
        bool eventFilter(QObject *object, QEvent *e) override;

        // My widget signals and events
        void styleChanged(int, bool force=false);
        void addChart(GcChartWindow* newone);
        void addChartFromMenu(QAction*action); // called with an action
        void appendChart(GcWinID id); // called from Context *to inset chart
        bool removeChart(int, bool confirm = true, bool keep = false);
        GcChartWindow *takeChart(GcChartWindow *wndow); // remove from view, but do not delete
        void titleChanged();

        // window wants to close...
        void closeWindow(GcWindow*);
        virtual void showControls();

        void userChartConfigChanged(UserChartWindow *);

        //notifiction that been made visible
        void selected();

        // moved or resized
        void windowMoving(GcWindow*);
        void windowResizing(GcWindow*);
        void windowMoved(GcWindow*);
        void windowResized(GcWindow*);

        // when moving tiles
        int pointTile(QPoint pos);
        void drawCursor();
        void rightClick(const QPoint &pos);

        // Realtime steering control of train window scrolling
        void steerScroll(int scrollAmount);

    protected:

        // Hide constructor to create an Abstract class
        Perspective(Context* context, const QString& title, const QString& view);

        virtual ViewParser* getViewParser(bool useDefault) const = 0;
        virtual QColor& getBackgroundColor() const;

        Context *context;

        bool active; // ignore gui signals when changing views
        bool resizing; // when resizing elements, don't double dip
        GcChartWindow *clicked; // keep track of selected charts
        bool dropPending;

        // what are we?
        const QString view_; // type of view:  "train", "analysis", "plan", "home"

        // top bar
        QString title_;
        QLineEdit *titleEdit;

        QComboBox *styleSelector;
        QStackedWidget *style; // tab, freeform, tiled
        QStackedWidget *controlStack; // window controls

        // each style has its own container widget
        ChartBar *chartbar;
        QStackedWidget *tabbed; 

        QScrollArea *tileArea;
        QWidget *tileWidget;
        QGridLayout *tileGrid;

        QScrollArea *winArea;
        QWidget *winWidget;
        GcWindowLayout *winFlow;

        // the charts!
        QList<GcChartWindow*> charts;
        int chartCursor;

        // expression
        DataFilter *df;
        QString expression_;

        // train view switching
        switchenum trainswitch;

        static void translateChartTitles(QList<GcChartWindow*> charts);
};

Q_DECLARE_METATYPE(Perspective*);

// setup the chart
class GcWindowDialog : public QDialog
{
    Q_OBJECT

    public:
        GcWindowDialog(GcWinID, Context *, GcChartWindow **, bool sidebar=false, LTMSettings *use=NULL);
        int exec();               // return pointer to window, or NULL if cancelled

    public slots:
        void okClicked();
        void cancelClicked();

    protected:
        Context *context;
        GcWinID type;
        GcChartWindow **here;
        bool sidebar;

        // we remove from the layout at the end
        QHBoxLayout *layout;
        QVBoxLayout *mainLayout;
        QVBoxLayout *chartLayout;
        QFormLayout *controlLayout;

        QPushButton *ok, *cancel;
        GcChartWindow *win;
        QLineEdit *title;
        QDoubleSpinBox *height, *width;
};

class ImportChartDialog : public QDialog
{
    Q_OBJECT

    public:
        ImportChartDialog(Context *context, const QList<QMap<QString,QString>>& list, QWidget *parent);

    protected:
        QTableWidget *table;
        QPushButton *import, *cancel;

    public slots:
        void importClicked();
        void cancelClicked();

    private:
        Context *context;
        QList<QMap<QString,QString> >list;

};

class AddPerspectiveDialog : public QDialog
{
    Q_OBJECT

    public:
        AddPerspectiveDialog(QWidget *parent, Context *context, QString &name, QString &expression, int type, Perspective::switchenum &trainswitch, bool edit=false);

    protected:
        QLineEdit *nameEdit;
        SearchBox *filterEdit;
        QPushButton *add, *cancel;
        QComboBox *trainSwitch;

    public slots:
        void addClicked();
        void cancelClicked();

    private:
        Context *context;
        QString &name;
        QString &expression;
        Perspective::switchenum &trainswitch;
        int type;
};
#endif // _GC_HomeWindow_h
