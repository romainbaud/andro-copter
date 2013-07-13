package com.romainflash.androcopter;

import android.content.Context;
import android.hardware.Sensor;
import android.hardware.SensorEvent;
import android.hardware.SensorEventListener;
import android.hardware.SensorManager;
import android.location.Location;
import android.location.LocationListener;
import android.location.LocationManager;
import android.os.Bundle;
import android.util.Log;

public class PosRotSensors implements SensorEventListener
{
	public static final float PI = 3.14159265359f;
	public static final float RAD_TO_DEG = 180.0f / PI;
	public static final double DEG_TO_RAD = PI / 180.0;
	public static final float ALTITUDE_SMOOTHING = 0.95f;
	public static final double EARTH_RADIUS = 6371000; // [m].
	public static final boolean USE_GPS = false;
	
	public PosRotSensors(Context context)
	{
		heliState = new HeliState();
		rotationMatrix = new float[9];
		yawPitchRollVec = new float[3];
		rotationVec = new float[3];
		
		yawZero = 0.0f;
		pitchZero = 0.0f;
		rollZero = 0.0f;
		
		// Get the sensors manager.
        sensorManager = (SensorManager) context.getSystemService(Context.SENSOR_SERVICE);   
        
        // List all the sensors present on the phone.
        /*List<Sensor> sensorsList = sensorManager.getSensorList(Sensor.TYPE_ALL);
        String sensorsString = "List of all sensors:\n";
        
        for(int i=0; i<sensorsList.size(); i++)
        {
        	Sensor sensor = sensorsList.get(i);
        	sensorsString += "### " + sensor.getName() + " ###\n"
        			+ "type: " + sensor.getType() + "\n"
        			+ "vendor: " + sensor.getVendor() + "\n"
        			+ "version: " + sensor.getVersion() + "\n"
        			+ "resolution: " + sensor.getResolution() + "\n"
        			+ "maximum range: " + sensor.getMaximumRange() + "\n"
        			+ "minimum delay: " + sensor.getMinDelay() + "\n"
        			+ "power: " + sensor.getPower() + "\n\n";
        }
        
        Log.i("androcopter", sensorsString);*/
        
        // Get the sensors.
        rotationSensor = sensorManager.getDefaultSensor(Sensor.TYPE_ROTATION_VECTOR);
        pressureSensor = sensorManager.getDefaultSensor(Sensor.TYPE_PRESSURE);
        
        if((rotationSensor == null) || (pressureSensor == null))
        	Log.e("androcopter", "A least one sensor is missing!");
        
        // Get the GPS manager.
        if(USE_GPS)
        	locationManager = (LocationManager) context.getSystemService(Context.LOCATION_SERVICE);
        
        locationListener = new LocationListener()
		{
        	public void onLocationChanged(Location location)
			{
				heliState.gpsElevation = (float) location.getAltitude();
				heliState.gpsAccuracy = location.getAccuracy();
				heliState.longitude = location.getLongitude();
				heliState.latitude = location.getLatitude();
				
				heliState.nSatellites = (Integer) location.getExtras().get("satellites");
				
				Log.i("AndroCopter", "GPS: " + "altitude: " + heliState.gpsElevation + ", "
											 + "accuracy: " + heliState.gpsAccuracy + ", "
											 + "longitude: " + heliState.longitude + ", "
											 + "latitude: " + heliState.latitude + ", "
											 + "bearing: " + location.getBearing() + ", "
											 + "speed: " + location.getSpeed() + ", "
											 + "provider: " + location.getProvider());
				
				// Convert longitude+latitude to x+y (using the small angles
				// approximation: sin(x)~=x).
				heliState.xPos = (float) (EARTH_RADIUS * (heliState.longitude-longitudeZero) * DEG_TO_RAD);
				heliState.yPos = (float) (EARTH_RADIUS * (heliState.latitude-latitudeZero) * DEG_TO_RAD);
				
				// Convert heading+speed to speedx+speedy.
				heliState.xSpeed = location.getSpeed() * (float) Math.cos(location.getBearing());
				heliState.ySpeed = location.getSpeed() * (float) Math.sin(location.getBearing());
			}
			
			public void onStatusChanged(String provider, int status, Bundle extras)
			{
				heliState.nSatellites = (Integer) extras.get("satellites");
				heliState.gpsStatus = status;
			}
			
			public void onProviderEnabled(String provider) {}
			public void onProviderDisabled(String provider){}
		};
        
		if(USE_GPS)
		{
			locationManager.requestLocationUpdates(LocationManager.GPS_PROVIDER,
        									   	   0, 0, locationListener);
		}
	}
	
