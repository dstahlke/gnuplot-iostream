cmake_minimum_required(VERSION 2.6.3)

# for finding Blitz
set(CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake")

find_package(Blitz)

# TODO
# what Boost version do we rely on?
find_package(Boost 1.35 COMPONENTS iostreams REQUIRED)
include_directories(${Boost_INCLUDE_DIR})

# common flags
SET(CMAKE_CXX_FLAGS "-Wall -Wextra -O0 -g")

add_library(gnuplot-iostream SHARED gnuplot-iostream.cc)
# TODO
# what is util
target_link_libraries(gnuplot-iostream ${Boost_LIBRARIES} util)

add_executable(example-make-png example-make-png.cc)
target_link_libraries(example-make-png gnuplot-iostream)

if(Blitz_FOUND)
	include_directories(${Blitz_INCLUDE_DIR})
	add_executable(example-interactive example-interactive.cc)
	target_link_libraries(example-interactive gnuplot-iostream)
endif()

install(FILES gnuplot-iostream.h DESTINATION include/gnuplot-iostream/)
install(TARGETS gnuplot-iostream
	RUNTIME DESTINATION bin
	LIBRARY DESTINATION lib
	ARCHIVE DESTINATION lib
)
