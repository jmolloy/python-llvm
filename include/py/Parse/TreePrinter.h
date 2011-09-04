//===--- Parser.cpp - Python Parser ---------------------------------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
//  This file implements the Parser interface.
//
//===----------------------------------------------------------------------===//
//===----------------------------------------------------------------------===//

#include "llvm/Support/raw_ostream.h"

namespace py {

/// A subclass of raw_file_ostream that can print S-Expressions and
/// format them prettily.
class TreePrinter : public llvm::raw_ostream {
  virtual void write_impl(const char *Ptr, size_t Size);
  virtual uint64_t current_pos() const { return 0; }

  void str(const char *Ptr, size_t Size);

  llvm::raw_ostream &OS;
  bool in_type;
  int indent;
public:
  TreePrinter(llvm::raw_ostream &S);
  ~TreePrinter();
};

}
