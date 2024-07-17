VERSION = 53
REVISION = 1

.macro DATE
.ascii "20.9.2013"
.endm

.macro VERS
.ascii "gtdrag.library 53.1"
.endm

.macro VSTRING
.ascii "gtdrag.library 53.1 (20.9.2013)"
.byte 13,10,0
.endm

.macro VERSTAG
.byte 0
.ascii "$VER: gtdrag.library 53.1 (20.9.2013)"
.byte 0
.endm
