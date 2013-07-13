#include "spacespin.h"

SpaceSpin::SpaceSpin(QWidget *parent) : QDoubleSpinBox(parent)
{
    this->parent = parent;
}

void SpaceSpin::keyPressEvent(QKeyEvent *event)
{
    if(event->key() == Qt::Key_Space)
    {
        event->ignore();
        qApp->sendEvent(parent, event);
    }
    else
        QDoubleSpinBox::keyPressEvent(event);
}
