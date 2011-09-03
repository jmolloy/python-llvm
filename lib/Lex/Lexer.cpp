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

#define BRACE_STACK_PUSH(X)                     \
  BraceStack[BraceStackTop++] = X;              \
  assert(BraceStackTop < BRACE_STACK_MAX && "Brace stack overflow!");

#define BRACE_STACK_POP(X)                              \
  if (BraceStackTop == 0 ||                             \
      BraceStack[--BraceStackTop] != X) {               \
    Diag("Mismatched parentheses", Diagnostic::Error);  \
    return false;                                       \
  }                                                     \


/// Lexer constructor - Create a new lexer object for the specified buffer
/// with the specified preprocessor managing the lexing process.  This lexer
/// assumes that the associated file buffer and Preprocessor objects will
/// outlive it, so it doesn't take ownership of either of them.
Lexer::Lexer(const MemoryBuffer *InputBuffer, LangFeatures features) :
  Buffer(InputBuffer), TokStart(0), Ptr(0),
  Features(features), AtLineStart(true), IndentStackTop(0),
  BraceStackTop(0),
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
  char C;
  while (isIdentifierBody(C = getAscii()))
    ;

  unget(); // Back up over the last character.

  // Recognise special identifiers.
  const char *T = TokStart;
  unsigned L = Ptr-TokStart;

  // Early exit - no identifiers are 1 char long.
  if (L == 1) {
    MakeToken(Result, tok::identifier);
    return true;
   }

  switch (*T++) {
  case 'a':
    if (L < 2) break;
    switch (*T++) {
    case 'n':
      if (L == 3 && *T++ == 'd') {
        MakeToken(Result, tok::kw_and);
        return true;
      }
      break;
    case 's':
      if (L == 2) {
        MakeToken(Result, tok::kw_as);
        return true;
      }
      if (*T++ == 's' &&
          !strncmp(T, "ert", 3) &&
          L == 6) {
        MakeToken(Result, tok::kw_assert);
        return true;
      }
      break;
    default:
      break;
    }
    break;

  case 'b':
    if (L == 5 &&
        !strncmp(T, "reak", 4)) {
      MakeToken(Result, tok::kw_break);
      return true;
    }
    break;

  case 'c':
    if (L < 5) break;
    switch (*T++) {
    case 'l':
      if (L == 5 && 
          !strncmp(T, "ass", 3)) {
        MakeToken(Result, tok::kw_class);
        return true;
      }
      break;
    case 'o':
      if (L == 8 &&
          !strncmp(T, "ntinue", 6)) {
        MakeToken(Result, tok::kw_continue);
        return true;
      }
      break;
    default:
      break;
    }
    break;

  case 'd':
    if (*T++ != 'e' || L != 3)
      break;
    switch (*T++) {
    case 'f':
      MakeToken(Result, tok::kw_def);
      return true;
    case 'l':
      MakeToken(Result, tok::kw_del);
      return true;
    default:
      break;
    }
    break;
  
  case 'e':
    if (L < 4) break;
    switch (*T++) {
    case 'l':
      if (L != 4) break;
      switch (*T++) {
      case 'i':
        if (*T++ == 'f') {
          MakeToken(Result, tok::kw_elif);
          return true;
        }
        break;
      case 's':
        if (*T++ == 'e') {
          MakeToken(Result, tok::kw_else);
          return true;
        }
        break;
      default:
        break;
      }
      break;
    case 'x':
      if (L == 4 && *T++ == 'e' && 
          *T++ == 'c') {
        MakeToken(Result, tok::kw_exec);
        return true;
      }
      if (L == 6 && !strncmp(T, "cept", 4)) {
        MakeToken(Result, tok::kw_except);
        return true;
      }
      break;
    default:
      break;
    }
    break;

  case 'f':
    if (L < 3) break;
    switch (*T++) {
    case 'i':
      if (L == 7 && !strncmp(T, "nally", 5)) {
        MakeToken(Result, tok::kw_finally);
        return true;
      }
      break;
    case 'o':
      if (L == 3 && *T == 'r') {
        MakeToken(Result, tok::kw_for);
        return true;
      }
      break;
    case 'r':
      if (L == 4 && *T++ == 'o' &&
          *T == 'm') {
        MakeToken(Result, tok::kw_from);
        return true;
      }
      break;
    default:
      break;
    }
    break;

  case 'g':
    if (L == 6 && !strncmp(T, "lobal", 5)) {
      MakeToken(Result, tok::kw_global);
      return true;
    }
    break;

  case 'i':
    switch (*T++) {
    case 'f':
      if (L == 2) {
        MakeToken(Result, tok::kw_if);
        return true;
      }
      break;
    case 'm':
      if (L == 6 && !strncmp(T, "port", 4)) {
        MakeToken(Result, tok::kw_import);
        return true;
      }
      break;
    case 'n':
      if (L == 2) {
        MakeToken(Result, tok::kw_in);
        return true;
      }
      break;
    case 's':
      if (L == 2) {
        MakeToken(Result, tok::kw_is);
        return true;
      }
      break;
    default:
      break;
    }

  case 'l':
    if (L == 6 && !strncmp(T, "ambda", 5)) {
      MakeToken(Result, tok::kw_lambda);
      return true;
    }
    break;

  case 'n':
    if (L == 3 && *T++ == 'o' && *T == 't') {
      MakeToken(Result, tok::kw_not);
      return true;
    }
    break;

  case 'o':
    if (L == 2 && *T == 'r') {
      MakeToken(Result, tok::kw_or);
      return true;
    }
    break;

  case 'p':
    if (L < 4) break;
    switch (*T++) {
    case 'a':
      if (L == 4 && !strncmp(T, "ss", 2)) {
        MakeToken(Result, tok::kw_pass);
        return true;
      }
      break;
    case 'r':
      // FIXME: Check LangFeatures
      if (L == 5 && !strncmp(T, "int", 3)) {
        MakeToken(Result, tok::kw_print);
        return true;
      }
      break;
    default:
      break;
    }
    break;

  case 'r':
    if (L < 5) break;
    switch (*T++) {
    case 'a':
      if (L == 5 && !strncmp(T, "ise", 3)) {
        MakeToken(Result, tok::kw_raise);
        return true;
      }
      break;
    case 'e':
      if (L == 6 && !strncmp(T, "turn", 4)) {
        MakeToken(Result, tok::kw_return);
        return true;
      }
      break;
    default:
      break;
    }
    break;

  case 't':
    if (L == 3 && *T++ == 'r' && *T == 'y') {
      MakeToken(Result, tok::kw_try);
      return true;
    }
    break;

  case 'w':
    if (L < 4) break;
    switch (*T++) {
    case 'h':
      if (L == 5 && !strncmp(T, "ile", 3)) {
        MakeToken(Result, tok::kw_while);
        return true;
      }
      break;
    case 'i':
      // FIXME: Check LangFeatures
      if (L == 4 && *T++ == 't' && *T == 'h') {
        MakeToken(Result, tok::kw_with);
        return true;
      }
      break;
    default:
      break;
    }
    break;

  case 'y':
    if (L == 5 && !strncmp(T, "ield", 4)) {
      MakeToken(Result, tok::kw_yield);
      return true;
    }
    break;

  default:
    break;
  }

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
        // Do we have unterminated indents that we need to emit a
        // dedent for?
        if (IndentStackTop > 0) {
          NumDedents = IndentStackTop;
          IndentStackTop = 0;
          Ptr = TokStart; // Reset back so we see the \0 again next time.
          return Lex(Result);
        }
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
        // Purely blank lines don't count.
        if (peekAscii() != '\r' && peekAscii() != '\n' && 
            LexPossibleIndent(Result, indent, &error))
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
      AtLineStart = BraceStackTop==0;
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
      AtLineStart = BraceStackTop==0;
      continue;
    }

    case '"':
    case '\'':
      INITIAL_INDENT();
      if ((peekAscii(0) == '"' && peekAscii(1) == '"') ||
          (peekAscii(0) == '\'' && peekAscii(1) == '\'')) {
        getAscii(); getAscii();
        return LexFatStringConstant(Result, Char);
      }
      return LexStringConstant(Result, Char);

    case 'a': case 'e': case 'i': case 'm': case 'q': case 'u': case 'y':
    case 'b': case 'f': case 'j': case 'n': case 'r': case 'v': case 'z':
    case 'c': case 'g': case 'k': case 'o': case 's': case 'w':
    case 'd': case 'h': case 'l': case 'p': case 't': case 'x':
    case 'A': case 'E': case 'I': case 'M': case 'Q': case 'U': case 'Y':
    case 'B': case 'F': case 'J': case 'N': case 'R': case 'V': case 'Z':
    case 'C': case 'G': case 'K': case 'O': case 'S': case 'W':
    case 'D': case 'H': case 'L': case 'P': case 'T': case 'X':
    case '_':
      INITIAL_INDENT();
      return LexIdentifier(Result);

    case '\\':
      if (getAscii() != '\n') {
        Diag("Spurious characters after line joining backslash", Diagnostic::Warning);
        while (getAscii() != '\n')
          ;
        return false;
      }
      continue;

    case '(':
      INITIAL_INDENT();
      BRACE_STACK_PUSH('(');
      MakeToken(Result, tok::l_paren);
      return true;
    case '[':
      INITIAL_INDENT();
      BRACE_STACK_PUSH('[');
      MakeToken(Result, tok::l_square);
      return true;
    case '{':
      INITIAL_INDENT();
      BRACE_STACK_PUSH('{');
      MakeToken(Result, tok::l_brace);
      return true;

    case ')':
      INITIAL_INDENT();
      BRACE_STACK_POP('(');
      MakeToken(Result, tok::r_paren);
      return true;
    case ']':
      INITIAL_INDENT();
      BRACE_STACK_POP('[');
      MakeToken(Result, tok::r_square);
      return true;
    case '}':
      INITIAL_INDENT();
      BRACE_STACK_POP('{');
      MakeToken(Result, tok::r_brace);
      return true;
    
    case '.':
      INITIAL_INDENT();
      if (peekAscii() == '.') {
        getAscii();
        if (getAscii() != '.') {
          Diag("Syntax error", Diagnostic::Error);
          return false;
        }
        MakeToken(Result, tok::ellipsis);
        return true;
      }
      MakeToken(Result, tok::period);
      return true;

    case '&':
      if (peekAscii() == '=') {
        getAscii();
        MakeToken(Result, tok::ampequal);
        return true;
      }
      MakeToken(Result, tok::amp);
      return true;

    case '*':
      if (peekAscii() == '*') {
        getAscii();
        MakeToken(Result, tok::starstar);
        return true;
      } else if (peekAscii() == '=') {
        getAscii();
        MakeToken(Result, tok::starequal);
        return true;
      }
      MakeToken(Result, tok::star);
      return true;

    case '+':
      if (peekAscii() == '=') {
        getAscii();
        MakeToken(Result, tok::plusequal);
        return true;
      }
      MakeToken(Result, tok::plus);
      return true;

    case '-':
      if (peekAscii() == '=') {
        getAscii();
        MakeToken(Result, tok::minusequal);
        return true;
      }
      MakeToken(Result, tok::minus);
      return true;
      
    case '~':
      MakeToken(Result, tok::tilde);
      return true;

    case '/':
      if (peekAscii() == '/') {
        getAscii();
        MakeToken(Result, tok::slashslash);
        return true;
      } else if (peekAscii() == '=') {
        getAscii();
        MakeToken(Result, tok::slashequal);
        return true;
      }
      MakeToken(Result, tok::slash);
      return true;

    case '%':
      if (peekAscii() == '=') {
        getAscii();
        MakeToken(Result, tok::percentequal);
        return true;
      }
      MakeToken(Result, tok::percent);
      return true;

    case '<':
      if (peekAscii() == '<') {
        getAscii();
        if (peekAscii() == '=') {
          getAscii();
          MakeToken(Result, tok::lesslessequal);
          return true;
        }
        MakeToken(Result, tok::lessless);
        return true;
      }
      if (peekAscii() == '=') {
        getAscii();
        MakeToken(Result, tok::lessequal);
        return true;
      }
      MakeToken(Result, tok::less);
      return true;

    case '>':
      if (peekAscii() == '>') {
        getAscii();
        if (peekAscii() == '=') {
          getAscii();
          MakeToken(Result, tok::greatergreaterequal);
          return true;
        }
        MakeToken(Result, tok::greatergreater);
        return true;
      }
      if (peekAscii() == '=') {
        getAscii();
        MakeToken(Result, tok::greaterequal);
        return true;
      }
      MakeToken(Result, tok::greater);
      return true;

    case '^':
      if (peekAscii() == '=') {
        getAscii();
        MakeToken(Result, tok::caretequal);
        return true;
      }
      MakeToken(Result, tok::caret);
      return true;

    case '|':
      if (peekAscii() == '=') {
        getAscii();
        MakeToken(Result, tok::pipeequal);
        return true;
      }
      MakeToken(Result, tok::pipe);
      return true;

    case ':':
      MakeToken(Result, tok::colon);
      return true;

    case ';':
      MakeToken(Result, tok::semi);
      return true;

    case '=':
      if (peekAscii() == '=') {
        getAscii();
        MakeToken(Result, tok::equalequal);
        return true;
      }
      MakeToken(Result, tok::equal);
      return true;

    case ',':
      MakeToken(Result, tok::comma);
      return true;

    case '@':
      MakeToken(Result, tok::at);
      return true;

    case '!':
      if (peekAscii() == '=') {
        getAscii();
        MakeToken(Result, tok::bangequal);
        return true;
      }
      // Fall through;

    default:
      assert(0 && "Unhandled character!");
      return false;
    }
  }
}
