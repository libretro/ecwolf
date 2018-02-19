cmake_minimum_required(VERSION 2.8.12)
project(ECWolf)

if(POLICY CMP0054)
	cmake_policy(SET CMP0054 NEW)
endif()

# Multiarch support
if(UNIX AND NOT APPLE)
	include(GNUInstallDirs)
elseif(NOT DEFINED CMAKE_INSTALL_LIBDIR)
	# CMAKE_ prefix used here to match the variable set by GNUInstallDirs.
	# Value currently doesn't matter outside of Linux.
	set(CMAKE_INSTALL_LIBDIR "lib")
endif()

option(ANDROID "Build for Android" OFF)

# Support cross compiling
option( FORCE_CROSSCOMPILE "Turn on cross compiling." NO )
if( FORCE_CROSSCOMPILE )
	set( CMAKE_CROSSCOMPILING TRUE )
endif()

if(CMAKE_CROSSCOMPILING)
	set(IMPORT_EXECUTABLES "IMPORTFILE-NOTFOUND" CACHE FILEPATH "Export file from native build.")
	include(${IMPORT_EXECUTABLES})
endif()

# Replacement variables for a possible long list of C/C++ compilers compatible with GCC
if( "${CMAKE_C_COMPILER_ID}" STREQUAL "GNU" OR "${CMAKE_C_COMPILER_ID}" STREQUAL "Clang" )
	set( ZD_CMAKE_COMPILER_IS_GNUC_COMPATIBLE TRUE )
else()
	set( ZD_CMAKE_COMPILER_IS_GNUC_COMPATIBLE FALSE )
endif()

if( "${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU" OR "${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang" )
	set( ZD_CMAKE_COMPILER_IS_GNUCXX_COMPATIBLE TRUE )
else()
	set( ZD_CMAKE_COMPILER_IS_GNUCXX_COMPATIBLE FALSE )
endif()

# Simplify pk3 building, add_pk3(filename srcdirectory)
function( add_pk3 PK3_NAME PK3_DIR )
	# Generate target name. Just use "pk3" for main pk3 target.
	string( REPLACE "." "_" PK3_TARGET ${PK3_NAME} )
	if( ${PK3_TARGET} STREQUAL "ecwolf_pk3" )
		set( PK3_TARGET "pk3" )
	endif()

	# Touch the zipdir executable here so that the pk3s are forced to rebuild
	# each time since their dependecy has "changed."
	if( NOT ANDROID )
		add_custom_target( ${PK3_TARGET} ALL
			COMMAND zipdir -udf $<TARGET_FILE_DIR:ecwolf>/${PK3_NAME} ${PK3_DIR}
			DEPENDS zipdir )
	else()
		# Prepare assets directory for APK
		add_custom_target( ${PK3_TARGET} ALL
			COMMAND zipdir -udf ${OUTPUT_DIR}/assets/${PK3_NAME} ${PK3_DIR}
			DEPENDS zipdir )
	endif()
endfunction()

set(OUTPUT_DIR ${CMAKE_BINARY_DIR} CACHE PATH "Directory in which to build ECWolf.")

set(LZMA_INCLUDE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/deps/lzma/C")

option(GPL "Build GPL edition" ON)
option(USE_LIBTEXTSCREEN "Use libtextscreen instead of console iwad picker." OFF)

option(INTERNAL_ZLIB "Force build with internal zlib" OFF)
option(INTERNAL_BZIP2 "Force build with internal bzip2" OFF)
option(INTERNAL_JPEG "Force build with internal libjpeg" OFF)

find_package(ZLIB QUIET)
find_package(BZip2 QUIET)
find_package(JPEG QUIET)

if(NOT ZLIB_FOUND OR INTERNAL_ZLIB)
	message(STATUS "Using internal zlib")
	set(ZLIB_INCLUDE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/deps/zlib)
	add_subdirectory(deps/zlib)
	set(ZLIB_LIBRARIES z)
	set(ZLIB_LIBRARY z)
else()
	message(STATUS "Using system zlib, includes found at ${ZLIB_INCLUDE_DIR}")
endif()

if(NOT BZIP2_FOUND OR INTERNAL_BZIP2)
	message(STATUS "Using internal libbz2")
	set(BZIP2_INCLUDE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/deps/bzip2)
	add_subdirectory(deps/bzip2)
	set(BZIP2_LIBRARIES bz2)
else()
	message(STATUS "Using system libbz2, includes found at ${BZIP2_INCLUDE_DIR}")
endif()

if(NOT JPEG_FOUND OR INTERNAL_JPEG)
	message(STATUS "Using internal libjpeg")
	set(JPEG_INCLUDE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/deps/jpeg-6b)
	add_subdirectory(deps/jpeg-6b)
	set(JPEG_LIBRARIES jpeg)
	set(JPEG_LIBRARY jpeg)
else()
	message(STATUS "Using system libjpeg, includes found at ${JPEG_INCLUDE_DIR}")
endif()

include(LocateSDL2.cmake)

if(MSVC)
	add_definitions(-D_CRT_SECURE_NO_DEPRECATE -D_CRT_NONSTDC_NO_DEPRECATE)
endif()

add_subdirectory(tools/zipdir)
add_subdirectory(tools/updaterevision)
add_subdirectory(wadsrc)

add_subdirectory(deps/gdtoa)
add_subdirectory(deps/lzma)
if(GPL AND USE_LIBTEXTSCREEN)
	add_subdirectory(deps/textscreen)
endif()

if(ANDROID)
	set(LIBPNG_INCLUDE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/android-libs)
	set(LIBPNG_LIBRARY png16_static)
	set(SIGCPP_INCLUDE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/android-libs/sigc++)
	set(SIGCPP_LIBRARY sigc++)
	set(TINYXML_INCLUDE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/android-libs/TinyXML)
	set(TINYXML_LIBRARY TinyXML)
	set(TOUCHCONTROLS_INCLUDE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/android-libs/TouchControls)
	set(TOUCHCONTROLS_LIBRARY touchcontrols)

	add_subdirectory(android-libs/libpng)
	add_subdirectory(android-libs/sigc++)
	add_subdirectory(android-libs/TinyXML)
	add_subdirectory(android-libs/TouchControls)
	add_subdirectory(android-libs/launcher)
endif()

add_subdirectory(src)

if( NOT CMAKE_CROSSCOMPILING )
	export(TARGETS ${CROSS_EXPORTS} FILE "${CMAKE_BINARY_DIR}/ImportExecutables.cmake" )
endif()
