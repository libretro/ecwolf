cmake_minimum_required( VERSION 2.4 )

if( CMAKE_COMPILER_IS_GNUC )
	set( CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -fomit-frame-pointer" )
endif( CMAKE_COMPILER_IS_GNUC )

#add_definitions( -DBZ_NO_STDIO )
if(MSVC)
	add_definitions(-D_CRT_SECURE_NO_DEPRECATE -D_CRT_NONSTDC_NO_DEPRECATE)
endif(MSVC)

add_library( bz2
    blocksort.c
    bzlib.c
    compress.c
    crctable.c
    decompress.c
    huffman.c
    randtable.c )
target_link_libraries( bz2 )
