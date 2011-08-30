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

#ifndef LLVM_PY_LANG_FEATURES_H
#define LLVM_PY_LANG_FEATURES_H

namespace py {

class LangFeatures {
    unsigned Flags;

    /// Describes the features of Python that can be
    /// allowed or disallowed.
    enum Features {
      AllowTypeDecorators = 1<<0UL, ///< Allow type hints in function declarations
      SpecialPrint = 1<<1UL,        ///< print is special-cased and is not parsed as a
                                    ///  normal function.
      AllowWith = 1<<2UL,           ///< 'with' clauses are allowed.
      AllowImportAs = 1<<3UL,       ///< 'import ... as ...' is allowed.
      AllowExceptAs = 1<<4UL,       ///< 'except ... as ...' is allowed.
      FeaturesEnd
    };

public:
    LangFeatures() :
        Flags(0) {
    }

#define X(x) inline bool has##x() {             \
    return Flags & x;                           \
  }                                             \
  inline void set##x(bool b) {                  \
    if (b) Flags |= x;                          \
    else   Flags &= ~x;                         \
  }

X(AllowTypeDecorators)
X(SpecialPrint)
X(AllowWith)
X(AllowImportAs)
X(AllowExceptAs)
#undef X
};

}

#endif
