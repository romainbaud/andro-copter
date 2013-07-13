package com.romainflash.androcopter;


import java.util.ArrayList;

import android.os.SystemClock;
import android.util.Log;

import com.romainflash.androcopter.AdkCommunicator.AdbListener;
import com.romainflash.androcopter.PosRotSensors.HeliState;
import com.romainflash.androcopter.TcpClient.TcpMessageReceiver;

public class MainController implements TcpMessageReceiver, AdbListener
{
	public static final int STATE_SEND_DIVIDER = 20;
	public static final double MAX_MOTOR_POWER = 255.0; // 255.0 normally, less for testing.
	public static final float MAX_TIME_WITHOUT_PC_RX = 1.0f; // Maximum time [s] without any message from the PC, before emergency stop.
	public static final float MAX_TIME_WITHOUT_ADK_RX = 1.0f; // Maximum time [s] without any message from the ADK, setting the temperature to 0 (error).
	public static final long INT_MAX = 2147483648L;
	public static final float MAX_SAFE_PITCH_ROLL = 60; // [deg].
	public static final float PID_DERIV_SMOOTHING = 0.5f;

	public MainController(QuadcopterActivity activity)
	{
		this.activity = activity;
		
		transmitter = new AdkCommunicator(this, activity);
		motorsPowers = new MotorsPowers();

		yawRegulator = new PidAngleRegulator(0.0f, 0.0f, 0.0f, PID_DERIV_SMOOTHING);
		pitchRegulator = new PidAngleRegulator(0.0f, 0.0f, 0.0f, PID_DERIV_SMOOTHING);
		rollRegulator = new PidAngleRegulator(0.0f, 0.0f, 0.0f, PID_DERIV_SMOOTHING);
		altitudeRegulator = new PidRegulator(0.0f,  0.0f,  0.0f, PID_DERIV_SMOOTHING, 0.0f);

		yawAngleTarget = 0.0f;
		pitchAngleTarget = 0.0f;
		rollAngleTarget = 0.0f;
		altitudeTarget = 0.0f;
		
		stateSendDividerCounter = 0;
		
		timeWithoutPcRx = 0.0f;
		timeWithoutAdkRx = 0.0f;
		
		// Create the server.
		client = new TcpClient(this);
		
		// Create the sensors manager.
        posRotSensors = new PosRotSensors(activity);
        heliState = posRotSensors.new HeliState();
        
        // Create the camera object.
        camera = (CameraPreview)activity.findViewById(R.id.cameraSurface);
	}

	public void start() throws Exception
	{
		// Initializations.
		regulatorEnabled = false;
		altitudeLockEnabled = false;
		meanThrust = 0.0f;
		
		// Start the sensors.
		posRotSensors.resume();
		
		// Start the main controller thread.
		controllerThread = new ControllerThread();
		controllerThread.start();
		
		// Start the USB transmitter.
		transmitter.start(false);
	}
	
	public void stop()
	{
		// Stop the main controller thread.
		controllerThread.requestStop();
		
		// Stop the motors controller.
		transmitter.stop();
		
		// Stop the server.
		client.stop();
		
		// Stop the sensors.
		posRotSensors.pause();
		
		// Stop the camera.
		camera.releaseCamera();
	}
	
	public void startClient(String serverIP)
	{
		client.start(serverIP);
	}
	
	public void stopClient()
	{
		client.stop();
	}
	
	public HeliState getSensorsData()
	{
		return heliState;
	}
	
	public MotorsPowers getMotorsPowers()
	{
		return motorsPowers;
	}
	
	public boolean getRegulatorsState()
	{
		return regulatorEnabled;
	}
	
