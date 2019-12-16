add_custom_command(OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/com/beloko/idtech/R.java"
	COMMAND "${ANDROID_AAPT_BINARY}" package -m -J ${CMAKE_CURRENT_BINARY_DIR} -M ../AndroidManifest.xml -S ../res -I "${ANDROID_SDK_JAR}" --custom-package com.beloko.idtech
	DEPENDS ../AndroidManifest.xml ../res
	WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
	COMMENT "Generating resources"
)

add_jar(engine-android
	${CMAKE_CURRENT_BINARY_DIR}/com/beloko/idtech/R.java
	com/beloko/idtech/wolf3d/Game.java
	com/beloko/idtech/wolf3d/NativeLib.java
	com/beloko/idtech/AboutDialog.java
	com/beloko/idtech/ActionInput.java
	com/beloko/idtech/AlphanumComparator.java
	com/beloko/idtech/AppSettings.java
	com/beloko/idtech/CDAudioPlayer.java
	com/beloko/idtech/DirectoryChooserDialog.java
	com/beloko/idtech/GamePadFragment.java
	com/beloko/idtech/GD.java
	com/beloko/idtech/GenericAxisValues.java
	com/beloko/idtech/IntroDialog.java
	com/beloko/idtech/NoticeDialog.java
	com/beloko/idtech/OptionsFragment.java
	com/beloko/idtech/QuakeControlConfig.java
	com/beloko/idtech/QuakeControlInterface.java
	com/beloko/idtech/QuakeControlInterpreter.java
	com/beloko/idtech/QuakeCustomCommands.java
	com/beloko/idtech/QuakeQuickCommand.java
	com/beloko/idtech/QuakeTouchControlsSettings.java
	com/beloko/idtech/ShowKeyboard.java
	com/beloko/idtech/SingleMediaScanner.java
	com/beloko/idtech/TouchControlsEditing.java
	com/beloko/idtech/Utils.java
	com/beloko/wolf3d/AboutDialog.java
	com/beloko/wolf3d/Advert.java
	com/beloko/wolf3d/EntryActivity.java
	com/beloko/wolf3d/LaunchFragment.java
	com/mobeta/android/dslv/DragSortController.java
	com/mobeta/android/dslv/DragSortCursorAdapter.java
	com/mobeta/android/dslv/DragSortItemView.java
	com/mobeta/android/dslv/DragSortItemViewCheckable.java
	com/mobeta/android/dslv/DragSortListView.java
	com/mobeta/android/dslv/ResourceDragSortCursorAdapter.java
	com/mobeta/android/dslv/SimpleDragSortCursorAdapter.java
	com/mobeta/android/dslv/SimpleFloatViewManager.java
	org/libsdl/app/HIDDeviceBLESteamController.java
	org/libsdl/app/HIDDevice.java
	org/libsdl/app/HIDDeviceManager.java
	org/libsdl/app/HIDDeviceUSB.java
	org/libsdl/app/SDL.java
	org/libsdl/app/SDLAudioManager.java
	org/libsdl/app/SDLActivity.java
	org/libsdl/app/SDLControllerManager.java
	INCLUDE_JARS
		"${ANDROID_SDK_JAR}"
		"${ANDROID_SUPPORT_V4_JAR}"
)

add_dependencies(engine-android engine touchcontrols pk3)

add_custom_command(TARGET engine-android POST_BUILD
	COMMAND "${CMAKE_COMMAND}" -E copy_if_different "$<TARGET_FILE:engine>" "${CMAKE_CURRENT_BINARY_DIR}/lib/${CMAKE_ANDROID_ARCH_ABI}/$<TARGET_FILE_NAME:engine>"
	COMMAND "${CMAKE_COMMAND}" -E copy_if_different "$<TARGET_FILE:touchcontrols>" "${CMAKE_CURRENT_BINARY_DIR}/lib/${CMAKE_ANDROID_ARCH_ABI}/$<TARGET_FILE_NAME:touchcontrols>"
	COMMAND "${CMAKE_COMMAND}" -E copy_if_different "$<TARGET_FILE:hidapi>" "${CMAKE_CURRENT_BINARY_DIR}/lib/${CMAKE_ANDROID_ARCH_ABI}/$<TARGET_FILE_NAME:hidapi>"
	COMMAND "${CMAKE_COMMAND}" -E copy_directory "${CMAKE_CURRENT_SOURCE_DIR}/../assets" "${OUTPUT_DIR}/assets"
	COMMAND "${ANDROID_AAPT_BINARY}" package -f $<$<CONFIG:DEBUG>:--debug-mode> -M "${CMAKE_CURRENT_SOURCE_DIR}/../AndroidManifest.xml" -S "${CMAKE_CURRENT_SOURCE_DIR}/../res" -A "${OUTPUT_DIR}/assets" -I "${ANDROID_SDK_JAR}" -F "${APK_OUTPUT}"
	COMMAND "${ANDROID_DX_BINARY}" --dex "--output=classes.dex"
		"engine-android.jar"
		"${ANDROID_SUPPORT_V4_JAR}"
	COMMAND "${ANDROID_AAPT_BINARY}" add -f "${APK_OUTPUT}" "classes.dex" "lib/${CMAKE_ANDROID_ARCH_ABI}/*"
	COMMAND "${ANDROID_JAR_SIGNER}" -storepass "${ANDROID_SIGN_STOREPASS}" -keystore "${ANDROID_SIGN_KEYSTORE}" -digestalg SHA1 -sigalg MD5withRSA "${APK_OUTPUT}" "${ANDROID_SIGN_KEYNAME}"
	WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}"
	COMMENT "Creating APK ${ENGINE_BINARY_NAME}.apk"
)
