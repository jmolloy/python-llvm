//===--- Lexer.cpp - C Language Family Lexer ------------------------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
//  This file implements the Lexer and Token interfaces.
//
//===----------------------------------------------------------------------===//
//===----------------------------------------------------------------------===//

#include "py/Lex/Lexer.h"
#include "py/Diagnostic.h"
#include "llvm/ADT/StringSwitch.h"
#include "llvm/Support/Compiler.h"
#include "llvm/Support/MemoryBuffer.h"
#include <iostream>
using namespace py;
using namespace llvm;

static void InitCharacterInfo();

//===----------------------------------------------------------------------===//
// Lexer Class Implementation
//===----------------------------------------------------------------------===//

#define INITIAL_INDENT()                                        \
  if (AtLineStart && LexPossibleIndent(Result, 0, &error)) {    \
    unget();                                                    \
    return true;                                                \
  }                                                             \
  if (error) {                                                  \
    unget();                                                    \
    return false;                                               \
  }                                                             \
  AtLineStart = false;                                          \


/// Lexer constructor - Create a new lexer object for the specified buffer
/// with the specified preprocessor managing the lexing process.  This lexer
/// assumes that the associated file buffer and Preprocessor objects will
/// outlive it, so it doesn't take ownership of either of them.
Lexer::Lexer(const MemoryBuffer *InputBuffer, LangFeatures features) :
  Buffer(InputBuffer), TokStart(0), Ptr(0),
  Features(features), AtLineStart(true), IndentStackTop(0),
  NumDedents(0), LastCharLen(0) {
  InitCharacterInfo();

  IndentStack[IndentStackTop] = 0;

  TokStart = Ptr = InputBuffer->getBufferStart();
}

//===----------------------------------------------------------------------===//
// Character information.
//===----------------------------------------------------------------------===//

enum {
  CHAR_HORZ_WS  = 0x01,  // ' ', '\t', '\f', '\v'.  Note, no '\0'
  CHAR_VERT_WS  = 0x02,  // '\r', '\n'
  CHAR_LETTER   = 0x04,  // a-z,A-Z
  CHAR_NUMBER   = 0x08,  // 0-9
  CHAR_UNDER    = 0x10,  // _
  CHAR_PERIOD   = 0x20   // .
};

