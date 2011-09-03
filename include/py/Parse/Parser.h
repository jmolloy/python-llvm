//===--- Parser.h - Python Parser -------------------------------*- C++ -*-===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
//  This file defines the Parser interface.
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_PY_PARSER_H
#define LLVM_PY_PARSER_H

#include "llvm/Support/SourceMgr.h"
#include "py/Lex/Token.h"

namespace py {

class Lexer;

/// Parser - This takes a Lexer and produces LLVM bitcode from it,
/// with calls to Python intrinsics for complex behaviour.
class Parser {
  /// The Lexer this Parser reads from.
  Lexer &L;

  /// Inner, private class defining the value that will be passed
  /// between parse calls.
  class PNode {
  public:
    PNode(Token &FirstTok) :
      Loc(FirstTok.getLocation()),
      Valid(true) {
    }
    PNode() : Valid(false) {
    }
    operator bool() {return Valid;}
  private:
    llvm::SMLoc Loc;
    bool Valid;
  };

public:
  /// Constructor - creates a new Parser.
  Parser(Lexer &L);

  /// Main parse routine - lexes tokens until EOF. Continues
  /// if Warning diagnostics produced, stops on Errors.
  bool ParseFile();
  bool ParseSingleInput();
  bool ParseEvalInput();
  
  /// Secondary parse routine, for testing. Parses, but starts
  /// at a particular rule.
  bool ParseRule(std::string Rule);

private:
  PNode ParseFileInput();
  PNode ParseStmt(Token &T);
  PNode ParseCompoundStmt(Token &T);
  PNode ParseSimpleStmt(Token &T);

  /// Checks the Lexer object to see if any fatal errors have
  /// occurred (or just warnings).
  bool AreLexerErrors();
  
};

}

#endif
