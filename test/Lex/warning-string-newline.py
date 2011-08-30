# RUN: cat %s | %py-lex 2>&1 | FileCheck %s 

"line break
in the middle of a constant"
# CHECK: warning: Newline in string constant
