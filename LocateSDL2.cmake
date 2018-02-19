#
# LocateSDL2.cmake
#
#---------------------------------------------------------------------------
# Copyright 2018 Braden Obrzut
# All rights reserved.
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions
# are met:
#
# 1. Redistributions of source code must retain the above copyright
#    notice, this list of conditions and the following disclaimer.
# 2. Redistributions in binary form must reproduce the above copyright
#    notice, this list of conditions and the following disclaimer in the
#    documentation and/or other materials provided with the distribution.
# 3. The name of the author may not be used to endorse or promote products
#    derived from this software without specific prior written permission.
#
# THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
# IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
# OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
# IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
# INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
# NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
# DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
# THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
# (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
# THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
#---------------------------------------------------------------------------
#
# Due to a combination of factors there's not a good unified answer to the
# qustion "How do I find SDL2 in CMake."  Ideally one just does
# find_package(SDL2) and either has the sdl2-config.cmake in the magic location
# or will point SDL2_DIR to the directory containing it.
#
# But not only are many people not aware of how this works, the SDL subprojects
# don't follow the convention, we'd really like to use imported targets, we'd
# like to also support SDL 1.2, and we can just in general do more to make this
# seamless.

option(FORCE_SDL12 "Use SDL 1.2 instead of SDL2" OFF)
if(FORCE_SDL12)
	find_package(SDL)
	find_package(SDL_mixer)
	find_package(SDL_net)
	if(NOT SDL_FOUND OR NOT SDL_mixer_FOUND OR NOT SDL_net_FOUND)
		message(FATAL_ERROR "Could not find SDL. Please set SDL_* variables manually.")
	endif()

	set(SDL2_LIBRARIES ${SDL_LIBRARY})
	set(SDL2_INCLUDE_DIRS ${SDL_INCLUDE_DIR})
	set(SDL2_MIXER_LIBRARIES ${SDL_MIXER_LIBRARY})
	set(SDL2_MIXER_INCLUDE_DIRS ${SDL_MIXER_INCLUDE_DIR})
	set(SDL2_NET_LIBRARIES ${SDL_NET_LIBRARY})
	set(SDL2_NET_INCLUDE_DIRS ${SDL_NET_INCLUDE_DIR})
else()
	find_package(SDL2 QUIET)
	find_package(SDL2_mixer QUIET)
	find_package(SDL2_net QUIET)

	set(SDL_FIND_ERROR NO)

	if(NOT SDL2_FOUND)
		find_library(SDL2_LIBRARIES SDL2)
		find_path(SDL2_INCLUDE_DIRS SDL.h PATH_SUFFIXES SDL2)
		if(NOT SDL2_LIBRARIES OR NOT SDL2_INCLUDE_DIRS)
			message(SEND_ERROR "Please set SDL2_DIR to the location of sdl2-config.cmake.")
			set(SDL_FIND_ERROR YES)
		endif()
	endif()
	message(STATUS "Using SDL2: ${SDL2_LIBRARIES}, ${SDL2_INCLUDE_DIRS}")

	if(NOT SDL2_mixer_FOUND)
		find_library(SDL2_MIXER_LIBRARIES SDL2_mixer)
		find_path(SDL2_MIXER_INCLUDE_DIRS SDL_mixer.h PATH_SUFFIXES SDL2)
		if(NOT SDL2_MIXER_LIBRARIES OR NOT SDL2_MIXER_INCLUDE_DIRS)
			message(SEND_ERROR "Please set SDL2_mixer_DIR to the location of sdl2_mixer-config.cmake.")
			set(SDL_FIND_ERROR YES)
		endif()
	endif()
	message(STATUS "Using SDL2_mixer: ${SDL2_MIXER_LIBRARIES}, ${SDL2_MIXER_INCLUDE_DIRS}")

	if(NOT SDL2_net_FOUND)
		find_library(SDL2_NET_LIBRARIES SDL2_net)
		find_path(SDL2_NET_INCLUDE_DIRS SDL_net.h PATH_SUFFIXES SDL2)
		if(NOT SDL2_NET_LIBRARIES OR NOT SDL2_NET_INCLUDE_DIRS)
			message(SEND_ERROR "Please set SDL2_net_DIR to the location of sdl2_net-config.cmake.")
			set(SDL_FIND_ERROR YES)
		endif()
	endif()
	message(STATUS "Using SDL2_net: ${SDL2_NET_LIBRARIES}, ${SDL2_NET_INCLUDE_DIRS}")

	if(SDL_FIND_ERROR)
		message(FATAL_ERROR "One or more required SDL libraries were not found.")
	endif()
endif()

# Function converts loose variables to a modern style target
function(sdl_modernize NEW_TARGET LIBS DIRS)
	# Lets be hopeful that eventually a target will appear upstream
	if(NOT TARGET ${NEW_TARGET})
		add_library(${NEW_TARGET} UNKNOWN IMPORTED)

		list(GET ${LIBS} 0 LIB)
		list(REMOVE_AT ${LIBS} 0)

		# On Linux SDL2's sdl2-config.cmake will specify -lSDL2 so we need to find that for CMake otherwise the target won't work
		if(${LIB} MATCHES "^-l")
			string(SUBSTRING ${LIB} 2 -1 LIB)
			find_library(LIB2 ${LIB})
			set(LIB ${LIB2})
		endif()

		set_property(TARGET ${NEW_TARGET} PROPERTY IMPORTED_LOCATION ${LIB})
		if(LIBS)
			set_property(TARGET ${NEW_TARGET} APPEND PROPERTY INTERFACE_LINK_LIBRARIES ${${LIBS}})
		endif()
		set_property(TARGET ${NEW_TARGET} APPEND PROPERTY INTERFACE_INCLUDE_DIRECTORIES ${${DIRS}})
	endif()
endfunction()

sdl_modernize(SDL::SDL SDL2_LIBRARIES SDL2_INCLUDE_DIRS)
sdl_modernize(SDL::SDL_mixer SDL2_MIXER_LIBRARIES SDL2_MIXER_INCLUDE_DIRS)
sdl_modernize(SDL::SDL_net SDL2_NET_LIBRARIES SDL2_NET_INCLUDE_DIRS)
