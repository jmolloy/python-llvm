set(LLVM_USED_LIBS
  pyLex
  )

set( LLVM_LINK_COMPONENTS
  support
  )

add_python_executable(py-lex
  py-lex.cpp
  )
