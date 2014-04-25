# - Try to find the TagLib library
# Once done this will define
#
#  TAGLIB_FOUND - system has the TagLib library
#  TAGLIB_INCLUDE_DIR - the TagLib include directory
#  TAGLIB_LIBRARY - Link this to use the TagLib library
#
# Copyright (c) 2014, Giuseppe Calà, <jiveaxe@gmail.com>
#
# Redistribution and use is allowed according to the terms of the BSD license.
# For details see the accompanying COPYING-CMAKE-SCRIPTS file.

if (TAGLIB_INCLUDE_DIR AND TAGLIB_LIBRARY)

  # in cache already
  set(TAGLIB_FOUND TRUE)

else (TAGLIB_INCLUDE_DIR AND TAGLIB_LIBRARY)
  if (NOT WIN32)
    find_package(PkgConfig)
    pkg_check_modules(PC_TAGLIB taglib)
  endif(NOT WIN32)

  find_path(TAGLIB_INCLUDE_DIR taglib/tag.h
    HINTS
    ${PC_TAGLIB_INCLUDE_DIRS}
  )

  find_library(TAGLIB_LIBRARY NAMES tag
    HINTS
    ${PC_TAGLIB_LIBRARY_DIRS}
  )

  include(FindPackageHandleStandardArgs)
  find_package_handle_standard_args(TagLib DEFAULT_MSG TAGLIB_LIBRARY TAGLIB_INCLUDE_DIR)
  
  mark_as_advanced(TAGLIB_INCLUDE_DIR TAGLIB_LIBRARY)

endif (TAGLIB_INCLUDE_DIR AND TAGLIB_LIBRARY)