	public class ControllerThread extends Thread
	{
		@Override
		public void run()
		{
			again = true;

			while(again)
			{
				// This loop runs at a very high frequency (1kHz), but all the
				// control only occurs when new measurements arrive.
				if(!posRotSensors.newMeasurementsReady())
				{
					SystemClock.sleep(1);
					continue;
				}

				// Get the sensors data.
				heliState = posRotSensors.getState();
				
				float currentYaw = heliState.yaw;
				float currentPitch = heliState.pitch;
				float currentRoll = heliState.roll;
				float currentAltitude = heliState.baroElevation;
				
				long currentTime = heliState.time;
				float dt = ((float)(currentTime-previousTime)) / 1000000000.0f; // [s].
				previousTime = currentTime;
				
				if(Math.abs(dt) > 1.0) // In case of the counter has wrapped around.
			        continue;
				
				// Check for dangerous situations.
				if(regulatorEnabled)
				{
					// If the quadcopter is too inclined, emergency stop it.
					if(Math.abs(currentPitch) > MAX_SAFE_PITCH_ROLL ||
					   Math.abs(currentRoll) > MAX_SAFE_PITCH_ROLL)
					{
						emergencyStop();
					}
					
					// If no message has been received from the computer, this
					// may mean that it crashed, so the flight should be
					// stopped.
					timeWithoutPcRx += dt;
					if(timeWithoutPcRx > MAX_TIME_WITHOUT_PC_RX)
						emergencyStop();
				}
				
				// If there the ADK has not sent the temperature for a long time
				// set the displayed temperature to 0 (error).
				timeWithoutAdkRx += dt;
				if(timeWithoutAdkRx > MAX_TIME_WITHOUT_ADK_RX)
					batteryVoltage = 0.0f;
				
				// Compute the motors powers.
				float yawForce, pitchForce, rollForce, altitudeForce;
				
				if(regulatorEnabled && meanThrust > 1.0)
				{
					// Compute the "forces" needed to move the quadcopter to the
					// set point.
					yawForce = yawRegulator.getInput(yawAngleTarget, currentYaw, dt);
					pitchForce = pitchRegulator.getInput(pitchAngleTarget, currentPitch, dt);
					rollForce = rollRegulator.getInput(rollAngleTarget, currentRoll, dt);
					
					if(altitudeLockEnabled)
						altitudeForce = altitudeRegulator.getInput(altitudeTarget, currentAltitude, dt);
					else
						altitudeForce = meanThrust;
					
					// Compute the power of each motor.
					double tempPowerNW, tempPowerNE, tempPowerSE, tempPowerSW;
					
					tempPowerNW = altitudeForce; // Vertical "force".
					tempPowerNE = altitudeForce; //
					tempPowerSE = altitudeForce; //
					tempPowerSW = altitudeForce; //
					
					tempPowerNW += pitchForce; // Pitch "force".
					tempPowerNE += pitchForce; //
					tempPowerSE -= pitchForce; //
					tempPowerSW -= pitchForce; //
	
					tempPowerNW += rollForce; // Roll "force".
					tempPowerNE -= rollForce; //
					tempPowerSE -= rollForce; //
					tempPowerSW += rollForce; //
	
					tempPowerNW += yawForce; // Yaw "force".
					tempPowerNE -= yawForce; //
					tempPowerSE += yawForce; //
					tempPowerSW -= yawForce; //
					
					// Saturate the values, because the motors input are 0-255.
					motorsPowers.nw = motorSaturation(tempPowerNW);
					motorsPowers.ne = motorSaturation(tempPowerNE);
					motorsPowers.se = motorSaturation(tempPowerSE);
					motorsPowers.sw = motorSaturation(tempPowerSW);
				}
				else
				{
					motorsPowers.nw = 0;
					motorsPowers.ne = 0;
					motorsPowers.se = 0;
					motorsPowers.sw = 0;
					yawForce = 0.0f;
					pitchForce = 0.0f;
					rollForce = 0.0f;
					altitudeForce = 0.0f;
				}
				
				transmitter.setPowers(motorsPowers);
				
				// Log the variables, if needed.
				if(log != null)
				{
					LogPoint p = new LogPoint();
					p.time = currentTime/1000000 % INT_MAX;
					p.yaw = currentYaw;
					p.pitch = currentPitch;
					p.roll = currentRoll;
					p.yawTarget = yawAngleTarget;
					p.pitchTarget = pitchAngleTarget;
					p.rollTarget = rollAngleTarget;
					p.yawForce = yawForce;
					p.pitchForce = pitchForce;
					p.rollForce = rollForce;
					p.meanThrust = meanThrust;
					p.nwPower = motorsPowers.nw;
					p.nePower = motorsPowers.ne;
					p.sePower = motorsPowers.se;
					p.swPower = motorsPowers.sw;
					p.altitude = currentAltitude;
					log.add(p);
				}
				
				// Transmit the current state to the computer.
				stateSendDividerCounter++;
				
				if(stateSendDividerCounter % STATE_SEND_DIVIDER == 0)
				{
					String currStateStr = "" + (currentTime/1000000 % INT_MAX) +
										  " " + currentYaw + " " + yawAngleTarget + " " + yawForce +
										  " " + currentPitch + " " + pitchAngleTarget + " " + pitchForce +
										  " " + currentRoll + " " + rollAngleTarget + " " + rollForce +
										  " " + batteryVoltage +
										  " " + 0 +
										  " " + (regulatorEnabled?1:0) +
										  " " + currentAltitude + " " + altitudeTarget + " " + altitudeForce;

					client.sendMessage(currStateStr.getBytes(),
									   TcpClient.TYPE_CURRENT_STATE);
				}
			}
		}
		
