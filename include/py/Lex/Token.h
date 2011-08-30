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

#ifndef LLVM_PY_TOKEN_H
#define LLVM_PY_TOKEN_H

#include "llvm/Support/SourceMgr.h"
#include "py/Lex/TokenKind.h"

namespace py {

class Token {
public:
  unsigned getLength() {
    return Length;
  }
  void setLength(unsigned len) {
    Length = len;
  }
  llvm::SMLoc getLocation() {
    return Loc;
  }
  void setLocation(llvm::SMLoc loc) {
    Loc = loc;
  }
  tok::TokenKind getKind() {
    return Kind;
  }
  void setKind(tok::TokenKind kind) {
    Kind = kind;
  }
  char *getContent() {
    return Content;
  }
  void setContent(char *c) {
    Content = c;
  }
  void setContent(const char *c) {
    Content = const_cast<char*>(c);
  }
  std::string getString() {
    return std::string(Content, Length);
  }

private:
  unsigned Length;
  llvm::SMLoc Loc;
  tok::TokenKind Kind;
  char *Content;
};

}

#endif
