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

#include "ScalingLabel.h"

#include <QDebug>


ScalingLabel::ScalingLabel
(QWidget *parent, Qt::WindowFlags f)
: ScalingLabel(4, 64, parent, f)
{
}

ScalingLabel::ScalingLabel
(int rescaleEvery, QWidget *parent, Qt::WindowFlags f)
: ScalingLabel(4, 64, parent, f)
{
    ScalingLabel(parent, f);
    counterLimit = rescaleEvery;
}

ScalingLabel::ScalingLabel
(int minFontPointSize, int maxFontPointSize, QWidget *parent, Qt::WindowFlags f)
    : QLabel(parent, f), minFontPointSize(minFontPointSize), maxFontPointSize(maxFontPointSize),
    counterLimit(10)
{
    QFont fnt = font();
    fnt.setPointSize(minFontPointSize);
    QLabel::setFont(fnt);
}


ScalingLabel::~ScalingLabel
()
{
}


void
ScalingLabel::resizeEvent
(QResizeEvent *evt)
{
    Q_UNUSED(evt)
    scaleFont(text(), ScalingLabelReason::ResizeEvent);
}


void
ScalingLabel::setText
(const QString &text)
{
    ++counter;
    if (text.length() > QLabel::text().length()) {
        scaleFont(text, ScalingLabelReason::TextLengthChanged);
    } else if (counter >= counterLimit) {
        scaleFont(text, ScalingLabelReason::CounterExceeded);
    }
    QLabel::setText(text);
}


void
ScalingLabel::setFont
(const QFont &font)
{
    if (! scaleFont(text(), font, ScalingLabelReason::FontChanged)) {
        QLabel::setFont(font);
    }
}


int
ScalingLabel::getMinFontPointSize
() const
{
    return minFontPointSize;
}


void
ScalingLabel::setMinFontPointSize
(int size)
{
    minFontPointSize = size;
}


int
ScalingLabel::getMaxFontPointSize
() const
{
    return maxFontPointSize;
}


void
ScalingLabel::setMaxFontPointSize
(int size)
{
    maxFontPointSize = size;
}


ScalingLabelStrategy
ScalingLabel::getStrategy
() const
{
    return strategy;
}


void
ScalingLabel::setStrategy
(ScalingLabelStrategy strategy)
{
    if (this->strategy != strategy) {
        this->strategy = strategy;
        scaleFont(text(), ScalingLabelReason::StrategyChanged);
    }
}


bool
ScalingLabel::scaleFont
(const QString &text, ScalingLabelReason reason)
{
    return scaleFont(text, font(), reason);
}


bool
ScalingLabel::scaleFont
(const QString &text, const QFont &font, ScalingLabelReason reason)
{
    counter = 0;
    switch (strategy) {
    case ScalingLabelStrategy::Linear:
        return scaleFontLinear(text, font, reason);
    case ScalingLabelStrategy::Exact:
        return scaleFontExact(text, font, reason);
    case ScalingLabelStrategy::HeightOnly:
    default:
        return scaleFontHeightOnly(text, font, reason);
    }
}


bool
ScalingLabel::scaleFontHeightOnly
(const QString &text, const QFont &font, ScalingLabelReason reason)
{
    Q_UNUSED(text)
    if (   reason != ScalingLabelReason::ResizeEvent
        && reason != ScalingLabelReason::StrategyChanged) {
        return false;
    }

    QFont f(font);
    // set point size within reasonable limits for low dpi screens
    int size = (geometry().height() - 24) * 72 / logicalDpiY();
    size = std::min(maxFontPointSize, std::max(minFontPointSize, size));
    f.setPointSize(size);
    setFont(f);

    return true;
}


bool
ScalingLabel::scaleFontExact
(const QString &text, const QFont &font, ScalingLabelReason reason)
{
    int size = maxFontPointSize + 1;
    if (reason == ScalingLabelReason::CounterExceeded) {
        size = font.pointSize() + 1;
    }
    QFont f(font);
    QRect br;
    do {
        f.setPointSize(--size);
        QFontMetrics fm = QFontMetrics(f, this);
        br = fm.boundingRect(text);
    } while (size >= minFontPointSize && (br.width() > width() || br.height() > height()));
    QLabel::setFont(f);
    return true;
}


bool
ScalingLabel::scaleFontLinear
(const QString &text, const QFont &font, ScalingLabelReason reason)
{
    if (text.isEmpty() || width() <= 0 || height() <= 0) {
        return false;
    }

    // Define margins
    const int horizontalMargin = 6;
    const int verticalMargin = 4;

    // Available space accounting for margins
    int availableWidth = width() - 2 * horizontalMargin;
    int availableHeight = height() - 2 * verticalMargin;

    int maxSize = (reason == ScalingLabelReason::CounterExceeded) ? font.pointSize() : maxFontPointSize;

    // Split text into lines
    QStringList lines = text.split('\n');

    // Binary search for optimal font size
    int low = minFontPointSize;
    int high = maxSize;
    int bestSize = minFontPointSize;

    QFont f(font);

    while (low <= high) {
        int mid = (low + high) / 2;
        f.setPointSize(mid);
        QFontMetrics fm(f, this);

        // Calculate height needed for all lines
        int totalHeight = fm.lineSpacing() * lines.size();

        // Find the widest line
        int maxWidth = 0;
        for (const QString &line : lines) {
            int lineWidth = fm.horizontalAdvance(line);
            maxWidth = qMax(maxWidth, lineWidth);
        }

        if (maxWidth <= availableWidth && totalHeight <= availableHeight) {
            bestSize = mid;  // This size works, try larger
            low = mid + 1;
        } else {
            high = mid - 1;  // Too large, try smaller
        }
    }

    f.setPointSize(bestSize);
    QLabel::setFont(f);
    return true;
}

// bool
// ScalingLabel::scaleFontLinear
// (const QString &text, const QFont &font, ScalingLabelReason reason)
// {
//     if (text.length() == 0 || width() <= 0 || height() <= 0) {
//         return false;
//     }
//     int maxSize = maxFontPointSize;
//     if (reason == ScalingLabelReason::CounterExceeded) {
//         maxSize = font.pointSize();
//     }
//     QFont f(font);
//     f.setPointSize(minFontPointSize);
//     QFontMetrics fmS = QFontMetrics(f, this);
//     f.setPointSize(maxSize);
//     QFontMetrics fmL = QFontMetrics(f, this);
//     QRect brS = fmS.boundingRect(text);
//     QRect brL = fmL.boundingRect(text);
//     int sizeWidth = (maxSize - minFontPointSize) / float(brL.width() - brS.width()) * width();
//     int sizeHeight = (maxSize - minFontPointSize) / float(brL.height() - brS.height()) * height();
//     int calcSize = std::min<int>(sizeWidth, sizeHeight);
//     f.setPointSize(std::max<int>(std::min<int>(calcSize, maxSize), minFontPointSize));
//     QLabel::setFont(f);
//     return true;
// }