// Statically initialize CharInfo table based on ASCII character set
// Reference: FreeBSD 7.2 /usr/share/misc/ascii
static const unsigned char CharInfo[256] =
{
// 0 NUL         1 SOH         2 STX         3 ETX
// 4 EOT         5 ENQ         6 ACK         7 BEL
   0           , 0           , 0           , 0           ,
   0           , 0           , 0           , 0           ,
// 8 BS          9 HT         10 NL         11 VT
//12 NP         13 CR         14 SO         15 SI
   0           , CHAR_HORZ_WS, CHAR_VERT_WS, CHAR_HORZ_WS,
   CHAR_HORZ_WS, CHAR_VERT_WS, 0           , 0           ,
//16 DLE        17 DC1        18 DC2        19 DC3
//20 DC4        21 NAK        22 SYN        23 ETB
   0           , 0           , 0           , 0           ,
   0           , 0           , 0           , 0           ,
//24 CAN        25 EM         26 SUB        27 ESC
//28 FS         29 GS         30 RS         31 US
   0           , 0           , 0           , 0           ,
   0           , 0           , 0           , 0           ,
//32 SP         33  !         34  "         35  #
//36  $         37  %         38  &         39  '
   CHAR_HORZ_WS, 0           , 0           , 0           ,
   0           , 0           , 0           , 0           ,
//40  (         41  )         42  *         43  +
//44  ,         45  -         46  .         47  /
   0           , 0           , 0           , 0           ,
   0           , 0           , CHAR_PERIOD , 0           ,
//48  0         49  1         50  2         51  3
//52  4         53  5         54  6         55  7
   CHAR_NUMBER , CHAR_NUMBER , CHAR_NUMBER , CHAR_NUMBER ,
   CHAR_NUMBER , CHAR_NUMBER , CHAR_NUMBER , CHAR_NUMBER ,
//56  8         57  9         58  :         59  ;
//60  <         61  =         62  >         63  ?
   CHAR_NUMBER , CHAR_NUMBER , 0           , 0           ,
   0           , 0           , 0           , 0           ,
//64  @         65  A         66  B         67  C
//68  D         69  E         70  F         71  G
   0           , CHAR_LETTER , CHAR_LETTER , CHAR_LETTER ,
   CHAR_LETTER , CHAR_LETTER , CHAR_LETTER , CHAR_LETTER ,
//72  H         73  I         74  J         75  K
//76  L         77  M         78  N         79  O
   CHAR_LETTER , CHAR_LETTER , CHAR_LETTER , CHAR_LETTER ,
   CHAR_LETTER , CHAR_LETTER , CHAR_LETTER , CHAR_LETTER ,
//80  P         81  Q         82  R         83  S
//84  T         85  U         86  V         87  W
   CHAR_LETTER , CHAR_LETTER , CHAR_LETTER , CHAR_LETTER ,
   CHAR_LETTER , CHAR_LETTER , CHAR_LETTER , CHAR_LETTER ,
//88  X         89  Y         90  Z         91  [
//92  \         93  ]         94  ^         95  _
   CHAR_LETTER , CHAR_LETTER , CHAR_LETTER , 0           ,
   0           , 0           , 0           , CHAR_UNDER  ,
//96  `         97  a         98  b         99  c
//100  d       101  e        102  f        103  g
   0           , CHAR_LETTER , CHAR_LETTER , CHAR_LETTER ,
   CHAR_LETTER , CHAR_LETTER , CHAR_LETTER , CHAR_LETTER ,
//104  h       105  i        106  j        107  k
//108  l       109  m        110  n        111  o
   CHAR_LETTER , CHAR_LETTER , CHAR_LETTER , CHAR_LETTER ,
   CHAR_LETTER , CHAR_LETTER , CHAR_LETTER , CHAR_LETTER ,
//112  p       113  q        114  r        115  s
//116  t       117  u        118  v        119  w
   CHAR_LETTER , CHAR_LETTER , CHAR_LETTER , CHAR_LETTER ,
   CHAR_LETTER , CHAR_LETTER , CHAR_LETTER , CHAR_LETTER ,
//120  x       121  y        122  z        123  {
//124  |        125  }        126  ~        127 DEL
   CHAR_LETTER , CHAR_LETTER , CHAR_LETTER , 0           ,
   0           , 0           , 0           , 0
};

static void InitCharacterInfo() {
  static bool isInited = false;
  if (isInited) return;
  // check the statically-initialized CharInfo table
  assert(CHAR_HORZ_WS == CharInfo[(int)' ']);
  assert(CHAR_HORZ_WS == CharInfo[(int)'\t']);
  assert(CHAR_HORZ_WS == CharInfo[(int)'\f']);
  assert(CHAR_HORZ_WS == CharInfo[(int)'\v']);
  assert(CHAR_VERT_WS == CharInfo[(int)'\n']);
  assert(CHAR_VERT_WS == CharInfo[(int)'\r']);
  assert(CHAR_UNDER   == CharInfo[(int)'_']);
  assert(CHAR_PERIOD  == CharInfo[(int)'.']);
  for (unsigned i = 'a'; i <= 'z'; ++i) {
    assert(CHAR_LETTER == CharInfo[i]);
    assert(CHAR_LETTER == CharInfo[i+'A'-'a']);
  }
  for (unsigned i = '0'; i <= '9'; ++i)
    assert(CHAR_NUMBER == CharInfo[i]);
    
  isInited = true;
}


/// isIdentifierBody - Return true if this is the body character of an
/// identifier, which is [a-zA-Z0-9_].
static inline bool isIdentifierBody(unsigned char c) {
  return (CharInfo[c] & (CHAR_LETTER|CHAR_NUMBER|CHAR_UNDER)) ? true : false;
}

/// isHorizontalWhitespace - Return true if this character is horizontal
/// whitespace: ' ', '\t', '\f', '\v'.  Note that this returns false for '\0'.
static inline bool isHorizontalWhitespace(unsigned char c) {
  return (CharInfo[c] & CHAR_HORZ_WS) ? true : false;
}

