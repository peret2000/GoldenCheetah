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

#ifndef _GC_Perspectives_h
#define _GC_Perspectives_h 1

#include "Perspective.h"

class AnalysisPerspective : public Perspective {

    public:
        AnalysisPerspective(Context* context, const QString& title);

        // am I relevant? (for switching when ride selected)
        bool relevant(RideItem*) const override;

        int type() const override { return VIEW_ANALYSIS; }

    protected:
        ViewParser* getViewParser(bool useDefault) const override;
};

class PlanPerspective : public Perspective {

    public:
        PlanPerspective(Context* context, const QString& title);

        int type() const override { return VIEW_PLAN; }

    protected:
        ViewParser* getViewParser(bool useDefault) const override;
};

class TrendsPerspective : public Perspective {

    public:
        TrendsPerspective(Context* context, const QString& title);

        int type() const override { return VIEW_TRENDS; }

        // the items I'd choose (for filtering on trends view, optionally refined by chart filter)
        bool isFiltered() const override { return (df != NULL); }

        bool setExpression(const QString& expr) override;

    protected:
        ViewParser* getViewParser(bool useDefault) const override;
};

class TrainPerspective : public Perspective {

    public:
        TrainPerspective(Context* context, const QString& title);

        int type() const override { return VIEW_TRAIN; }

    protected:
        ViewParser* getViewParser(bool useDefault) const override;

        QColor& getBackgroundColor() const override;
};

class EquipmentPerspective : public Perspective {

    public:
        EquipmentPerspective(Context* context, const QString& title);

        int type() const override { return VIEW_EQUIPMENT; }

        void showControls() override;

    protected:
        ViewParser* getViewParser(bool useDefault) const override;
};


#endif // _GC_Perspectives_h
