# RUN: %py-lex %s | FileCheck %s

x
  y
     z

# CHECK: Indent
# CHECK: Indent
# CHECK: Dedent
# CHECK: Dedent
