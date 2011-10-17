//===--- Support.cpp - Python Parser --------------------------------------===//
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

#include "py/Parse/Parser.h"
#include "py/Diagnostic.h"
#include "llvm/ADT/StringRef.h"
#include "llvm/ADT/Twine.h"
#include "llvm/Support/IRBuilder.h"
#include "llvm/Constants.h"

#include "GroupBlock.h"

using namespace py;
using namespace llvm;

void Parser::Diag(Token &T, const char *Str, Diagnostic::Severity Sev) {
  Diagnostic d(T.getLocation(),
               Sev,
               Str);
  Diagnostics.push_back(d);
}

StringRef Parser::SanitizeString(StringRef S) {
  // 1. Look for u/r/R leading chars.
  
  bool Unicode = false;
  bool Raw = false;
  bool Fat = false;
  char Quote;

  const char *it = S.begin();

  switch (*it) {
  case 'u':
  case 'U':
    Unicode = true;
    ++it;
    break;
  case 'r':
  case 'R':
    Raw = true;
    ++it;
    break;
  default:
    break;
  }
  
  switch (*it) {
  case 'u':
  case 'U':
    Unicode = true;
    ++it;
    break;
  case 'r':
  case 'R':
    Raw = true;
    ++it;
    break;
  default:
    break;
  }

  // 2. Find quote type.
  assert((*it == '\'' || *it == '"')  &&
         "Bad quote type!");
  
  Quote = *it++;
  // Handle the fat string case.
  // > 5 because: 3 quotes + possible 2 u/r qualifiers is 5 minimum length
  // ignoring trailing quotes.
  if (S.size() > 5 && it[0] == Quote && it[1] == Quote) {
    Fat = true;
    it += 2;
  }

  // 3. Process escape sequences if not raw.

  char *C = new char[S.size()];
  unsigned C_it = 0;
  for (; it != S.end(); ++it) {
    if (!Raw && *it == '\\') {
      ++it;
      assert(it != S.end() &&
             "Consistency error - string shouldn't be able to end with '\\'");
      switch (*it) {
        // From http://docs.python.org/reference/lexical_analysis.html
      case '\n':
        // Ignore
        break;

      case '\\': C[C_it++] = '\\'; break;

      case '\'': C[C_it++] = '\''; break;
      case '"': C[C_it++] = '"'; break;
      case 'a': C[C_it++] = '\a'; break;
      case 'b': C[C_it++] = '\b'; break;
      case 'f': C[C_it++] = '\f'; break;
      case 'n': C[C_it++] = '\n'; break;
      case 'N': {
        // \N{name} Character named named in the Unicode database
        // (unicode only)
        assert(0 && "Named Unicode entities not implemented!");
        break;
      }
      case 'r': C[C_it++] = '\r'; break;
      case 't': C[C_it++] = '\t'; break;
      case 'u': {
        // \uxxxx Character with 16-bit hex value xxxx (Unicode only)
        assert(0 && "16-bit unicode entities not implemented!");
        break;
      }
      case 'U': {
        // \Uxxxxxxxx Character with 32-bit hex value (Unicode only)
        assert(0 && "32-bit unicode entities not implemented!");
        break;
      }
      case 'v': C[C_it++] = '\v'; break;
      case '0': case '1': case '2': case '3': case '4':
      case '5': case '6': case '7': {
        assert (0 && "Octal entities not implemented!");
        break;
      }
      case 'x': {
        assert (0 && "Hex entities not implemented!");
        break;
      }
      default:
        // Unrecognised; put straight in the string
        // NOTE: Backslash should be put in too (see page
        // linked at start of switch statement)
        C[C_it++] = '\\';
        C[C_it++] = *it;
      }

    } else {
      C[C_it++] = *it;
    }
  }

  // 4. Strip quote from end.
  assert(S.endswith(std::string(1, Quote)));
  if (Fat)
    assert(S.endswith(std::string(3, Quote)));

  return StringRef(C, Fat ? C_it-3 : C_it-1);
}

Constant *Parser::GetConstantString(const Twine &T) {
  return ConstantArray::get(Context, T.str());
}

llvm::Value *Parser::MakeTuple(const std::vector<PNode> &List,
                               BasicBlock **BB) {
  assert(0);
  return NULL;
}

BasicBlock *Parser::ConcatBlocks(BasicBlock *BB1, BasicBlock *BB2) {
  assert(!BB1->getTerminator() && "Asked to concatenate a block that is already terminated!");
  IRBuilder<> IRB(BB1);
  IRB.CreateBr(BB2);
  return BB2;
}

BasicBlock *Parser::ConcatBlocks(BasicBlock *BB1, GroupBlock GB2) {
  assert(!BB1->getTerminator() && "Asked to concatenate a block that is already terminated!");
  IRBuilder<> IRB(BB1);
  IRB.CreateBr(GB2.First());
  return GB2.Last();
}

BasicBlock *Parser::ConcatBlocks(GroupBlock GB1, BasicBlock *BB2) {
  assert(GB1.Last() &&
         !GB1.Last()->getTerminator() &&
         "Asked to concatenate a block that is already terminated!");
  IRBuilder<> IRB(GB1.Last());
  IRB.CreateBr(BB2);
  return BB2;
}

BasicBlock *Parser::ConcatBlocks(GroupBlock GB1, GroupBlock GB2) {
  assert(GB1.Last() &&
         !GB1.Last()->getTerminator() &&
         "Asked to concatenate a block that is already terminated!");
  assert(GB2.First());
  IRBuilder<> IRB(GB1.Last());
  IRB.CreateBr(GB2.First());
  return GB2.Last();
}
