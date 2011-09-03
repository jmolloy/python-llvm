#include "py/Lex/Lexer.h"

#include "llvm/ADT/OwningPtr.h"
#include "llvm/Support/CommandLine.h"
#include "llvm/Support/FileUtilities.h"
#include "llvm/Support/FormattedStream.h"
#include "llvm/Support/ManagedStatic.h"
#include "llvm/Support/SourceMgr.h"
#include "llvm/Support/MemoryBuffer.h"
#include "llvm/Support/ToolOutputFile.h"
#include <iostream>
using namespace llvm;
using namespace py;

static cl::opt<std::string>
InputFilename(cl::Positional, cl::desc("<input file>"), cl::init("-"));

static cl::opt<std::string>
OutputFilename("o", cl::desc("Output filename"),
               cl::value_desc("filename"));

static tool_output_file *GetOutputStream() {
  if (OutputFilename == "")
    OutputFilename = "-";

  std::string Err;
  tool_output_file *Out = new tool_output_file(OutputFilename.c_str(), Err,
                                               raw_fd_ostream::F_Binary);
  if (!Err.empty()) {
    errs() << Err << '\n';
    delete Out;
    return 0;
  }

  return Out;
}

int main(int argc, char **argv)  {
  char *ProgName = argv[0];
  cl::ParseCommandLineOptions(argc, argv,
                              "python lexing playground");
  
  OwningPtr<MemoryBuffer> BufferPtr;
  if (error_code ec = MemoryBuffer::getFileOrSTDIN(InputFilename, BufferPtr)) {
    errs() << ProgName << ": " << ec.message() << '\n';
    return 1;
  }
  MemoryBuffer *Buffer = BufferPtr.take();

  SourceMgr SrcMgr;

  // Tell SrcMgr about this buffer, which is what TGParser will pick up.
  SrcMgr.AddNewSourceBuffer(Buffer, SMLoc());

  OwningPtr<tool_output_file> Out(GetOutputStream());
  if (!Out)
    return 1;

  LangFeatures features;
  
  Lexer lex(Buffer, features);
  Token Result;
  while (lex.Lex(Result)) {
    switch (Result.getKind()) {
    case tok::eof:
      return 0;
    case tok::unknown: Out->os() << "Unknown"; break;
    case tok::identifier: Out->os() << "Identifier"; break;
    case tok::numeric_constant: Out->os() << "Number"; break;
    case tok::string_constant: Out->os() << "String"; break;
    case tok::newline: Out->os() << "Newline\n"; continue;
    case tok::indent: Out->os() << "Indent\n"; continue;
    case tok::dedent: Out->os() << "Dedent\n"; continue;

    case tok::kw_and: Out->os() << "And\n"; continue;
    case tok::kw_as: Out->os() << "As\n"; continue;
    case tok::kw_assert: Out->os() << "Assert\n"; continue;
    case tok::kw_break: Out->os() << "Break\n"; continue;
    case tok::kw_class: Out->os() << "Class\n"; continue;
    case tok::kw_continue: Out->os() << "Continue\n"; continue;
    case tok::kw_def: Out->os() << "Def\n"; continue;
    case tok::kw_del: Out->os() << "Del\n"; continue;
    case tok::kw_elif: Out->os() << "Elif\n"; continue;
    case tok::kw_else: Out->os() << "Else\n"; continue;
    case tok::kw_except: Out->os() << "Except\n"; continue;
    case tok::kw_exec: Out->os() << "Exec\n"; continue;
    case tok::kw_finally: Out->os() << "Finally\n"; continue;
    case tok::kw_for: Out->os() << "For\n"; continue;
    case tok::kw_from: Out->os() << "From\n"; continue;
    case tok::kw_global: Out->os() << "Global\n"; continue;
    case tok::kw_if: Out->os() << "If\n"; continue;
    case tok::kw_import: Out->os() << "Import\n"; continue;
    case tok::kw_in: Out->os() << "In\n"; continue;
    case tok::kw_is: Out->os() << "Is\n"; continue;
    case tok::kw_lambda: Out->os() << "Lambda\n"; continue;
    case tok::kw_not: Out->os() << "Not\n"; continue;
    case tok::kw_or: Out->os() << "Or\n"; continue;
    case tok::kw_pass: Out->os() << "Pass\n"; continue;
    case tok::kw_print: Out->os() << "Print\n"; continue;
    case tok::kw_raise: Out->os() << "Raise\n"; continue;
    case tok::kw_return: Out->os() << "Return\n"; continue;
    case tok::kw_try: Out->os() << "Try\n"; continue;
    case tok::kw_while: Out->os() << "While\n"; continue;
    case tok::kw_with: Out->os() << "With\n"; continue;
    case tok::kw_yield: Out->os() << "Yield\n"; continue;

    case tok::l_square: Out->os() << "[\n"; continue;
    case tok::r_square: Out->os() << "]\n"; continue;
    case tok::l_paren: Out->os() << "(\n"; continue;
    case tok::r_paren: Out->os() << ")\n"; continue;
    case tok::l_brace: Out->os() << "{\n"; continue;
    case tok::r_brace: Out->os() << "}\n"; continue;

    case tok::period: Out->os() << ".\n"; continue;
    case tok::ellipsis: Out->os() << "...\n"; continue;
    case tok::amp: Out->os() << "&\n"; continue;
    case tok::ampequal: Out->os() << "&=\n"; continue;
    case tok::star: Out->os() << "*\n"; continue;
    case tok::starstar: Out->os() << "**\n"; continue;
    case tok::starequal: Out->os() << "*=\n"; continue;
    case tok::plus: Out->os() << "+\n"; continue;
    case tok::plusequal: Out->os() << "+=\n"; continue;
    case tok::minus: Out->os() << "-\n"; continue;
    case tok::minusequal: Out->os() << "-=\n"; continue;
    case tok::tilde: Out->os() << "~\n"; continue;
    case tok::slash: Out->os() << "/\n"; continue;
    case tok::slashslash: Out->os() << "//\n"; continue;
    case tok::slashequal: Out->os() << "/=\n"; continue;
    case tok::percent: Out->os() << "%\n"; continue;
    case tok::percentequal: Out->os() << "%=\n"; continue;
    case tok::less: Out->os() << "<\n"; continue;
    case tok::lessless: Out->os() << "<<\n"; continue;
    case tok::lessequal: Out->os() << "<=\n"; continue;
    case tok::lesslessequal: Out->os() << "<<=\n"; continue;
    case tok::greater: Out->os() << ">\n"; continue;
    case tok::greaterequal: Out->os() << ">=\n"; continue;
    case tok::greatergreater: Out->os() << ">>\n"; continue;
    case tok::greatergreaterequal: Out->os() << ">>=\n"; continue;
    case tok::caret: Out->os() << "^\n"; continue;
    case tok::caretequal: Out->os() << "^=\n"; continue;
    case tok::pipe: Out->os() << "|\n"; continue;
    case tok::pipeequal: Out->os() << "|=\n"; continue;
    case tok::colon: Out->os() << ":\n"; continue;
    case tok::semi: Out->os() << ";\n"; continue;
    case tok::equal: Out->os() << "=\n"; continue;
    case tok::equalequal: Out->os() << "==\n"; continue;
    case tok::comma: Out->os() << ",\n"; continue;
    case tok::at: Out->os() << "@\n"; continue;
    case tok::bangequal: Out->os() << "!=\n"; continue;

    default: Out->os() << "UnhandledToken"; break;
    }
    
    Out->os() << "<";
    Out->os().write_escaped(Result.getString());
    Out->os() << ">\n";
  }
  for (SmallVector<Diagnostic, 5>::iterator it = lex.getDiagnostics().begin(),
         end = lex.getDiagnostics().end();
       it != end;
       ++it) {
    std::cerr << "About to print message: " << it->getMessage() << std::endl;
    SrcMgr.PrintMessage(it->getLoc(), it->getMessage(), it->getSeverityAsText());
  }

  return 1;
}
