#include "plotter.h"

Plotter::Plotter(QWidget *parent) : QGraphicsView(parent)
{
    axisItem = 0;

    setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

    setScene(&scene);

    // Just to avoid doing the NULL test after.
    axisItem = scene.addLine(0.0, 0.0, 0.0, 0.0);
    currentAngleItem = scene.addLine(0.0, 0.0, 0.0, 0.0);
    targetAngleItem = scene.addLine(0.0, 0.0, 0.0, 0.0);
    commandItem = scene.addLine(0.0, 0.0, 0.0, 0.0);
}

void Plotter::setup(int nPointsPerCurveMax, double angleAmpl, double commandAmpl)
{
    this->nStepsMax = nPointsPerCurveMax;
    this->angleAmplitude = angleAmpl;
    this->commandAmplitude = commandAmpl;

    clearGraph();
}

void Plotter::nextStep(int time, double currentAngle, double targetAngle, double command)
{
    steps.append(Step(time, currentAngle, targetAngle, command));

    if(steps.size() > nStepsMax)
        steps.removeFirst();

    drawAll();
}

void Plotter::drawAll()
{
    if(steps.size() > 0)
    {
        double h = (double)height();
        double w = (double)width();

        scene.setSceneRect(0.0, 0.0, w, h);

        // Draw the time axis.
        scene.removeItem(axisItem);

        axisItem = scene.addLine(0.0, h/2.0, w, h/2.0);

        // Draw the curves.
        int firstTime = steps.first().time;
        int lastTime = steps.last().time;
        double timeSpan = (double)(lastTime-firstTime);

        scene.removeItem(currentAngleItem);
        scene.removeItem(targetAngleItem);
        scene.removeItem(commandItem);

        QPainterPath currentAnglePath, targetAnglePath, commandPath;

        currentAnglePath.moveTo(0.0, (-steps.first().currentAngle/angleAmplitude+1.0)*h/2.0);
        targetAnglePath.moveTo(0.0, (-steps.first().targetAngle/angleAmplitude+1.0)*h/2.0);
        commandPath.moveTo(0.0, (-steps.first().command/commandAmplitude+1.0)*h/2.0);

        for(QLinkedList<Step>::iterator i = ++steps.begin(); i != steps.end(); ++i)
        {
            double xPos = ((double)(i->time-firstTime)) / timeSpan * w;
            currentAnglePath.lineTo(xPos, (-i->currentAngle/angleAmplitude+1.0)*h/2.0);
            targetAnglePath.lineTo(xPos, (-i->targetAngle/angleAmplitude+1.0)*h/2.0);
            commandPath.lineTo(xPos, (-i->command/commandAmplitude+1.0)*h/2.0);
        }

        currentAngleItem = scene.addPath(currentAnglePath, QPen(Qt::blue));
        targetAngleItem = scene.addPath(targetAnglePath, QPen(Qt::green));
        commandItem = scene.addPath(commandPath, QPen(Qt::black));
    }
}

void Plotter::resizeEvent(QResizeEvent *event)
{
    Q_UNUSED(event);

    drawAll();
}

void Plotter::clearGraph()
{
    steps.clear();
}
