#ifndef SDI_ENDIAN_H
#define SDI_ENDIAN_H

/* Includeheader

        Name:           SDI_endian.h
        Versionstring:  $VER: SDI_endian.h 1.0 (06.12.2008)
        Author:         Matthias Rustler
        Distribution:   PD
        Project page:   http://www.sf.net/projects/sditools/
        Description:    endian check and conversion

 1.0   06.12.08 : initial version
*/


/*
    Note that BIG_ENDIAN is always defined, so that you can use it
    with both preprocessor "#if" and C "if".
*/
#if defined(__AROS__)
  #define SDI_BIG_ENDIAN (AROS_BIG_ENDIAN)
#else
  #define SDI_BIG_ENDIAN (1)
#endif


#if defined(__AROS__)
  #include <aros/macros.h>
  
  /* Convert a word or long to big endian and vice versa on the current hardware */
  #define WORD2BE(w) AROS_WORD2BE(w)
  #define LONG2BE(l) AROS_LONG2BE(l)
  #define BE2WORD(w) AROS_BE2WORD(w)
  #define BE2LONG(l) AROS_BE2LONG(l)

  /* Convert a word or long to little endian and vice versa on the current hardware */
  #define WORD2LE(w) AROS_WORD2LE(w)
  #define LONG2LE(l) AROS_LONG2LE(l)
  #define LE2WORD(w) AROS_LE2WORD(w)
  #define LE2LONG(l) AROS_LE2LONG(l)
#else
  /* Convert a word or long to big endian and vice versa on the current hardware */
  #define WORD2BE(w) (w)
  #define LONG2BE(l) (l)
  #define BE2WORD(w) (w)
  #define BE2LONG(l) (l)

  /* Convert a word or long to little endian and vice versa on the current hardware */
  #define WORD2LE(w) (w) (((w >> 8) & 0x00ff) | ((w << 8) & 0xff00))
  #define LONG2LE(l) (l) (((l >>24) & 0x000000ff) | ((l >> 8) & 0x0000ff00) | ((l << 8) & 0x00ff0000) | ((l <<24) & 0xff000000))

  #define LE2WORD(w) (w) (((w >> 8) & 0x00ff) | ((w << 8) & 0xff00))
  #define LE2LONG(l) (l) (((l >>24) & 0x000000ff) | ((l >> 8) & 0x0000ff00) | ((l << 8) & 0x00ff0000) | ((l <<24) & 0xff000000))
#endif

#endif /* SDI_ENDIAN_H */