	public void resume()
	{
		sensorManager.registerListener(this, rotationSensor,
				   SensorManager.SENSOR_DELAY_FASTEST);
		sensorManager.registerListener(this, pressureSensor,
				   SensorManager.SENSOR_DELAY_FASTEST);
	}
	
	public void pause()
	{
		// Disable the GPS.
		if(USE_GPS)
			locationManager.removeUpdates(locationListener);
		
		// Disable the inertial sensors.
		sensorManager.unregisterListener(this);
	}

    @Override
    public final void onAccuracyChanged(Sensor sensor, int accuracy)
    {
    	// These sensors should not change precision.
    }

    @Override
    public final void onSensorChanged(SensorEvent event)
    {
    	if(event.sensor == rotationSensor)
    	{
    		// Get the time and the rotation vector.
    		heliState.time = event.timestamp;
    		System.arraycopy(event.values, 0, rotationVec, 0, 3);

    		// Convert the to "yaw, pitch, roll".
    		SensorManager.getRotationMatrixFromVector(rotationMatrix, rotationVec);
    		SensorManager.getOrientation(rotationMatrix, yawPitchRollVec);
    		
    		// Make the measurements relative to the user-defined zero orientation.
    		heliState.yaw = getMainAngle(-(yawPitchRollVec[0]-yawZero) * RAD_TO_DEG);
    		heliState.pitch = getMainAngle(-(yawPitchRollVec[1]-pitchZero) * RAD_TO_DEG);
    		heliState.roll = getMainAngle((yawPitchRollVec[2]-rollZero) * RAD_TO_DEG);

    		// New sensors data are ready.
    		newMeasurementsReady = true;
    	}
    	else if(event.sensor == pressureSensor)
    	{
    		float pressure = event.values[0];
    		float rawAltitudeUnsmoothed = SensorManager.getAltitude(SensorManager.PRESSURE_STANDARD_ATMOSPHERE, pressure);
    		absoluteElevation = (absoluteElevation*ALTITUDE_SMOOTHING) + (rawAltitudeUnsmoothed*(1.0f-ALTITUDE_SMOOTHING));
    		heliState.baroElevation = absoluteElevation - elevationZero;
    	}
    }
    
    public class HeliState
    {
    	public float yaw, pitch, roll, // [degrees].
    				 baroElevation, gpsElevation; // [m].
    	public long time; // [nanoseconds].
		public double longitude, latitude; // [degrees].
		public float gpsAccuracy, xPos, yPos; // [m].
		public float xSpeed, ySpeed; // [m/s].
		public int nSatellites, gpsStatus; // [].
    }
    
    public HeliState getState()
    {
		newMeasurementsReady = false;
    				
    	return heliState;
    }
    
    public void setCurrentStateAsZero()
    {
    	yawZero = yawPitchRollVec[0];
		pitchZero = yawPitchRollVec[1];
		rollZero = yawPitchRollVec[2];
		elevationZero = absoluteElevation;
		longitudeZero = absoluteLongitude;
		latitudeZero = absoluteLatitude;
    }
    
    public boolean newMeasurementsReady()
    {
    	return newMeasurementsReady;
    }
    
    // Return the smallest angle between two segments.
    public static float getMainAngle(float angle)
    {
    	while(angle < -180.0f)
			angle += 360.0f;
		while(angle > 180.0f)
			angle -= 360.0f;
    	
    	return angle;
    }

    private SensorManager sensorManager;
    private Sensor rotationSensor, pressureSensor;
    private HeliState heliState;
    private float[] rotationVec, rotationMatrix, yawPitchRollVec;
    private float yawZero, pitchZero, rollZero, elevationZero, latitudeZero,
    			  longitudeZero, absoluteLongitude, absoluteLatitude,
    			  absoluteElevation;
    private boolean newMeasurementsReady;
    private LocationManager locationManager;
    private LocationListener locationListener;
}
