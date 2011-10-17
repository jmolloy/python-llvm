//===--- GroupBlock.h - Python Parser -------------------------------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
//  The GroupBlock is a wrapper around a graph of BasicBlocks that has exactly
//  one entry block and one exit block.
//
//===----------------------------------------------------------------------===//
//===----------------------------------------------------------------------===//

#ifndef PARSER_GROUPBLOCK_H
#define PARSER_GROUPBLOCK_H

namespace llvm {
    class BasicBlock;
}

namespace py {

class GroupBlock {
public:
    GroupBlock(llvm::BasicBlock *BB1, llvm::BasicBlock *BB2) :
        BB1(BB1), BB2(BB2) {
    }
    
    llvm::BasicBlock *First() {
        return BB1;
    }
    llvm::BasicBlock *Last() {
        return BB2;
    }

private:
    llvm::BasicBlock *BB1, *BB2;
};

}

#endif
