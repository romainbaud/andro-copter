/*!
* \file pid.h
* \brief Simple class for a parallel PID controller.
* \author Romain Baud
* \version 0.1
* \date 2012.12.29
*/

#ifndef PID_H
#define PID_H

/// Simple parallel PID class.
class Pid
{
public:
	/// Constructor.
	/// \param minSaturation the minimal value the PID can output. If the
	/// linear equations of the PID give a lower command, it will be saturated
	/// to the given value.
	/// Enter minus a very large number if you do not want to be limited.
	/// \param maxSaturation the maximal value the PID can output. If the
	/// linear equations of the PID give a higher command, it will be saturated
	/// to the given value.
	/// Enter a very large number if you do not want to be limited.
	/// \param aPriori the a-priori value which will be added to the linear PID
	/// command.
	/// \param antiResetWindup true to enable the anti-reset windup feature,
	/// false to deactivate it. The anti-reset windup freezes the integrator if
	/// the output command is equal to the saturation value.
    Pid(double minSaturation, double maxSaturation, double aPriori,
        bool antiResetWindup);
		
	/// Computes the next command from the given current states.
	/// \param current the current state.
	/// \param target the current target.
	/// \param dt the timestep of the last time.
	/// \return the computed command.
    double computeCommand(double current, double target, double dt);
	
	/// Set the PID coefficents.
	/// \param kp the Kp coefficent.
	/// \param ki the Ki coefficent.
	/// \param kd the Kd coefficent.
    void setCoefficients(double kp, double ki, double kd);
	
	/// Reset the integrator and the derivator's last stored value.
    void reset();

private:
    double kp, ki, kd, integrator, prevDiff, minSaturation, maxSaturation,
           aPriori, antiResetWindup;
};

#endif // PID_H
