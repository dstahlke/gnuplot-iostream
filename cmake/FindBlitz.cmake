# copied from
# http://code.google.com/p/mcgeometry/source/browse/trunk/tools/FindBlitz.cmake?spec=svn102&r=102
# TODO
# check licence issues

# - Find Blitz library
# Find the native Blitz includes and library
# This module defines
#  Blitz_INCLUDE_DIR, where to find tiff.h, etc.
#  Blitz_LIBRARIES, libraries to link against to use Blitz.
#  Blitz_FOUND, If false, do not try to use Blitz.
# also defined, but not for general use are
#  Blitz_LIBRARY, where to find the Blitz library.

FIND_PATH(Blitz_INCLUDE_DIR blitz/blitz.h)

FIND_LIBRARY(Blitz_LIBRARY
  NAMES blitz )

# handle the QUIETLY and REQUIRED arguments and set Blitz_FOUND to TRUE if 
# all listed variables are TRUE
INCLUDE(FindPackageHandleStandardArgs)
FIND_PACKAGE_HANDLE_STANDARD_ARGS(Blitz  DEFAULT_MSG  Blitz_LIBRARY  Blitz_INCLUDE_DIR)

IF(Blitz_FOUND)
  SET( Blitz_LIBRARIES ${Blitz_LIBRARY} )
ENDIF(Blitz_FOUND)

MARK_AS_ADVANCED(Blitz_INCLUDE_DIR Blitz_LIBRARY)