/// isVerticalWhitespace - Return true if this character is vertical
/// whitespace: '\n', '\r'.  Note that this returns false for '\0'.
static inline bool isVerticalWhitespace(unsigned char c) {
  return (CharInfo[c] & CHAR_VERT_WS) ? true : false;
}

/// isWhitespace - Return true if this character is horizontal or vertical
/// whitespace: ' ', '\t', '\f', '\v', '\n', '\r'.  Note that this returns false
/// for '\0'.
static inline bool isWhitespace(unsigned char c) {
  return (CharInfo[c] & (CHAR_HORZ_WS|CHAR_VERT_WS)) ? true : false;
}

/// isNumberBody - Return true if this is the body character of an
/// preprocessing number, which is [a-zA-Z0-9_.].
static inline bool isNumberBody(unsigned char c) {
  return (CharInfo[c] & (CHAR_LETTER|CHAR_NUMBER|CHAR_UNDER|CHAR_PERIOD)) ?
    true : false;
}


//===----------------------------------------------------------------------===//
// Helper methods for lexing.
//===----------------------------------------------------------------------===//

char Lexer::getAscii() {
  // FIXME: Check for ascii-ness and encodings.
  LastCharLen = 1;
  return *Ptr++;
}
unsigned Lexer::getUnicode() {
  // FIXME: Encodings, and stuff.
  LastCharLen = 1;
  return *Ptr++;
}
void Lexer::unget() {
  Ptr -= LastCharLen;
}

char Lexer::peekAscii(unsigned lookahead) {
  // FIXME: Check for ascii-ness and encodings.
  const char *P = Ptr;
  unsigned C = *P;
  while (lookahead--) {
    if (*P == '\0') return '\0';
    P += 1; // FIXME: Char length;
    C = *P;
  }
  return (C > 127) ? 0 : (char)C;
}

void Lexer::Diag(const char *str, Diagnostic::Severity s) {
  Diagnostic d(llvm::SMLoc::getFromPointer(TokStart),
               s,
               str);
  Diagnostics.push_back(d);
}              

unsigned Lexer::CountWhitespace(char C) {
  unsigned n = (C == '\t') ? TAB_WIDTH : 1;

  while (true) {
    C = getAscii();
    switch (C) {
    case ' ':
      ++n;
      break;
    case '\t':
      n += TAB_WIDTH;
      break;
    default:
      // Not whitespace.
      unget();
      return n;
    }
  }
}

bool Lexer::LexPossibleIndent(Token &Result, unsigned indent, bool *error) {
  unsigned tos = IndentStack[IndentStackTop];
  if (indent > tos) {
    IndentStack[++IndentStackTop] = indent;
    MakeToken(Result, tok::indent);
    return true;
  }

  unsigned dedents = 0;
  while (IndentStackTop >= 0) {
    if (indent == tos && !dedents)
      return false;
    else if (indent == tos && dedents) {
      MakeToken(Result, tok::dedent);
      NumDedents = dedents-1;
      return true;
    }

    if (indent > tos) {
      Diag("Unexpected indent", Diagnostic::Error);
      *error = true;
      return false;
    }

    ++dedents;
    tos = IndentStack[--IndentStackTop];
  }

  assert(0 && "Fell off the end of the indent stack!"); 
}

bool Lexer::LexIdentifier(Token &Result) {
  // Match [_A-Za-z0-9]*, we have already matched [_A-Za-z$]
  char C = getAscii();
  while (isIdentifierBody(C = getAscii()))
    ;

  unget(); // Back up over the last character.

  MakeToken(Result, tok::identifier);
  return true;
}

