# RUN: %py-lex %s 2>&1 | FileCheck %s

x
  y
 

# The line below 'y' deliberately has one space on it. This test checks
# that indented lines that are purely whitespace aren't checked by the
# indent stack (i.e. it shouldn't error with "unexpected indent"!).

# CHECK: Indent
# CHECK: Dedent
# CHECK-NOT: error
