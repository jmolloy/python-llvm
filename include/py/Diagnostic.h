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

#ifndef LLVM_PY_DIAGNOSTIC_H
#define LLVM_PY_DIAGNOSTIC_H

#include "llvm/Support/SourceMgr.h"

namespace py {

class Diagnostic {
public:    
    enum Severity {
        Note,
        Warning,
        Error
    };

    llvm::SMLoc getLoc() {return Loc;}
    Severity getSeverity() {return Severity_;}
    const char *getMessage() {return Message;}

    Diagnostic(llvm::SMLoc Loc, Severity Severity_,
               const char *Message) :
        Loc(Loc), Severity_(Severity_), Message(Message)
    {
    }

private:
    llvm::SMLoc Loc;
    Sev Severity_;
    const char *Message;
};

}

#endif
