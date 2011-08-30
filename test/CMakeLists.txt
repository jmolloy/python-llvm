set(PYTHON_TEST_DIRECTORIES
  )

set(LLVM_SOURCE_DIR "${LLVM_MAIN_SRC_DIR}")
set(LLVM_BINARY_DIR "${LLVM_BINARY_DIR}")
set(LLVM_BUILD_MODE "%(build_mode)s")
set(LLVM_TOOLS_DIR "${LLVM_TOOLS_BINARY_DIR}/%(build_config)s")
set(LLVM_LIBS_DIR "${LLVM_BINARY_DIR}/lib/%(build_config)s")
set(PYTHON_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/..")
set(PYTHON_BINARY_DIR "${CMAKE_CURRENT_BINARY_DIR}/..")
if(BUILD_SHARED_LIBS)
  set(ENABLE_SHARED 1)
else()
  set(ENABLE_SHARED 0)
endif(BUILD_SHARED_LIBS)

configure_file(
  ${CMAKE_CURRENT_SOURCE_DIR}/lit.site.cfg.in
  ${CMAKE_CURRENT_BINARY_DIR}/lit.site.cfg)

include(FindPythonInterp)
if(PYTHONINTERP_FOUND)
  if( LLVM_MAIN_SRC_DIR )
    set(LIT "${LLVM_SOURCE_DIR}/utils/lit/lit.py")
  else()
    set(LIT "${PATH_TO_LLVM_BUILD}/bin/${CMAKE_CFG_INTDIR}/llvm-lit")
    # Installed LLVM does not contain ${CMAKE_CFG_INTDIR} in paths.
    if( NOT EXISTS ${LIT} )
      set(LIT "${PATH_TO_LLVM_BUILD}/bin/llvm-lit")
    endif()
  endif()

  if( PATH_TO_LLVM_BUILD )
    set(PYTHON_TEST_EXTRA_ARGS "--path=${PYTHON_BINARY_DIR}/bin/${CMAKE_CFG_INTDIR}")
  endif()

  option(PYTHON_TEST_USE_VG "Run Python tests under Valgrind" OFF)
  if(PYTHON_TEST_USE_VG)
    set(PYTHON_TEST_EXTRA_ARGS ${PYTHON_TEST_EXTRA_ARGS} "--vg")
  endif ()

  set(LIT_ARGS "${PYTHON_TEST_EXTRA_ARGS} ${LLVM_LIT_ARGS}")
  separate_arguments(LIT_ARGS)

  add_custom_target(python-test.deps)
  set_target_properties(python-test.deps PROPERTIES FOLDER "Python tests")

  add_custom_target(python-test
    COMMAND ${PYTHON_EXECUTABLE}
                ${LIT}
                --param python_site_config=${CMAKE_CURRENT_BINARY_DIR}/lit.site.cfg
                --param python_unit_site_config=${CMAKE_CURRENT_BINARY_DIR}/Unit/lit.site.cfg
                --param build_config=${CMAKE_CFG_INTDIR}
                --param build_mode=${RUNTIME_BUILD_MODE}
                ${LIT_ARGS}
                ${CMAKE_CURRENT_BINARY_DIR}
                COMMENT "Running Python regression tests")
  set_target_properties(python-test PROPERTIES FOLDER "Python tests")

  if( NOT PYTHON_BUILT_STANDALONE )
    add_custom_target(check-py
      COMMAND ${PYTHON_EXECUTABLE}
                ${LIT}
                --param build_config=${CMAKE_CFG_INTDIR}
                --param build_mode=${RUNTIME_BUILD_MODE}
                ${LIT_ARGS}
                ${LLVM_BINARY_DIR}/test
                ${CMAKE_CURRENT_BINARY_DIR}
                COMMENT "Running Python and LLVM regression tests")
    add_dependencies(check-py python-test.deps)
    if ( LLVM_INCLUDE_TESTS )
      add_dependencies(python-test.deps PythonUnitTests)
      add_dependencies(check-py check.deps)
    endif ( LLVM_INCLUDE_TESTS )
    add_dependencies(python-test.deps
                     llvm-dis llc opt
                     FileCheck count not
                     )
    set_target_properties(check-py PROPERTIES FOLDER "Python tests")
  endif()

  add_dependencies(python-test python-test.deps)
  #add_dependencies(python-test.deps
  #               
  #               )

endif()
