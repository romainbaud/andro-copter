package com.romainflash.androcopter;

import java.io.ByteArrayOutputStream;
import java.io.IOException;

import android.app.Activity;
import android.content.Context;
import android.graphics.Bitmap;
import android.hardware.Camera;
import android.util.AttributeSet;
import android.util.Log;
import android.view.SurfaceHolder;
import android.view.SurfaceView;
import android.view.SurfaceHolder.Callback;
import android.widget.LinearLayout;

public class CameraPreview extends SurfaceView implements Callback
{
	public static final int PHOTO_WIDTH = 1920; // [px].
	public static final int PHOTO_HEIGHT = 1080; // [px].
	public static final long MIN_FRAME_SPACING = 200000000; // [ns].
	
	private SurfaceHolder mHolder;
	private Camera mCamera;
    private volatile boolean frameRequested = false;
    private TcpClient tcp;
    private int[] rgbData;
    private Bitmap bitmap;
    private Camera.PictureCallback pictureReceiver;
    private long lastPictureTxTime;

	public CameraPreview(Context context, AttributeSet as)
	{
		super(context, as);
		
		pictureReceiver = new PictureReceiver();
		lastPictureTxTime = System.nanoTime();

		// Install a SurfaceHolder.Callback so we get notified when the
        // underlying surface is created and destroyed.
        mHolder = getHolder();
        mHolder.addCallback(this);
	}
	
	public void surfaceCreated(SurfaceHolder holder)
	{
        // The Surface has been created, acquire the camera and tell it where
        // to draw.
		if(mCamera != null)
			return;
		
        mCamera = Camera.open();
        
        try
        {
           mCamera.setPreviewDisplay(holder);
           mCamera.startPreview();
        }
        catch (IOException exception)
        {
            mCamera.release();
            mCamera = null;
            Log.e("AndroCopter", "Can't set the preview display!");
            return;
        }
        
        // List all the possible image formats.
        /*Camera.Parameters params = mCamera.getParameters();

        List<Size> sizes = params.getSupportedPictureSizes();

        for(int i=0; i<sizes.size(); i++)
        	Log.i("AndroCopter", "Size: " + sizes.get(i).width + " " + sizes.get(i).height);*/
	}

	public void surfaceDestroyed(SurfaceHolder arg0)
	{
        // Surface will be destroyed when we return, so stop the preview.
        // Because the CameraDevice object is not a shared resource, it's very
        // important to release it when the activity is paused.
		frameRequested = false;
		
		releaseCamera();
	}
	
	public void surfaceChanged(SurfaceHolder holder, int format, int w, int h)
	{
		mCamera.stopPreview();
		
    	mCamera.setPreviewCallback(new PreviewReceiver(w, h));
    	
    	// Allocation des "tableaux de pixels".
    	rgbData = new int[w * h];
    	bitmap = Bitmap.createBitmap(w, h, Bitmap.Config.ARGB_8888);
    	
        // Now that the size is known, set up the camera parameters and begin
        // the preview.
        Camera.Parameters parameters = mCamera.getParameters();
        parameters.setPreviewSize(w, h);
        parameters.setPictureSize(PHOTO_WIDTH, PHOTO_HEIGHT);
        mCamera.setParameters(parameters);
        mCamera.startPreview();
	}
	
	public void releaseCamera()
	{
		if(mCamera != null)
		{
	        mCamera.stopPreview();
	        mCamera.setPreviewCallback(null);
	        mCamera.release();
	        mCamera = null;
		}
	}

    private class PreviewReceiver implements Camera.PreviewCallback
    {
    	public PreviewReceiver(int width, int height)
    	{
    		this.width = width;
    		this.height = height;
    	}
		public void onPreviewFrame(byte[] data, Camera camera)
		{
			long currentTime = System.nanoTime();
			
			if(frameRequested && (currentTime-lastPictureTxTime>MIN_FRAME_SPACING))
			{
				lastPictureTxTime = currentTime;
				
				try
				{
					// Convert from YUV420 to RGB.
					decodeYUV420SP(data, rgbData, width, height);

					// Convert from RGB to bitmap.
					bitmap.setPixels(rgbData, 0, width, 0, 0, width, height);

					// Convert from bitmap to jpeg.
					ByteArrayOutputStream baos = new ByteArrayOutputStream();
					bitmap.compress(Bitmap.CompressFormat.JPEG, 80, baos);
					
					// Send the frame to the computer.
					tcp.sendMessage(baos.toByteArray(), TcpClient.TYPE_VIDEO_FRAME);

				} catch(IndexOutOfBoundsException e)
				{
					Log.w("AndroCopter", "Can't read preview data !");
				}
			}
		}
		
		private int width, height;
    }
    
    private class PictureReceiver implements Camera.PictureCallback
    {
    	@Override
        public void onPictureTaken(byte[] data, Camera camera)
    	{
    		// Send the picture to the computer.
    		Log.d("AndroCopter", "Start sending picture.");
			tcp.sendMessage(data, TcpClient.TYPE_PHOTO);
			
			// Resume the preview.
			mCamera.startPreview();
			
			Log.d("AndroCopter", "Finished sending picture.");
        }
    }
    
    public void setFrameSize(Activity activity, boolean hd)
	{
    	if(hd)
    	{
    		activity.runOnUiThread(new Runnable()
			{
				public void run()
				{
					setLayoutParams(new LinearLayout.LayoutParams(320,240));
				}
			});
    	}
    	else
    	{
    		activity.runOnUiThread(new Runnable()
			{
				public void run()
				{
					setLayoutParams(new LinearLayout.LayoutParams(176,144));
				}
			});
    	}
	}
    
    public synchronized void startStreaming(TcpClient tcp)
    {
    	this.tcp = tcp;
    	frameRequested = true;
    }
    
    public synchronized void stopStreaming()
    {
    	frameRequested = false;
    }
    
    public void takePicture(TcpClient tcp)
	{
    	this.tcp = tcp;
    	Log.d("AndroCopter", "TCP asked for picture.");
		mCamera.takePicture(null, null, pictureReceiver);
	}
    
    // From: http://stackoverflow.com/questions/4768165/android-problem-whith-converting-preview-frame-to-bitmap
	public int[] decodeYUV420SP(byte[] yuv420sp, int[] rgb,
								int width, int height)
	{
		final int frameSize = width * height;

		for(int j = 0, yp = 0; j < height; j++)
		{
			int uvp = frameSize + (j >> 1) * width, u = 0, v = 0;
			
			for(int i = 0; i < width; i++, yp++)
			{
				int y = (0xff & ((int) yuv420sp[yp])) - 16;
				if (y < 0)
					y = 0;
				if ((i & 1) == 0)
				{
					v = (0xff & yuv420sp[uvp++]) - 128;
					u = (0xff & yuv420sp[uvp++]) - 128;
				}

				int y1192 = 1192 * y;
				int r = (y1192 + 1634 * v);
				int g = (y1192 - 833 * v - 400 * u);
				int b = (y1192 + 2066 * u);

				if (r < 0)
					r = 0;
				else if (r > 262143)
					r = 262143;
				if (g < 0)
					g = 0;
				else if (g > 262143)
					g = 262143;
				if (b < 0)
					b = 0;
				else if (b > 262143)
					b = 262143;

				rgb[yp] = 0xff000000 | ((r << 6) & 0xff0000)
						| ((g >> 2) & 0xff00) | ((b >> 10) & 0xff);
			}
		}
		return rgb;
	}
}
