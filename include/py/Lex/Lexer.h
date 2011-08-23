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
#include "llvm/MemoryBuffer.h"
#include "llvm/Support/SourceMgr.h"
#include "py/LangFeatures.h"
#include <string>
#include <cassert>

namespace py {

/// Lexer - This provides a simple interface that turns a text buffer into a
/// stream of tokens.  This provides no support for file reading or buffering,
/// or buffering/seeking of tokens, only forward lexing is supported.
class Lexer {

  /// The buffer for this lexer to read from.
  MemoryBuffer *Buffer;
  
  /// If true, comments are lexed and returned as tokens.
  bool KeepComments;

  /// Start of the current token in Buffer.
  const char *TokStart;

  // IsAtStartOfLine - True if the next lexed token should get the "start of
  // line" flag set on it.
  bool IsAtStartOfLine;

  /// Currently enabled language features.
  LangFeatures Features;

  Lexer(const Lexer&);          // DO NOT IMPLEMENT
  void operator=(const Lexer&); // DO NOT IMPLEMENT

public:

  /// Lexer constructor - Create a new lexer object for the specified buffer
  /// with the specified language features enabled/disabled.
  Lexer(const llvm::MemoryBuffer *InputBuffer, LangFeatures features);

  /// getFeatures - 
  const LangOptions &getFeatures() const { return Features; }

  /// Lex - Return the next token in the file.  If this is the end of file, it
  /// return the tok::eof token.  Return false if an error occurred and
  /// compilation should terminate, true if normal.
  bool Lex(Token &Result) {
    if (IsAtStartOfLine) {
      Result.setFlag(Token::StartOfLine);
      IsAtStartOfLine = false;
    }

    // Get a token.  Note that this may delete the current lexer if the end of
    // file is reached.
    return LexTokenInternal(Result);
  }

  /// inKeepCommentMode - Return true if the lexer should return comments as
  /// tokens.
  bool inKeepCommentMode() const {
    return KeepComments;
  }

  /// \brief Return the current location in the buffer.
  const char *getBufferLocation() const { return BufferPtr; }

  llvm::SmallVector<Diagnostic> &getDiagnostics() {
    return Diagnostics;
  }

  char getCharAndSizeNoWarn(const char *ptr, unsigned &Size, const LangFeatures &Features) {
    Size = 1;
    return *ptr;
  }
  char getCharAndSize(const char *ptr, unsigned &Size, const LangFeatures &Features) {
    Size = 1;
    return *ptr;
  }


private:

  /// LexTokenInternal - Internal interface to lex a preprocessing token. Called
  /// by Lex.
  ///
  void LexTokenInternal(Token &Result);

  /// FormTokenWithChars - When we lex a token, we have identified a span
  /// starting at BufferPtr, going to TokEnd that forms the token.  This method
  /// takes that range and assigns it to the token as its location and size.  In
  /// addition, since tokens cannot overlap, this also updates BufferPtr to be
  /// TokEnd.
  void FormTokenWithChars(Token &Result, const char *TokEnd,
                          tok::TokenKind Kind) {
    unsigned TokLen = TokEnd-BufferPtr;
    Result.setLength(TokLen);
    Result.setLocation(llvm::SMLoc(BufferPtr));
    Result.setKind(Kind);
    BufferPtr = TokEnd;
  }

  //===--------------------------------------------------------------------===//
  // Lexer character reading interfaces.
public:

  inline char get(const char *&Ptr, Token &Tok) {
    return *Ptr++;
  }

private:

  //===--------------------------------------------------------------------===//
  // Other lexer functions.

  void SkipBytes(unsigned Bytes, bool StartOfLine);
  
  // Helper functions to lex the remainder of a token of the specific type.
  void LexIdentifier         (Token &Result, const char *CurPtr);
  void LexNumericConstant    (Token &Result, const char *CurPtr);
  void LexStringLiteral      (Token &Result, const char *CurPtr,
                              tok::TokenKind Kind);
  bool LexEndOfFile          (Token &Result, const char *CurPtr);

  bool SkipWhitespace        (Token &Result, const char *CurPtr);
  bool SkipBCPLComment       (Token &Result, const char *CurPtr);
  bool SkipBlockComment      (Token &Result, const char *CurPtr);
  bool SaveBCPLComment       (Token &Result, const char *CurPtr);  
};


}  // end namespace clang

#endif
