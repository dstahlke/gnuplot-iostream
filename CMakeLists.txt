# cmake stuff by Jens Mueller

cmake_minimum_required(VERSION 2.6.3)

# for finding Blitz
set(CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake")

find_package(Blitz)

# TODO
# what Boost version do we rely on?
find_package(Boost 1.35 COMPONENTS iostreams REQUIRED)
include_directories(${Boost_INCLUDE_DIR})

include(CheckFunctionExists)

# check for openpty
set(CMAKE_REQUIRED_LIBRARIES ${Boost_LIBRARIES} util)
check_function_exists(openpty HAVE_OPENPTY)
# the user has to set GNUPLOT_ENABLE_PTY before
# including the header
#if(HAVE_OPENPTY)
#	add_definitions(-DGNUPLOT_ENABLE_PTY)
#endif()

# common flags
SET(CMAKE_CXX_FLAGS "-Wall -Wextra -O0 -g")

#add_library(gnuplot-iostream SHARED gnuplot-iostream.cc)
## util is for openpty
#target_link_libraries(gnuplot-iostream ${Boost_LIBRARIES} ${CMAKE_REQUIRED_LIBRARIES})

add_executable(example-make-png example-make-png.cc)
target_link_libraries(example-make-png ${CMAKE_REQUIRED_LIBRARIES})

add_executable(example-popup example-popup.cc)
target_link_libraries(example-popup ${CMAKE_REQUIRED_LIBRARIES})

add_executable(example-interactive example-interactive.cc)
target_link_libraries(example-interactive ${CMAKE_REQUIRED_LIBRARIES})

add_executable(example-vectors example-vectors.cc)
target_link_libraries(example-vectors ${CMAKE_REQUIRED_LIBRARIES})

if(BLITZ_FOUND)
	include_directories(${BLITZ_INCLUDE_DIR})
	add_executable(example-blitz example-blitz.cc)
	target_link_libraries(example-blitz ${CMAKE_REQUIRED_LIBRARIES})
endif()

install(FILES gnuplot-iostream.h DESTINATION include/gnuplot-iostream/)
#install(TARGETS gnuplot-iostream
#	RUNTIME DESTINATION bin
#	LIBRARY DESTINATION lib
#	ARCHIVE DESTINATION lib
#)
