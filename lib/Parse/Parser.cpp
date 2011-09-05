//===--- Parser.cpp - Python Parser ---------------------------------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
//  This file implements the Parser interface.
//
//===----------------------------------------------------------------------===//
//===----------------------------------------------------------------------===//

#include "py/Lex/Lexer.h"
#include "py/Parse/Parser.h"
#include "py/Diagnostic.h"
#include "llvm/ADT/StringSwitch.h"
#include "llvm/BasicBlock.h"
#include "llvm/Type.h"
#include <iostream>

using namespace py;
using namespace llvm;

#define LEX(T) do {                             \
    if (!L.Lex(T) && AreLexerErrors())          \
      return PNode();                           \
    if (T.getKind() == tok::eof)                \
      return PNode();                           \
  } while(0)

Parser::Parser(Lexer &L, LLVMContext &C, Module &M,
               llvm::raw_ostream &DS) :
  L(L), Context(C), Mod(M), DebugStream(DS) {
}

bool Parser::AreLexerErrors() {
  for (llvm::SmallVector<Diagnostic, 5>::iterator
         it = L.getDiagnostics().begin(),
         end = L.getDiagnostics().end();
       it != end;
       ++it) {
    if (it->getSeverity() == Diagnostic::Error)
      return true;
  }
  return false;
}     

bool Parser::ParseFile() {
  return ParseFileInput();
}

bool Parser::ParseSingleInput() {
  assert(0 && "Unimplemented!");
}

bool Parser::ParseEvalInput() {
  assert(0 && "Unimplemented!");
}

bool Parser::ParseRule(std::string Rule) {
  int I = llvm::StringSwitch<int>(Rule)
    .Case("file_input", 0)
    .Case("single_input", 1)
    .Case("eval_input", 2)
    .Case("compound_stmt", 3)
    .Case("simple_stmt", 4)
    .Case("atom", 999)
    .Case("STRING", 1000)
    .Default(-1);

  switch (I) {
  case 0: return ParseFileInput();
  case 1: return ParseSingleInput();
  case 2: return ParseEvalInput();
  default:
    break;
  }

  Function *F = cast<Function>(
    Mod.getOrInsertFunction("parse-rule",
                            Type::getVoidTy(Context),
                            NULL));
  BasicBlock *BB = &F->getEntryBlock();
  
  Token T;
  LEX(T);
  switch (I) {
  case 3: return ParseCompoundStmt(T);
  case 4: return ParseSimpleStmt(T);
  case 999: return ParseAtom(T, &BB);
  case 1000: return ParseOneString(T);
  default:
    assert(0 && "Unhandled case!");
  }
}

// file_input: (NEWLINE | stmt)* ENDMARKER
Parser::PNode Parser::ParseFileInput() {
  Token T;

  while (true) {
    LEX(T);

    switch (T.getKind()) {
    case tok::newline:
      continue;
    case tok::eof:
      return Parser::PNode();
    default:
      if (!ParseStmt(T))
        return Parser::PNode();
      continue;
    }
  }
  assert(0 && "Unreachable!");
}

// stmt: simple_stmt | compound_stmt
// Disambiguate:
//   compound_stmt: if_stmt | while_stmt | for_stmt | try_stmt | with_stmt | funcdef | classdef | decorated
Parser::PNode Parser::ParseStmt(Token &T) {
  switch (T.getKind()) {
  case tok::kw_if:
  case tok::kw_while:
  case tok::kw_for:
  case tok::kw_try:
  case tok::kw_with:
  case tok::kw_def:
  case tok::kw_class:
  case tok::at:
    return ParseCompoundStmt(T);
  default:
    return ParseSimpleStmt(T);
  }
}

Parser::PNode Parser::ParseCompoundStmt(Token &T) {
  std::cerr << "Compound\n";
  return PNode();
}

Parser::PNode Parser::ParseSimpleStmt(Token &T) {
  std::cerr << "Simple\n";
  return PNode();
}
