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

#ifndef LLVM_PY_LEXER_H
#define LLVM_PY_LEXER_H

#include "llvm/ADT/SmallVector.h"
#include "llvm/Support/MemoryBuffer.h"
#include "llvm/Support/SourceMgr.h"
#include "py/LangFeatures.h"
#include "py/Diagnostic.h"
#include "Token.h"

/// Maximum extent of the indent stack.
#define INDENT_STACK_MAX 256
/// Maximum extent of the brace stack.
#define BRACE_STACK_MAX 256
/// The width a tab should have in spaces.
#define TAB_WIDTH 8

namespace py {

/// Lexer - This provides a simple interface that turns a text buffer into a
/// stream of tokens.  This provides no support for file reading or buffering,
/// or buffering/seeking of tokens, only forward lexing is supported.
class Lexer {

  /// The buffer for this lexer to read from.
  const llvm::MemoryBuffer *Buffer;
  
  /// Start of the current token in Buffer.
  const char *TokStart;
  
  /// Current location in buffer.
  const char *Ptr;

  /// Currently enabled language features.
  LangFeatures Features;

  /// Currently at the start of a line?
  bool AtLineStart;
  
  llvm::SmallVector<Diagnostic, 5> Diagnostics;

  unsigned IndentStack[INDENT_STACK_MAX];
  signed IndentStackTop;

  unsigned BraceStack[BRACE_STACK_MAX];
  signed BraceStackTop;

  unsigned NumDedents;

  unsigned LastCharLen;

  Lexer(const Lexer&);          // DO NOT IMPLEMENT
  void operator=(const Lexer&); // DO NOT IMPLEMENT

public:

  /// Lexer constructor - Create a new lexer object for the specified buffer
  /// with the specified language features enabled/disabled.
  Lexer(const llvm::MemoryBuffer *InputBuffer, LangFeatures features);

  /// getFeatures - 
  const LangFeatures &getFeatures() const { return Features; }

  /// Lex - Return the next token in the file.  If this is the end of file, it
  /// return the tok::eof token.  Return false if an error occurred and
  /// compilation should terminate, true if normal.
  bool Lex(Token &Result);

  llvm::SmallVector<Diagnostic, 5> &getDiagnostics() {
    return Diagnostics;
  }

private:

  void MakeToken(Token &Result, tok::TokenKind Kind) {
    unsigned TokLen = Ptr-TokStart;
    Result.setLength(TokLen);
    Result.setLocation(llvm::SMLoc::getFromPointer(TokStart));
    Result.setContent(TokStart);
    Result.setKind(Kind);
    TokStart = Ptr;
  }

  char getAscii();
  unsigned getUnicode();
  void unget();

  char peekAscii(unsigned Lookahead=0);

  void Diag(const char *str, Diagnostic::Severity s);

  // Helper functions to lex the remainder of a token of the specific type.
  bool LexIdentifier         (Token &Result);
  bool LexNumericConstant    (Token &Result);
  bool LexStringConstant     (Token &Result, char Delimiter);
  bool LexFatStringConstant  (Token &Result, char Delimiter);
  bool LexPossibleIndent     (Token &Result, unsigned indent, bool *error);

  unsigned CountWhitespace(char C);
  // void LexStringLiteral      (Token &Result, const char *CurPtr,
  //                             tok::TokenKind Kind);
  // bool LexEndOfFile          (Token &Result, const char *CurPtr);

  // bool SkipWhitespace        (Token &Result, const char *CurPtr);
  // bool SkipBCPLComment       (Token &Result, const char *CurPtr);
  // bool SkipBlockComment      (Token &Result, const char *CurPtr);
  // bool SaveBCPLComment       (Token &Result, const char *CurPtr);  
};


}  // end namespace py

#endif