/// LexNumericConstant - Lex the remainder of a integer or floating point
/// constant. From[-1] is the first character lexed.  Return the end of the
/// constant.
bool Lexer::LexNumericConstant(Token &Result) {
  char C = getAscii();
  char PrevCh = Ptr[-1];
  while (isNumberBody(C)) { // FIXME: UCNs?
    PrevCh = C;
    C = getAscii();
  }

  // If we fell out, check for a sign, due to 1e+12.  If we have one, continue.
  if ((C == '-' || C == '+') && (PrevCh == 'E' || PrevCh == 'e')) {
    return LexNumericConstant(Result);
  }

  // If we have a hex FP constant, continue.
  if ((C == '-' || C == '+') && (PrevCh == 'P' || PrevCh == 'p'))
    return LexNumericConstant(Result);

  // Back up over previous bad character.
  unget();
  
  // Update the location of token as well as BufferPtr.
  MakeToken(Result, tok::numeric_constant);
  return true;
}

bool Lexer::LexStringConstant(Token &Result, char Delimiter) {
  bool IsEscape = false;
  bool Success = true;
  unsigned C = getUnicode();
  while (C) {
    if (C == (unsigned)'\\' && !IsEscape) {
      IsEscape = true;
    } else if (C == (unsigned)Delimiter && !IsEscape) {
      MakeToken(Result, tok::string_constant);
      return Success;
    } else if (IsEscape) {
      IsEscape = false;
    } else if (C == '\n') {
      Diag("Newline in string constant", Diagnostic::Warning);
      Success = false;
    }
    C = getUnicode();
  }
  MakeToken(Result, tok::string_constant);
  Diag("Unterminated string constant", Diagnostic::Error);
  return false;
}

bool Lexer::LexFatStringConstant(Token &Result, char Delimiter) {
  bool IsEscape = false;
  bool Success = true;
  unsigned C = getUnicode();
  while (C) {
    if (C == (unsigned)'\\' && !IsEscape) {
      IsEscape = true;
    } else if (C == (unsigned)Delimiter && !IsEscape &&
               peekAscii(0) == Delimiter &&
               peekAscii(1) == Delimiter) {
      getAscii(); getAscii();
      MakeToken(Result, tok::string_constant);
      return Success;
    } else if (IsEscape) {
      IsEscape = false;
    }
    C = getUnicode();
  }
  MakeToken(Result, tok::string_constant);
  Diag("Unterminated fat string constant", Diagnostic::Error);
  return false;
}

//===----------------------------------------------------------------------===//
// Main lexer entry point.
//===----------------------------------------------------------------------===//


bool Lexer::Lex(Token &Result) {
  if (NumDedents) {
    MakeToken(Result, tok::dedent);
    --NumDedents;
    return true;
  }

  while (true) {
    TokStart = Ptr;
    char Char = getAscii();
    bool error = false;

    switch (Char) {
    case '\0':
      // EOF?
      if (Ptr >= Buffer->getBufferEnd()) {
        Result.setKind(tok::eof);
        return true;
      }
      AtLineStart = false;
      continue;

    case ' ':
    case '\t':
      if (AtLineStart) {
        AtLineStart = false;
        // Whitespace significant at beginning of line.
        int indent = CountWhitespace(Char);
        if (LexPossibleIndent(Result, indent, &error))
          return true;
        if (error) return false;
        // Fall through - insignificant whitespace.
      }
      continue;

    case '\n':
    case '\r':
      MakeToken(Result, tok::newline);
      // Skip multiple newlines, but not all whitespace
      // as start-of-line whitespace is significant.
      while (*Ptr == '\n' || *Ptr == '\r')
        ++Ptr;
      AtLineStart = true;
      return true;

    case '0': case '1': case '2': case '3': case '4':
    case '5': case '6': case '7': case '8': case '9':
      INITIAL_INDENT();
      return LexNumericConstant(Result);

    case '#': {
      // Comment - zap to end of line.
      unsigned I = getUnicode();
      while (I && I != (unsigned)'\n')
        I = getUnicode();
      while (*Ptr == '\n' || *Ptr == '\r')
        ++Ptr;
      AtLineStart = true;
      continue;
    }

    case '"':
    case '\'':
      AtLineStart = false;
      if ((peekAscii(0) == '"' && peekAscii(1) == '"') ||
          (peekAscii(1) == '\'' && peekAscii(1) == '\'')) {
        getAscii(); getAscii();
        return LexFatStringConstant(Result, Char);
      }
      return LexStringConstant(Result, Char);

    default:
      assert(0 && "Unhandled character!");
      return false;
    }
  }
}
