include_directories(${ECWOLF_SOURCE_DIR}/src)

find_package(SDL)
find_package(SDL_mixer)
if(NOT SDL_FOUND OR NOT SDLMIXER_FOUND)
	message(SEND_ERROR "SDL(_mixer) is required for building.")
endif(NOT SDL_FOUND OR NOT SDLMIXER_FOUND)
include_directories(${SDLMIXER_INCLUDE_DIR})
include_directories(${SDL_INCLUDE_DIR})

add_executable(../ecwolf
	fmopl.cpp
	id_ca.cpp
	id_in.cpp
	id_pm.cpp
	id_sd.cpp
	id_us_1.cpp
	id_vh.cpp
	id_vl.cpp
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
	actors/scanner.cpp
)

target_link_libraries(../ecwolf ${SDL_LIBRARY} ${SDLMIXER_LIBRARY})
