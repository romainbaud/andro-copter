/*!
* \file spacespin.h
* \brief Qt QDoubleSpinBox, which does not handles the spacebar presses, to
* allow the parent widget to get them.
* \author Romain Baud
* \version 0.1
* \date 2013.02.01
*/

#ifndef SPACESPIN_H
#define SPACESPIN_H

#include <QDoubleSpinBox>
#include <QKeyEvent>
#include <QDebug>
#include <QApplication>

/// This class is a modified QDoubleSpinBox, with the only difference that it
/// does not handle the spacebar event. This prevents that a spacebar press
/// is captured by the spinbox (when it has the focus), and does not propagate
/// to the main widget, to trigger the emergency stop.
class SpaceSpin : public QDoubleSpinBox
{
    Q_OBJECT
public:
    /// Default constructor.
    /// \param parent parent of this widget.
    explicit SpaceSpin(QWidget *parent = 0);

protected:
    /// Reimplemented method, to handle all usual keys, except the spacebar.
    /// The spacebar presses will be sent to the parent widget, while the other
    /// keypresses will be sent to the base class (QDoubleSpinBox).
    void keyPressEvent(QKeyEvent *event);

private:
    QWidget *parent;
};

#endif // SPACESPIN_H
