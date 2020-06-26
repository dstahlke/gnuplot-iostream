cmake_minimum_required(VERSION 3.10)
Project(GnuPlot_iostream)

# Target.
add_library(gnuplot_iostream INTERFACE)
target_include_directories(gnuplot_iostream INTERFACE
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/>
    $<INSTALL_INTERFACE:>)



find_package(Boost REQUIRED COMPONENTS
    iostreams system filesystem
)

set(GnuPlotIostream_tests
    test-assert-depth
    test-assert-depth-colmajor
    test-empty
    test-noncopyable
    test-outputs
)
set(GnuPlotIostream_examples
    example-data-1d
    example-data-2d
    example-interactive
    example-misc
)

# foreach(atest ${GnuPlotIostream_tests})
#     add_executable(${atest} ${atest}.cc)
#     target_compile_features(${atest} PRIVATE cxx_std_17)
#     target_compile_options(${atest} PRIVATE -Wall -Wextra)
#     target_link_libraries(${atest} PRIVATE 
#         gnuplot_iostream
#         boost_iostreams
#         boost_system
#         boost_filesystem
#     )
# endforeach()

foreach(an_example ${GnuPlotIostream_examples})
    add_executable(${an_example} ${an_example}.cc)
    target_compile_features(${an_example} PRIVATE cxx_std_17)
    target_compile_options(${an_example} PRIVATE -Wall -Wextra)
    target_link_libraries(${an_example} PRIVATE 
        gnuplot_iostream
        boost_iostreams
        boost_system
        boost_filesystem
        util
    )
endforeach()