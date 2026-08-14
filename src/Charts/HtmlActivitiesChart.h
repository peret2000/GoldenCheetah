#ifndef _GC_HtmlActivitiesChart_h
#define _GC_HtmlActivitiesChart_h

#include "HtmlChart.h"

class HtmlActivitiesChart : public HtmlChart
{
    Q_OBJECT

public:
    HtmlActivitiesChart(Context *context);
    ~HtmlActivitiesChart();

protected:
    void setupBridges(QWebChannel *channel) override;
    QString defaultHtml() const override;
    int backgroundColorIndex() const override;
};

#endif
