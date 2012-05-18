cmake_minimum_required(VERSION 2.4)
project(ECWOLF)

set(OUTPUT_DIR ${CMAKE_BINARY_DIR} CACHE PATH "Directory in which to build ECWolf.")

set(LZMA_INCLUDE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/lzma/C")

option(INTERNAL_ZLIB "Force build with internal zlib" OFF)
option(INTERNAL_BZIP2 "Force build with internal bzip2" OFF)
option(INTERNAL_JPEG "Force build with internal libjpeg" OFF)

find_package(ZLIB)
find_package(BZip2)
find_package(JPEG)

if(NOT ZLIB_FOUND OR INTERNAL_ZLIB)
	set(ZLIB_INCLUDE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/zlib)
	add_subdirectory(zlib)
	set(ZLIB_LIBRARY z)
endif(NOT ZLIB_FOUND OR INTERNAL_ZLIB)

if(NOT BZIP2_FOUND OR INTERNAL_BZIP2)
	set(BZIP2_INCLUDE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/bzip2)
	add_subdirectory(bzip2)
	set(BZIP2_LIBRARIES bz2)
endif(NOT BZIP2_FOUND OR INTERNAL_BZIP2)

if(NOT JPEG_FOUND OR INTERNAL_JPEG)
	set(JPEG_INCLUDE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/jpeg-6b)
	add_subdirectory(jpeg-6b)
	set(JPEG_LIBRARIES jpeg)
	set(JPEG_LIBRARY jpeg)
endif(NOT JPEG_FOUND OR INTERNAL_JPEG)

if(MSVC)
	add_definitions(-D_CRT_SECURE_NO_DEPRECATE)
endif(MSVC)

add_subdirectory(tools/zipdir)
add_subdirectory(wadsrc)

add_subdirectory(gdtoa)
add_subdirectory(lzma)
add_subdirectory(src)
