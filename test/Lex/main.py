# RUN: cat %s | %py-lex | FileCheck %s

789
0x123
456e-3
43.5f

# CHECK: Number<789>
# CHECK: Number<0x123>
# CHECK: Number<456e-3>
# CHECK: Number<43.5f>

  0
    1
  2
   3
   4
5

# CHECK: Indent
# CHECK: Number<0>
# CHECK: Newline
# CHECK: Indent
# CHECK: Number<1>
# CHECK: Newline
# CHECK: Dedent
# CHECK: Number<2>
# CHECK: Newline
# CHECK: Indent
# CHECK: Number<3>
# CHECK: Newline
# CHECK: Number<4>
# CHECK: Newline
# CHECK: Dedent
# CHECK: Dedent
# CHECK: Number<5>