		public void requestStop()
		{
			again = false;
		}
		
		private boolean again;
	}
	
	private int motorSaturation(double val)
	{
		if(val > MAX_MOTOR_POWER)
			return (int)MAX_MOTOR_POWER;
		else if(val < 0.0)
			return 0;
		else
			return (int)val;
	}

	public void onConnectionEstablished()
	{
		// Reset the orientation.
		posRotSensors.setCurrentStateAsZero();
	}

	public void onConnectionLost()
	{
		// Emergency stop of the quadcopter.
		emergencyStop();
	}
	
	public void onMessageReceived(String message)
	{		
		// Reset the timer.
		timeWithoutPcRx = 0.0f;
		
		// Interpret the message, and do an action corresponding to the enclosed
		// order.
		if(message.equals("heartbeat"))
		{
			// Do nothing, this is just to reset the timer.
		}
		else if(message.equals("emergency_stop"))
			emergencyStop();
		else if(message.startsWith("command "))
		{
			String[] values = message.replace("command ", "").split("\\s");
			
			if(values.length == 4)
			{
				meanThrust = (float)Integer.parseInt(values[0]);
				yawAngleTarget = Float.parseFloat(values[1]);
				pitchAngleTarget = Float.parseFloat(values[2]);
				rollAngleTarget = Float.parseFloat(values[3]);
				
				//Log.d("dp", "meanThrust: " + meanThrust);
				
				// Reset the integrators if the motors are off.
				if(Integer.parseInt(values[0]) == 0)
				{
					yawRegulator.resetIntegrator();
					pitchRegulator.resetIntegrator();
					rollRegulator.resetIntegrator();
				}
			}
		}
		else if(message.startsWith("regulator_coefs "))
		{
			String[] values = message.replace("regulator_coefs ", "").split("\\s");
			
			if(values.length == 12)
			{
				float yawP = Float.parseFloat(values[0]);
				float yawI = Float.parseFloat(values[1]);
				float yawD = Float.parseFloat(values[2]);
				float pitchP = Float.parseFloat(values[3]);
				float pitchI = Float.parseFloat(values[4]);
				float pitchD = Float.parseFloat(values[5]);
				float rollP = Float.parseFloat(values[6]);
				float rollI = Float.parseFloat(values[7]);
				float rollD = Float.parseFloat(values[8]);
				float altiP = Float.parseFloat(values[9]);
				float altiI = Float.parseFloat(values[10]);
				float altiD = Float.parseFloat(values[11]);
				yawRegulator.setCoefficients(yawP, yawI, yawD);
				pitchRegulator.setCoefficients(pitchP, pitchI, pitchD);
				rollRegulator.setCoefficients(rollP, rollI, rollD);
				altitudeRegulator.setCoefficients(altiP, altiI, altiD);
			}
		}
		else if(message.equals("regulator_state on"))
			regulatorEnabled = true;
		else if(message.equals("regulator_state off"))
		{
			regulatorEnabled = false;
		}
		else if(message.equals("log on"))
		{
			// Create a log file.
			log = new ArrayList<LogPoint>();
			
			client.sendMessage("Logging started.".getBytes(), TcpClient.TYPE_TEXT);
		}
		else if(message.equals("log off"))
		{
			// Generate the string.
			StringBuilder sb = new StringBuilder();
			
			for(int i=0; i<log.size(); i++)
			{
				LogPoint p = log.get(i);
				sb.append("" + p.time + " "
						  + p.yaw + " " + p.pitch + " " + p.roll + " "
						  + p.yawTarget  + " " + p.pitchTarget + " "
						  + p.rollTarget + " " + p.yawForce + " "
						  + p.pitchForce + " " + p.rollForce + " "
						  + p.meanThrust + " " + p.nwPower + " "
						  + p.nePower + " " + p.sePower + " " + p.swPower + " "
						  + p.altitude + "\n");
			}

			// Send the log.
			client.sendMessage(sb.toString().getBytes(), TcpClient.TYPE_LOG);
			client.sendMessage("Logging finished.".getBytes(), TcpClient.TYPE_TEXT);
			
			// Stop logging.
			log = null;
		}
		else if(message.equals("orientation_reset"))
		{
			posRotSensors.setCurrentStateAsZero();
		}
		else if(message.startsWith("fpv "))
		{
			String newStateString = message.replace("fpv ", "");
			
			if(newStateString.equals("stop"))
				camera.stopStreaming();
			else if(newStateString.equals("sd"))
			{
				camera.setFrameSize(activity, false);
				camera.startStreaming(client);
			}
			else if(newStateString.equals("hd"))
			{
				camera.setFrameSize(activity, true);
				camera.startStreaming(client);
			}
		}
		else if(message.equals("take_picture"))
			camera.takePicture(client);
		else if(message.startsWith("altitude_lock "))
		{
			String newStateString = message.replace("altitude_lock ", "");
			
			if(newStateString.equals("on"))
			{
				altitudeTarget = heliState.baroElevation;
				altitudeRegulator.setAPriori((float)motorsPowers.getMean());
				altitudeLockEnabled = true;
			}
			else if(newStateString.equals("off"))
				altitudeLockEnabled = false;
		}
	}
	
