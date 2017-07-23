#ifndef PLOTWINDOW_H
#define PLOTWINDOW_H

#include <QDialog>
#include "qcustomplot.h"
#include "ui_plotwindow.h"

class PlotWindow : public QDialog, public Ui::PlotWindow
{
    Q_OBJECT
public:
    explicit PlotWindow(QWidget *parent = nullptr);
    ~PlotWindow();
    void addPoint(double x,double y);
    void clearData();
    void plot();

signals:

public slots:

private:
    Ui::PlotWindow *ui;

    QVector <double> qv_x,qv_y;

    double max();

    double absolute_double(const double &val) {return val>0?val:-val;}

    double max_range;
};

#endif // PLOTWINDOW_H
