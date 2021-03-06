#include "py/Lex/Lexer.h"
#include "py/Parse/Parser.h"
#include "py/Runtime/Runtime.h"

#include "llvm/ADT/OwningPtr.h"
#include "llvm/Support/CommandLine.h"
#include "llvm/Support/FileUtilities.h"
#include "llvm/Support/FormattedStream.h"
#include "llvm/Support/ManagedStatic.h"
#include "llvm/Support/SourceMgr.h"
#include "llvm/Support/MemoryBuffer.h"
#include "llvm/Support/ToolOutputFile.h"
#include "llvm/LLVMContext.h"
#include "llvm/Module.h"
#include <iostream>
using namespace llvm;
using namespace py;

static cl::opt<std::string>
InputFilename(cl::Positional, cl::desc("<input file>"), cl::init("-"));

static cl::opt<std::string>
OutputFilename("o", cl::desc("Output filename"),
               cl::value_desc("filename"));

static cl::opt<std::string>
Rule("rule", cl::desc("Starting grammar rule"),
     cl::value_desc("rule"));

static cl::opt<bool>
PrintTree("print-tree", cl::desc("Print out the AST?"),
          cl::value_desc("print-tree"));

static cl::opt<bool>
PrintModule("print-module", cl::desc("Print out the generated Module?"),
            cl::value_desc("print-module"));

static void EmitDiagnostics(Lexer &lex, Parser &P, SourceMgr &SrcMgr);

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
                              "python parsing playground");
  
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
  
  LLVMContext C;
  Module M("py-parse playpen", C);

  Lexer lex(Buffer, features);
  Runtime R(C);
  Parser P(lex, R, C, M, PrintTree ? errs() : nulls());

  bool Result = true;
  if (Rule.length()) {
    Token T;
    while (lex.Peek(T) && T.getKind() != tok::eof) {
      while (lex.Peek(T) && T.getKind() == tok::newline)
        lex.Lex(T);
      if (T.getKind() == tok::eof) break;

      lex.Lex(T);
      Result = P.ParseRule(Rule, T);
      EmitDiagnostics(lex, P, SrcMgr);
//      if (!Result) {
// /       lex.Lex(T);
//      }
    }
  } else {
    Result = P.ParseFile();
  }

  errs().flush();

  if (PrintModule)
    M.dump();

  return Result ? 1 : 0;
}

static void EmitDiagnostics(Lexer &lex, Parser &P, SourceMgr &SrcMgr) {
  for (SmallVector<Diagnostic, 5>::iterator it = lex.getDiagnostics().begin(),
         end = lex.getDiagnostics().end();
       it != end;
       ++it) {
    SrcMgr.PrintMessage(it->getLoc(), it->getMessage(), it->getSeverityAsText());
  }
  for (SmallVector<Diagnostic, 5>::iterator it = P.getDiagnostics().begin(),
         end = P.getDiagnostics().end();
       it != end;
       ++it) {
    SrcMgr.PrintMessage(it->getLoc(), it->getMessage(), it->getSeverityAsText());
  }
  lex.getDiagnostics().clear();
  P.getDiagnostics().clear();
}

