# If we are not building as a part of LLVM, build Python as an
# standalone project, using LLVM as an external library:
if( CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR )
  project(Python)
  cmake_minimum_required(VERSION 2.8)

  set(PYTHON_PATH_TO_LLVM_SOURCE "" CACHE PATH
    "Path to LLVM source code. Not necessary if using an installed LLVM.")
  set(PYTHON_PATH_TO_LLVM_BUILD "" CACHE PATH
    "Path to the directory where LLVM was built or installed.")

  if( PYTHON_PATH_TO_LLVM_SOURCE )
    if( NOT EXISTS "${PYTHON_PATH_TO_LLVM_SOURCE}/cmake/config-ix.cmake" )
      message(FATAL_ERROR "Please set PYTHON_PATH_TO_LLVM_SOURCE to the root directory of LLVM source code.")
    else()
      get_filename_component(LLVM_MAIN_SRC_DIR ${PYTHON_PATH_TO_LLVM_SOURCE}
	ABSOLUTE)
      list(APPEND CMAKE_MODULE_PATH "${LLVM_MAIN_SRC_DIR}/cmake/modules")
    endif()
  endif()

  if( NOT EXISTS "${PYTHON_PATH_TO_LLVM_BUILD}/bin/tblgen${CMAKE_EXECUTABLE_SUFFIX}" )
    # Looking for bin/Debug/tblgen is a complete hack. How can we get
    # around this?
    if( NOT EXISTS "${PYTHON_PATH_TO_LLVM_BUILD}/bin/Debug/tblgen${CMAKE_EXECUTABLE_SUFFIX}" )
      message(FATAL_ERROR "Please set PYTHON_PATH_TO_LLVM_BUILD to a directory containing a LLVM build.")
    endif()
  endif()

  list(APPEND CMAKE_MODULE_PATH "${PYTHON_PATH_TO_LLVM_BUILD}/share/llvm/cmake")

  get_filename_component(PATH_TO_LLVM_BUILD ${PYTHON_PATH_TO_LLVM_BUILD}
    ABSOLUTE)

  include(AddLLVM)
  include(TableGen)
  include("${PYTHON_PATH_TO_LLVM_BUILD}/share/llvm/cmake/LLVMConfig.cmake")
  include(HandleLLVMOptions)

  set(PACKAGE_VERSION "${LLVM_PACKAGE_VERSION}")

  set(LLVM_MAIN_INCLUDE_DIR "${LLVM_MAIN_SRC_DIR}/include")
  set(LLVM_BINARY_DIR ${CMAKE_BINARY_DIR})

  set(CMAKE_INCLUDE_CURRENT_DIR ON)
  include_directories("${PATH_TO_LLVM_BUILD}/include" "${LLVM_MAIN_INCLUDE_DIR}")
  link_directories("${PATH_TO_LLVM_BUILD}/lib")

  if( EXISTS "${PYTHON_PATH_TO_LLVM_BUILD}/bin/tblgen${CMAKE_EXECUTABLE_SUFFIX}" )
    set(LLVM_TABLEGEN_EXE "${PATH_TO_LLVM_BUILD}/bin/tblgen")
  else()
    # FIXME: This is an utter hack.
    set(LLVM_TABLEGEN_EXE "${PATH_TO_LLVM_BUILD}/bin/Debug/tblgen")
  endif()

  set( CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin )
  set( CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib )
  set( CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib )

  set( PYTHON_BUILT_STANDALONE 1 )
endif()

set(PYTHON_RESOURCE_DIR "" CACHE STRING
  "Relative directory from the Python binary to its resource files.")

set(C_INCLUDE_DIRS "" CACHE STRING
  "Colon separated list of directories python will search for headers.")

set(PYTHON_VENDOR "" CACHE STRING
  "Vendor-specific text for showing with version information.")

if( PYTHON_VENDOR )
  add_definitions( -DPYTHON_VENDOR="${PYTHON_VENDOR} " )
endif()

set(PYTHON_SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR})
set(PYTHON_BINARY_DIR ${CMAKE_CURRENT_BINARY_DIR})

if( CMAKE_SOURCE_DIR STREQUAL CMAKE_BINARY_DIR AND NOT MSVC_IDE )
  message(FATAL_ERROR "In-source builds are not allowed. CMake would overwrite "
"the makefiles distributed with LLVM. Please create a directory and run cmake "
"from there, passing the path to this source directory as the last argument. "
"This process created the file `CMakeCache.txt' and the directory "
"`CMakeFiles'. Please delete them.")
endif()

