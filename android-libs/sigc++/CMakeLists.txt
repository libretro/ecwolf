cmake_minimum_required( VERSION 3.5.0 )

add_library(sigc++ STATIC
	sigc++/adaptors/lambda/lambda.cc
	sigc++/functors/slot.cc
	sigc++/functors/slot_base.cc
	sigc++/connection.cc
	sigc++/signal.cc
	sigc++/signal_base.cc
	sigc++/trackable.cc
)

target_include_directories(sigc++ PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
