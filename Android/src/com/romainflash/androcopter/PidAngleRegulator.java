package com.romainflash.androcopter;

// This PID regulator is for angles (in degrees) only, because it handles the
// transition "+180°, -180°".
public class PidAngleRegulator
{
	public PidAngleRegulator(float kp, float ki, float kd, float smoothingStrength)
	{
		this.kp = kp;
		this.ki = ki;
		this.kd = kd;
		this.smoothingStrength = smoothingStrength;
		previousDifference = 0.0f;
		
		integrator = 0.0f;
		differencesMean = 0.0f;
	}
	
	public float getInput(float targetAngle, float currentAngle, float dt)
	{
		// The complete turn can be done, so we have to be careful around the
		// "+180°, -180°" limit.
		float rawDifference = targetAngle - currentAngle;
		float difference = PosRotSensors.getMainAngle(rawDifference);
		boolean differenceJump = (difference != rawDifference);
		
		// Now, the PID computation can be done.
		float input = 0.0f;
		
		// Proportional part.
		input += difference * kp;
		
		// Integral part.
		integrator += difference * ki * dt;
		input += integrator;
		
		// Derivative part, with filtering.
		if(!differenceJump)
		{
			differencesMean = differencesMean * smoothingStrength
							  + difference * (1-smoothingStrength);
			float derivative = (differencesMean - previousDifference) / dt;
			previousDifference = differencesMean;
			input += derivative * kd;
		}
		else
		{
			// Erase the history, because we are not reaching the target from
			// the "same side".
			differencesMean = 0.0f;
		}
		
		return input;
	}
	
	public void setCoefficients(float kp, float ki, float kd)
	{
		this.kp = kp;
		this.ki = ki;
		this.kd = kd;
	}
	
	public void resetIntegrator()
	{
		integrator = 0.0f;
	}
	
	private float kp, ki, kd, integrator, smoothingStrength, differencesMean,
				  previousDifference;
}
