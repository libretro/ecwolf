cmake_minimum_required( VERSION 2.4 )

get_target_property(ZIPDIR_EXE zipdir LOCATION)

file(GLOB_RECURSE PK3_FILES ${CMAKE_CURRENT_SOURCE_DIR}/static *)

add_custom_command( OUTPUT ${OUTPUT_DIR}/ecwolf.pk3 COMMAND ${ZIPDIR_EXE} -udf ${OUTPUT_DIR}/ecwolf.pk3 ${CMAKE_CURRENT_SOURCE_DIR}/static DEPENDS zipdir ${PK3_FILES} )

add_custom_target(pk3 ALL DEPENDS ${OUTPUT_DIR}/ecwolf.pk3)
