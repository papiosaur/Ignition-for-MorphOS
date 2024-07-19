|
| Name:        clip.s 
| Description: Routines for clipping line segments and filled polygons
| Author:      pak@star.sr.bham.ac.uk (Peter Knight)
| Notes:       Requires 68020+
|

.EXPORT	_lineclip
.EXPORT	_clip2d
.EXPORT _CLP_xmin
.EXPORT _CLP_ymin
.EXPORT _CLP_xmax
.EXPORT _CLP_ymax

| Clip to x boundary
.MACRO  CLIPX
        movem.l (a0),d2-d5              | get (x1,y1) and (x2,y2)
        sub.l   d2,d4                   | x2-x1
        sub.l   d3,d5                   | y2-y1
        sub.l   d6,d2                   | x1-xint
        muls.l  d2,d7:d5
        divs.l  d4,d7:d5
        sub.l   d5,d3                   | y intercept
        move.l  d6,(a1)+                | save x intercept
        move.l  d3,(a1)+                | save y intercept
.ENDM

| Clip to y boundary
.MACRO  CLIPY
        movem.l (a0),d2-d5              | get (x1,y1) and (x2,y2)
        sub.l   d2,d4                   | x2-x1
        sub.l   d3,d5                   | y2-y1
        sub.l   d6,d3                   | y1-xint
        muls.l  d3,d7:d4
        divs.l  d5,d7:d4
        sub.l   d4,d2                   | x intercept
        move.l  d2,(a1)+                | save x intercept
        move.l  d6,(a1)+                | save y intercept
.ENDM

|                
| lineclip - Clip line segment to viewport
|
| on entry:
| a0.l - address of vertices.
|
| on exit:
| d0.l - TRUE if some part of line drawn, else FALSE
| all other registers preserved
|
_lineclip:
        movem.l d1-d7/a0-a5,-(sp)       | Save registers.

| Bad return code
	moveq	#0,d0

| Duplicate first end point
	move.l	(a0),16(a0)
	move.l	4(a0),20(a0) 

| Clip against xmin
	move.l	_CLP_xmin,d6
	cmp.l	(a0),d6
	ble.s   ok1
	cmp.l	8(a0),d6
	bgt	quit
	move.l	a0,a1
	CLIPX
	bra.s	ok2
ok1:
	cmp.l	8(a0),d6
	ble.s	ok2
	lea	8(a0),a1
	CLIPX

| Clip against xmax
ok2:
	move.l	_CLP_xmax,d6
	cmp.l	(a0),d6
	bge.s	ok3
	cmp.l	8(a0),d6
	blt	quit
	move.l	a0,a1
	CLIPX
	bra.s	ok4
ok3:
	cmp.l	8(a0),d6
	bge.s	ok4
	lea	8(a0),a1
	CLIPX

| Clip against ymin
ok4:
	move.l	_CLP_ymin,d6
	cmp.l	4(a0),d6
	ble.s	ok5
	cmp.l	12(a0),d6
	bgt	quit
	move.l	a0,a1
	CLIPY
	bra.s	ok6
ok5:
	cmp.l	12(a0),d6
	ble.s	ok6
	lea	8(a0),a1
	CLIPY

| Clip against ymax
ok6:
	move.l	_CLP_ymax,d6
	cmp.l	4(a0),d6
	bge.s	ok7
	cmp.l	12(a0),d6
	blt	quit
	move.l	a0,a1
	CLIPY
	bra.s	ok8
ok7:
	cmp.l	12(a0),d6
	bge.s	ok8
	lea	8(a0),a1
	CLIPY
ok8:
	moveq	#1,d0
quit:
        movem.l (sp)+,d1-d7/a0-a5
        rts


|                
| clip2d - Clip 2-d polygon to viewport
|
| on entry:
| d0.w - number of vertices
| a0.l - address of vertices.
| a1.l - address of draw buffer.
|
| on exit:
| d0.w - number of vertices in clipped polygon
| all other registers preserved
|
_clip2d:
        movem.l d1-d7/a0-a5,-(sp)       | Save registers.

| Save address of draw buffer.
	move.l	a0,a5
        move.l  a1,a4
	exg	d0,d1
        
| Clip against xmin.
        subq.w  #2,d1
        move.w  d1,d0                   | d0 is from count
        move.l	a5,a0                   | from buffer
        move.l	a4,a1                   | to buffer is a1
        moveq   #0,d1                   | d1 is to count
        move.l  _CLP_xmin,d6            | d6 is XMIN.
        cmp.l   (a0),d6
        bge.s   xmin_loop
        move.l  (a0),(a1)+
        addq.w  #1,d1
        move.l  4(a0),(a1)+
xmin_loop:
        cmp.l   (a0),d6
        blt.s   xmin_over
        cmp.l   8(a0),d6
        bge.s   xmin_skip
	CLIPX
	addq.w	#1,d1
        bra.s   xmin_next
xmin_over:
        cmp.l   8(a0),d6
        blt.s   xmin_next
	CLIPX
	addq.w	#1,d1
        bra.s   xmin_skip
xmin_next:
        lea     8(a0),a2
        move.l  (a2)+,(a1)+
        addq.w  #1,d1
        move.l  (a2)+,(a1)+
