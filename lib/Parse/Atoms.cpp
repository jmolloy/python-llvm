//===--- Atoms.cpp - Python Parser ----------------------------------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
//  This file implements parsing for Atoms.
//
//===----------------------------------------------------------------------===//
//===----------------------------------------------------------------------===//

#include "py/Parse/Parser.h"
#include "py/Lex/Lexer.h"
#include "py/Lex/Token.h"
#include "llvm/ADT/StringRef.h"
#include "llvm/ADT/Twine.h"
#include "llvm/Constants.h"

#include "Name.h"

#include <errno.h>
#include <stdlib.h>

using namespace py;
using namespace llvm;

// atom: ('(' [yield_expr|testlist_comp] ')' |
//        '[' [listmaker] ']' |
//        '{' [dictorsetmaker] '}' |
//        '`' testlist1 '`' |
//        NAME | NUMBER | STRING+)
Parser::PNode Parser::ParseAtom(Token &T) {
  switch (T.getKind()) {
  case tok::l_paren:
    return ParseYieldExprOrTestlistComp(T);
  case tok::l_square:
    return ParseListMaker(T);
  case tok::l_brace:
    return ParseDictOrSetMaker(T);
  case tok::backtick:
    return ParseTestlist1(T);
  case tok::identifier:
    return ParseName(T);
  case tok::numeric_constant:
    return ParseNumber(T);
  case tok::string_constant:
    return ParseOneOrMoreStrings(T);
  default:
    Diag(T, "Expected an atom", Diagnostic::Error);
    return PNode();
  }
}


// FIXME
Parser::PNode Parser::ParseYieldExprOrTestlistComp(Token &T) {
  assert(0);
}

// FIXME
Parser::PNode Parser::ParseListMaker(Token &T) {
  assert(0);
}

// FIXME
Parser::PNode Parser::ParseDictOrSetMaker(Token &T) {
  assert(0);
}

// FIXME
Parser::PNode Parser::ParseTestlist1(Token &T) {
  assert(0);
}

// NAME
Parser::PNode Parser::ParseName(Token &T) {
  DebugStream << "(name \"" << T.getString() << "\")";
  return PNode(T, new Name(Context, T.getString()));
}

// NUMBER
Parser::PNode Parser::ParseNumber(Token &T) {
  // FIXME: I'm not sure this implements the Python number specification *exactly*...

  const char *S = T.getString().c_str();
  char *EndPtr = 0;
  errno = 0;
  long long int L = strtoll(S, &EndPtr, 0);

  if (errno || *EndPtr != '\0') {
    EndPtr = 0;
    errno = 0;
    long double D = strtod(S, &EndPtr);

    if (errno || *EndPtr != '\0') {
      Diag(T, "Bad number format", Diagnostic::Error);
      return PNode(T);
    }
  
    DebugStream << "(number " << (double)D << ")";
    return PNode(T, ConstantFP::get(Type::getDoubleTy(Context), D));
  }

  DebugStream << "(number " << L << ")";
  
  if (L < 1LL<<32)
    return PNode(T, ConstantInt::get(Type::getInt32Ty(Context), L));
  else
    return PNode(T, ConstantInt::get(Type::getInt64Ty(Context), L));
}

// STRING+
Parser::PNode Parser::ParseOneOrMoreStrings(Token &T) {
  Twine Str = Twine(SanitizeString(T.getString()));
  Token FirstToken = T;
  while (L.Peek(T) && T.getKind() == tok::string_constant) {
    L.Lex(T);
    Str = Str.concat(SanitizeString(T.getString()));
  }
  DebugStream << "(string \"";
  DebugStream.write_escaped(Str.str(), true /*UseHexEscapes*/) << "\")";
  return PNode(FirstToken, GetConstantString(Str));
}

Parser::PNode Parser::ParseOneString(Token &T) {
  StringRef S = SanitizeString(T.getString());
  DebugStream << "(string \"";
  DebugStream.write_escaped(S, true /*UseHexEscapes*/) << "\")";
  return PNode(T, GetConstantString(S));
}
