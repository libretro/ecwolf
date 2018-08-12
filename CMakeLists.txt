
cmake_minimum_required(VERSION 2.8.12)
project(ECWolf)

if(POLICY CMP0054)
	cmake_policy(SET CMP0054 NEW)
endif()

include(src/versiondefs.cmake)

# We need a newer CPack version to get desirable results.
# Note: CMake 3.11+ recommended for best results
if(NOT CMAKE_VERSION VERSION_LESS "3.6.0")
	set(USE_CPACK ON)
else()
	set(USE_CPACK OFF)
endif()

if(USE_CPACK)
	# Right now all we support is DEB, but we'll probably want to make this a
	# user option later?
	set(CPACK_GENERATOR "DEB")
	set(CPACK_PACKAGE_CONTACT "First Last <example@example.com>" CACHE STRING "Contact info for archive maintainer.")
	set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "Advanced source port of the Wolfenstein 3D engine.\n Provides enhanced game engine for playing Wolfenstein 3D, Spear of Destiny, and\n Super 3-D Noah's Ark. Requires the original game data files.")
	string(TOLOWER "${PRODUCT_NAME}" CPACK_PACKAGE_NAME)
	# Use same prefix for packaging for consistency
	set(CPACK_PACKAGING_INSTALL_PREFIX "${CMAKE_INSTALL_PREFIX}")
	set(CPACK_STRIP_FILES ON)
	set(CPACK_INSTALL_DEFAULT_DIRECTORY_PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE GROUP_READ GROUP_EXECUTE WORLD_READ WORLD_EXECUTE)
	set(CPACK_PACKAGE_VERSION_MAJOR ${VERSION_MAJOR})
	set(CPACK_PACKAGE_VERSION_MINOR ${VERSION_MINOR})
	set(CPACK_PACKAGE_VERSION_PATCH ${VERSION_PATCH})

	set(CPACK_DEB_COMPONENT_INSTALL ON)
	set(CPACK_DEBIAN_FILE_NAME "DEB-DEFAULT")
	set(CPACK_DEBIAN_PACKAGE_HOMEPAGE "https://maniacsvault.net/ecwolf/")
	set(CPACK_DEBIAN_PACKAGE_SECTION "games")
	set(CPACK_DEBIAN_PACKAGE_SHLIBDEPS ON)
	set(CPACK_DEBIAN_PACKAGE_CONTROL_STRICT_PERMISSION ON)
	set(CPACK_DEBIAN_COMPRESSION_TYPE "xz")
	include(CPack)
endif()

# Multiarch support
if(UNIX AND NOT APPLE)
	# We are in the games section
	set(CMAKE_INSTALL_BINDIR "games")

	include(GNUInstallDirs)
elseif(NOT DEFINED CMAKE_INSTALL_LIBDIR)
	# CMAKE_ prefix used here to match the variable set by GNUInstallDirs.
	# Value currently doesn't matter outside of Linux these are more or less
	# to keep the script working where values are expected.
	set(CMAKE_INSTALL_BINDIR "bin")
	set(CMAKE_INSTALL_DATAROOTDIR "share")
	set(CMAKE_INSTALL_LIBDIR "lib")
	set(CMAKE_INSTALL_MANDIR "share/man")
	set(CMAKE_INSTALL_FULL_DATADIR "${CMAKE_INSTALL_PREFIX}/${CMAKE_INSTALL_DATAROOTDIR}")
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
		install(FILES $<TARGET_FILE_DIR:ecwolf>/${PK3_NAME} DESTINATION "${CMAKE_INSTALL_FULL_DATADIR}/${PRODUCT_DIRECTORY}")
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
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/deps/SDL/CMakeLists.txt")
	# If user checked out the submodule then they probably want to use it
	option(INTERNAL_SDL "Force build with internal SDL2" ON)
else()
	set(INTERNAL_SDL OFF)
endif()
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/deps/SDL_mixer/CMakeLists.txt")
	# If user checked out the submodule then they probably want to use it
	option(INTERNAL_SDL_MIXER "Force build with internal SDL2_mixer" ON)
else()
	set(INTERNAL_SDL_MIXER OFF)
endif()
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/deps/SDL_net/CMakeLists.txt")
	# If user checked out the submodule then they probably want to use it
	option(INTERNAL_SDL_NET "Force build with internal SDL2_net" ON)
else()
	set(INTERNAL_SDL_NET OFF)
endif()

if(NOT INTERNAL_ZLIB)
	find_package(ZLIB QUIET)
endif()
if(NOT INTERNAL_BZIP2)
	find_package(BZip2 QUIET)
endif()
if(NOT INTERNAL_JPEG)
	find_package(JPEG QUIET)
endif()

if(NOT ZLIB_FOUND OR INTERNAL_ZLIB)
	message(STATUS "Using internal zlib")
	set(ZLIB_INCLUDE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/deps/zlib)
	add_subdirectory(deps/zlib)
	set(ZLIB_LIBRARIES ZLIB::ZLIB)
	set(ZLIB_LIBRARY ZLIB::ZLIB)
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

if(INTERNAL_SDL)
	set(SDL2_DIR "${CMAKE_CURRENT_SOURCE_DIR}/deps")
	add_subdirectory(deps/SDL)
endif()
if(INTERNAL_SDL_MIXER)
	set(SDL2_mixer_DIR "${CMAKE_CURRENT_SOURCE_DIR}/deps")
	add_subdirectory(deps/SDL_mixer)
endif()
if(INTERNAL_SDL_NET)
	set(SDL2_net_DIR "${CMAKE_CURRENT_SOURCE_DIR}/deps")
	add_subdirectory(deps/SDL_net)
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

if(USE_CPACK AND NOT APPLE AND NOT WIN32)
	install(FILES docs/changelog DESTINATION "${CMAKE_INSTALL_DATAROOTDIR}/doc/${CPACK_PACKAGE_NAME}")
	install(FILES docs/copyright DESTINATION "${CMAKE_INSTALL_DATAROOTDIR}/doc/${CPACK_PACKAGE_NAME}")
	install(FILES docs/ecwolf.6 DESTINATION "${CMAKE_INSTALL_MANDIR}/man6")

	# Lintian requires that man pages in deb packages are compressed.
	find_program(GZIP_BINARY NAMES gzip DOC "gzip program for man page compression.")
	if(GZIP_BINARY)
		install(CODE "execute_process(COMMAND \"${GZIP_BINARY}\" -n9 \"\$ENV{DESTDIR}\${CMAKE_INSTALL_PREFIX}/${CMAKE_INSTALL_DATAROOTDIR}/doc/${CPACK_PACKAGE_NAME}/changelog\")")
		install(CODE "execute_process(COMMAND \"${GZIP_BINARY}\" -n9 \"\$ENV{DESTDIR}\${CMAKE_INSTALL_PREFIX}/${CMAKE_INSTALL_MANDIR}/man6/ecwolf.6\")")
	endif()
endif()

if( NOT CMAKE_CROSSCOMPILING )
	export(TARGETS ${CROSS_EXPORTS} FILE "${CMAKE_BINARY_DIR}/ImportExecutables.cmake" )
endif()
