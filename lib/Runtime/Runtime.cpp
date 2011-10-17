//===--- Runtime.coo - Python Runtime -------------------------------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// This file provides LLVM prototypes for runtime support functions.
//
//===----------------------------------------------------------------------===//
//===----------------------------------------------------------------------===//

#include "llvm/LLVMContext.h"
#include "llvm/Function.h"
#include "llvm/DerivedTypes.h"
#include "llvm/ADT/ArrayRef.h"

#include "py/Runtime/Runtime.h"

#include <vector>

using namespace llvm;
using namespace py;

static const char *FunctionNames[] = {
  "getlocals",
  "getglobals",
  "", /* SentinelZero */
  "startiteration",
  "nextiteration",
  "yield",
  "", /* SentinelOne */
  "bind",
  "", /* SentinelTwo */
  "generatorfactory",
  "", /* SentinelThree */
  "" /* End */
};

Runtime::Runtime(LLVMContext &Context) :
  Context(Context) {
  StructType *Ty = StructType::create(Context, "PythonObject");
  Ty->setBody(PointerType::get(
                Type::getInt8Ty(Context), 0), NULL, NULL);
  ObjectTy = Ty;
  PtrObjectTy = PointerType::get(ObjectTy, 0);
  PtrVoidTy = PointerType::get(Type::getInt8Ty(Context), 0);

  memset(Functions, 0, sizeof(llvm::Function*) * SentinelEnd);
}

Function *Runtime::Function(Fns Fn) {
  assert(Fn < SentinelEnd && "Invalid function index!");
  llvm::Function *F = Functions[Fn];
  if (!F) {
    FunctionType *FTy = 0;
    if (Fn < SentinelZero) {
      FTy = FunctionType::get(PtrObjectTy, false /*VarArg*/);
    } else if (Fn < SentinelOne) {
      std::vector<Type*> Params(1, PtrObjectTy);
      FTy = FunctionType::get(PtrObjectTy, ArrayRef<Type*>(Params), false /*VarArg*/);
    } else if (Fn < SentinelTwo) {
      std::vector<Type*> Params(2, PtrObjectTy);
      FTy = FunctionType::get(PtrObjectTy, ArrayRef<Type*>(Params), false /*VarArg*/);
    } else if (Fn < SentinelThree) {
      std::vector<Type*> Params(3, PtrObjectTy);
      FTy = FunctionType::get(PtrObjectTy, ArrayRef<Type*>(Params), false /*VarArg*/); 
    } else assert(0 && "Function is not any known type!");

    F = Function::Create(FTy, GlobalValue::ExternalLinkage,
                         FunctionNames[Fn]);
    Functions[Fn] = F;
  }
  assert(F);
  return F;
}
