package com.beloko.idtech.wolf3d;

import com.beloko.idtech.AppSettings;
import com.beloko.idtech.GD.IDGame;
import com.beloko.idtech.QuakeControlInterpreter;
import com.beloko.idtech.QuakeCustomCommands;
import com.beloko.idtech.QuakeTouchControlsSettings;
import com.beloko.idtech.ShowKeyboard;
import com.beloko.idtech.Utils;

import org.libsdl.app.SDLActivity;

import android.app.Activity;
import android.os.Bundle;
import android.os.Handler;
import android.util.Log;
import android.view.KeyEvent;
import android.view.MotionEvent;
import android.view.View;

import com.bda.controller.Controller;
import com.bda.controller.ControllerListener;
import com.bda.controller.StateEvent;

public class Game extends SDLActivity
{
	static final String LOG = "ECWolf Activity";

	public static Activity act = null;

	private String args;
	private String gamePath;

	private ControlAdapter controlAdapter;
	static QuakeControlInterpreter controlInterp;

	private final MogaControllerListener mogaListener = new MogaControllerListener();
	Controller mogaController = null;

	protected void onCreate(Bundle savedInstanceState)
	{
		super.onCreate(savedInstanceState);

		controlAdapter = new ControlAdapter(mLayout.getChildAt(0));

		act = this;

		AppSettings.setGame(IDGame.Wolf3d);
		AppSettings.reloadSettings(getApplication());

		args = getIntent().getStringExtra("args");
		gamePath  = getIntent().getStringExtra("game_path");

		mogaController = Controller.getInstance(this);
		mogaController.init();
		mogaController.setListener(mogaListener,new Handler());

		Utils.copyPNGAssets(getApplicationContext(),AppSettings.graphicsDir);

		start_quake2();
	}

	protected String[] getArguments()
	{
		return Utils.creatArgs(gamePath + " " + AppSettings.graphicsDir + " " + args);
	}

	protected String[] getLibraries() {
		return new String[] {
			"ecwolf",
			"touchcontrols"
		};
    }

	public void start_quake2() {
		NativeLib engine = new NativeLib();

		ShowKeyboard.setup(act, null);

		controlInterp = new QuakeControlInterpreter(engine,AppSettings.game,AppSettings.gamePadControlsFile,AppSettings.gamePadEnabled);

		QuakeTouchControlsSettings.setup(act, engine);
		QuakeTouchControlsSettings.loadSettings(act);
		QuakeTouchControlsSettings.sendToQuake();

		QuakeCustomCommands.setup(act, engine,getIntent().getStringExtra("main_qc"),getIntent().getStringExtra("mod_qc"));
	}

	private class ControlAdapter implements View.OnGenericMotionListener, View.OnKeyListener, View.OnTouchListener
	{
		Object sdlSurface;

		// Intercept events from SDLSurface
		public ControlAdapter(View view)
		{
			sdlSurface = view;
			view.setOnGenericMotionListener(this);
			view.setOnKeyListener(this);
			view.setOnTouchListener(this);
		}

		public boolean onGenericMotion(View v, MotionEvent event)
		{
			return controlInterp.onGenericMotionEvent(event);
		}

		public boolean onKey(View  v, int keyCode, KeyEvent event)
		{
			switch(event.getAction())
			{
			case KeyEvent.ACTION_UP:
				controlInterp.onKeyUp(keyCode, event);
				break;
			case KeyEvent.ACTION_DOWN:
				controlInterp.onKeyDown(keyCode, event);
				break;
			}

			return ((View.OnKeyListener)sdlSurface).onKey(v, keyCode, event);
		}

		public boolean onTouch(View v, MotionEvent event)
		{
			controlInterp.onTouchEvent(event);
			return ((View.OnTouchListener)sdlSurface).onTouch(v, event);
		}
	}

	class MogaControllerListener implements ControllerListener {
		@Override
		public void onKeyEvent(com.bda.controller.KeyEvent event) {
			//Log.d(LOG,"onKeyEvent " + event.getKeyCode());
			controlInterp.onMogaKeyEvent(event,mogaController.getState(Controller.STATE_CURRENT_PRODUCT_VERSION));
		}

		@Override
		public void onMotionEvent(com.bda.controller.MotionEvent event) {
			// TODO Auto-generated method stub
			Log.d(LOG,"onGenericMotionEvent " + event.toString());
			controlInterp.onGenericMotionEvent(event);
		}

		@Override
		public void onStateEvent(StateEvent event) {
			Log.d(LOG,"onStateEvent " + event.getState());
		}
	}
}
