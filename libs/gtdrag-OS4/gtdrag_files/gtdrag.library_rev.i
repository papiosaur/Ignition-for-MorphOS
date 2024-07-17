VERSION		EQU	53
REVISION	EQU	1

DATE	MACRO
		dc.b '20.9.2013'
		ENDM

VERS	MACRO
		dc.b 'gtdrag.library 53.1'
		ENDM

VSTRING	MACRO
		dc.b 'gtdrag.library 53.1 (20.9.2013)',13,10,0
		ENDM

VERSTAG	MACRO
		dc.b 0,'$VER: gtdrag.library 53.1 (20.9.2013)',0
		ENDM