if( NOT CMAKE_SOURCE_DIR STREQUAL CMAKE_BINARY_DIR )
  file(GLOB_RECURSE
    tablegenned_files_on_include_dir
    "${PYTHON_SOURCE_DIR}/include/python/*.inc")
  if( tablegenned_files_on_include_dir )
    message(FATAL_ERROR "Apparently there is a previous in-source build, "
"probably as the result of running `configure' and `make' on "
"${PYTHON_SOURCE_DIR}. This may cause problems. The suspicious files are:\n"
"${tablegenned_files_on_include_dir}\nPlease clean the source directory.")
  endif()
endif()

# Compute the Python version from the LLVM version.
string(REGEX MATCH "[0-9]+\\.[0-9]+(\\.[0-9]+)?" PYTHON_VERSION
  ${PACKAGE_VERSION})
message(STATUS "Python version: ${PYTHON_VERSION}")

string(REGEX REPLACE "([0-9]+)\\.[0-9]+(\\.[0-9]+)?" "\\1" PYTHON_VERSION_MAJOR
  ${PYTHON_VERSION})
string(REGEX REPLACE "[0-9]+\\.([0-9]+)(\\.[0-9]+)?" "\\1" PYTHON_VERSION_MINOR
  ${PYTHON_VERSION})
if (${PYTHON_VERSION} MATCHES "[0-9]+\\.[0-9]+\\.[0-9]+")
  set(PYTHON_HAS_VERSION_PATCHLEVEL 1)
  string(REGEX REPLACE "[0-9]+\\.[0-9]+\\.([0-9]+)" "\\1" PYTHON_VERSION_PATCHLEVEL
    ${PYTHON_VERSION})
else()
  set(PYTHON_HAS_VERSION_PATCHLEVEL 0)
endif()

# Configure the Version.inc file.
# configure_file(
#   ${CMAKE_CURRENT_SOURCE_DIR}/include/python/Basic/Version.inc.in
#   ${CMAKE_CURRENT_BINARY_DIR}/include/python/Basic/Version.inc)

# Add appropriate flags for GCC
if (LLVM_COMPILER_IS_GCC_COMPATIBLE)
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fno-common -Woverloaded-virtual -Wcast-qual -fno-strict-aliasing -pedantic -Wno-long-long -Wall -W -Wno-unused-parameter -Wwrite-strings")
endif ()

# if (APPLE)
#   set(CMAKE_MODULE_LINKER_FLAGS "-Wl,-flat_namespace -Wl,-undefined -Wl,suppress")
# endif ()

# configure_file(
#   ${PYTHON_SOURCE_DIR}/include/python/Config/config.h.cmake
#   ${PYTHON_BINARY_DIR}/include/python/Config/config.h)

include(LLVMParseArguments)

function(python_tablegen)
  # Syntax:
  # python_tablegen output-file [tablegen-arg ...] SOURCE source-file
  # [[TARGET cmake-target-name] [DEPENDS extra-dependency ...]]
  #
  # Generates a custom command for invoking tblgen as
  #
  # tblgen source-file -o=output-file tablegen-arg ...
  #
  # and, if cmake-target-name is provided, creates a custom target for
  # executing the custom command depending on output-file. It is
  # possible to list more files to depend after DEPENDS.

  parse_arguments( CTG "SOURCE;TARGET;DEPENDS" "" ${ARGN} )

  if( NOT CTG_SOURCE )
    message(FATAL_ERROR "SOURCE source-file required by python_tablegen")
  endif()

  set( LLVM_TARGET_DEFINITIONS ${CTG_SOURCE} )
  tablegen( ${CTG_DEFAULT_ARGS} )

  list( GET CTG_DEFAULT_ARGS 0 output_file )
  if( CTG_TARGET )
    add_custom_target( ${CTG_TARGET} DEPENDS ${output_file} ${CTG_DEPENDS} )
    set_target_properties( ${CTG_TARGET} PROPERTIES FOLDER "Python tablegenning")
  endif()
endfunction(python_tablegen)

