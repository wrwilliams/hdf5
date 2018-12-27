
# Copyright by The HDF Group.
# All rights reserved.
#
# This file is part of HDF5.  The full HDF5 copyright notice, including
# terms governing use, modification, and redistribution, is contained in
# the COPYING file, which can be found at the root of the source code
# distribution tree, or in https://support.hdfgroup.org/ftp/HDF5/releases.
# If you do not have access to either file, you may request a copy from
# help@hdfgroup.org.
#

##############################################################################
##############################################################################
###           T E S T I N G                                                ###
##############################################################################
##############################################################################
# included from CMakeTests.cmake

set (VOL_LIST
    vol_native
    vol_pass_through1
    vol_pass_through2
)

set (vol_native native)
set (vol_pass_through1 "pass_through under_vol=0\;under_info={}")
set (vol_pass_through2 "pass_through under_vol=505\;under_info={under_vol=0\;under_info={}}")

foreach (voltest ${VOL_LIST})
  file (MAKE_DIRECTORY "${PROJECT_BINARY_DIR}/${voltest}")
  file (MAKE_DIRECTORY "${PROJECT_BINARY_DIR}/${voltest}/testfiles")
  file (MAKE_DIRECTORY "${PROJECT_BINARY_DIR}/${voltest}/testfiles/plist_files")
  if (BUILD_SHARED_LIBS)
    file (MAKE_DIRECTORY "${PROJECT_BINARY_DIR}/${voltest}-shared")
    file (MAKE_DIRECTORY "${PROJECT_BINARY_DIR}/${voltest}-shared/testfiles")
    file (MAKE_DIRECTORY "${PROJECT_BINARY_DIR}/${voltest}-shared/testfiles/plist_files")
  endif ()
endforeach ()

foreach (voltest ${VOL_LIST})
  foreach (h5_tfile ${HDF5_TEST_FILES})
    HDFTEST_COPY_FILE("${HDF5_TOOLS_DIR}/testfiles/${h5_tfile}" "${PROJECT_BINARY_DIR}/${voltest}/${h5_tfile}" "HDF5_VOLTEST_LIB_files")
    if (BUILD_SHARED_LIBS)
      HDFTEST_COPY_FILE("${HDF5_TOOLS_DIR}/testfiles/${h5_tfile}" "${PROJECT_BINARY_DIR}/${voltest}-shared/${h5_tfile}" "HDF5_VOLTEST_LIBSH_files")
    endif ()
  endforeach ()
endforeach ()

foreach (voltest ${VOL_LIST})
  foreach (ref_file ${HDF5_REFERENCE_FILES})
    HDFTEST_COPY_FILE("${HDF5_TEST_SOURCE_DIR}/testfiles/${ref_file}" "${PROJECT_BINARY_DIR}/${voltest}/${ref_file}" "HDF5_VOLTEST_LIB_files")
    if (BUILD_SHARED_LIBS)
      HDFTEST_COPY_FILE("${HDF5_TEST_SOURCE_DIR}/testfiles/${ref_file}" "${PROJECT_BINARY_DIR}/${voltest}-shared/${ref_file}" "HDF5_VOLTEST_LIBSH_files")
    endif ()
  endforeach ()
endforeach ()

foreach (voltest ${VOL_LIST})
  foreach (h5_file ${HDF5_REFERENCE_TEST_FILES})
    HDFTEST_COPY_FILE("${HDF5_TEST_SOURCE_DIR}/${h5_file}" "${HDF5_TEST_BINARY_DIR}/${voltest}/${h5_file}" "HDF5_VOLTEST_LIB_files")
    if (BUILD_SHARED_LIBS)
      HDFTEST_COPY_FILE("${HDF5_TEST_SOURCE_DIR}/${h5_file}" "${HDF5_TEST_BINARY_DIR}/${voltest}-shared/${h5_file}" "HDF5_VOLTEST_LIBSH_files")
    endif ()
  endforeach ()
