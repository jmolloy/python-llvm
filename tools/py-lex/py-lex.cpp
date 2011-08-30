#include "py/Lex/Lexer.h"

#include "llvm/ADT/OwningPtr.h"
#include "llvm/Support/CommandLine.h"
#include "llvm/Support/FileUtilities.h"
#include "llvm/Support/FormattedStream.h"
#include "llvm/Support/ManagedStatic.h"
#include "llvm/Support/SourceMgr.h"
#include "llvm/Support/MemoryBuffer.h"
#include "llvm/Support/ToolOutputFile.h"

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
    SrcMgr.PrintMessage(it->getLoc(), it->getMessage(), it->getSeverityAsText());
  }

  return 1;
}
