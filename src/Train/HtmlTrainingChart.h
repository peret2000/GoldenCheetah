#ifndef _GC_HtmlTrainingChart_h
#define _GC_HtmlTrainingChart_h

#include "HtmlChart.h"

class HtmlTrainingChart : public HtmlChart
{
    Q_OBJECT

public:
    HtmlTrainingChart(Context *context);
    ~HtmlTrainingChart();

protected:
    void setupBridges(QWebChannel *channel) override;
    QString defaultHtml() const override;
    int backgroundColorIndex() const override;
};

#endif
