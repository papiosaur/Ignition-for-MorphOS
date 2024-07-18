VERSION         EQU     3
REVISION        EQU     6
VSTRING MACRO
				dc.b    'gtdrag.library 3.6 (12.8.2003)',13,10,0
        ENDM
VERSTAG MACRO
				dc.b    0,'$VER: gtdrag.library 3.6 (12.8.2003)',0
        ENDM
