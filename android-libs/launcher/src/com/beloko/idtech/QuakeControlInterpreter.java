package com.beloko.idtech;

import java.io.IOException;
import java.util.HashMap;

import android.util.Log;
import android.view.KeyEvent;
import android.view.MotionEvent;

import com.beloko.idtech.GD.IDGame;
import com.beloko.idtech.QuakeControlConfig.Type;

public class QuakeControlInterpreter {

	String LOG = "QuakeControlInterpreter";

	QuakeControlInterface quakeIf;
	QuakeControlConfig config;

	boolean gamePadEnabled;

	HashMap<Integer, Boolean> analogButtonState = new HashMap<Integer, Boolean>(); //Saves current state of analog buttons so all sent each time
	
	public QuakeControlInterpreter(QuakeControlInterface qif,IDGame game,String controlfile,boolean ctrlEn)
	{
		Log.d("QuakeControlInterpreter", "file = " + controlfile);

		gamePadEnabled = ctrlEn;

		config = new QuakeControlConfig(controlfile,game);
		try {
			config.loadControls();
		} catch (IOException e) {
			// TODO Auto-generated catch block
			//e.printStackTrace();
		} catch (ClassNotFoundException e) {
			// TODO Auto-generated catch block
			//e.printStackTrace();
		}

		for (ActionInput ai: config.actions)
		{
			if ((ai.sourceType == Type.ANALOG) && ((ai.actionType == Type.MENU) || (ai.actionType == Type.BUTTON)))
			{
				analogButtonState.put(ai.actionCode, false);
			}
		}

		quakeIf = qif;
	}

	float deadRegion = 0.2f;
	private float analogCalibrate(float v)
	{
		if ((v < deadRegion) && (v > -deadRegion))
			return 0;
		else
		{
			if (v > 0)
				return(v-deadRegion) / (1-deadRegion);
			else
				return(v+deadRegion) / (1-deadRegion);
			//return v;
		}
	}

	GenericAxisValues genericAxisValues = new GenericAxisValues();


	//This is for normal Android motioon event
	public boolean onGenericMotionEvent(MotionEvent event) {
		genericAxisValues.setAndroidValues(event);

		return onGenericMotionEvent(genericAxisValues);
	}

	public boolean onGenericMotionEvent(GenericAxisValues event) {
		if (GD.DEBUG) Log.d(LOG,"onGenericMotionEvent" );

		boolean used = false;
		if (gamePadEnabled)
		{
			for (ActionInput ai: config.actions)
			{
				if ((ai.sourceType == Type.ANALOG) && (ai.source != -1))
				{
					int invert;
					invert = ai.invert?-1:1;
					if (ai.actionCode == QuakeControlConfig.ACTION_ANALOG_PITCH)
						quakeIf.analogPitch_if(QuakeControlConfig.LOOK_MODE_JOYSTICK, analogCalibrate(event.getAxisValue(ai.source)) * invert * ai.scale);
					else if (ai.actionCode == QuakeControlConfig.ACTION_ANALOG_YAW)
						quakeIf.analogYaw_if(QuakeControlConfig.LOOK_MODE_JOYSTICK, -analogCalibrate(event.getAxisValue(ai.source)) * invert * ai.scale);
					else if (ai.actionCode == QuakeControlConfig.ACTION_ANALOG_FWD)
						quakeIf.analogFwd_if(-analogCalibrate(event.getAxisValue(ai.source)) * invert * ai.scale);
					else if (ai.actionCode == QuakeControlConfig.ACTION_ANALOG_STRAFE)
						quakeIf.analogSide_if(analogCalibrate(event.getAxisValue(ai.source)) * invert * ai.scale);
					else //Must be using analog as a button
					{  
						if (GD.DEBUG) Log.d(LOG,"Analog as button" );

						if (GD.DEBUG) Log.d(LOG,ai.toString());

						if (((ai.sourcePositive) && (event.getAxisValue(ai.source)) > 0.5) ||
								((!ai.sourcePositive) && (event.getAxisValue(ai.source)) < -0.5) )
						{
							if (!analogButtonState.get(ai.actionCode)) //Check internal state, only send if different
							{
								quakeIf.doAction_if(1, ai.actionCode); //press
								analogButtonState.put(ai.actionCode, true);
							}
						}
						else
						{
							if (analogButtonState.get(ai.actionCode)) //Check internal state, only send if different
							{
								quakeIf.doAction_if(0, ai.actionCode); //un-press
								analogButtonState.put(ai.actionCode, false);
							}
						}

					}
					used = true;
				}
				/*
				//Menu buttons
				if ((ai.sourceType == Type.ANALOG) && (ai.actionType == Type.MENU) && (ai.source != -1))
				{
					if (GD.DEBUG) Log.d(LOG,"Analog as MENU button" );
					if (GD.DEBUG) Log.d(LOG,ai.toString());
					if (((ai.sourcePositive) && (event.getAxisValue(ai.source)) > 0.5) ||
							((!ai.sourcePositive) && (event.getAxisValue(ai.source)) < -0.5) )
						quakeIf.doAction_if(1, ai.actionCode); //press
					else
						quakeIf.doAction_if(0, ai.actionCode); //un-press
				}
				 */
			}

		}


		return used;

	}
}
