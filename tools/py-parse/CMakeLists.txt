set(LLVM_USED_LIBS
  pyLex
  pyParse
  pyRuntime
  )

set( LLVM_LINK_COMPONENTS
  support
  codegen
  )

add_python_executable(py-parse
  py-parse.cpp
  )
