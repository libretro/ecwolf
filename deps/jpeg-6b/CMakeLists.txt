cmake_minimum_required( VERSION 2.8.12 )

if(LIBRETRO AND NOT LIBRETRO_STATIC)
	set(CMAKE_POSITION_INDEPENDENT_CODE ON)
endif()

if( CMAKE_COMPILER_IS_GNUC )
	set( CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -Wextra -fomit-frame-pointer" )
endif()

add_library( jpeg STATIC
    jcomapi.c
    jdapimin.c
    jdapistd.c
    jdatasrc.c
    jdcoefct.c
    jdcolor.c
    jddctmgr.c
    jdhuff.c
    jdinput.c
    jdmainct.c
    jdmarker.c
    jdmaster.c
    jdmerge.c
    jdphuff.c
    jdpostct.c
    jdsample.c
    jerror.c
    jidctint.c
    jmemmgr.c
    jutils.c )
target_link_libraries( jpeg )
