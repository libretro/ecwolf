cmake_minimum_required(VERSION 2.4)
project(ECWOLF)

set(OUTPUT_DIR ${CMAKE_BINARY_DIR} CACHE PATH "Directory in which to build ECWolf.")

set(LZMA_INCLUDE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/lzma/C")

find_package(ZLIB)
find_package(BZip2)

if(NOT ZLIB_FOUND)
	set(ZLIB_INCLUDE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/zlib)
	add_subdirectory(zlib)
	set(ZLIB_LIBRARY z)
endif(NOT ZLIB_FOUND)

if(NOT BZIP2_FOUND)
	set(BZIP2_INCLUDE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/bzip2)
	add_subdirectory(bzip2)
	set(BZIP2_LIBRARIES bz2)
endif(NOT BZIP2_FOUND)

add_subdirectory(tools/zipdir)

add_custom_command( OUTPUT ${CMAKE_CURRENT_SOURCE_DIR}/ecwolf.pk3
   COMMAND ${CMAKE_BINARY_DIR}/tools/zipdir/zipdir -udf ${CMAKE_CURRENT_SOURCE_DIR}/ecwolf.pk3 ${CMAKE_CURRENT_SOURCE_DIR}/wadsrc
   DEPENDS zipdir ${CMAKE_CURRENT_SOURCE_DIR}/wadsrc )

add_custom_target( pk3 ALL
	DEPENDS ${OUTPUT_DIR}/ecwolf.pk3 )

add_subdirectory(gdtoa)
add_subdirectory(lzma)
add_subdirectory(src)
