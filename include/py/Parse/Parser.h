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
#include "llvm/Module.h"
#include "llvm/LLVMContext.h"
#include "py/Lex/Token.h"
#include "py/Diagnostic.h"
#include "py/Parse/TreePrinter.h"

namespace llvm {
  class Constant;
  class StringRef;
  class Twine;
  class raw_ostream;
  class BasicBlock;
}

namespace py {

class Lexer;

/// Parser - This takes a Lexer and produces LLVM bitcode from it,
/// with calls to Python intrinsics for complex behaviour.
class Parser {
  /// The Lexer this Parser reads from.
  Lexer &L;

  /// LLVMContext to create all constants in.
  llvm::LLVMContext &Context;

  /// Module to populate during parsing.
  llvm::Module &Mod;

  /// Diagnostic array.
  llvm::SmallVector<Diagnostic, 5> Diagnostics;

  /// Output stream for dumping the tree structure to.
  /// FIXME: #ifdef DEBUG
  TreePrinter DebugStream;

  /// Inner, private class defining the value that will be passed
  /// between parse calls.
  class PNode {
  public:
    PNode(Token &FirstTok, llvm::Value *V=0) :
      Loc(FirstTok.getLocation()),
      V(V) {
    }
    PNode() : V(0) {
    }
    operator bool() {return V != 0;}
    
    llvm::Value *Value() {
      return V;
    }

  private:
    llvm::SMLoc Loc;
    llvm::Value *V;
    bool Valid;
  };

public:
  /// Constructor - creates a new Parser, given a Lexer to pull
  /// tokens from, a Context and Module to populate, and a 
  /// stream to write debug info to (can be nulls()).
  Parser(Lexer &L, llvm::LLVMContext &C, llvm::Module &M,
         llvm::raw_ostream &DS);

  /// Main parse routine - lexes tokens until EOF. Continues
  /// if Warning diagnostics produced, stops on Errors.
  bool ParseFile();
  bool ParseSingleInput();
  bool ParseEvalInput();
  
  /// Secondary parse routine, for testing. Parses, but starts
  /// at a particular rule.
  bool ParseRule(std::string Rule);

  llvm::SmallVector<Diagnostic, 5> &getDiagnostics() {
    return Diagnostics;
  }

private:
  PNode ParseFileInput();
  PNode ParseStmt(Token &T);
  PNode ParseCompoundStmt(Token &T);
  PNode ParseSimpleStmt(Token &T);

  PNode ParseAtom(Token &T, llvm::BasicBlock **BB);
  PNode ParseYieldExprOrTestlistComp(Token &T, llvm::BasicBlock **BB);
  PNode ParseListMaker(Token &T, llvm::BasicBlock **BB);
  PNode ParseDictOrSetMaker(Token &T, llvm::BasicBlock **BB);
  PNode ParseTestlist1(Token &T, llvm::BasicBlock **BB);
  PNode ParseName(Token &T);
  PNode ParseNumber(Token &T);
  PNode ParseOneOrMoreStrings(Token &T);
  PNode ParseOneString(Token &T);

  /// Emits a diagnostic to the diagnostics list.
  void Diag(Token &T, const char *str, Diagnostic::Severity s);

  /// Checks the Lexer object to see if any fatal errors have
  /// occurred (or just warnings).
  bool AreLexerErrors();

  /// Removes escapes from the string, and removes the surrounding
  /// quotes.
  ///
  /// Encodes as UTF-8.
  llvm::StringRef SanitizeString(llvm::StringRef S);
  
  /// Returns a ConstantArray initialized to T.
  llvm::Constant *GetConstantString(const llvm::Twine &T);

};

}

#endif
