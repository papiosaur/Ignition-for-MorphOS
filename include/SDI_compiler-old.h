#ifndef SDI_COMPILER_H
#define SDI_COMPILER_H

/* Includeheader

        Name:           SDI_compiler.h
        Versionstring:  $VER: SDI_compiler.h 1.26 (14.10.2007)
        Author:         Dirk St�cker & Jens Langner
        Distribution:   PD
        Project page:   http://www.sf.net/projects/sditools/
        Description:    defines to hide compiler stuff

 1.1   25.06.98 : created from data made by Gunter Nikl
 1.2   17.11.99 : added VBCC
 1.3   29.02.00 : fixed VBCC REG define
 1.4   30.03.00 : fixed SAVEDS for VBCC
 1.5   29.07.00 : added #undef statements (needed e.g. for AmiTCP together
                  with vbcc)
 1.6   19.05.01 : added STACKEXT and Dice stuff
 1.7   16.06.01 : added MorphOS specials and VARARGS68K
 1.8   21.09.02 : added MorphOS register stuff
 1.9   26.09.02 : added OFFSET macro. Thanks Frank Wille for suggestion
 1.10  18.10.02 : reverted to old MorphOS-method for GCC
 1.11  09.11.02 : added REGARGS define to MorphOS section
 1.12  18.01.04 : some adaptions for AmigaOS4 compatibility
 1.13  17.02.04 : changed ASM macros to be a simple define and added
                  INTERRUPT, CHIP and FAR
 1.14  02.03.04 : added UNUSED which can be used to specify a function parameter
                  or variable as "unused" which will not cause any compiler warning.
 1.15  02.03.04 : added special INLINE define for gcc > 3.0 version
 1.17  07.03.04 : changed INLINE definition of gcc <= 2.95.3 to be static aswell.
 1.18  21.06.04 : added USED and USED_VAR attribute to allow placing a
                  __attribute__((used)) to a function and a variable, taking care of
                  different compiler versions.
 1.19  04.07.04 : register specification for variables is not supported on MorphOS,
                  so we modified the REG() macro accordingly.
 1.20  28.02.05 : correct INLINE for VBCC.
 1.21  28.02.05 : cleanup __GCC__ case.
 1.22  16.05.05 : changed the vbcc/REG() macro.
                  added missing vbcc/VARARGS68K define.
                  moved morphos SDI_EmulLib Stuff into compilers.h. I know it's not
                  compiler specific,  (Guido Mersmann)
 1.23  30.04.06 : modified to get it compatible to AROS. (Guido Mersmann)
 1.24  06.05.06 : __linearvarargs is only valid for vbcc and PPC, so I moved VARARGS68K
                  to prevent problems with 68K and i86 targets. (Guido Mersmann)
 1.25  21.06.07 : added NEAR to be usable for __near specification for SAS/C
 1.26  14.10.07 : added DEPRECATED macro which defaults to __attribute__((deprecated))
                  for GCC compiles.
       29.11.08   Added some defines for Ignition and support modules (Mazze)
*/

/*
** This is PD (Public Domain). This means you can do with it whatever you want
** without any restrictions. I only ask you to tell me improvements, so I may
** fix the main line of this files as well.
**
** To keep confusion level low: When changing this file, please note it in
** above history list and indicate that the change was not made by myself
** (e.g.�add your name or nick name).
**
** Find the latest version of this file at:
** http://cvs.sourceforge.net/viewcvs.py/sditools/sditools/headers/
**
** Jens Langner <Jens.Langner@light-speed.de> and
** Dirk St�cker <soft@dstoecker.de>
*/

/* Some SDI internal header */

#undef ASM
#undef REG
#undef LREG
#undef CONST
#undef SAVEDS
#undef INLINE
#undef REGARGS
#undef STDARGS
#undef OFFSET
#undef INTERRUPT
#undef CHIP
#undef FAR
#undef NEAR
#undef UNUSED
#undef USED
#undef USED_VAR
#undef DEPRECATED

/* first "exceptions" */

#if defined(__MAXON__)
  #define STDARGS
  #define STACKEXT
  #define REGARGS
  #define SAVEDS
  #define INLINE inline
/*************************************************************************/
#elif defined(__VBCC__)
  #define STDARGS
  #define STACKEXT
  #define REGARGS
  #define INLINE static
  #define OFFSET(p,m) __offsetof(struct p,m)

  #if defined(__PPC__)
    #define VARARGS68K __linearvarargs
    #define REG(reg,arg) arg
  #else
    #define REG(reg,arg) __reg(#reg) arg
  #endif
/*************************************************************************/
#elif defined(__STORM__)
  #define STDARGS
  #define STACKEXT
  #define REGARGS
  #define INLINE inline
/*************************************************************************/
#elif defined(__SASC)
  #define ASM __asm
