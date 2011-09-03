# RUN: cat %s | %py-lex | FileCheck %s

.
# CHECK-NOT: UnhandledToken
# CHECK: .

...
# CHECK-NOT: UnhandledToken
# CHECK: ...

&
# CHECK-NOT: UnhandledToken
# CHECK: &

&=
# CHECK-NOT: UnhandledToken
# CHECK: &=

*
# CHECK-NOT: UnhandledToken
# CHECK: *

**
# CHECK-NOT: UnhandledToken
# CHECK: **

*=
# CHECK-NOT: UnhandledToken
# CHECK: *=

+
# CHECK-NOT: UnhandledToken
# CHECK: +

+=
# CHECK-NOT: UnhandledToken
# CHECK: +=

-
# CHECK-NOT: UnhandledToken
# CHECK: -

-=
# CHECK-NOT: UnhandledToken
# CHECK: -=

~
# CHECK-NOT: UnhandledToken
# CHECK: ~

/
# CHECK-NOT: UnhandledToken
# CHECK: /

//
# CHECK-NOT: UnhandledToken
# CHECK: //

/=
# CHECK-NOT: UnhandledToken
# CHECK: /=

%
# CHECK-NOT: UnhandledToken
# CHECK: %

%=
# CHECK-NOT: UnhandledToken
# CHECK: %=

<
# CHECK-NOT: UnhandledToken
# CHECK: <

<<
# CHECK-NOT: UnhandledToken
# CHECK: <<

<=
# CHECK-NOT: UnhandledToken
# CHECK: <=

<<=
# CHECK-NOT: UnhandledToken
# CHECK: <<=

>
# CHECK-NOT: UnhandledToken
# CHECK: >

>>
# CHECK-NOT: UnhandledToken
# CHECK: >>

>=
# CHECK-NOT: UnhandledToken
# CHECK: >=

>>=
# CHECK-NOT: UnhandledToken
# CHECK: >>=

^
# CHECK-NOT: UnhandledToken
# CHECK: ^

^=
# CHECK-NOT: UnhandledToken
# CHECK: ^=

|
# CHECK-NOT: UnhandledToken
# CHECK: |

|=
# CHECK-NOT: UnhandledToken
# CHECK: |=

:
# CHECK-NOT: UnhandledToken
# CHECK: :

;
# CHECK-NOT: UnhandledToken
# CHECK: ;

=
# CHECK-NOT: UnhandledToken
# CHECK: =

==
# CHECK-NOT: UnhandledToken
# CHECK: ==

,
# CHECK-NOT: UnhandledToken
# CHECK: ,

@
# CHECK-NOT: UnhandledToken
# CHECK: @

!=
# CHECK-NOT: UnhandledToken
# CHECK: !=