endforeach ()

foreach (voltest ${VOL_LIST})
  foreach (plistfile ${HDF5_REFERENCE_PLIST_FILES})
    HDFTEST_COPY_FILE("${HDF5_TEST_SOURCE_DIR}/testfiles/plist_files/${plistfile}" "${PROJECT_BINARY_DIR}/${voltest}/testfiles/plist_files/${plistfile}" "HDF5_VOLTEST_LIB_files")
    HDFTEST_COPY_FILE("${HDF5_TEST_SOURCE_DIR}/testfiles/plist_files/def_${plistfile}" "${PROJECT_BINARY_DIR}/${voltest}/testfiles/plist_files/def_${plistfile}" "HDF5_VOLTEST_LIB_files")
    if (BUILD_SHARED_LIBS)
      HDFTEST_COPY_FILE("${HDF5_TEST_SOURCE_DIR}/testfiles/plist_files/${plistfile}" "${PROJECT_BINARY_DIR}/${voltest}-shared/testfiles/plist_files/${plistfile}" "HDF5_VOLTEST_LIBSH_files")
      HDFTEST_COPY_FILE("${HDF5_TEST_SOURCE_DIR}/testfiles/plist_files/def_${plistfile}" "${PROJECT_BINARY_DIR}/${voltest}-shared/testfiles/plist_files/def_${plistfile}" "HDF5_VOLTEST_LIBSH_files")
    endif ()
  endforeach ()
endforeach ()

add_custom_target(HDF5_VOLTEST_LIB_files ALL COMMENT "Copying files needed by HDF5_VOLTEST_LIB tests" DEPENDS ${HDF5_VOLTEST_LIB_files_list})
if (BUILD_SHARED_LIBS)
  add_custom_target(HDF5_VOLTEST_LIBSH_files ALL COMMENT "Copying files needed by HDF5_VOLTEST_LIBSH tests" DEPENDS ${HDF5_VOLTEST_LIBSH_files_list})
endif ()

