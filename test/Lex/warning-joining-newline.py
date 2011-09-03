# RUN: cat %s | %py-lex 2>&1 | FileCheck %s 

0 \x
2
# CHECK: warning: Spurious characters after line joining backslash
