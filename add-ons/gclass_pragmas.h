/**/
/* memory */
/**/
#pragma libcall gcBase AllocStringLength 6 0802
#pragma libcall gcBase AllocString c 801
#pragma libcall gcBase FreeString 12 801
/**/
/* method calling */
/**/
#pragma libcall gcBase gDoMethodA 18 9802
#pragma tagcall gcBase gDoMethod 18 9802
#pragma libcall gcBase gDoSuperMethodA 1e a9803
#pragma tagcall gcBase gDoSuperMethod 1e a9803
/**/
/* graphics */
/**/
#pragma libcall gcBase SetHighColor 24 0802
#pragma libcall gcBase SetColors 2a 10803
#pragma libcall gcBase FindColorPen 30 21003
#pragma libcall gcBase DrawRect 36 3210805
#pragma libcall gcBase DrawLine 3c 6543210808
#pragma libcall gcBase gAreaMove 42 1002
#pragma libcall gcBase gAreaDraw 48 1002
#pragma libcall gcBase gAreaEnd 4e 801
#pragma libcall gcBase GetDPI 54 801
#pragma libcall gcBase GetOffset 5a 0802
/**/
/* fonts */
/**/
#pragma libcall gcBase FreeFontInfo 60 801
#pragma libcall gcBase SetFontInfoA 66 90803
#pragma tagcall gcBase SetFontInfo 66 90803
#pragma libcall gcBase CopyFontInfo 6c 801
#pragma libcall gcBase NewFontInfoA 72 90803
#pragma libcall gcBase DrawText 78 10a9805
/**/
/* misc*/
/**/
#pragma libcall gcBase OutlineLength 7e 09803
#pragma libcall gcBase OutlineHeight 84 09803
#pragma libcall gcBase pixel 8a 10803
#pragma libcall gcBase mm 90 10803
#pragma libcall gcBase CreateTerm 96 9802
#pragma libcall gcBase DeleteTerm 9c 801
#pragma libcall gcBase CopyTerm a2 801
#pragma libcall gcBase CalcTerm a8 ba9804
#pragma libcall gcBase gInsertRemoveCellsTerm ae ba9804
#pragma libcall gcBase gInsertRemoveCellsTablePos b4 ba9804
#pragma libcall gcBase TintColor ba 1002
#pragma libcall gcBase gSuperDraw c0 ba981006
#pragma libcall gcBase gGetLink c6 10803
#pragma libcall gcBase SetLowColor cc 0802
#pragma libcall gcBase SetOutlineColor d2 0802
