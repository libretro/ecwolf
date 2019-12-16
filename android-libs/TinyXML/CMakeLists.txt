cmake_minimum_required( VERSION 3.5.0 )

add_library(TinyXML STATIC
	tinystr.cpp
	tinyxml.cpp
	tinyxmlerror.cpp
	tinyxmlparser.cpp
)

target_compile_definitions(TinyXML PRIVATE -DTIXML_USE_STL)

target_include_directories(TinyXML PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
