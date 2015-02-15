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
		UsbAccessory accessory = (accessories == null ? null : accessories[0]);
		if (accessory != null)
		{
			if (usbManager.hasPermission(accessory))
			{
				openAccessory(accessory);
			}
			else
			{
				synchronized (usbReceiver)
				{
					if (!mPermissionRequestPending)
					{
						usbManager.requestPermission(accessory, mPermissionIntent);
						mPermissionRequestPending = true;
					}
				}
			}
		}
		else
		{
			Log.d("AndroCopter", "No accessory, please connect one.");
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
	
	private final BroadcastReceiver usbReceiver = new BroadcastReceiver()
	{ 
	    public void onReceive(Context context, Intent intent)
	    {
	    	String action = intent.getAction();
			if (ACTION_USB_PERMISSION.equals(action))
			{
				synchronized (this)
				{
					UsbAccessory accessory = (UsbAccessory) intent.getParcelableExtra(UsbManager.EXTRA_ACCESSORY);
					if (intent.getBooleanExtra(UsbManager.EXTRA_PERMISSION_GRANTED, false))
					{
						openAccessory(accessory);
					}
					else
					{
						Log.d("AndroCopter", "permission denied for accessory " + accessory);
					}
					mPermissionRequestPending = false;
				}
			}
			else if (UsbManager.ACTION_USB_ACCESSORY_DETACHED.equals(action))
			{
				UsbAccessory accessory = (UsbAccessory)intent.getParcelableExtra(UsbManager.EXTRA_ACCESSORY);
				if (accessory != null && accessory.equals(usbAccessory))
				{
					closeAccessory();
				}
			}
	    }
	};
	
	private void openAccessory(UsbAccessory accessory)
	{
	    Log.d("AndroCopter", "openAccessory: " + accessory);
	    fileDescriptor = usbManager.openAccessory(accessory);
	    
	    if (fileDescriptor != null)
	    {
	    	usbAccessory = accessory;
	        FileDescriptor fd = fileDescriptor.getFileDescriptor();
	        inputStream = new FileInputStream(fd);
	        outputStream = new FileOutputStream(fd);
	        rxThread = new Thread(null, this, "AccessoryThread");
	        rxThread.start();
	        
	        Log.d("AndroCopter", "USB accessory opened.");
	    }
	    else
			Log.d("AndroCopter", "USB accessory open failed.");
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
		int nBytesRead = 0;
		readAgain = true;
		byte[] rxBuffer = new byte[16384];
		
		int i;
		while(readAgain)
		{
			try
			{
				nBytesRead = inputStream.read(rxBuffer);
			}
			catch (IOException e)
			{
				Log.e("AndroCopter", "IO error while reading the ADK!");
			}
			
			i = 0;
			while (i < nBytesRead)
			{
				int len = nBytesRead - i;
				
				if(len >= 2)
				{
					int adcVal = ((rxBuffer[i+1]&0xff) << 8) | (rxBuffer[i+0]&0xff);
					i += 2;
					
					float batteryLevel = ADC_TO_VOLTAGE * (float)adcVal;
					
					//Log.i("AndroCopter", "Battery voltage: " + batteryLevel);
					
					if(adbListener != null)
						adbListener.onBatteryVoltageArrived(batteryLevel);
				}
			}
		}
	}
	
	public interface AdbListener
	{
		void onBatteryVoltageArrived(float batteryVoltage);
	}
	
	private byte[] txBuffer;
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
