#include "py/Parse/Parser.h"
#include "py/Lex/Lexer.h"
#include "py/Lex/Token.h"
#include "py/Runtime/Runtime.h"
#include "llvm/ADT/StringRef.h"
#include "llvm/ADT/Twine.h"
#include "llvm/Support/IRBuilder.h"
#include "llvm/Constants.h"
#include "llvm/LLVMContext.h"

#include "Name.h"
#include "GroupBlock.h"

#include <errno.h>
#include <stdlib.h>

using namespace py;
using namespace llvm;

Parser::PNode Parser::ParseYieldExpr(Token &T, BasicBlock **BB) {
  assert(0);
  return PNode();
}

// test: or_test ['if' or_test 'else' test] | lambdef
Parser::PNode Parser::ParseTest(Token &T, BasicBlock **BB) {
  if (T.getKind() == tok::kw_lambda) {
    assert(0 && "Lambdas!");
    return PNode();
  }

  PNode OrTest = ParseOrTest(T, BB);
  if (!OrTest)
    return OrTest;

  // FIXME: Check for if...else
  // FIXME: Check LangFeatures
  return OrTest;
}

Parser::PNode Parser::ParseOrTest(Token &T, BasicBlock **BB) {
  // FIXME: 
  return ParseName(T);
  assert(0);
  return PNode();
}

Parser::PNode Parser::ParseExprList(Token &T, BasicBlock **BB) {
  // FIXME:
  return ParseName(T);
  return PNode();
}
