//===--- Diagnostic.h - Python diagnostics ----------------------*- C++ -*-===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
//  This file defines a class for wrapping diagnostics generated during 
//  lexing and parsing.
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_PY_DIAGNOSTIC_H
#define LLVM_PY_DIAGNOSTIC_H

#include "llvm/Support/SourceMgr.h"
#include <cassert>

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

  const char *getSeverityAsText() {
    switch (Severity_) {
    case Note: return "note";
    case Warning: return "warning";
    case Error: return "error";
    default: assert(0 && "Unhandled case.");
      return "";
    }
  }

  Diagnostic(llvm::SMLoc Loc, Severity Severity_,
             const char *Message) :
    Loc(Loc), Severity_(Severity_), Message(Message)
  {
  }

private:
  llvm::SMLoc Loc;
  Severity Severity_;
  const char *Message;
};

}

#endif
