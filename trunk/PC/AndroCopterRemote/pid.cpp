#include "pid.h"

Pid::Pid(double minSaturation, double maxSaturation, double aPriori,
         bool antiResetWindup)
{
    kp = 0.0;
    ki = 0.0;
    kd = 0.0;
    integrator = 0.0;
    prevDiff = 0.0;
    this->minSaturation = minSaturation;
    this->maxSaturation = maxSaturation;
    this->aPriori = aPriori;
    this->antiResetWindup = antiResetWindup;
}

double Pid::computeCommand(double current, double target, double dt)
{
    double difference = target - current;

    double command = 0.0;

    // A-priori part.
    command += aPriori;

    // Proportional part.
    command += difference * kp;

    // Integral part.
    integrator += difference * dt * ki;
    command += integrator;

    // Derivative part.
    double derivative = (difference - prevDiff) / dt;
    prevDiff = difference;
    command += derivative * kd;

    // Saturation.
    if(command < minSaturation)
    {
        command = minSaturation;

        if(antiResetWindup)
            integrator -= difference * dt * ki; // Freeze the integrator.
    }
    else if(command > maxSaturation)
    {
        command = maxSaturation;

        if(antiResetWindup)
            integrator -= difference * dt * ki; // Freeze the integrator.
    }

    return command;
}

void Pid::setCoefficients(double kp, double ki, double kd)
{
    this->kp = kp;
    this->ki = ki;
    this->kd = kd;
}

void Pid::reset()
{
    integrator = 0.0;
    prevDiff = 0.0;
}
