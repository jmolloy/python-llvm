# RUN: %py-parse -rule atom -print-tree %s 2>&1 | FileCheck %s

Hello
# CHECK: (name "Hello")

_World
# CHECK: (name "_World")

_
# CHECK: (name "_")

__
# CHECK: (name "__")

