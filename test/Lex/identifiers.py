# RUN: cat %s | %py-lex | FileCheck %s

# CHECK: And
and
# CHECK: As
as
# CHECK: Assert
assert
# CHECK: Break
break
# CHECK: Class
class
# CHECK: Continue
continue
# CHECK: Def
def
# CHECK: Del
del
# CHECK: Elif
elif
# CHECK: Else
else
# CHECK: Except
except
# CHECK: Exec
exec
# CHECK: Finally
finally
# CHECK: For
for
# CHECK: From
from
# CHECK: Global
global
# CHECK: If
if
# CHECK: Import
import
# CHECK: In
in
# CHECK: Is
is
# CHECK: Lambda
lambda
# CHECK: Not
not
# CHECK: Or
or
# CHECK: Pass
pass
# CHECK: Print
print
# CHECK: Raise
raise
# CHECK: Return
return
# CHECK: Try
try
# CHECK: While
while
# CHECK: With
with
# CHECK: Yield
yield

# CHECK: Identifier<a>
a
# CHECK: Identifier<forx>
forx
# CHECK: Identifier<fo>
fo
# CHECK: Identifier<i>
i
# CHECK: Identifier<ife>
ife
# CHECK: Identifier<ifelse>
ifelse
# CHECK: Identifier<try_>
try_

Hello
# CHECK: Identifier<Hello>
True
# CHECK: Identifier<True>
