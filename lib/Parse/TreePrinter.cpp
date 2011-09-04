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

#include "py/Parse/TreePrinter.h"

using namespace py;
using namespace llvm;

/// Base indent amount
#define INDENT 2

/// Amount to "scan ahead" and possibly display on its own line.
#define SCANAHEAD_MAX 48

/// Taken from my "Horizon" project: http://www.quokforge.org/projects/horizon

#define NUM_COLOURS 6
static raw_ostream::Colors s_colours[] = {raw_ostream::BLUE,
                                          raw_ostream::GREEN,
                                          raw_ostream::CYAN,
                                          raw_ostream::RED,
                                          raw_ostream::MAGENTA,
                                          raw_ostream::YELLOW};

static bool ScanaheadLParen(const char *c) {
	int stack = 0;
	for(int i = 0; i < SCANAHEAD_MAX; i++) {
		if(c[i] == '\0') {
			return false;
		}
		if(c[i] == '(') {
			stack++;
		} else if(c[i] == ')') {
			if(stack == 0) {
				/* Found the matching brace we were looking for */
				return true;
			}
			stack--;
		}
	}
	return false;
}

static bool ScanaheadRParen(const char *c) {
    if(c[1] == ')') {
		return true;
	}

	int stack = 0;
	for(int i = 0; i < SCANAHEAD_MAX; i++) {
		if(c[-i] == ')') {
			stack++;
		} else if(c[-i] == '(') {
			if(stack == 0) {
				/* Found the matching brace we were looking for */
				return true;
			}
			stack--;
		}
	}
	return false;
}

static std::string Spaces(int n) {
	std::string str;
	for(int i = 0; i < n; i++) {
		str += " ";
	}
	return str;
}

static const char *SlurpSpace(const char *c) {
	if(c[1] == ' ') {
		return &c[1];
	} else {
		return c;
	}
}

static raw_ostream::Colors ColourForIndent(int indent) {
	return s_colours[(indent/INDENT) % NUM_COLOURS];
}

void TreePrinter::str(const char *Ptr, size_t Size) {
  const char *c = Ptr;

  /* The algorithm used is naive but should work:
	   
     Maintain an indent; when a '(' is seen, scan ahead X characters
     and look for the closing ')'. If that is found, do nothing.

     When ')' is seen, lookahead 1 character. If the next char is ')',
     do nothing, else line break. */


  for (size_t i = 0; i < Size; ++i) {
    if(*c == '\'' && c[1] == '(' && !in_type) {

      if(!ScanaheadLParen(c) && c != Ptr) {
        OS <<  "\n";
        OS << Spaces(indent);
      }
      indent += INDENT;
      OS << "'";
      if (OS.is_displayed())
        OS.changeColor(ColourForIndent(indent));
      OS << "(";
      if (OS.is_displayed())
        OS.resetColor();
      c++;

    } else if(*c == '(' && !in_type) {

      if(!ScanaheadLParen(c) && c != Ptr) {
        OS << "\n";
        OS.indent(indent);
      }
      indent += INDENT;
      if (OS.is_displayed())
        OS.changeColor(ColourForIndent(indent));
      OS << *c;
      if (OS.is_displayed())
        OS.resetColor();

    } else if(*c == ')' && !in_type) {
      
      if (OS.is_displayed())
        OS.changeColor(ColourForIndent(indent));
      OS <<  *c;
      if (OS.is_displayed())
        OS.resetColor();
      indent -= INDENT;
      if(!ScanaheadRParen(c)) {
        OS << "\n";
        OS.indent(indent);
        c = SlurpSpace(c);
      }
    } else {
      OS << *c;
    }
    c++;
  }
}

TreePrinter::TreePrinter(raw_ostream &S) :
  OS(S), in_type(false), indent(0) {
}
TreePrinter::~TreePrinter() {
  flush();
  OS.flush();
}

void TreePrinter::write_impl(const char *Ptr, size_t Size) {
  str(Ptr, Size);
}
