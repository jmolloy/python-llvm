# RUN: %py-parse -rule atom -print-tree %s 2>&1 | FileCheck %s

#1.0f
# CHECK-: error: Bad number format

0x5gf
# CHECK: error: Bad number format

0080
# Test fails - recognises as float.
# CHECK-: error: Bad number format

1
# CHECK: (number 1)

0x1
# CHECK: (number 1)

-1
# CHECK: (number -1)

-0x1
# CHECK: (number -1)

-0xF
# CHECK: (number -15)

0xff
# CHECK: (number 255)

010
# CHECK: (number 8)

1.0
# CHECK: (number 1.000000e+00)

-1.0
# CHECK: (number -1.000000e+00)

3.4e-5
# CHECK: (number 3.400000e-05)

1e4
# CHECK: (number 1.000000e+04)

1.0e4
# CHECK: (number 1.000000e+04)
