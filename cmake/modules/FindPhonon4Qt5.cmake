# - Try to find the QJson library
# Once done this will define
#
#  PHONON4QT5_FOUND - system has the QJson library
#  PHONON4QT5_INCLUDE_DIR - the QJson include directory
#  PHONON4QT5_LIBRARY - Link this to use the QJson library
#
# Copyright (c) 2010, Pino Toscano, <toscano.pino@tiscali.it>
#
# Redistribution and use is allowed according to the terms of the BSD license.
# For details see the accompanying COPYING-CMAKE-SCRIPTS file.

if (PHONON4QT5_INCLUDE_DIR AND PHONON4QT5_LIBRARY)

  # in cache already
  set(PHONON4QT5_FOUND TRUE)

else (PHONON4QT5_INCLUDE_DIR AND PHONON4QT5_LIBRARY)
  if (NOT WIN32)
    find_package(PkgConfig)
    pkg_check_modules(PC_PHONON4QT5 phonon4qt5)
  endif(NOT WIN32)

  find_path(PHONON4QT5_INCLUDE_DIR phonon4qt5/phonon/phononnamespace.h
    HINTS
    ${PC_PHONON4QT5_INCLUDE_DIRS}
  )

  find_library(PHONON4QT5_LIBRARY NAMES phonon4qt5
    HINTS
    ${PC_PHONON4QT5_LIBRARY_DIRS}
  )

  include(FindPackageHandleStandardArgs)
  find_package_handle_standard_args(Phonon4Qt5 DEFAULT_MSG PHONON4QT5_LIBRARY PHONON4QT5_INCLUDE_DIR)

  mark_as_advanced(PHONON4QT5_INCLUDE_DIR PHONON4QT5_LIBRARY)

endif (PHONON4QT5_INCLUDE_DIR AND PHONON4QT5_LIBRARY)
