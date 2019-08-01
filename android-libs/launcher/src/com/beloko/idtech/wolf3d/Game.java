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

public class Game extends SDLActivity
{
	static final String LOG = "ECWolf Activity";

	public static Activity act = null;

	private String args;
	private String gamePath;

	static QuakeControlInterpreter controlInterp;

	protected void onCreate(Bundle savedInstanceState)
	{
		super.onCreate(savedInstanceState);

		act = this;

		AppSettings.setGame(IDGame.Wolf3d);
		AppSettings.reloadSettings(getApplication());

		args = getIntent().getStringExtra("args");
		gamePath  = getIntent().getStringExtra("game_path");

		Utils.copyPNGAssets(getApplicationContext(),AppSettings.graphicsDir);

		start_quake2();
	}

	protected String[] getArguments()
	{
		return Utils.creatArgs(gamePath + " " + AppSettings.graphicsDir + " " + args);
	}

	protected String[] getLibraries() {
		return new String[] {
			"hidapi",
			"touchcontrols",
			"ecwolf"
		};
    }

	protected String getMainSharedObject() {
		return getContext().getApplicationInfo().nativeLibraryDir + "/libecwolf.so";
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
}