macro(add_python_library name)
  llvm_process_sources(srcs ${ARGN})
  if(MSVC_IDE OR XCODE)
    string( REGEX MATCHALL "/[^/]+" split_path ${CMAKE_CURRENT_SOURCE_DIR})
    list( GET split_path -1 dir)
    file( GLOB_RECURSE headers
      ../../../include/python/StaticAnalyzer${dir}/*.h
      ../../../include/python/StaticAnalyzer${dir}/*.td
      ../../../include/python/StaticAnalyzer${dir}/*.def
      ../../include/python${dir}/*.h
      ../../include/python${dir}/*.td
      ../../include/python${dir}/*.def)
    set(srcs ${srcs} ${headers})
  endif(MSVC_IDE OR XCODE)
  if (MODULE)
    set(libkind MODULE)
  elseif (SHARED_LIBRARY)
    set(libkind SHARED)
  else()
    set(libkind)
  endif()
  add_library( ${name} ${libkind} ${srcs} )
  if( LLVM_COMMON_DEPENDS )
    add_dependencies( ${name} ${LLVM_COMMON_DEPENDS} )
  endif( LLVM_COMMON_DEPENDS )

  target_link_libraries( ${name} ${LLVM_USED_LIBS} )
  llvm_config( ${name} ${LLVM_LINK_COMPONENTS} )
  target_link_libraries( ${name} ${LLVM_COMMON_LIBS} )
  link_system_libs( ${name} )

  add_dependencies(${name} PythonDiagnosticCommon)
  if(MSVC)
    get_target_property(cflag ${name} COMPILE_FLAGS)
    if(NOT cflag)
      set(cflag "")
    endif(NOT cflag)
    set(cflag "${cflag} /Za")
    set_target_properties(${name} PROPERTIES COMPILE_FLAGS ${cflag})
  endif(MSVC)
  install(TARGETS ${name}
    LIBRARY DESTINATION lib${LLVM_LIBDIR_SUFFIX}
    ARCHIVE DESTINATION lib${LLVM_LIBDIR_SUFFIX})
  set_target_properties(${name} PROPERTIES FOLDER "Python libraries")
endmacro(add_python_library)

macro(add_python_executable name)
  add_llvm_executable( ${name} ${ARGN} )
  set_target_properties(${name} PROPERTIES FOLDER "Python executables")
endmacro(add_python_executable)

include_directories(BEFORE
  ${CMAKE_CURRENT_BINARY_DIR}/include
  ${CMAKE_CURRENT_SOURCE_DIR}/include
  )

install(DIRECTORY include/
  DESTINATION include
  FILES_MATCHING
  PATTERN "*.def"
  PATTERN "*.h"
  PATTERN ".svn" EXCLUDE
  )

install(DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/include/
  DESTINATION include
  FILES_MATCHING
  PATTERN "CMakeFiles" EXCLUDE
  PATTERN "*.inc"
  )

add_definitions( -D_GNU_SOURCE -DHAVE_PYTHON_CONFIG_H )

# Python version information
set(PYTHON_EXECUTABLE_VERSION
     "${PYTHON_VERSION_MAJOR}.${PYTHON_VERSION_MINOR}" CACHE STRING
    "Version number that will be placed into the python executable, in the form XX.YY")
set(LIBPYTHON_LIBRARY_VERSION
     "${PYTHON_VERSION_MAJOR}.${PYTHON_VERSION_MINOR}" CACHE STRING
    "Version number that will be placed into the libpython library , in the form XX.YY")
mark_as_advanced(PYTHON_EXECUTABLE_VERSION LIBPYTHON_LIBRARY_VERSION)


# option(PYTHON_BUILD_EXAMPLES "Build PYTHON example programs." OFF)
# if(PYTHON_BUILD_EXAMPLES)
#   add_subdirectory(examples)
# endif ()

add_subdirectory(include)
add_subdirectory(lib)
add_subdirectory(tools)
add_subdirectory(runtime)

# TODO: docs.
add_subdirectory(test)

if( LLVM_INCLUDE_TESTS )
  if( NOT PYTHON_BUILT_STANDALONE )
    add_subdirectory(unittests)
  endif()
endif()

# Workaround for MSVS10 to avoid the Dialog Hell
# FIXME: This could be removed with future version of CMake.
# if( PYTHON_BUILT_STANDALONE AND MSVC_VERSION EQUAL 1600 )
#   set(PYTHON_SLN_FILENAME "${CMAKE_CURRENT_BINARY_DIR}/Python.sln")
#   if( EXISTS "${PYTHON_SLN_FILENAME}" )
#     file(APPEND "${PYTHON_SLN_FILENAME}" "\n# This should be regenerated!\n")
#   endif()
# endif()

set(BUG_REPORT_URL "http://jamesmolloy.co.uk/bugs/" CACHE STRING
  "Default URL where bug reports are to be submitted.")

