package com.romainflash.androcopter;

import android.os.Bundle;
import android.view.KeyEvent;
import android.view.View;
import android.widget.CheckBox;
import android.widget.EditText;
import android.widget.Toast;
import android.app.Activity;
import android.content.SharedPreferences;

public class QuadcopterActivity extends Activity
{
	public static final long UI_REFRESH_PERIOD_MS = 250;
	public static final String PREFS_NAME = "MyPrefsFile";
	public static final String PREFS_ID_LAST_IP = "lastServerIP";

    @Override
    public void onCreate(Bundle savedInstanceState)
    {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);
        
        // Get UI elements.
        serverIpEditText = (EditText)findViewById(R.id.serverIpEditText);
        connectToServerCheckBox = (CheckBox)findViewById(R.id.connectToServerCheckBox);
        
        // In the "server IP" field, insert the last used IP address.
        SharedPreferences settings = getSharedPreferences(PREFS_NAME, 0);
        String lastIP = settings.getString(PREFS_ID_LAST_IP, "192.168.0.8");
        serverIpEditText.setText(lastIP);

        // Create the main controller.
        mainController = new MainController(this);
    }
    
    // Deactivate some buttons.
    public boolean onKeyDown(int keyCode, KeyEvent event)
    {
    	if(keyCode == KeyEvent.KEYCODE_CALL)
    		return true;
    	else
    		return super.onKeyDown(keyCode, event);
    }
    
	@Override
    protected void onResume()
    {
		super.onResume();
		
		// Prevent sleep mode.
		getWindow().addFlags(android.view.WindowManager.LayoutParams.FLAG_KEEP_SCREEN_ON);
		
		// Start the main controller.
		try
		{
			mainController.start();
		}
		catch (Exception e)
		{
			Toast.makeText(this, "The USB transmission could not start.",
						   Toast.LENGTH_SHORT).show();
			e.printStackTrace();
		}
		
		// Allow the user starting the TCP client.
		serverIpEditText.setEnabled(true);

		// Connect automatically to the computer.
		// This way, it is possible to start the communication just by plugging
		// the ADK (if the auto-start of this application is checked).
		connectToServerCheckBox.setChecked(true);
		onConnectToServerCheckBoxToggled(null);
    }

    @Override
    protected void onPause()
    {
		super.onPause();

    	// Reallow sleep mode.
		getWindow().clearFlags(android.view.WindowManager.LayoutParams.FLAG_KEEP_SCREEN_ON);
		
		// Stop the main controller.
		mainController.stop();
    }
    
    @Override
    protected void onStop()
    {
    	super.onStop();
    	
    	// Save the server IP.
    	SharedPreferences settings = getSharedPreferences(PREFS_NAME, 0);
        SharedPreferences.Editor editor = settings.edit();
        editor.putString(PREFS_ID_LAST_IP, serverIpEditText.getText().toString());
        editor.commit();
    }
    
    public void onConnectToServerCheckBoxToggled(View v)
    {
    	if(connectToServerCheckBox.isChecked()) // Connect.
    	{
    		mainController.startClient(serverIpEditText.getText().toString());
    		
    		serverIpEditText.setEnabled(false);
    	}
    	else // Disconnect.
    	{
    		serverIpEditText.setEnabled(true);
    		
    		mainController.stopClient();
    	}
    }

    private CheckBox connectToServerCheckBox;
    private EditText serverIpEditText;
    private MainController mainController;
}
