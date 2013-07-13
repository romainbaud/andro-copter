package com.romainflash.androcopter;

import java.io.BufferedReader;
import java.io.IOException;
import java.io.InputStreamReader;
import java.io.OutputStream;
import java.net.Socket;
import java.net.UnknownHostException;

import android.os.SystemClock;
import android.util.Log;

public class TcpClient
{
	public static final int SERVER_PORT = 7444;
	public static final int MAX_SILENCE_PERIOD = 2000; // In [ms].
	public static final long RECONNECT_DELAY = 100; // In [ms].
	public static final int TX_BUFFER_SIZE = 10000;
	
	public static final int TYPE_TEXT = 0;
	public static final int TYPE_VIDEO_FRAME = 1;
	public static final int TYPE_LOG = 2;
	public static final int TYPE_CURRENT_STATE = 3;
	public static final int TYPE_PHOTO = 4;
	
	TcpClient(TcpMessageReceiver receiver)
	{
		this.tcpReceiver = receiver;
		
		txHeaderBuffer = new byte[5];
	}
	
	void start(String serverIp)
	{
		// Open the socket server.
		if(connectThread != null && connectThread.isAlive())
			connectThread.requestStop();
			
		connectThread = new ConnectThread(serverIp);
		connectThread.start();
	}
	
	void stop()
	{
		if(connectThread != null)
			connectThread.requestStop();
	}
	
	private class ConnectThread extends Thread
	{
		public ConnectThread(String serverIp)
		{
			this.serverIp = serverIp;
		}
		
		public void run()
		{
			again = true;
			previouslyConnected = false;
			
			while(again)
			{
				// Setup a UDP server socket.
				if(socket == null || socket.isClosed())
				{
					try
					{
						socket = new Socket(serverIp, SERVER_PORT);
						socket.setSoTimeout(0); // Inifinite time for reading.
						socket.setTcpNoDelay(true);
						
						tcpReceiver.onConnectionEstablished();
						previouslyConnected = true;
					}
					catch (UnknownHostException e)
					{
						Log.w("AndroCopter", "Can't reach the server!");
					}
					catch (IOException e)
					{
						Log.w("AndroCopter", "Error when creating the socket.");
					}
				}
				
				// Get the incomming messages.
				if(socket != null && socket.isConnected())
				{
					try
					{
						BufferedReader in = new BufferedReader(new InputStreamReader(socket.getInputStream()));
						
						while(true)
						{
							// Block until a message arrives.
							String inMessage = in.readLine();
							
							if(inMessage != null)
							{
					            // Read the message.
								tcpReceiver.onMessageReceived(inMessage);
							}
							else // Stream has ended, so the socket disconnected.
								break;
						}
					}
					catch (IOException e)
					{
						// Nothing to do, we will reconnect after.
					}

					if(socket != null)
					{
						try
						{
							socket.close();
						}
						catch (IOException e1)
						{
							e1.printStackTrace();
						}
					}
				}
				
				// We reach this point because of a disconnection, or if we are
				// unable to establish a connection.
				// Notify the receiver in the first case.
				if(previouslyConnected)
				{
					tcpReceiver.onConnectionLost();
					previouslyConnected = false;
				}
				
				// Try to reconnect after some time.
				SystemClock.sleep(RECONNECT_DELAY);
			}
		}
		
		public synchronized void requestStop()
		{
			again = false;
			
	    	if(socket != null)
	    	{
	    		try
				{
					socket.close();
					socket = null;
				}
	    		catch (IOException e)
				{
					e.printStackTrace();
				}
	    	}
		}
		
		public synchronized void sendMessage(byte[] message, int type)
		{
			if(socket == null || !socket.isConnected())
				return;
			
			int messageSize = message.length;
			int messageWithTypeSize = messageSize + 1;
			
	    	// The message has 3 parts :
	    	// -the 4 first bytes are the size of the rest of the message.
	    	// -the next byte is the type of the message.
	    	// -the remaining bytes are the useful part.
	    	//txBuffer = new byte[4 + 1 + messageSize];
			txHeaderBuffer[0] = (byte) (messageWithTypeSize >> 24); // "Rest of the
			txHeaderBuffer[1] = (byte) (messageWithTypeSize >> 16); // message"
			txHeaderBuffer[2] = (byte) (messageWithTypeSize >> 8);  // length.
			txHeaderBuffer[3] = (byte) messageWithTypeSize;         //
			txHeaderBuffer[4] = (byte)type; // Type of the message.

			sendRawBytes(txHeaderBuffer); // Send the message header.
			sendRawBytes(message); // Send the actual content of the message.
		}
		
		public boolean isConnected()
		{
			if(socket == null)
				return false;
			else
				return socket.isConnected();
		}
		
		private void sendRawBytes(byte[] bytes)
		{
			try
			{
				OutputStream out = socket.getOutputStream();
	            out.write(bytes);
	            out.flush();
			}
			catch(IOException e)
			{
				Log.e("NetTest", "Can't send the message!");
				e.printStackTrace();
			}
		}
		
		private volatile boolean again, previouslyConnected;
		private Socket socket;
		private String serverIp;
	}
	
	public interface TcpMessageReceiver
	{
		public void onConnectionEstablished();
		public void onConnectionLost();
		public void onMessageReceived(String message);
	}
	
	public void sendMessage(byte[] message, int type)
	{
		if(connectThread != null)
			connectThread.sendMessage(message, type);
	}
	
	public boolean isConnected()
	{
		if(connectThread != null)
			return connectThread.isConnected();
		else
			return false;
	}

	private byte[] txHeaderBuffer;
	private TcpMessageReceiver tcpReceiver;
	private ConnectThread connectThread;
}
