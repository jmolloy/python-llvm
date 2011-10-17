//===--- Name.h - Python Parser -------------------------------------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
//  This file implements Parser support functions.
//
//===----------------------------------------------------------------------===//
//===----------------------------------------------------------------------===//

#ifndef _PARSE_NAME_H
#define _PARSE_NAME_H

#include "llvm/Value.h"
#include "llvm/DerivedTypes.h"

using namespace llvm;
namespace py {

/// Pretend as if we have a place in the ValueTy enum.
/// This is specifically chosen as Value::dump() is hardcoded
/// to different Value* types. We choose a known Value type
/// that uses a custom virtual function for printing.
///
/// @see VMCore/AsmWriter.cpp:2053
const unsigned NameVal = llvm::Value::PseudoSourceValueVal;

/// This class is a placeholder for a name that is yet to be
/// resolved.
///
/// It is intended to only last as long as parsing the enclosing block;
/// LLVM will surely assert() if it is passed to any Pass or validation
/// method.
///
/// Find out whether this Name is a Global or Local, create a GlobalVariable
/// or AllocaInst and run replaceAllUsesWith().
class Name : public Value {
public:
  Name(Runtime &R, StringRef N) :
    Value(R.GetObjectTyPtr(), NameVal), N(N) {
  }
  virtual ~Name() {
  }

  StringRef GetName() const {return N;}

  static bool classof(const Value *V) {
    return true;
  }
  static bool classof(const Name *N) {
    return N->getValueID() == NameVal;
  }

  virtual void printCustom(raw_ostream &OS) const {
    OS << "$" << N;
  }

private:
  /// Name, as a string.
  std::string N;
};

}
#endif
