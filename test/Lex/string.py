# RUN: cat %s | %py-lex 2>&1 | FileCheck %s 

"hello"
# CHECK: String<\"hello\">

"world\n"
# CHECK: String<\"world\\n\">

"Nested 'quotes'"
# CHECK: String<\"Nested 'quotes'\">

'single quotes "nested"'
# CHECK: String<'single quotes \"nested\"'>

"Unicode: \u2048"
# CHECK: String<\"Unicode: \\u2048\">

"""Big quoted mutha: can use " quotes " inside"""
# CHECK: String<\"\"\"Big quoted mutha: can use \" quotes \" inside\"\"\">

'''and
newlines'''
# CHECK: String<'''and\nnewlines'''>

""
# CHECK: String<\"\">
