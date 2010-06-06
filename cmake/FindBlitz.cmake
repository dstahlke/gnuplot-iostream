# copied from
# http://code.google.com/p/mcgeometry/source/browse/trunk/tools/FindBlitz.cmake?spec=svn102&r=102
# TODO
# check licence issues

# - Find Blitz library
# Find the native Blitz includes and library
# This module defines
#  BLITZ_INCLUDE_DIR, where to find tiff.h, etc.
#  BLITZ_LIBRARY, libraries to link against to use Blitz.
#  BLITZ_FOUND, If false, do not try to use Blitz.

FIND_PATH(BLITZ_INCLUDE_DIR blitz/blitz.h)
# FIXME - Fedora keeps this file in a funny place so it needs to be searched
# separately.  Unfortunately this breaks portability since it won't be called
# "gnu" on other systems.
FIND_PATH(BLITZ_INCLUDE_DIR2 blitz/gnu/bzconfig.h)
FIND_LIBRARY(BLITZ_LIBRARY NAMES blitz)

# handle the QUIETLY and REQUIRED arguments and set BLITZ_FOUND to TRUE if 
# all listed variables are TRUE
INCLUDE(FindPackageHandleStandardArgs)
FIND_PACKAGE_HANDLE_STANDARD_ARGS(Blitz DEFAULT_MSG BLITZ_LIBRARY BLITZ_INCLUDE_DIR BLITZ_INCLUDE_DIR2)
