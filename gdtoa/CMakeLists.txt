cmake_minimum_required( VERSION 2.8.7 )

if(POLICY CMP0054)
	cmake_policy(SET CMP0054 NEW)
endif()

set( CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} -D_DEBUG" )

# Disable warnings for << operator precedence (4554) and
# unreferenced labels (4102) from VC
if( MSVC )
	set( CMAKE_C_FLAGS "${CMAKE_C_FLAGS} /wd4554 /wd4102" )
endif()

if( "${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU" OR "${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang" )
	set( CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra" )
endif()

include_directories( ${CMAKE_CURRENT_BINARY_DIR} )
add_definitions( -DINFNAN_CHECK -DMULTIPLE_THREADS )

if( NOT MSVC AND NOT APPLE )
	if( NOT CMAKE_CROSSCOMPILING )
		add_executable( arithchk arithchk.c )
	endif()
	get_target_property( ARITHCHK_EXE arithchk LOCATION )
	add_custom_command( OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/arith.h 
		COMMAND ${ARITHCHK_EXE} >${CMAKE_CURRENT_BINARY_DIR}/arith.h
		DEPENDS arithchk )

	if( NOT CMAKE_CROSSCOMPILING )
		add_executable( qnan qnan.c arith.h )
		set( CROSS_EXPORTS ${CROSS_EXPORTS} arithchk qnan PARENT_SCOPE )
	endif()
	get_target_property( QNAN_EXE qnan LOCATION )
	add_custom_command( OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/gd_qnan.h
		COMMAND ${QNAN_EXE} >${CMAKE_CURRENT_BINARY_DIR}/gd_qnan.h
		DEPENDS qnan )
		
	set( GEN_FP_FILES arith.h gd_qnan.h )
	set( GEN_FP_DEPS ${CMAKE_CURRENT_BINARY_DIR}/arith.h ${CMAKE_CURRENTY_BINARY_DIR}/gd_qnan.h )
endif()

add_library( gdtoa
	${GEN_FP_FILES}
	dmisc.c
	dtoa.c
	misc.c
	)
target_link_libraries( gdtoa )
if( GEN_FP_DEPS )
	add_dependencies( gdtoa ${GEN_FP_DEPS} )
endif()

