/*
;  macros.ash -- macros
;
;  This file is part of the UCL data compression library.
;
;  Copyright (C) 1996-2025 Markus Franz Xaver Johannes Oberhumer
;  All Rights Reserved.
;
;  The UCL library is free software; you can redistribute it and/or
;  modify it under the terms of the GNU General Public License as
;  published by the Free Software Foundation; either version 2 of
;  the License, or (at your option) any later version.
;
;  The UCL library is distributed in the hope that it will be useful,
;  but WITHOUT ANY WARRANTY; without even the implied warranty of
;  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
;  GNU General Public License for more details.
;
;  You should have received a copy of the GNU General Public License
;  along with the UCL library; see the file COPYING.
;  If not, write to the Free Software Foundation, Inc.,
;  59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
;
;  Markus F.X.J. Oberhumer
;  <markus@oberhumer.com>
;  http://www.oberhumer.com/opensource/ucl/
;
;  Jens Medoch
;  <jssg@users.sourceforge.net>
;
 */

#ifndef _MR3K_MACROS_ASH  /*{*/
#define _MR3K_MACROS_ASH 1

.macro  section name
        .section \name,"ax"
        .align  0
.endm

/* http://math-atlas.sourceforge.net/devel/assembly/mipsabi32.pdf
 *   When calling position independent functions $25 must contain
 *   the address of the called function.  [$25 === $t9 === $jp]
 */
#define zero    $0
#define at      $1
#define v0      $2
#define v1      $3
#define a0      $4
#define a1      $5
#define a2      $6
#define a3      $7
#define t0      $8
  #define a4 t0
#define t1      $9
  #define a5 t1
#define t2      $10
  #define a6 t2
#define t3      $11
  #define a7 t3
#define t4      $12
#define t5      $13
#define t6      $14
#define t7      $15
#define s0      $16
#define s1      $17
#define s2      $18
#define s3      $19
#define s4      $20
#define s5      $21
#define s6      $22
#define s7      $23
#define t8      $24
#define t9      $25
#define jp      $25
#define k0      $26
#define k1      $27
#define gp      $28
#define sp      $29
#define fp      $30
#define s8      $30
#define ra      $31

#define IF      .if
#define ELSE    .else
#define ENDIF   .endif

.macro  subiu   reg, p1, p2
    .ifnb p2
        addiu   \reg, p1, -p2
    .else
        addiu   \reg, -p1
    .endif
.endm

#ifndef DEBUG
#   define PRINT(str)   /*empty*/
#else
#   define PRINT(str)   .print str
#endif

.macro call dst
    bal \dst  /* WARNING: delay slot */
.endm
.macro ret
    jr ra  /* WARNING: delay slot */
.endm

.macro PUSH4 p1,p2,p3,p4
    addiu sp,-4*NBPW
    sw \p1,0*NBPW(sp)
    sw \p2,1*NBPW(sp)
    sw \p3,2*NBPW(sp)
    sw \p4,3*NBPW(sp)
.endm
.macro POP4 p1,p2,p3,p4
    lw \p1,0*NBPW(sp)
    lw \p2,1*NBPW(sp)
    lw \p3,2*NBPW(sp)
    lw \p4,3*NBPW(sp)
    addiu sp,4*NBPW
.endm
.macro PUSH3 p1,p2,p3
    addiu sp,-3*NBPW
    sw \p1,0*NBPW(sp)
    sw \p2,1*NBPW(sp)
    sw \p3,2*NBPW(sp)
.endm
.macro POP3 p1,p2,p3
    lw \p1,0*NBPW(sp)
    lw \p2,1*NBPW(sp)
    lw \p3,2*NBPW(sp)
    addiu sp,3*NBPW
.endm
.macro POP2 p1,p2
    lw \p1,0*NBPW(sp)
    lw \p2,1*NBPW(sp)
    addiu sp,2*NBPW
.endm
.macro PUSH1 p1
    addiu sp,-1*NBPW
    sw \p1,0*NBPW(sp)
.endm
.macro POP1 p1
    lw \p1,0*NBPW(sp)
    addiu sp,1*NBPW
.endm
#endif  /*} _MR3K_MACROS_ASH */
