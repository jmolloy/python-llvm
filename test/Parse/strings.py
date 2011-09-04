# RUN: %py-parse -rule STRING -print-tree %s 2>&1 | FileCheck %s

"Hello, world!"
# CHECK: (string "Hello, world!")

'Hello, world!'
# CHECK: (string "Hello, world!")

'Hello, ""!'
# CHECK: (string "Hello, \"\"!")

'Newline\n'
# CHECK: (string "Newline\n")

'Newline\\n'
# CHECK: (string "Newline\\n")

'''Newline
'''
# CHECK: (string "Newline\n")

r'Raw newline\n'
# CHECK: (string "Raw newline\\n")

ur"Tab\t"
# CHECK: (string "Tab\\t")

""
# CHECK: (string "")

"Bad escape: \g"
# CHECK: (string "Bad escape: \\g")

"Escape quote \""
# CHECK: (string "Escape quote \"")
