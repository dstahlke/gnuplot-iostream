cmake_minimum_required(VERSION 3.10)
Project(GnuPlot_iostream)

#options.
option(GnuPlotIostream_BuildTests "build gnuplot iostream tests" ON)
option(GnuPlotIostream_BuildExamples "build gnuplot iostream examples" ON)

# packages.
find_package(Boost REQUIRED COMPONENTS
    iostreams system filesystem
)

# Target.
add_library(gnuplot_iostream INTERFACE)
target_include_directories(gnuplot_iostream INTERFACE
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/>
    $<INSTALL_INTERFACE:>)
target_link_libraries(gnuplot_iostream INTERFACE
    Boost::iostreams
    Boost::system
    Boost::filesystem
)

if(GnuPlotIostream_BuildTests)
  set(GnuPlotIostream_tests
    test-assert-depth
    test-assert-depth-colmajor
    test-empty
    test-noncopyable
    test-outputs
    )
  foreach(atest ${GnuPlotIostream_tests})
    add_executable(${atest} ${atest}.cc)
    target_compile_features(${atest} PRIVATE cxx_std_17)
    target_compile_options(${atest} PRIVATE -Wall -Wextra)
    target_link_libraries(${atest} PRIVATE
      gnuplot_iostream
      boost_iostreams
      boost_system
      boost_filesystem
      )
  endforeach()
endif()

if(GnuPlotIostream_BuildExamples)
  set(GnuPlotIostream_examples
    example-data-1d
    example-data-2d
    example-interactive
    example-misc
    )
  foreach(an_example ${GnuPlotIostream_examples})
    add_executable(${an_example} ${an_example}.cc)
    target_compile_features(${an_example} PRIVATE cxx_std_17)
    target_compile_options(${an_example} PRIVATE -Wall -Wextra)
    target_link_libraries(${an_example} PRIVATE 
      gnuplot_iostream
      util
      )
  endforeach()
endif()