/*************************************************************************/
#elif defined(__GNUC__)
  #define UNUSED __attribute__((unused)) /* for functions, variables and types */
  #define USED   __attribute__((used))   /* for functions only! */
  #define DEPRECATED __attribute__((deprecated))
  #if __GNUC__ > 3 || (__GNUC__ == 3 && __GNUC_MINOR__ > 0)
    #define USED_VAR USED /* for variables only! */
    #define INLINE static __inline __attribute__((always_inline))
  #endif
  /* we have to distinguish between AmigaOS4 and MorphOS */
  #if defined(__PPC__)
    #define REG(reg,arg) arg
    #define SAVEDS
    #define STDARGS
    #define REGARGS
    #define STACKEXT
    #if defined(__MORPHOS__)
      #define VARARGS68K  __attribute__((varargs68k))
    #endif
    #define INTERRUPT
    #define CHIP
  #else
    #define REG(reg,arg) arg __asm(#reg)
    #define LREG(reg,arg) register REG(reg,arg)
  #endif
  #define FAR
  #define NEAR
#elif defined(_DCC)
  #define REG(reg,arg) __##reg arg
  #define STACKEXT __stkcheck
  #define STDARGS __stkargs
  #define INLINE static
#endif

/* then "common" ones */

#if !defined(ASM)
  #define ASM
#endif
#if !defined(REG)
  #define REG(reg,arg) register __##reg arg
#endif
#if !defined(LREG)
  #define LREG(reg,arg) register arg
#endif
#if !defined(CONST)
  #define CONST const
#endif
#if !defined(SAVEDS)
  #define SAVEDS __saveds
#endif
#if !defined(INLINE)
  #define INLINE static __inline
#endif
#if !defined(REGARGS)
  #define REGARGS __regargs
#endif
#if !defined(STDARGS)
  #define STDARGS __stdargs
#endif
#if !defined(STACKEXT)
  #define STACKEXT __stackext
#endif
#if !defined(VARARGS68K)
  #define VARARGS68K
#endif
#if !defined(OFFSET)
  #define OFFSET(structName, structEntry) \
    ((char *)(&(((struct structName *)0)->structEntry))-(char *)0)
#endif
#if !defined(INTERRUPT)
  #define INTERRUPT __interrupt
#endif
#if !defined(CHIP)
  #define CHIP __chip
#endif
#if !defined(FAR)
  #define FAR __far
#endif
#if !defined(NEAR)
  #define NEAR __near
#endif
#if !defined(UNUSED)
  #define UNUSED
#endif
#if !defined(USED)
  #define USED
#endif
#if !defined(USED_VAR)
  #define USED_VAR
#endif
#if !defined(DEPRECATED)
  #define DEPRECATED
#endif

/*************************************************************************/

#ifdef __AROS__
  #undef REG
  #define REG(reg, arg) arg

  #undef SAVEDS
  #define SAVEDS

  #undef ASM
  #define ASM

  #undef REGARGS
  #define REGARGS

  #undef VARARGS68K
  #define VARARGS68K __stackparm
#else
  #ifndef CONST_STRPTR
	#define CONST_STRPTR UBYTE const *
  #endif
#endif /* __AROS__ */
 
/*************************************************************************/

/* Ignition support */

#define PUBLIC ASM SAVEDS
#define PRIVATE REGARGS

/* IPTR is an integer type which is large enough to store an address.*/
#if !defined(__AROS__) && !defined(__MORPHOS__)
  typedef ULONG IPTR;
#endif

#ifdef __AMIGAOS41__
#  define min(a,b) ((a)<(b)?(a):(b))
#  define max(a,b) ((a)>(b)?(a):(b))
#endif

#if defined(__AROS__)
#  define LIBFUNC_INIT AROS_LIBFUNC_INIT
#  define LIBFUNC_EXIT AROS_LIBFUNC_EXIT
#  define LIB_LH0 AROS_LH0
#  define LIB_LH1 AROS_LH1
#  define LIB_LH2 AROS_LH2
#  define LIB_LH3 AROS_LH3
#  define LIB_LH4 AROS_LH4
#  define LIB_LH5 AROS_LH5
#  define LIB_LHA AROS_LHA
#  define min(a,b) ((a)<(b)?(a):(b))
#  define max(a,b) ((a)>(b)?(a):(b))
#  define PI (M_PI)

#  undef CHIP
#  define CHIP

#  define ALIGNED __attribute__ ((aligned (4)))
#else
   // TODO write LIB_... macros
#  define LIBFUNC_INIT
#  define LIBFUNC_EXIT

#  if !defined(ALIGNED)
#    define ALIGNED
#  endif

#endif

#if defined(__AROS__)
   /* Platforms which need HookEntry from amiga.lib */
   //#  define SETHOOK(hookname, funcname) hookname.h_Entry = HookEntry; hookname.h_SubEntry = (HOOKFUNC)funcname
   // FIXME for now we set funcname to h_Entry. This must later be changed to h_SubEntry (uncomment
   // the line above)
#  define SETHOOK(hookname, funcname) hookname.h_Entry = (HOOKFUNC)funcname;

#  define SETDISPATCHER(classname, funcname) classname->cl_Dispatcher.h_Entry = HookEntry; \
   classname->cl_Dispatcher.h_SubEntry = (HOOKFUNC)funcname
#else
#  define SETHOOK(hookname, funcname) hookname.h_Entry = (HOOKFUNC)funcname
#  define SETDISPATCHER(classname, funcname) classname->cl_Dispatcher.h_Entry = (HOOKFUNC)funcname
#endif

#endif /* SDI_COMPILER_H */
