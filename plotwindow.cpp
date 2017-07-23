#include "plotwindow.h"

PlotWindow::PlotWindow(QWidget *parent) : QDialog(parent),ui(new Ui::PlotWindow)
{
    ui->setupUi(this);
    ui->plot->addGraph();
    ui->plot->graph(0)->setScatterStyle(QCPScatterStyle::ssCircle);
    ui->plot->graph(0)->setLineStyle(QCPGraph::lsLine);
}

PlotWindow::~PlotWindow()
{
    delete ui;
}

void PlotWindow::addPoint(double x, double y)
{
    qv_x.append(x);
    qv_y.append(y);
    if(!ui->plot->yAxis->range().contains(y))
    {
        max_range = y;
        ui->plot->yAxis->setRange(-y,y);
    }
    if(!ui->plot->xAxis->range().contains(x))
    {
        ui->plot->xAxis->setRange(x-2000,x);

    }
    if(qv_x.length() > 40)
    {
        qv_x.pop_front();
        if(absolute_double(qv_y[0]) == max_range)
        {
            double new_range = max();
            max_range = new_range;
            ui->plot->yAxis->setRange(-new_range,new_range);
        }
        qv_y.pop_front();
    }
}

void PlotWindow::clearData()
{
    qv_x.clear();
    qv_y.clear();

}

void PlotWindow::plot()
{
    ui->plot->graph(0)->setData(qv_x,qv_y);
    ui->plot->replot();
    ui->plot->update();
}

double PlotWindow::max()
{
    double res = 0;
    for(int i = 1;i<qv_y.length();i++)
    {
        if(res < absolute_double(qv_y[i]))
        {
            res = absolute_double(qv_y[i]);
        }
    }

    return res;
}
