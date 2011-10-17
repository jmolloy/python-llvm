set(LLVM_LINK_COMPONENTS support)

set(LLVM_USED_LIBS )

add_python_library(pyParse
  Parser.cpp
  Atoms.cpp
  Support.cpp
  Exprs.cpp
  TreePrinter.cpp
  )

#add_dependencies(clangLex )
