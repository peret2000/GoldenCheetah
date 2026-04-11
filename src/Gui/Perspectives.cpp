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

#include "Perspectives.h"
#include "MainWindow.h"
#include "Colors.h"
#include "Views.h" // for EquipmentView
#include "DataFilter.h" // for Result

AnalysisPerspective::AnalysisPerspective(Context* context, const QString& title)
    : Perspective(context, title, "analysis")
{
}

PlanPerspective::PlanPerspective(Context* context, const QString& title)
    : Perspective(context, title, "plan")
{
    // plan view we should select a library chart when a chart is selected.
    connect(context, &Context::presetSelected, this, &PlanPerspective::presetSelected);
}

TrendsPerspective::TrendsPerspective(Context* context, const QString& title)
    : Perspective(context, title, "home")
{
    // trends view we should select a library chart when a chart is selected.
    connect(context, &Context::presetSelected, this, &TrendsPerspective::presetSelected);
}

bool
TrendsPerspective::setExpression(const QString& expr)
{
    if (Perspective::setExpression(expr)) {
        // notify charts that the filter changed
        // but only for trends views where it matters
        foreach(GcWindow * chart, charts)
            chart->notifyPerspectiveFilterChanged(expression_);
        return true;
    }
    return false;
}

TrainPerspective::TrainPerspective(Context* context, const QString& title)
    : Perspective(context, title, "train")
{
    // Allow realtime controllers to scroll train view with steering movements
    connect(context, &Context::steerScroll, this, &TrainPerspective::steerScroll);
}

QColor&
TrainPerspective::getBackgroundColor() const
{
    static QColor col = GColor(CTRAINPLOTBACKGROUND);
    return col;
}

EquipmentPerspective::EquipmentPerspective(Context* context, const QString& title)
    : Perspective(context, title, "equipment")
{
}

void
EquipmentPerspective::showControls()
{
    context->mainWindow->equipView()->chartsettings->adjustSize();
    context->mainWindow->equipView()->chartsettings->show();
}

ViewParser*
AnalysisPerspective::getViewParser(bool useDefault) const
{
    return new AnalysisViewParser(context, useDefault);
}

bool
AnalysisPerspective::relevant(RideItem* item) const
{
    if ((df == NULL) || (item == NULL)) return false;

    // validate
    Result ret = df->evaluate(item, NULL);
    return ret.number();
}

ViewParser*
PlanPerspective::getViewParser(bool useDefault) const
{
    return new PlanViewParser(context, useDefault);
}

ViewParser*
TrendsPerspective::getViewParser(bool useDefault) const
{
    return new TrendsViewParser(context, useDefault);
}

ViewParser*
TrainPerspective::getViewParser(bool useDefault) const
{
    return new TrainViewParser(context, useDefault);
}

ViewParser*
EquipmentPerspective::getViewParser(bool useDefault) const
{
    return new EquipmentViewParser(context, useDefault);
}
