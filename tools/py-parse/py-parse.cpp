#include "py/Lex/Lexer.h"
#include "py/Parse/Parser.h"

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

static cl::opt<std::string>
Rule("rule", cl::desc("Starting grammar rule"),
     cl::value_desc("rule"));

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
  
  Lexer lex(Buffer, features);
  Parser P(lex);

  bool Result;
  if (Rule.length()) {
    Result = P.ParseRule(Rule);
  } else {
    Result = P.ParseFile();
  }

  for (SmallVector<Diagnostic, 5>::iterator it = lex.getDiagnostics().begin(),
         end = lex.getDiagnostics().end();
       it != end;
       ++it) {
    SrcMgr.PrintMessage(it->getLoc(), it->getMessage(), it->getSeverityAsText());
  }

  return Result ? 1 : 0;
}
