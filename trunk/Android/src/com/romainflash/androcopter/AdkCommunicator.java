package com.romainflash.androcopter;

import java.io.FileDescriptor;
import java.io.FileInputStream;
import java.io.FileOutputStream;
import java.io.IOException;

import com.romainflash.androcopter.MainController.MotorsPowers;

import android.app.PendingIntent;
import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;
import android.hardware.usb.UsbAccessory;
import android.hardware.usb.UsbManager;
import android.os.ParcelFileDescriptor;
import android.util.Log;

public class AdkCommunicator implements Runnable
{
	public static final int PERIOD_MS = 5;
	public static final float ADC_TO_VOLTAGE = 0.0208f; // R1=0.98kO, R2=3.2kO?, V = ADC/(2^12)*5V/(R1/(R1+R2)).
	private static final String ACTION_USB_PERMISSION = "com.google.android.DemoKit.action.USB_PERMISSION";
	
	public AdkCommunicator(AdbListener adbListener, Context context)
	{
		this.adbListener = adbListener;
		this.context = context;
		
		txBuffer = new byte[4];
		rxBuffer = new byte[2];
	}
	
	public void start(boolean continuousMode) throws Exception
	{
		// Create the ADK manager.
		usbManager = (UsbManager) context.getSystemService(Context.USB_SERVICE);
		
		mPermissionIntent = PendingIntent.getBroadcast(context, 0, new Intent(ACTION_USB_PERMISSION), 0);
		IntentFilter filter = new IntentFilter(ACTION_USB_PERMISSION);
		filter.addAction(UsbManager.ACTION_USB_ACCESSORY_DETACHED);
		context.registerReceiver(usbReceiver, filter);
		
		if (inputStream != null && outputStream != null)
			return;
		
		UsbAccessory[] accessories = usbManager.getAccessoryList();
		
		if(accessories == null)
			Log.d("AndroCopter", "No accessory, please connect one.");
		else
		{
			usbAccessory = accessories[0];
			
			if(usbManager.hasPermission(usbAccessory))
				openAccessory();
			else
			{
				synchronized(usbReceiver)
				{
					if (!mPermissionRequestPending)
					{
						usbManager.requestPermission(usbAccessory, mPermissionIntent);
						mPermissionRequestPending = true;
					}
				}
			}
		}
	}
	
	public void stop()
	{
		closeAccessory();
		context.unregisterReceiver(usbReceiver);
	}
	
	public void setPowers(MotorsPowers powers)
	{
		// Prepare the data to send.
		txBuffer[0] = (byte) powers.nw;
		txBuffer[1] = (byte) powers.ne;
		txBuffer[2] = (byte) powers.se;
		txBuffer[3] = (byte) powers.sw;

		// Send to the ADK.
		if (outputStream != null)
		{
			try
			{
				outputStream.write(txBuffer);
			} catch (IOException e)
			{
				Log.e("AndroCopter", "write failed", e);
				
			}
		}
	}
	
	private final BroadcastReceiver usbReceiver = new BroadcastReceiver() {
		 
	    public void onReceive(Context context, Intent intent) {
	        String action = intent.getAction();
	        
	        if (ACTION_USB_PERMISSION.equals(action))
	        {
	            synchronized (this)
	            {
	            	usbAccessory = (UsbAccessory) intent.getParcelableExtra(UsbManager.EXTRA_ACCESSORY);

	                if (intent.getBooleanExtra(UsbManager.EXTRA_PERMISSION_GRANTED, false))
	                	openAccessory();
	                else
	                    Log.d("AndroCopter", "permission denied for accessory " + usbAccessory);
	            }
	        }
	        else if (UsbManager.ACTION_USB_ACCESSORY_DETACHED.equals(action))
	        {
	        	usbAccessory = (UsbAccessory)intent.getParcelableExtra(UsbManager.EXTRA_ACCESSORY);
	            if(usbAccessory != null)
	            	closeAccessory();
	        }

	    }
	};
	
	private void openAccessory()
	{
	    Log.d("AndroCopter", "openAccessory: " + usbAccessory);
	    fileDescriptor = usbManager.openAccessory(usbAccessory);
	    
	    if (fileDescriptor != null)
	    {
	        FileDescriptor fd = fileDescriptor.getFileDescriptor();
	        inputStream = new FileInputStream(fd);
	        outputStream = new FileOutputStream(fd);
	        rxThread = new Thread(null, this, "AccessoryThread");
	        rxThread.start();
	    }
	    else
			Log.d("AndroCopter", "accessory open fail");
	}
	
	private void closeAccessory()
	{
		// Request the interruption of the RX loop, and wait for it.
		try
		{
			readAgain = false;
			
			if(rxThread != null)
				rxThread.join();
		} catch (InterruptedException e1)
		{
			e1.printStackTrace();
		}
		
		// Close the connection.
		try
		{
			if(fileDescriptor != null)
				fileDescriptor.close();
		}
		catch (IOException e)
		{
		}
		finally
		{
			fileDescriptor = null;
			usbAccessory = null;
		}
	}
	
	public void run()
	{
		int nBytesActuallyRead = 0;
		readAgain = true;
		
		try
		{
			while(readAgain)
			{
				nBytesActuallyRead = inputStream.read(rxBuffer);
				
				if(nBytesActuallyRead == 2)
				{
					int adcVal = ((rxBuffer[1]&0xff) << 8) | (rxBuffer[0]&0xff);
					
					Log.d("AndroCopter", "adcVal=" + adcVal);
					
					float batteryLevel = ADC_TO_VOLTAGE * (float)adcVal;
					
					if(adbListener != null)
						adbListener.onBatteryVoltageArrived(batteryLevel);
				}
			}
		}
		catch (IOException e)
		{
			Log.e("AndroCopter", "IO erreur while reading the ADK!");
		}
	}
	
	public interface AdbListener
	{
		void onBatteryVoltageArrived(float batteryVoltage);
	}
	
	private byte[] txBuffer, rxBuffer;
	private Context context;
	private AdbListener adbListener;
	private UsbManager usbManager;
	private UsbAccessory usbAccessory;
	private PendingIntent mPermissionIntent;
	private boolean mPermissionRequestPending;
	private ParcelFileDescriptor fileDescriptor;
	private FileInputStream inputStream;
	private FileOutputStream outputStream;
	private volatile boolean readAgain;
	private Thread rxThread;
}
