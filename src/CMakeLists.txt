if(COMMAND cmake_policy)
	cmake_policy(SET CMP0003 NEW)
endif(COMMAND cmake_policy)

include(CheckFunctionExists)

include_directories(${ECWOLF_SOURCE_DIR}/src ${LZMA_INCLUDE_DIR} ${CMAKE_BINARY_DIR}/gdtoa)

option(SPEAR "Spear of Destiny")
option(SPEARDEMO "Spear of Destiny Demo")
option(WOLF3DSW "Wolfenstein 3D Shareware")

if(SPEAR)
	add_definitions(-DSPEAR)
	if(SPEARDEMO)
		add_definitions(-DSPEARDEMO)
	endif(SPEARDEMO)
else(SPEAR)
	if(WOLF3DSW)
		add_definitions(-DUPLOAD)
	endif(WOLF3DSW)
endif(SPEAR)

find_package(SDL REQUIRED)
find_package(SDL_mixer REQUIRED)
include_directories(${SDLMIXER_INCLUDE_DIR} ${SDL_INCLUDE_DIR} ${ZLIB_INCLUDE_DIR} ${BZIP2_INCLUDE_DIR})

check_function_exists(stricmp STRICMP_EXISTS)
check_function_exists(strnicmp STRNICMP_EXISTS)
if(NOT STRICMP_EXISTS)
	add_definitions(-Dstricmp=strcasecmp)
endif(NOT STRICMP_EXISTS)
if(NOT STRNICMP_EXISTS)
	add_definitions(-Dstrnicmp=strncasecmp)
endif(NOT STRNICMP_EXISTS)

if(WIN32)
	add_definitions(-DWINDOWS)

	set(EXTRA_FILES
		sdl_winmain.cpp
	)
endif(WIN32)

add_executable(../ecwolf
	resourcefiles/resourcefile.cpp
	resourcefiles/ancientzip.cpp
	resourcefiles/file_7z.cpp
	resourcefiles/file_directory.cpp
	resourcefiles/file_grp.cpp
	resourcefiles/file_pak.cpp
	resourcefiles/file_rff.cpp
	resourcefiles/file_lump.cpp
	resourcefiles/file_audiot.cpp
	resourcefiles/file_vgagraph.cpp
	resourcefiles/file_vswap.cpp
	resourcefiles/file_wad.cpp
	resourcefiles/file_zip.cpp
	config.cpp
	file.cpp
	files.cpp
	fmopl.cpp
	id_ca.cpp
	id_in.cpp
	id_pm.cpp
	id_sd.cpp
	id_us_1.cpp
	id_vh.cpp
	id_vl.cpp
	language.cpp
	lumpremap.cpp
	m_classes.cpp
	scanner.cpp
	sndinfo.cpp
#	tiles.cpp
	w_wad.cpp
	wl_act1.cpp
	wl_act2.cpp
	wl_agent.cpp
	wl_atmos.cpp
	wl_cloudsky.cpp
	wl_debug.cpp
	wl_draw.cpp
	wl_floorceiling.cpp
	wl_game.cpp
	wl_inter.cpp
	wl_main.cpp
	wl_menu.cpp
	wl_parallax.cpp
	wl_play.cpp
	wl_state.cpp
	wl_text.cpp
	zstrformat.cpp
	zstring.cpp
	${EXTRA_FILES}
)

add_dependencies(../ecwolf lzma gdtoa)

target_link_libraries(../ecwolf ${SDL_LIBRARY} ${SDLMIXER_LIBRARY} ${ZLIB_LIBRARY} ${BZIP2_LIBRARIES} lzma gdtoa)
