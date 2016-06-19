cmake_minimum_required( VERSION 2.8.7 )

if(POLICY CMP0054)
	cmake_policy(SET CMP0054 NEW)
endif()

#make_release_only()

if( "${CMAKE_C_COMPILER_ID}" STREQUAL "GNU" OR "${CMAKE_C_COMPILER_ID}" STREQUAL "Clang" )
	set( CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -Wextra -fomit-frame-pointer" )
endif()

set( LZMA_FILES
	C/7zBuf.c
	C/7zCrc.c
	C/7zCrcOpt.c
	C/7zDec.c
	C/7zIn.c
	C/7zStream.c
	C/Bcj2.c
	C/Bra.c
	C/Bra86.c
	C/CpuArch.c
	C/LzFind.c
	C/Lzma2Dec.c
	C/LzmaDec.c
	C/LzmaEnc.c )

if( WIN32 )
	set( LZMA_FILES ${LZMA_FILES} C/LzFindMt.c C/Threads.c )
else()
	set( CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -D_7ZIP_ST" )
endif()

add_library( lzma ${LZMA_FILES} )
target_link_libraries( lzma )
