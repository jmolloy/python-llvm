set(LLVM_LINK_COMPONENTS support)

set(LLVM_USED_LIBS )

add_python_library(pyRuntime
  Runtime.cpp
  )

#add_dependencies(clangLex )