xmin_skip:
        lea     8(a0),a0
        dbra    d0,xmin_loop
        tst.w   d1
        beq     exit 
	move.l	a4,a0
        move.l  (a0)+,d5
        move.l  (a0)+,d6
        lea     -8(a1),a2
        cmp.l   (a2)+,d5
        bne.s   xmin_close
        cmp.l   (a2)+,d6
        beq.s   clipxmax
xmin_close:
        move.l  d5,(a1)+
        addq.w  #1,d1
        move.l  d6,(a1)+ 
        
| Clip against xmax.
clipxmax:
        subq.w  #2,d1
        move.w  d1,d0                   | d0 is from count
        move.l	a4,a0                   | from buffer
        move.l	a5,a1                   | to buffer is a1
        moveq   #0,d1                   | d1 is to count
        move.l  _CLP_xmax,d6            | d6 is xmax
        cmp.l   (a0),d6
        blt.s   xmax_loop
        move.l  (a0),(a1)+
        addq.w  #1,d1
        move.l  4(a0),(a1)+
xmax_loop:
        cmp.l   (a0),d6
        bge.s   xmax_over
        cmp.l   8(a0),d6
        blt.s   xmax_skip
	CLIPX
	addq.w	#1,d1
        bra.s   xmax_next
xmax_over:
        cmp.l   8(a0),d6
        bge.s   xmax_next
	CLIPX
	addq.w	#1,d1
        bra.s   xmax_skip
xmax_next:
        lea     8(a0),a2
        move.l  (a2)+,(a1)+
        addq.w  #1,d1
        move.l  (a2)+,(a1)+
xmax_skip:
        lea     8(a0),a0
        dbra    d0,xmax_loop
        tst.w   d1
        beq     exit	
	move.l	a5,a0
        move.l  (a0)+,d5
        move.l  (a0)+,d6
        lea     -8(a1),a2
        cmp.l   (a2)+,d5
        bne.s   xmax_close
        cmp.l   (a2)+,d6
        beq.s   clipymin
xmax_close:
        move.l  d5,(a1)+
        addq.w  #1,d1
        move.l  d6,(a1)+
        
| Clip against ymin.
clipymin:
        subq.w  #2,d1
        move.w  d1,d0                   | d0 is from count
        move.l	a5,a0                   | from buffer
        move.l	a4,a1                   | to buffer is a1
        moveq   #0,d1                   | d1 is to count
        move.l  _CLP_ymin,d6            | d6 is ymin
	cmp.l   4(a0),d6
        bge.s   ymin_loop
        move.l  (a0),(a1)+
        addq.w  #1,d1
        move.l  4(a0),(a1)+
ymin_loop:
        cmp.l   4(a0),d6
        blt.s   ymin_over
        cmp.l   12(a0),d6
        bge.s   ymin_skip
	CLIPY
	addq.w	#1,d1
        bra.s   ymin_next
ymin_over:
        cmp.l   12(a0),d6
        blt.s   ymin_next
	CLIPY
	addq.w	#1,d1
        bra.s   ymin_skip
ymin_next:
        lea     8(a0),a2
        move.l  (a2)+,(a1)+
        addq.w  #1,d1
        move.l  (a2)+,(a1)+
ymin_skip:
        lea     8(a0),a0
        dbra    d0,ymin_loop
        tst.w   d1
        beq     exit
	move.l	a4,a0
        move.l  (a0)+,d5
        move.l  (a0)+,d6
        lea     -8(a1),a2
        cmp.l   (a2)+,d5
        bne.s   ymin_close
        cmp.l   (a2)+,d6
        beq.s   clipymax
ymin_close:
        move.l  d5,(a1)+
        addq.w  #1,d1
        move.l  d6,(a1)+
        
| Clip against ymax.
clipymax:
        subq.w  #2,d1
        move.w  d1,d0                   | d0 is from count
        move.l  a4,a0                   | from buffer
        move.l  a5,a1                   | to buffer is a1
        moveq   #0,d1                   | d1 is to count
        move.l  _CLP_ymax,d6            | d6 is ymax
        cmp.l   4(a0),d6
        blt.s   ymax_loop
        move.l  (a0),(a1)+
        addq.w  #1,d1
        move.l  4(a0),(a1)+
ymax_loop:
        cmp.l   4(a0),d6
        bge.s   ymax_over
        cmp.l   12(a0),d6
        blt.s   ymax_skip
	CLIPY
	addq.w	#1,d1
        bra.s   ymax_next
ymax_over:
        cmp.l   12(a0),d6
        bge.s   ymax_next
	CLIPY
	addq.w	#1,d1
        bra.s   ymax_skip
ymax_next:
        lea     8(a0),a2
        move.l  (a2)+,(a1)+
        addq.w  #1,d1
        move.l  (a2)+,(a1)+
ymax_skip:
        lea     8(a0),a0
        dbra    d0,ymax_loop
        tst.w   d1
        beq.s   exit
        move.l	a5,a0
        move.l  (a0)+,d5
        move.l  (a0)+,d6
        lea     -8(a1),a2
        cmp.l   (a2)+,d5
        bne.s   ymax_close
        cmp.l   (a2)+,d6
        beq.s   exit
ymax_close:
        move.l  d5,(a1)+
        addq.w  #1,d1
        move.l  d6,(a1)+
        
| Restore registers and exit.
exit:
	move.l	d1,d0
        movem.l (sp)+,d1-d7/a0-a5
        rts

.END
