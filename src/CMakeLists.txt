include_directories(${ECWOLF_SOURCE_DIR}/src)

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
include_directories(${SDLMIXER_INCLUDE_DIR})
include_directories(${SDL_INCLUDE_DIR})

if(WIN32)
	set(EXTRA_FILES
		sdl_winmain.cpp
	)
endif(WIN32)

add_executable(../ecwolf
#	config.cpp
	file.cpp
	fmopl.cpp
	id_ca.cpp
	id_in.cpp
	id_pm.cpp
	id_sd.cpp
	id_us_1.cpp
	id_vh.cpp
	id_vl.cpp
	m_classes.cpp
#	scanner.cpp
	signon.cpp
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
	${EXTRA_FILES}
)

target_link_libraries(../ecwolf ${SDL_LIBRARY} ${SDLMIXER_LIBRARY})
