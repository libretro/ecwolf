if(COMMAND cmake_policy)
	cmake_policy(SET CMP0003 NEW)
endif(COMMAND cmake_policy)

include(CheckFunctionExists)
include(FindPkgConfig)

include_directories(
	${ECWolf_SOURCE_DIR}/src
	${ECWolf_SOURCE_DIR}/src/g_shared
	${LZMA_INCLUDE_DIR}
	${ECWolf_SOURCE_DIR}/gdtoa
	${CMAKE_BINARY_DIR}/gdtoa
)

option(GPL "Build GPL edition")

if(GPL)
	add_definitions(-DUSE_GPL)
	set(OPL_SOURCES dosbox/dbopl.cpp)
else(GPL)
	set(OPL_SOURCES mame/fmopl.cpp)
endif(GPL)

find_package(SDL REQUIRED)
find_package(SDL_mixer REQUIRED)
include_directories(${SDLMIXER_INCLUDE_DIR} ${SDL_INCLUDE_DIR} ${ZLIB_INCLUDE_DIR} ${BZIP2_INCLUDE_DIR} ${JPEG_INCLUDE_DIR})

check_function_exists(stricmp STRICMP_EXISTS)
check_function_exists(strnicmp STRNICMP_EXISTS)
if(NOT STRICMP_EXISTS)
	add_definitions(-Dstricmp=strcasecmp)
endif(NOT STRICMP_EXISTS)
if(NOT STRNICMP_EXISTS)
	add_definitions(-Dstrnicmp=strncasecmp)
endif(NOT STRNICMP_EXISTS)

if(CMAKE_COMPILER_IS_GNUCXX AND NOT APPLE)
	set(CMAKE_CXX_FLAGS "-Wall -Wno-unused -Wno-unused-result -Wno-ignored-qualifiers -Wextra -Wno-missing-field-initializers ${CMAKE_CXX_FLAGS}")
endif(CMAKE_COMPILER_IS_GNUCXX AND NOT APPLE)

if(WIN32)
	set(NO_GTK ON)
	set(GTK2_LIBRARIES)
	add_definitions(-DWINDOWS -DNO_STDIO_REDIRECT)

	set(EXTRA_FILES
		sdl_winmain.cpp
	)
elseif(APPLE)
	set(NO_GTK ON)
	set(GTK2_LIBRARIES)
else(WIN32)
	option( NO_GTK "Disable GTK+ dialogs (Not applicable to Windows)" )

	# Use GTK+ for the IWAD picker, if available.
	if( NOT NO_GTK )
		pkg_check_modules( GTK2 gtk+-2.0 )
		if( GTK2_FOUND )
			set( ZDOOM_LIBS ${ZDOOM_LIBS} ${GTK2_LIBRARIES} )
			include_directories( ${GTK2_INCLUDE_DIRS} )
		else( GTK2_FOUND )
			set( NO_GTK ON )
		endif( GTK2_FOUND )
	else( NOT NO_GTK)
		set(GTK2_LIBRARIES)
	endif( NOT NO_GTK )
endif(WIN32)

if(NO_GTK)
	add_definitions(-DNO_GTK=1)
endif(NO_GTK)

if(APPLE)
	set(EXTRA_FILES
		SDLMain.m
		wl_iwad_picker_cocoa.mm
	)
endif(APPLE)

add_executable(ecwolf #WIN32
	${OPL_SOURCES}
	g_shared/a_deathcam.cpp
	g_shared/a_inventory.cpp
	g_shared/a_keys.cpp
	g_shared/a_patrolpoint.cpp
	g_shared/a_playerpawn.cpp
	r_data/colormaps.cpp
	r_data/r_translate.cpp
	resourcefiles/resourcefile.cpp
	resourcefiles/ancientzip.cpp
	resourcefiles/file_7z.cpp
	resourcefiles/file_directory.cpp
	resourcefiles/file_grp.cpp
	resourcefiles/file_pak.cpp
	resourcefiles/file_rff.cpp
	resourcefiles/file_lump.cpp
	resourcefiles/file_audiot.cpp
	resourcefiles/file_gamemaps.cpp
	resourcefiles/file_vgagraph.cpp
	resourcefiles/file_vswap.cpp
	resourcefiles/file_wad.cpp
	resourcefiles/file_zip.cpp
	sfmt/SFMT.cpp
	textures/anim_switches.cpp
	textures/animations.cpp
	textures/automaptexture.cpp
	textures/bitmap.cpp
	textures/ddstexture.cpp
	textures/emptytexture.cpp
	textures/flattexture.cpp
	textures/imgztexture.cpp
	textures/jpegtexture.cpp
	textures/multipatchtexture.cpp
	textures/patchtexture.cpp
	textures/pcxtexture.cpp
	textures/pngtexture.cpp
	textures/rawpagetexture.cpp
	textures/solidtexture.cpp
	textures/texture.cpp
	textures/texturemanager.cpp
	textures/tgatexture.cpp
	textures/warptexture.cpp
	textures/wolfrawtexture.cpp
	textures/wolfshapetexture.cpp
	thingdef/thingdef.cpp
	thingdef/thingdef_codeptr.cpp
	thingdef/thingdef_expression.cpp
	thingdef/thingdef_properties.cpp
	thingdef/thingdef_type.cpp
	actor.cpp
	colormatcher.cpp
	config.cpp
	c_cvars.cpp
	dobject.cpp
	dobjgc.cpp
	farchive.cpp
	file.cpp
	files.cpp
	g_mapinfo.cpp
	gamemap.cpp
	gamemap_planes.cpp
	gamemap_uwmf.cpp
	id_ca.cpp
	id_in.cpp
	id_sd.cpp
	id_us_1.cpp
	id_vh.cpp
	id_vl.cpp
	language.cpp
	lnspec.cpp
	lumpremap.cpp
	m_alloc.cpp
	m_classes.cpp
	m_random.cpp
	m_png.cpp
	name.cpp
	p_switch.cpp
	r_sprites.cpp
	scanner.cpp
	sndinfo.cpp
	thinker.cpp
	v_font.cpp
	v_palette.cpp
	v_video.cpp
	w_wad.cpp
	weaponslots.cpp
	wl_act2.cpp
	wl_agent.cpp
	wl_atmos.cpp
	wl_cloudsky.cpp
	wl_debug.cpp
	wl_draw.cpp
	wl_floorceiling.cpp
	wl_game.cpp
	wl_inter.cpp
	wl_iwad.cpp
	wl_loadsave.cpp
	wl_main.cpp
	wl_menu.cpp
	wl_parallax.cpp
	wl_play.cpp
	wl_shade.cpp
	wl_state.cpp
	wl_text.cpp
	zstrformat.cpp
	zstring.cpp
	${EXTRA_FILES}
)

add_dependencies(ecwolf lzma gdtoa)
target_link_libraries(ecwolf ${GTK2_LIBRARIES} ${SDL_LIBRARY} ${SDLMIXER_LIBRARY} ${ZLIB_LIBRARY} ${BZIP2_LIBRARIES} ${JPEG_LIBRARIES} lzma gdtoa)
set_target_properties(ecwolf PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${OUTPUT_DIR})
