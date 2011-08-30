//===--- Lexer.h - Python Lexer ---------------------------------*- C++ -*-===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
//  This file defines the Lexer interface.
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_PY_TOKENKIND_H
#define LLVM_PY_TOKENKIND_H

namespace py {
namespace tok {

enum TokenKind {
#define TOK(X) X,
#include "py/Lex/TokenKinds.def"
end
};

}
}

#endif
