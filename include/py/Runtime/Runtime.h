//===--- Runtime.cpp - Python Runtime -------------------------------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
//  This file implements an interface for accessing the runtime support.
//
//===----------------------------------------------------------------------===//
//===----------------------------------------------------------------------===//

#ifndef RUNTIME_RUNTIME_H
#define RUNTIME_RUNTIME_H

namespace llvm {
    class LLVMContext;
    class Function;
    class Type;
}

namespace py {

class Runtime {
public:
    enum Fns {
        GetLocals,
        GetGlobals,

        SentinelZero, //< All functions above this take no arguments

        StartIteration,
        NextIteration,
        Yield,

        SentinelOne, //< All functions above this take 1 argument.

        Bind,

        SentinelTwo, //< All functions above this take 2 arguments.

        GeneratorFactory,

        SentinelThree, //< All functions above this take 3 arguments.

        SentinelEnd
    };

    /// Creates a new Runtime instance, using the given context.
    Runtime(llvm::LLVMContext &Context);

    /// Returns an LLVM Function* for the given runtime
    /// support function.
    llvm::Function *Function(Fns Fn);

    /// Returns the type of all python objects.
    llvm::Type *GetObjectTy() const {
        return ObjectTy;
    }
    /// Returns a pointer to the type of all python objects.
    llvm::Type *GetObjectTyPtr() const {
        return PtrObjectTy;
    }

    llvm::Type *GetVoidTyPtr() const {
        return PtrVoidTy;
    }

private:
    /// This array is lazily populated - an entry can be NULL.
    llvm::Function *Functions[SentinelEnd];

    llvm::Type *ObjectTy, *PtrObjectTy, *PtrVoidTy;

    llvm::LLVMContext &Context;
};

}

#endif
