/*
 * Copyright (c) 2022 Joachim Kohlhammer (joachim.kohlhammer@gmx.de)
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

#ifndef _GC_ScalingLabel_h
#define _GC_ScalingLabel_h 1

#include <QLabel>

enum class ScalingLabelReason : uint8_t {
    TextLengthChanged,
    FontChanged,
    ResizeEvent,
    CounterExceeded,
    StrategyChanged
};

enum class ScalingLabelStrategy : uint8_t {
    Exact,
    Linear,
    HeightOnly
};


class ScalingLabel : public QLabel
{
    Q_OBJECT

    public:
        ScalingLabel(QWidget *parent = nullptr, Qt::WindowFlags f = Qt::WindowFlags());
        ScalingLabel(int minFontPointSize, int maxFontPointSize, QWidget *parent = nullptr, Qt::WindowFlags f = Qt::WindowFlags());
        virtual ~ScalingLabel();

        virtual void resizeEvent(QResizeEvent *evt);
        virtual void setFont(const QFont &font);

        int getMinFontPointSize() const;
        int getMaxFontPointSize() const;
        ScalingLabelStrategy getStrategy() const;

    public slots:
        void setText(const QString &text);
        void setMinFontPointSize(int size);
        void setMaxFontPointSize(int size);
        void setStrategy(ScalingLabelStrategy strategy);

    private:
        bool scaleFont(const QString &text, const QFont &font, ScalingLabelReason reason);
        bool scaleFont(const QString &text, ScalingLabelReason reason);
        bool scaleFontExact(const QString &text, const QFont &font, ScalingLabelReason reason);
        bool scaleFontLinear(const QString &text, const QFont &font, ScalingLabelReason reason);
        bool scaleFontHeightOnly(const QString &text, const QFont &font, ScalingLabelReason reason);

        int minFontPointSize;
        int maxFontPointSize;
        ScalingLabelStrategy strategy = ScalingLabelStrategy::Linear;
        int counter = 0;
};

#endif
