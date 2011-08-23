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
        AllowTypeDecorators = 1, ///< Allow type hints in function declarations
        SpecialPrint = 2, ///< print is special-cased and is not parsed as a
                          ///  normal function.
        FeaturesEnd
    };

public:
    LangFeatures() :
        Flags(0) {
    }

    inline bool hasAllowTypeDecorators() {
        return Flags & AllowTypeDecorators;
    }
    inline void setAllowTypeDecorators(bool b) {
        if (b) Flags |= AllowTypeDecorators;
        else   Flags &= ~AllowTypeDecorators;
    }
            
    inline bool hasSpecialPrint() {
        return Flags & SpecialPrint;
    }
    inline void setSpecialPrint(bool b) {
        if (b) Flags |= SpecialPrint;
        else   Flags &= ~SpecialPrint;
    }

};

}

#endif