	private void emergencyStop()
	{
		// TODO
		// The motors should stop gradually, to avoid hitting the ground too
		// hard ?
		Log.w("dp", "Emergency stop!");
		regulatorEnabled = false;
	}
	
	public void onBatteryVoltageArrived(float batteryVoltage)
	{
		this.batteryVoltage = batteryVoltage;
		
		// Reset the timer.
		timeWithoutAdkRx = 0.0f;
	}
	
	public class MotorsPowers
	{
		public int nw, ne, se, sw; // 0-1023 (10 bits values).
		
		public int getMean()
		{
			return (nw+ne+se+sw) / 4;
		}
	}
	
	private class LogPoint
	{
		public long time;
		public float yaw, pitch, roll, yawTarget, pitchTarget, rollTarget,
					 yawForce, pitchForce, rollForce, meanThrust,
					 nwPower, nePower, sePower, swPower, altitude;
	}

	private TcpClient client;
	private QuadcopterActivity activity;
	private AdkCommunicator transmitter;
	private CameraPreview camera;
	private float meanThrust, yawAngleTarget, pitchAngleTarget, rollAngleTarget,
				  altitudeTarget, batteryVoltage, timeWithoutPcRx,
				  timeWithoutAdkRx;
	private PosRotSensors posRotSensors;
	private MotorsPowers motorsPowers;
	private boolean regulatorEnabled, altitudeLockEnabled;
	private PidAngleRegulator yawRegulator, pitchRegulator, rollRegulator;
	private PidRegulator altitudeRegulator;
	private ArrayList<LogPoint> log;
	private HeliState heliState;
	private int stateSendDividerCounter;
	private ControllerThread controllerThread;
	private long previousTime;
}
