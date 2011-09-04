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
const unsigned NameVal = ~0U;

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
  Name(LLVMContext &C, StringRef N) :
    Value(IntegerType::get(C, 8), NameVal), N(N) {
  }
  
  StringRef GetName() const {return N;}

  static bool classof(const Value *V) {
    return true;
  }
  static bool classof(const Name *N) {
    return N->getValueID() == NameVal;
  }

private:
  /// Name, as a string.
  StringRef N;
};

}
#endif
