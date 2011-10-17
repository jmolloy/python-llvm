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
#include <iostream>

using namespace py;
using namespace llvm;

// atom: ('(' [yield_expr|testlist_comp] ')' |
//        '[' [listmaker] ']' |
//        '{' [dictorsetmaker] '}' |
//        '`' testlist1 '`' |
//        NAME | NUMBER | STRING+)
Parser::PNode Parser::ParseAtom(Token &T, BasicBlock **BB) {
  switch (T.getKind()) {
  case tok::l_paren:
    return ParseYieldExprOrTestlistComp(T, BB);
  case tok::l_square:
    return ParseListMaker(T, BB);
  case tok::l_brace:
    return ParseDictOrSetMaker(T, BB);
  case tok::backtick:
    return ParseTestlist1(T, BB);
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


// .. yield_expr: 'yield' [testlist]
// .. testlist_comp: test ( comp_for | (',' test)* [','] )
Parser::PNode Parser::ParseYieldExprOrTestlistComp(Token &T, BasicBlock **BB) {
  assert(T.getKind() == tok::l_paren);
  L.Lex(T);

  if (T.getKind() == tok::kw_yield)
    return ParseYieldExpr(T, BB);

  // Create a temporary BB to store the test at the moment.
  std::vector<Token> TestTokens;
  if (!SkipTest(T, ArrayRef<Token>(TestTokens)))
    return PNode(T);

  GroupBlock GB(OrigTmpBB, TmpBB);
  if (T.getKind() != tok::kw_for) {
    return ParseTestlist(T, BB);
  }

  // Generator comprehension.
    std::vector<Type*> ATys(1, R.GetObjectTyPtr());
    Function *G = Function::Create(FunctionType::get(
                                     R.GetObjectTyPtr(),
                                     ArrayRef<Type*>(ATys),
                                     false),
                                   GlobalValue::InternalLinkage,
                                   "Generator",
                                   &Mod);
    assert(G);



    TmpBB->setParent(G);

    BasicBlock *GBB = BasicBlock::Create(Context, "entry", G);
    IRBuilder<> IRB(*BB);
    IRB.CreateCall(R.Function(Runtime::Yield),
                   N.Value());

    BasicBlock *TmpBB = BasicBlock::Create(Context, "TmpBB", (*BB)->getParent());
    BasicBlock *OrigTmpBB = TmpBB;
    PNode N = ParseTest(

    if (!ParseCompFor(T, &GBB, GB)) 
      return PNode();

    // Finally add a "return null" at the end of the generator.
    IRB.SetInsertPoint(GBB);
    IRB.CreateRet(ConstantPointerNull::get(
                    cast<PointerType>(R.GetObjectTyPtr())));
    
    // Create a call to the generator factory function, passing
    // in our locals and globals.
    IRB.SetInsertPoint(*BB);
    return PNode(T,
                 IRB.CreateCall3(R.Function(Runtime::GeneratorFactory),
                                 IRB.CreateBitCast(G, R.GetObjectTyPtr()),
                                 IRB.CreateCall(R.Function(Runtime::GetLocals)),
                                 IRB.CreateCall(R.Function(Runtime::GetGlobals))));

}

// FIXME
Parser::PNode Parser::ParseListMaker(Token &T, BasicBlock **BB) {
  assert(0);
}

// FIXME
Parser::PNode Parser::ParseDictOrSetMaker(Token &T, BasicBlock **BB) {
  assert(0);
}

// FIXME
Parser::PNode Parser::ParseTestlist1(Token &T, BasicBlock **BB) {
  assert(0);
}

// NAME
Parser::PNode Parser::ParseName(Token &T) {
  DebugStream << "(name \"" << T.getString() << "\")";
  PNode N(T, new Name(R, T.getString()));
  L.Lex(T);
  return N;
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
      return PNode();
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

// testlist: test (',' test)* [',']
// The initial 'test' may have already been parsed, and passed in as N.
Parser::PNode Parser::ParseTestlist(Token &T, BasicBlock **BB, PNode N) {
  std::vector<PNode> List;
  if (!N)
    N = ParseTest(T, BB);
  if (!N)
    return N;
  List.push_back(N);

  bool LastTokenWasComma = false;
  while (L.Peek(T) && T.getKind() == tok::comma) {
    L.Lex(T); // skip tok::comma
    L.Lex(T);
    N = ParseTest(T, BB);
    if (N) {
      LastTokenWasComma = false;
      List.push_back(N);
    } else {
      LastTokenWasComma = true;
      break;
    }
  }

  if (List.size() > 1 || LastTokenWasComma)
    return PNode(T, MakeTuple(List, BB));
  else
    return List.front();
}

// comp_for: 'for' exprlist  'in' or_test [comp_iter]
//
// Parses a comprehension, putting the block "Kernel" in the middle of the innermost loop.
Parser::PNode Parser::ParseCompFor(Token &T, BasicBlock **BB, GroupBlock Kernel) {
  assert(T.getKind() == tok::kw_for);
  L.Lex(T);
  
  // Wrap the passed in loop kernel with the expression evaluations in the exprlist.
  // FIXME: Support 'if' trailing conditions.
  BasicBlock *K = BasicBlock::Create(Context, "ComprehensionKernel", (*BB)->getParent());
  BasicBlock *OrigK = K;
  PNode IndVar = ParseExprList(T, &K);
  if (!IndVar)
    return IndVar;
  Kernel = GroupBlock(OrigK,
                      ConcatBlocks(K, Kernel));

  if (T.getKind() != tok::kw_in) {
    Diag(T, "Expected 'in'", Diagnostic::Error);
    return PNode();
  }
  L.Lex(T);

  // Parse the container to iterate over.
  PNode Container = ParseOrTest(T, BB);

  BasicBlock *Exit = BasicBlock::Create(Context, "ComprehensionLoopExit", (*BB)->getParent());
  Kernel = GroupBlock(ParseCompFor_WrapKernel(Kernel, IndVar.Value(),
                                              Container.Value(), Exit),
                      Exit);

  // Handle multiple for conditions by iteratively wrapping the loop kernel
  // with loops.
  //
  // At the end of this "while", the "kernel" will no longer be a kernel but the 
  // entire loop.
  while (L.Peek(T) && T.getKind() == tok::kw_for) {
    L.Lex(T);
    BasicBlock *Entry = BasicBlock::Create(Context, "ComprehensionLoopSetup", (*BB)->getParent());
    BasicBlock *OrigEntry = Entry;
    IndVar = ParseExprList(T, &Entry);
    if (L.Peek(T) && T.getKind() != tok::kw_in) {
      Diag(T, "Expected 'in'", Diagnostic::Error);
      return PNode();
    }
    Container = ParseOrTest(T, &Entry);

    // Only concatenate the last block in a possible multi-block entry.
    Kernel = GroupBlock(OrigEntry,
                        ConcatBlocks(Entry, Kernel));
    
    Exit = BasicBlock::Create(Context, "ComprehensionLoopExit", (*BB)->getParent());
    Kernel = GroupBlock(ParseCompFor_WrapKernel(Kernel, IndVar.Value(),
                                                Container.Value(), Exit),
                        Exit);
  }

  ConcatBlocks(*BB, Kernel);
  *BB = Exit;
  return PNode(T, *BB);
}

// Creates a loop with "Kernel" in the middle, assigning the induction variable to IndVar on each iteration.
// At the end, the loop will branch to "Exit".
//
// This will return the new entry block.
BasicBlock *Parser::ParseCompFor_WrapKernel(GroupBlock Kernel, Value *IndVar, Value *Container, BasicBlock *Exit) {
  // Create entry block and populate it with a call to StartIteration, which will give us an iterator.
  BasicBlock *Entry = BasicBlock::Create(Context, "ComprehensionWrapEntry", Exit->getParent());
  IRBuilder<> IRB(Entry);
  Value *Iter = IRB.CreateCall(R.Function(Runtime::StartIteration),
                               Container);

  // Create block for loop condition checking. The loop will branch back to here to iterate.
  BasicBlock *LoopCond = BasicBlock::Create(Context, "ComprehensionLoopCond", Exit->getParent());
  // Simple branch from entry to loop cond block.
  IRB.CreateBr(LoopCond);
  IRB.SetInsertPoint(LoopCond);

  // Induction variable assigning block. Used as branch target soon.
  BasicBlock *IndVarAssign = BasicBlock::Create(Context, "IndVarAssign", Exit->getParent());

  // Emit exit condition - iterate.
  Value *Cond = IRB.CreateCall(R.Function(Runtime::NextIteration),
                               Iter);
  // If the iterator returned zero, branch to Exit, else to IndVarAssign
  Value *Quit = IRB.CreateICmpEQ(Cond, ConstantPointerNull::get(cast<PointerType>(R.GetObjectTyPtr())));
  
  IRB.CreateCondBr(Quit, Exit, IndVarAssign);

  // Assign the induction variable.
  IRB.SetInsertPoint(IndVarAssign);
  IRB.CreateCall2(R.Function(Runtime::Bind),
                  IndVar,
                  Cond);
  // This block can be concatenated onto the front of the loop kernel.
  ConcatBlocks(IndVarAssign, Kernel);

  // Emit a branch back to the loop condition at the end of the kernel.
  //IRB.SetInsertPoint(Kernel.Last());
  //IRB.CreateBr(LoopCond);

  // Return first block.
  return Entry;
}



