# cmake stuff by Jens Mueller

cmake_minimum_required(VERSION 2.6.3)

# for finding Blitz
set(CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake")

find_package(Blitz)

# TODO
# what Boost version do we rely on?
find_package(Boost 1.35 COMPONENTS iostreams system filesystem REQUIRED)
include_directories(${Boost_INCLUDE_DIR})

include(CheckFunctionExists)

# check for openpty
set(CMAKE_REQUIRED_LIBRARIES ${Boost_LIBRARIES} util)
check_function_exists(openpty HAVE_OPENPTY)

# common flags
SET(CMAKE_CXX_FLAGS "-Wall -Wextra -O0 -g")

#add_library(gnuplot-iostream SHARED gnuplot-iostream.cc)
## util is for openpty
#target_link_libraries(gnuplot-iostream ${Boost_LIBRARIES} ${CMAKE_REQUIRED_LIBRARIES})

add_executable(examples examples.cc)
target_link_libraries(examples ${CMAKE_REQUIRED_LIBRARIES})

if(HAVE_OPENPTY)
	add_executable(examples-interactive examples-interactive.cc)
	target_link_libraries(examples-interactive ${CMAKE_REQUIRED_LIBRARIES})
endif()

if(BLITZ_FOUND)
	include_directories(${BLITZ_INCLUDE_DIR})
	add_executable(examples-blitz examples-blitz.cc)
	target_link_libraries(examples-blitz ${CMAKE_REQUIRED_LIBRARIES})
endif()

install(FILES gnuplot-iostream.h DESTINATION include/gnuplot-iostream/)
#install(TARGETS gnuplot-iostream
#	RUNTIME DESTINATION bin
#	LIBRARY DESTINATION lib
#	ARCHIVE DESTINATION lib
#)
