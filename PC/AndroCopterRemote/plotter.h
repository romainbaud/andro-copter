/*!
* \file plotter.h
* \brief Qt widget for a live chart.
* \author Romain Baud
* \version 0.1
* \date 2012.12.29
*/

#ifndef PLOTTER_H
#define PLOTTER_H

#include <QGraphicsView>
#include <QGraphicsItem>
#include <QList>
#include <QLinkedList>
#include <QPen>

/// Internal class used by the Plotter class to represent a point on the
/// chart. This chart displays the nStepsMax last given points.
/// It is designed for displaying data of PID controllers, current value,
/// target and command.
class Step
{
public:
    Step(int time, double currentAngle, double targetAngle, double command)
    {
        this->time = time;
        this->currentAngle = currentAngle;
        this->targetAngle = targetAngle;
        this->command = command;
    }

    int time;
    double currentAngle, targetAngle, command;
};

/// Qt widget for a live chart.
class Plotter : public QGraphicsView
{
    Q_OBJECT
public:
	/// Constructor.
	/// \param parent parent widget.
    explicit Plotter(QWidget *parent = 0);

	/// Setup the chart. It is necessary to call this function before trying to
	/// call the nextStep() method.
	/// \param nStepsMax max number of timesteps displayed.
	/// \param angleAmp- determines the -ylim and ylim of the chart, for the
	/// angle.
	/// \param commandAmpl determines the -ylim and ylim of the chart, for the
	/// commande.
    void setup(int nStepsMax, double angleAmpl, double commandAmpl);
	
	/// Add a new point to the graph.
	/// \param time time of the point.
	/// \param currentAngle the current angle given to the PID.
	/// \param targetAngle the target angle given to the PID.
	/// \param command the command computed by the PID.
    void nextStep(int time, double currentAngle, double targetAngle, double command);
	
	/// Removes all the points from the chart.
    void clearGraph();

	/// Method called by Qt when the widget is resized.
    void resizeEvent(QResizeEvent * event);

private:
	/// Draws the lines on the chart.
    void drawAll();

    QLinkedList<Step> steps;
    int nStepsMax;
    QGraphicsScene scene;
    QGraphicsItem *axisItem, *currentAngleItem, *targetAngleItem, *commandItem;
    double angleAmplitude, commandAmplitude;
};

#endif // PLOTTER_H