##############################################################################
##############################################################################
###                         V O L   T E S T S                              ###
##############################################################################
##############################################################################

  set (H5_VOL_SKIP_TESTS
      cache
      accum
      fheap
      big
      vol
      error_test
      err_compat
      tcheck_version
      testmeta
      links_env
  )
  if (NOT CYGWIN)
    list (REMOVE_ITEM H5_VOL_SKIP_TESTS big cache)
  endif ()

  # Windows only macro
  macro (CHECK_VOL_TEST voltest volname volinfo resultcode)
    if ("${voltest}" STREQUAL "flush1" OR "${voltest}" STREQUAL "flush2")
      if ("${volname}" STREQUAL "multi" OR "${volname}" STREQUAL "split")
        if (NOT BUILD_SHARED_LIBS AND NOT ${HDF_CFG_NAME} MATCHES "Debug")
          add_test (NAME VOL-${volname}-${voltest}
              COMMAND "${CMAKE_COMMAND}"
                  -D "TEST_PROGRAM=$<TARGET_FILE:${voltest}>"
                  -D "TEST_ARGS:STRING="
                  -D "TEST_VOL:STRING=${volinfo}"
                  -D "TEST_EXPECT=${resultcode}"
                  -D "TEST_OUTPUT=${volname}-${voltest}"
                  -D "TEST_FOLDER=${PROJECT_BINARY_DIR}/${volname}"
                  -P "${HDF_RESOURCES_DIR}/volTest.cmake"
          )
          set_tests_properties (VOL-${volname}-${voltest} PROPERTIES
              ENVIRONMENT "srcdir=${HDF5_TEST_BINARY_DIR}/${volname}"
              WORKING_DIRECTORY ${HDF5_TEST_BINARY_DIR}/${volname}
          )
          if (BUILD_SHARED_LIBS)
            add_test (NAME VOL-${volname}-${test}-shared
                COMMAND "${CMAKE_COMMAND}"
                    -D "TEST_PROGRAM=$<TARGET_FILE:${voltest}-shared>"
                    -D "TEST_ARGS:STRING="
                    -D "TEST_VOL:STRING=${volinfo}"
                    -D "TEST_EXPECT=${resultcode}"
                    -D "TEST_OUTPUT=${volname}-${voltest}-shared"
                    -D "TEST_FOLDER=${PROJECT_BINARY_DIR}/${volname}-shared"
                    -P "${HDF_RESOURCES_DIR}/volTest.cmake"
            )
            set_tests_properties (VOL-${volname}-${voltest}-shared PROPERTIES
                ENVIRONMENT "srcdir=${HDF5_TEST_BINARY_DIR}/${volname}-shared"
                WORKING_DIRECTORY ${HDF5_TEST_BINARY_DIR}/${volname}-shared
            )
          endif ()
        else ()
          add_test (NAME VOL-${volname}-${voltest}
              COMMAND ${CMAKE_COMMAND} -E echo "SKIP VOL-${volname}-${voltest}"
          )
          if (BUILD_SHARED_LIBS)
            add_test (NAME VOL-${volname}-${test}-shared
                COMMAND ${CMAKE_COMMAND} -E echo "SKIP VOL-${volname}-${voltest}-shared"
            )
          endif ()
        endif ()
      else ()
        add_test (NAME VOL-${volname}-${voltest}
            COMMAND "${CMAKE_COMMAND}"
                -D "TEST_PROGRAM=$<TARGET_FILE:${voltest}>"
                -D "TEST_ARGS:STRING="
                -D "TEST_VOL:STRING=${volinfo}"
                -D "TEST_EXPECT=${resultcode}"
                -D "TEST_OUTPUT=${volname}-${voltest}"
                -D "TEST_FOLDER=${PROJECT_BINARY_DIR}/${volname}"
                -P "${HDF_RESOURCES_DIR}/volTest.cmake"
        )
        set_tests_properties (VOL-${volname}-${voltest} PROPERTIES
            ENVIRONMENT "srcdir=${HDF5_TEST_BINARY_DIR}/${volname}"
            WORKING_DIRECTORY ${HDF5_TEST_BINARY_DIR}/${volname}
        )
        if (BUILD_SHARED_LIBS)
          add_test (NAME VOL-${volname}-${test}-shared
              COMMAND "${CMAKE_COMMAND}"
                -D "TEST_PROGRAM=$<TARGET_FILE:${voltest}-shared>"
                -D "TEST_ARGS:STRING="
                -D "TEST_VOL:STRING=${volinfo}"
                -D "TEST_EXPECT=${resultcode}"
                -D "TEST_OUTPUT=${volname}-${voltest}-shared"
                -D "TEST_FOLDER=${PROJECT_BINARY_DIR}/${volname}-shared"
                -P "${HDF_RESOURCES_DIR}/volTest.cmake"
          )
          set_tests_properties (VOL-${volname}-${voltest}-shared PROPERTIES
              ENVIRONMENT "srcdir=${HDF5_TEST_BINARY_DIR}/${volname}-shared"
              WORKING_DIRECTORY ${HDF5_TEST_BINARY_DIR}/${volname}-shared
          )
        endif ()
      endif ()
    else ()
      add_test (NAME VOL-${volname}-${voltest}
          COMMAND "${CMAKE_COMMAND}"
              -D "TEST_PROGRAM=$<TARGET_FILE:${voltest}>"
              -D "TEST_ARGS:STRING="
              -D "TEST_VOL:STRING=${volinfo}"
              -D "TEST_EXPECT=${resultcode}"
              -D "TEST_OUTPUT=${volname}-${voltest}"
              -D "TEST_FOLDER=${PROJECT_BINARY_DIR}/${volname}"
              -P "${HDF_RESOURCES_DIR}/volTest.cmake"
      )
      set_tests_properties (VOL-${volname}-${voltest} PROPERTIES
          ENVIRONMENT "srcdir=${HDF5_TEST_BINARY_DIR}/${volname};HDF5TestExpress=${HDF_TEST_EXPRESS}"
          WORKING_DIRECTORY ${HDF5_TEST_BINARY_DIR}/${volname}
      )
      if (BUILD_SHARED_LIBS AND NOT "${voltest}" STREQUAL "cache")
        add_test (NAME VOL-${volname}-${voltest}-shared
            COMMAND "${CMAKE_COMMAND}"
                -D "TEST_PROGRAM=$<TARGET_FILE:${voltest}-shared>"
                -D "TEST_ARGS:STRING="
                -D "TEST_VOL:STRING=${volinfo}"
                -D "TEST_EXPECT=${resultcode}"
                -D "TEST_OUTPUT=${volname}-${voltest}-shared"
                -D "TEST_FOLDER=${PROJECT_BINARY_DIR}/${volname}-shared"
                -P "${HDF_RESOURCES_DIR}/volTest.cmake"
        )
        set_tests_properties (VOL-${volname}-${voltest}-shared PROPERTIES
            ENVIRONMENT "srcdir=${HDF5_TEST_BINARY_DIR}/${volname}-shared;HDF5TestExpress=${HDF_TEST_EXPRESS}"
            WORKING_DIRECTORY ${HDF5_TEST_BINARY_DIR}/${volname}-shared
        )
        endif ()
    endif ()
  endmacro ()

  macro (DO_VOL_TEST voltest volname volinfo resultcode)
      #message(STATUS "${voltest}-${volname} with ${volinfo}")
      add_test (NAME VOL-${volname}-${voltest}
          COMMAND "${CMAKE_COMMAND}"
              -D "TEST_PROGRAM=$<TARGET_FILE:${voltest}>"
              -D "TEST_ARGS:STRING="
              -D "TEST_VOL:STRING=${volinfo}"
              -D "TEST_EXPECT=${resultcode}"
              -D "TEST_OUTPUT=${volname}-${voltest}"
              -D "TEST_FOLDER=${PROJECT_BINARY_DIR}/${volname}"
              -P "${HDF_RESOURCES_DIR}/volTest.cmake"
      )
      set_tests_properties (VOL-${volname}-${voltest} PROPERTIES
          ENVIRONMENT "srcdir=${HDF5_TEST_BINARY_DIR}/${volname}"
          WORKING_DIRECTORY ${HDF5_TEST_BINARY_DIR}/${volname}
      )
      if (BUILD_SHARED_LIBS)
        add_test (NAME VOL-${volname}-${voltest}-shared
            COMMAND "${CMAKE_COMMAND}"
                -D "TEST_PROGRAM=$<TARGET_FILE:${voltest}-shared>"
                -D "TEST_ARGS:STRING="
                -D "TEST_VOL:STRING=${volinfo}"
                -D "TEST_EXPECT=${resultcode}"
                -D "TEST_OUTPUT=${volname}-${voltest}-shared"
                -D "TEST_FOLDER=${PROJECT_BINARY_DIR}/${volname}-shared"
                -P "${HDF_RESOURCES_DIR}/volTest.cmake"
        )
        set_tests_properties (VOL-${volname}-${voltest}-shared PROPERTIES
            ENVIRONMENT "srcdir=${HDF5_TEST_BINARY_DIR}/${volname}-shared"
            WORKING_DIRECTORY ${HDF5_TEST_BINARY_DIR}/${volname}-shared
        )
      endif ()
  endmacro ()

  macro (ADD_VOL_TEST volname volinfo resultcode)
    #message(STATUS "volname=${volname} volinfo=${volinfo}")
    foreach (test ${H5_TESTS})
      if (NOT ${test} IN_LIST H5_VOL_SKIP_TESTS)
        if (WIN32)
          CHECK_VOL_TEST (${test} ${volname} "${volinfo}" ${resultcode})
        else ()
          DO_VOL_TEST (${test} ${volname} "${volinfo}" ${resultcode})
        endif ()
      endif ()
    endforeach ()
    set_tests_properties (VOL-${volname}-flush2 PROPERTIES DEPENDS VOL-${volname}-flush1)
    set_tests_properties (VOL-${volname}-flush1 PROPERTIES TIMEOUT 10)
    set_tests_properties (VOL-${volname}-flush2 PROPERTIES TIMEOUT 10)
    set_tests_properties (VOL-${volname}-istore PROPERTIES TIMEOUT 1800)
    if (NOT CYGWIN)
      set_tests_properties (VOL-${volname}-cache PROPERTIES TIMEOUT 1800)
    endif ()
    if (BUILD_SHARED_LIBS)
      set_tests_properties (VOL-${volname}-flush2-shared PROPERTIES DEPENDS VOL-${volname}-flush1-shared)
      set_tests_properties (VOL-${volname}-flush1-shared PROPERTIES TIMEOUT 10)
      set_tests_properties (VOL-${volname}-flush2-shared PROPERTIES TIMEOUT 10)
      set_tests_properties (VOL-${volname}-istore-shared PROPERTIES TIMEOUT 1800)
      if (NOT CYGWIN AND NOT WIN32)
        set_tests_properties (VOL-${volname}-cache-shared PROPERTIES TIMEOUT 1800)
      endif ()
    endif ()
    if (HDF5_TEST_FHEAP_VOL)
      add_test (NAME VOL-${volname}-fheap
          COMMAND "${CMAKE_COMMAND}"
              -D "TEST_PROGRAM=$<TARGET_FILE:fheap>"
              -D "TEST_ARGS:STRING="
              -D "TEST_VOL:STRING=${volinfo}"
              -D "TEST_EXPECT=${resultcode}"
              -D "TEST_OUTPUT=${volname}-fheap"
              -D "TEST_FOLDER=${PROJECT_BINARY_DIR}/${volname}"
              -P "${HDF_RESOURCES_DIR}/volTest.cmake"
      )
      set_tests_properties (VOL-${volname}-fheap PROPERTIES
          TIMEOUT 1800
          ENVIRONMENT "srcdir=${HDF5_TEST_BINARY_DIR}/${volname};HDF5TestExpress=${HDF_TEST_EXPRESS}"
          WORKING_DIRECTORY ${HDF5_TEST_BINARY_DIR}/${volname}
      )
      if (BUILD_SHARED_LIBS)
        add_test (NAME VOL-${volname}-fheap-shared
            COMMAND "${CMAKE_COMMAND}"
                -D "TEST_PROGRAM=$<TARGET_FILE:fheap-shared>"
                -D "TEST_ARGS:STRING="
                -D "TEST_VOL:STRING=${volinfo}"
                -D "TEST_EXPECT=${resultcode}"
                -D "TEST_OUTPUT=${volname}-fheap-shared"
                -D "TEST_FOLDER=${PROJECT_BINARY_DIR}/${volname}-shared"
                -P "${HDF_RESOURCES_DIR}/volTest.cmake"
        )
        set_tests_properties (VOL-${volname}-fheap-shared PROPERTIES
            TIMEOUT 1800
            ENVIRONMENT "srcdir=${HDF5_TEST_BINARY_DIR}/${volname}-shared;HDF5TestExpress=${HDF_TEST_EXPRESS}"
            WORKING_DIRECTORY ${HDF5_TEST_BINARY_DIR}/${volname}-shared
        )
      endif ()
    endif ()
  endmacro ()

  # Run test with different Virtual File Driver
  foreach (volname ${VOL_LIST})
    #message(STATUS "volname=${volname}")
    foreach (volinfo IN LISTS ${volname})
      #message(STATUS "${volname} volinfo=${volinfo}")
      ADD_VOL_TEST (${volname} "${volinfo}" 0)
    endforeach ()
  endforeach ()

