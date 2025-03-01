/* i386-linux.elf-main.c -- stub loader for Linux 32-bit ELF executable

   This file is part of the UPX executable compressor.

   Copyright (C) 1996-2025 Markus Franz Xaver Johannes Oberhumer
   Copyright (C) 1996-2025 Laszlo Molnar
   Copyright (C) 2000-2025 John F. Reiser
   All Rights Reserved.

   UPX and the UCL library are free software; you can redistribute them
   and/or modify them under the terms of the GNU General Public License as
   published by the Free Software Foundation; either version 2 of
   the License, or (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; see the file COPYING.
   If not, write to the Free Software Foundation, Inc.,
   59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.

   Markus F.X.J. Oberhumer              Laszlo Molnar
   <markus@oberhumer.com>               <ezerotven+github@gmail.com>

   John F. Reiser
   <jreiser@users.sourceforge.net>
 */

#ifndef DEBUG  //{
#define DEBUG 0
#endif  //}

#ifdef __mips__  //{
// We want to supersede in *.elf-fold.S, not use include/linux.h
#define NO_WANT_CLOSE 1
#define NO_WANT_EXIT 1
#define NO_WANT_MMAP 1
#define NO_WANT_MPROTECT 1
#define NO_WANT_MSYNC 1
#define NO_WANT_OPEN 1
#define NO_WANT_READ 1
#define NO_WANT_WRITE 1
extern int open(char const *pathname, int flags, unsigned mode);
extern int read(int fd, void *buf, unsigned count);
#endif  //}
#include "include/linux.h"

#define MFD_EXEC 0x0010
#define MS_SYNC 4
#define nullptr 0

extern void *memcpy(void *dst, void const *src, size_t n);
// Pprotect is mprotect but uses page-aligned address (Linux requirement)
extern unsigned Pprotect(void *, size_t, unsigned);
extern int Psync(void const *, size_t, unsigned);
//extern void *Pmap(void *, size_t, unsigned, unsigned, int, size_t);
//extern int Punmap(void *, size_t);
extern size_t Pwrite(unsigned, void const *, size_t);
extern ssize_t write(int, void const *, size_t);
extern int munmap(void *, size_t);
extern int close(int);
extern void exit(int code) __attribute__ ((__noreturn__));
#  define mmap_privanon(addr,len,prot,flgs) mmap((addr),(len),(prot), \
        MAP_PRIVATE|MAP_ANONYMOUS|(flgs),-1,0)

extern void my_bkpt(void const *, ...);

#if defined(__powerpc__) //}{
#define addr_string(string) ({ \
    char const *str; \
    asm("bl 0f; .string \"" string "\"; .balign 4; 0: mflr %0" \
/*out*/ : "=r"(str) \
/* in*/ : \
/*und*/ : "lr"); \
    str; \
})
#elif defined(__i386__) //}{
#define addr_string(string) ({ \
    char const *str; \
    asm("call 0f; .asciz \"" string "\"; 0: pop %0" \
/*out*/ : "=r"(str) ); \
    str; \
})
#elif defined(__arm__) //}{
#define addr_string(string) ({ \
    char const *str; \
    asm("bl 0f; .string \"" string "\"; .balign 4; 0: mov %0,x30" \
/*out*/ : "=r"(str) \
/* in*/ : \
/*und*/ : "x30"); \
    str; \
})
#elif defined(__mips__) //}{
#define addr_string(string) ({ \
    char const *str; \
    asm("bal 0f; .string \"" string "\"; .balign 4; 0: move %0,ra" \
/*out*/ : "=r"(str) \
/* in*/ : \
/*und*/ : "ra"); \
    str; \
})
#else  //}{
#error;
#endif  //}
#if !DEBUG //{
#define DPRINTF(fmt, args...) /*empty*/
#else  //}{
// DPRINTF is defined as an expression using "({ ... })"
// so that DPRINTF can be invoked inside an expression,
// and then followed by a comma to ignore the return value.
// The only complication is that percent and backslash
// must be doubled in the format string, because the format
// string is processd twice: once at compile-time by 'asm'
// to produce the assembled value, and once at runtime to use it.
#if defined(__powerpc__)  //{
#define DPRINTF(fmt, args...) ({ \
    char const *r_fmt; \
    asm("bl 0f; .string \"" fmt "\"; .balign 4; 0: mflr %0" \
/*out*/ : "=r"(r_fmt) \
/* in*/ : \
/*und*/ : "lr"); \
    dprintf(r_fmt, args); \
})
#elif defined(__x86_64) || defined(__i386__) //}{
#define DPRINTF(fmt, args...) ({ \
    char const *r_fmt; \
    asm("call 0f; .asciz \"" fmt "\"; 0: pop %0" \
/*out*/ : "=r"(r_fmt) ); \
    dprintf(r_fmt, args); \
})
#elif defined(__arm__)  /*}{*/
#define DPRINTF(fmt, args...) ({ \
    char const *r_fmt; \
    asm("mov %0,pc; b 0f; \
        .asciz \"" fmt "\"; .balign 4; \
      0: " \
/*out*/ : "=r"(r_fmt) ); \
    dprintf(r_fmt, args); \
})
#elif defined(__mips__)  /*}{*/
#define DPRINTF(fmt, args...) ({ \
    char const *r_fmt; \
    asm(".set noreorder; bal L%=j; move %0,$31; .set reorder; \
        .asciz \"" fmt "\"; .balign 4; \
      L%=j: " \
/*out*/ : "=r"(r_fmt) \
/* in*/ : \
/*und*/ : "ra"); \
    dprintf(r_fmt, args); \
})
#endif  //}

static int dprintf(char const *fmt, ...); // forward
#endif  /*}*/

/*************************************************************************
// configuration section
**************************************************************************/

// In order to make it much easier to move this code at runtime and execute
// it at an address different from it load address:  there must be no
// static data, and no string constants.

#define ElfW(sym) Elf32_ ## sym

#include "MAX_ELF_HDR.c"

/*************************************************************************
// "file" util
**************************************************************************/

typedef struct {
    size_t size;  // must be first to match size[0] uncompressed size
    char *buf;
} Extent;


static void
xread(Extent *x, char *buf, size_t count)
{
    DPRINTF("xread x.size=%%x  x.buf=%%p  buf=%%p  count=%%x\\n",
        x->size, x->buf, buf, count);
    char *p=x->buf, *q=buf;
    size_t j;
    if (x->size < count) {
        exit(127);
    }
    for (j = count; 0!=j--; ++p, ++q) {
        *q = *p;
    }
    x->buf  += count;
    x->size -= count;
    DPRINTF("xread done count=%%x\\n", count);
}


/*************************************************************************
// util
**************************************************************************/

#if !DEBUG  //{  save space
#define ERR_LAB error: exit(127);
#define err_exit(a) goto error
#else  //}{  save debugging time
#define ERR_LAB /*empty*/
static void
err_exit(int a)
{
    (void)a;  // debugging convenience
    DPRINTF("err_exit %%d\\n", a);
    exit(127);
}
#endif  //}

/*************************************************************************
// UPX & NRV stuff
**************************************************************************/

extern size_t get_page_mask(void);  // variable page size AT_PAGESZ; see *-fold.S

int f_expand( // .globl in $(ARCH)-expand.S
    nrv_byte const *binfo, nrv_byte *dst, size_t *dstlen);

static void
unpackExtent(
    Extent *const xi,  // input includes struct b_info
    Extent *const xo   // output
)
{
    while (xo->size) {
        DPRINTF("unpackExtent xi=(%%p %%p)  xo=(%%p %%p)\\n",
            xi->size, xi->buf, xo->size, xo->buf);
        struct b_info h;
        //   Note: if h.sz_unc == h.sz_cpr then the block was not
        //   compressible and is stored in its uncompressed form.

        // Read and check block sizes.
        xread(xi, (char *)&h, sizeof(h));
        DPRINTF("h.sz_unc=%%x  h.sz_cpr=%%x  h.b_method=%%x\\n",
            h.sz_unc, h.sz_cpr, h.b_method);
        if (h.sz_unc == 0) {                     // uncompressed size 0 -> EOF
            if (h.sz_cpr != UPX_MAGIC_LE32)      // h.sz_cpr must be h->magic
                err_exit(2);
            if (xi->size != 0)                 // all bytes must be written
                err_exit(3);
            break;
        }
        if (h.sz_cpr <= 0) {
            err_exit(4);
ERR_LAB
        }
        if (h.sz_cpr > h.sz_unc
        ||  h.sz_unc > xo->size ) {
            err_exit(5);
        }
        // Now we have:
        //   assert(h.sz_cpr <= h.sz_unc);
        //   assert(h.sz_unc > 0 && h.sz_unc <= blocksize);
        //   assert(h.sz_cpr > 0 && h.sz_cpr <= blocksize);

        if (h.sz_cpr < h.sz_unc) { // Decompress block
            size_t out_len = h.sz_unc;  // EOF for lzma
            int const j = f_expand((unsigned char *)xi->buf - sizeof(h),
                (unsigned char *)xo->buf, &out_len);
            if (j != 0 || out_len != (nrv_uint)h.sz_unc) {
                DPRINTF("  j=%%x  out_len=%%x  &h=%%p\\n", j, out_len, &h);
                err_exit(6);
            }
            xi->buf  += h.sz_cpr;
            xi->size -= h.sz_cpr;
        }
        else { // copy literal block
            DPRINTF("  copy %%p  %%p  %%p\\n", xi->buf, xo->buf, h.sz_cpr);
            xi->size += sizeof(h);  // xread(xi, &h, sizeof(h)) was a peek
            xread(xi, xo->buf, h.sz_cpr);
        }
        xo->buf  += h.sz_unc;
        xo->size -= h.sz_unc;
    }
}

extern long upx_mmap_and_fd(  // x86_64 Android emulator of i386 is not faithful
     void *ptr
     , unsigned len  // also pre-allocate space in file
     , char *pathname  // 0 ==> call get_upxfn_path, which stores if 1st time
#if 0  //{ always 0
     , unsigned frag_mask  // ~page_mask
#endif  //}
);
#if defined(__i386__)  //
// Create (or find) an escape hatch to use when munmapping ourselves the stub.
// Called by do_xmap to create it; remembered in AT_NULL.d_val
static char *
make_hatch(
    ElfW(Phdr) const *const phdr,
    char *next_unc,
    unsigned page_mask
)
{
    unsigned *hatch = 0;
    unsigned frag_mask = ~page_mask;
    unsigned code[2] = {
        0x586180cd,  // int #0x80; popa; pop %eax
        0x90e0ff3e,  // notrack jmp *%eax; nop
    };
    DPRINTF("make_hatch %%p %%p %%x\\n", phdr, next_unc, frag_mask);
    if (phdr->p_type==PT_LOAD && phdr->p_flags & PF_X) {
        next_unc += phdr->p_memsz - phdr->p_filesz;  // Skip over local .bss
        frag_mask &= -(long)next_unc;  // bytes left on page
        if (sizeof(code) <= frag_mask) {
            hatch = (unsigned *)next_unc;
            hatch[0] = code[0];
            hatch[1] = code[1];
        }
        else { // Does not fit at hi end of .text, so must use a new page "permanently"
            unsigned long fdmap = upx_mmap_and_fd((void *)0, sizeof(code), nullptr);
            unsigned mfd = -1+ (0xfff& fdmap);
            write(mfd, &code, sizeof(code));
            hatch = mmap((void *)(fdmap & ~0xffful), sizeof(code),
              PROT_READ|PROT_EXEC, MAP_PRIVATE, mfd, 0);
            close(mfd);
        }
    }
    DPRINTF("hatch=%%p\\n", hatch);
    return (char *)hatch;
}
#elif defined(__arm__)  /*}{*/
extern unsigned get_sys_munmap(void);
#define NINSTR 3
#define NBPI 4

static void *
make_hatch(
    ElfW(Phdr) const *const phdr,
    char *next_unc,
    unsigned page_mask
)
{
    unsigned const sys_munmap = get_sys_munmap();
    unsigned frag_mask = ~page_mask;
    unsigned code[2] = {
        sys_munmap,  // syscall __NR_unmap
        0xe8bd8003,  // ldmia sp!,{r0,r1,pc}
     };
    unsigned *hatch = 0;
    DPRINTF("make_hatch %%p %%p %%x\\n", phdr, next_unc, frag_mask);

    if (phdr->p_type==PT_LOAD && phdr->p_flags & PF_X) {
        next_unc += phdr->p_memsz - phdr->p_filesz;  // Skip over local .bss
        frag_mask &= -(long)next_unc;  // bytes left on page
        if (2*4 <= frag_mask) {
            hatch = (unsigned *)(void *)(~3ul & (long)(3+ next_unc));
            hatch[0]= code[0];
            hatch[1]= code[1];
            //__clear_cache(&hatch[0], &hatch[2]);  subsumed by write()+mmap()
        }
        else { // Does not fit at hi end of .text, so must use a new page "permanently"
            unsigned long fdmap = upx_mmap_and_fd((void *)0, sizeof(code), nullptr);
            unsigned mfd = -1+ (0xfff& fdmap);
            write(mfd, &code, sizeof(code));
            hatch = mmap((void *)(fdmap & ~0xffful), sizeof(code),
                PROT_READ|PROT_EXEC, MAP_PRIVATE, mfd, 0);
            close(mfd);
        }
    }
    DPRINTF("hatch=%%p\\n", hatch);
    return hatch;
}
#elif defined(__mips__)  /*}{*/
static void *
make_hatch(
    ElfW(Phdr) const *const phdr,
    char *next_unc,
    unsigned page_mask)
{
    unsigned frag_mask = ~page_mask;
    unsigned code[3];
    // avoid gcc initializing array by copying .rodata
    code[0] = 0x0000000c;  // syscall
#define RS(r) ((037&(r))<<21)
#define JR 010
    code[1] = RS(30)|JR;   // jr $30  # s8
    code[2] = 0x00000000;  //   nop

    unsigned *hatch = 0;
    DPRINTF("make_hatch_mips %%p %%p %%x\\n", phdr, next_unc, frag_mask);

    if (phdr->p_type==PT_LOAD && phdr->p_flags & PF_X) {
        next_unc += phdr->p_memsz - phdr->p_filesz;  // Skip over local .bss
        frag_mask &= -(long)next_unc;  // bytes left on page
        if (3*4 <= frag_mask) {
            hatch = (unsigned *)(void *)(~3ul & (long)(3+ next_unc));
            hatch[0]= code[0];
            hatch[1]= code[1];
            hatch[2]= code[2];
            //__clear_cache(&hatch[0], &hatch[3]);  subsumed by write()+mmap()
        }
        else { // Does not fit at hi end of .text, so must use a new page "permanently"
            unsigned long fdmap = upx_mmap_and_fd((void *)0, sizeof(code), nullptr);
            unsigned mfd = -1+ (0xfff& fdmap);
            write(mfd, &code, sizeof(code));
            hatch = mmap((void *)(fdmap & ~0xffful), sizeof(code),
                PROT_READ|PROT_EXEC, MAP_PRIVATE, mfd, 0);
            close(mfd);
        }
    }
    DPRINTF("hatch=%%p\\n", hatch);
    return hatch;
}
#elif defined(__powerpc__)  /*}{*/
static void *
make_hatch(
    ElfW(Phdr) const *const phdr,
    char *next_unc,
    unsigned page_mask)
{
    unsigned frag_mask = ~page_mask;
    unsigned code[2];
    // avoid gcc initializing array by copying .rodata
    code[0] = 0x44000002;  // sc
    code[1] = 0x4e800020;  // blr

    unsigned *hatch = 0;
    DPRINTF("make_hatch %%p %%p %%x\\n", phdr, next_unc, frag_mask);

    if (phdr->p_type==PT_LOAD && phdr->p_flags & PF_X) {
        next_unc += phdr->p_memsz - phdr->p_filesz;  // Skip over local .bss
        frag_mask &= -(long)next_unc;  // bytes left on page
        if (2*4 <= frag_mask) {
            hatch = (unsigned *)(void *)(~3ul & (long)(3+ next_unc));
            hatch[0]= code[0];
            hatch[1]= code[1];
            //__clear_cache(&hatch[0], &hatch[2]);  subsumed by write()+mmap()
        }
        else { // Does not fit at hi end of .text, so must use a new page "permanently"
            unsigned long fdmap = upx_mmap_and_fd((void *)0, sizeof(code), nullptr);
            unsigned mfd = -1+ (0xfff& fdmap);
            write(mfd, &code, sizeof(code));
            hatch = mmap((void *)(fdmap & ~0xffful), sizeof(code),
                PROT_READ|PROT_EXEC, MAP_PRIVATE, mfd, 0);
            close(mfd);
        }
    }
    DPRINTF("hatch=%%p\\n", hatch);
    return hatch;
}
#endif  /*}*/

#if defined(__powerpc64__) || defined(__aarch64__)  //{ bzero
static void
upx_bzero(char *p, size_t len)
{
    DPRINTF("bzero %%x  %%x\\n", p, len);
    if (len) do {
        *p++= 0;
    } while (--len);
}
#define bzero upx_bzero
#else  //}{
#define bzero(a,b)  __builtin_memset(a,0,b)
#endif  //}

static void
auxv_up(ElfW(auxv_t) *av, unsigned const type, uint64_t const value)
{
    DPRINTF("\\nauxv_up av=%%p  %%d  %%p\\n", av, type, value);
    if (!av || (1& (size_t)av)) { // none, or inhibited for PT_INTERP
        return;
    }
    // Multiple slots can have 'type' which wastes space but is legal.
    // rtld (ld-linux) uses the last one, so we must scan the whole table.
    ElfW(auxv_t) *ignore_slot = 0;
    int found = 0;
    for (;; ++av) {
        DPRINTF("  %%d  %%p\\n", av->a_type, av->a_un.a_val);
        if (av->a_type == type) {
            av->a_un.a_val = value;
            ++found;
        }
        else if (av->a_type == AT_IGNORE) {
            ignore_slot = av;
        }
        if (av->a_type==AT_NULL) { // done scanning
            if (found) {
                return;
            }
            if (ignore_slot) {
                ignore_slot->a_type = type;
                ignore_slot->a_un.a_val = value;
                return;
            }
            err_exit(20);
ERR_LAB
            return;
        }
    }
}

// Segregate large local array, to avoid code bloat due to large displacements.
// Not 'static' to disaable inlining, to control sizeof stack frame in callers.
/*static*/ void
underlay(unsigned size, char *ptr, unsigned len)  // len <= PAGE_SIZE
{
    DPRINTF("underlay size=%%u  ptr=%%p  len=%%u\\n", size, ptr, len);
    unsigned saved[4096/sizeof(unsigned)];
    memcpy(saved, ptr, len);
    mmap(ptr, size, PROT_WRITE|PROT_READ,
        MAP_FIXED|MAP_PRIVATE|MAP_ANONYMOUS, -1, 0);
    memcpy(ptr, saved, len);
}

#if 0  //{
// Exchange the bits with values 4 (PF_R, PROT_EXEC) and 1 (PF_X, PROT_READ)
// Use table lookup into a PIC-string that pre-computes the result.
unsigned PF_TO_PROT(unsigned flags)
{
    char const *table = addr_string("\x80\x04\x02\x06\x01\x05\x03\x07");
    return 7& table[flags & (PF_R|PF_W|PF_X)];
}
#else  //}{
// The PF_* and PROT_* bits are {1,2,4}; the conversion table fits in 32 bits.
#define REP8(x) \
    ((x)|((x)<<4)|((x)<<8)|((x)<<12)|((x)<<16)|((x)<<20)|((x)<<24)|((x)<<28))
#define EXP8(y) \
    ((1&(y)) ? 0xf0f0f0f0 : (2&(y)) ? 0xff00ff00 : (4&(y)) ? 0xffff0000 : 0)
#define PF_TO_PROT(pf) \
    ((PROT_READ|PROT_WRITE|PROT_EXEC) & ( \
        ( (REP8(PROT_EXEC ) & EXP8(PF_X)) \
         |(REP8(PROT_READ ) & EXP8(PF_R)) \
         |(REP8(PROT_WRITE) & EXP8(PF_W)) \
        ) >> ((pf & (PF_R|PF_W|PF_X))<<2) ))
#endif  //}

// Find convex hull of PT_LOAD (the minimal interval which covers all PT_LOAD),
// and mmap that much, to be sure that a kernel using exec-shield-randomize
// won't place the first piece in a way that leaves no room for the rest.
static ptrdiff_t // returns relocation constant
#if defined(__i386__)  /*{*/
__attribute__((regparm(3), stdcall))
#endif  /*}*/
xfind_pages(unsigned mflags, Elf32_Phdr const *phdr, int phnum,
    Elf32_Addr *const p_brk
)
{
    size_t const page_mask = get_page_mask();
    Elf32_Addr lo= ~0, hi= 0, addr = 0, p_align = 0x1000;
    DPRINTF("xfind_pages  %%x  %%p  %%d  %%p  %%p\\n", mflags, phdr, phnum, p_brk, page_mask);
    for (; --phnum>=0; ++phdr) if (PT_LOAD==phdr->p_type
#if defined(__arm__)  /*{*/
                               &&  phdr->p_memsz
// Android < 4.1 (kernel < 3.0.31) often has PT_INTERP of /system/bin/linker
// with bad PT_LOAD[0].  https://sourceforge.net/p/upx/bugs/221
// Type: EXEC (Executable file)
//
// Program Headers:
// Type   Offset   VirtAddr   PhysAddr FileSiz MemSiz  Flg  Align
// LOAD 0x0000d4 0x00000000 0xb0000000 0x00000 0x00000 R   0x1000
// LOAD 0x001000 0xb0001000 0xb0001000 0x07614 0x07614 R E 0x1000
// LOAD 0x009000 0xb0009000 0xb0009000 0x006f8 0x0ccdc RW  0x1000
#endif  /*}*/
    ) {
        if (phdr->p_vaddr < lo) {
            lo = phdr->p_vaddr;
        }
        if (hi < (phdr->p_memsz + phdr->p_vaddr)) {
            hi =  phdr->p_memsz + phdr->p_vaddr;
        }
        if (p_align < phdr->p_align) {
            p_align = phdr->p_align;
        }
    } // end scan of PT_LOADs
    size_t page_size = 0u - page_mask;
    lo &= page_mask;  // round down to page boundary
    size_t len1 = page_mask & (hi - lo + page_size -1);  // desired length

    // Linux lacks mmap_aligned(), so allocate a larger space, then trim the ends.
    // Avoid division (p_align / page_size); both are powers of 2
    unsigned q = 1;
    while ((q * page_size) < p_align) q <<= 1;
    --q;  // number of extra pages
    unsigned len2 = len1 + (q * page_size);  // get enough space to align
    addr = (Elf32_Addr)mmap_privanon((void *)lo, len2, PROT_NONE, mflags);
    DPRINTF("  addr=%%p  lo=%%p  hi=%%p align=%%p  q=%%p  len1=%%p  len2=%%p\\n",
        addr, lo, hi, p_align, q, len1, len2, p_align);
    if (q) {
        size_t len3 = (-1 + p_align) & -addr;  // up to p_align boundary
        if (len3) {
            munmap((void *)addr, len3);  // trim the low end
            addr += len3;
            len2 -= len3;
        }
        if (len2 -= len1) {
            munmap((void *)(addr + len1), len2); // trim the high end
        }
    }
    DPRINTF("  addr=%%p\\n", addr);
    *p_brk = len1 + addr;  // the logical value of brk(0)
    return (ptrdiff_t)addr - lo;
}

static ElfW(Addr)  // entry address
do_xmap(
    ElfW(Ehdr) const *const ehdr,
    Extent *const xi,
    int const fdi,
    ElfW(auxv_t) *const av,
    ElfW(Addr) *const p_reloc
)
{
    ElfW(Phdr) const *phdr = (ElfW(Phdr) const *)(void const *)(ehdr->e_phoff +
        (char const *)ehdr);
    ElfW(Addr) v_brk = 0;
    ElfW(Addr) reloc = 0;
    if (xi) { // compressed main program:
        // C_BASE space reservation, C_TEXT compressed data and stub
        ElfW(Addr)  ehdr0 = *p_reloc;  // the 'hi' copy!
        ElfW(Phdr) *phdr0 = (ElfW(Phdr) *)(1+ (ElfW(Ehdr) *)ehdr0);  // cheats .e_phoff
        // Clear the 'lo' space reservation for use by PT_LOADs
        ehdr0 -= phdr0[1].p_vaddr;  // the 'lo' copy
        if (ET_EXEC == ehdr->e_type) {
            ehdr0 = phdr0[0].p_vaddr;
        }
        else {
            reloc = ehdr0;
        }
        v_brk = phdr0->p_memsz + ehdr0;
        munmap((void *)ehdr0, phdr0->p_memsz);
    }
    else { // PT_INTERP
        DPRINTF("INTERP ehdr=%%p  av=%%p\\n", ehdr, av);
        reloc = xfind_pages( ((ET_DYN!=ehdr->e_type) ? MAP_FIXED : 0),
            phdr, ehdr->e_phnum, &v_brk);
    }
    DPRINTF("do_xmap  ehdr=%%p  xi=%%p(%%x %%p)  fdi=%%x\\n"
          "  av=%%p  reloc=%%p  p_reloc=%%p/%%p\\n",
        ehdr, xi, (xi? xi->size: 0), (xi? xi->buf: 0), fdi,
        av, reloc, p_reloc, *p_reloc);

    size_t const page_mask = get_page_mask();
    int j;
    for (j=0; j < ehdr->e_phnum; ++phdr, ++j)
    if (xi && PT_PHDR==phdr->p_type) {
        auxv_up(av, AT_PHDR, phdr->p_vaddr + reloc);
    } else
    if (PT_LOAD==phdr->p_type && phdr->p_memsz != 0) {
        unsigned const prot = PF_TO_PROT(phdr->p_flags);
        DPRINTF("\\n\\nLOAD@%%p  p_offset=%%p  p_vaddr=%%p  p_filesz=%%p"
            "  p_memsz=%%p  p_flags=%%x  prot=%%x\\n",
            phdr, phdr->p_offset, phdr->p_vaddr, phdr->p_filesz,
            phdr->p_memsz, phdr->p_flags, prot);
        if (xi && !phdr->p_offset /*&& ET_EXEC==ehdr->e_type*/) { // 1st PT_LOAD
            // ? Compressed PT_INTERP must not overwrite values from compressed a.out?
            auxv_up(av, AT_PHDR, phdr->p_vaddr + reloc + ehdr->e_phoff);
            auxv_up(av, AT_PHNUM, ehdr->e_phnum);
            auxv_up(av, AT_PHENT, ehdr->e_phentsize);  /* ancient kernels might omit! */
            //auxv_up(av, AT_PAGESZ, PAGE_SIZE);  /* ld-linux.so.2 does not need this */
        }
        Extent xo;
        size_t mlen = xo.size = phdr->p_filesz;
        char * addr = xo.buf  = reloc + (char *)phdr->p_vaddr;
            // xo.size, xo.buf are not changed except by unpackExtent()
        char *const hi_addr = phdr->p_memsz + addr;  // end of local .bss
        char *addr2 = mlen + addr;  // end of local .data
        size_t frag  = ~page_mask & (ElfW(Addr))addr;
        mlen += frag;
        addr -= frag;

#if defined(__powerpc64__) || defined(__aarch64__)
        // Round up to hardware PAGE_SIZE; allows emulator with smaller.
        // But (later) still need bzero when .p_filesz < .p_memsz .
        mlen += -(mlen + (size_t)addr) &~ page_mask;
        DPRINTF("  mlen=%%p\\n", mlen);
#endif

        DPRINTF("mmap addr=%%p  mlen=%%p  offset=%%p  frag=%%p  prot=%%x\\n",
            addr, mlen, phdr->p_offset - frag, frag, prot);
        int mfd = 0;
        if (xi && phdr->p_flags & PF_X) { // SELinux
            // Cannot set PROT_EXEC except via mmap() into a region (Linux "vma")
            // that has never had PROT_WRITE.  So use a Linux-only "memory file"
            // to hold the contents.
            unsigned long val = upx_mmap_and_fd(addr, mlen, nullptr);
            mfd = 0xfff & val;
            val -= mfd; if ((char *)val != addr) err_exit(17);
            --mfd;
        }
        else {
            unsigned tprot = prot;
            // Notice that first 4 args are same: mmap vs mmap_privanon
            if (xi) {
                tprot |=  PROT_WRITE;  // De-compression needs Write
                tprot &= ~PROT_EXEC;  // Avoid simultaneous Write and eXecute
                if (addr != mmap_privanon(addr, mlen, tprot, MAP_FIXED|MAP_PRIVATE)) {
                    err_exit(11);
                }
            }
            else if (addr != mmap(addr, mlen, tprot, MAP_FIXED|MAP_PRIVATE,
                        fdi, phdr->p_offset - frag)) {
                err_exit(8);
            }
        }
        DPRINTF("addr= %%p\\n", addr);

        if (xi) {
            DPRINTF("before unpack xi=(%%p %%p  xo=(%%p %%p)\\n", xi->size, xi->buf, xo.size, xo.buf);
            unpackExtent(xi, &xo);  // updates xi and xo
            DPRINTF(" after unpack xi=(%%p %%p  xo=(%%p %%p)\\n", xi->size, xi->buf, xo.size, xo.buf);
        }
        if (PROT_WRITE & prot) { // note: read-only .bss not supported here
            // Clear to end-of-page (first part of .bss or &_end)
            unsigned hi_frag = -(long)addr2 &~ page_mask;
            bzero(addr2, hi_frag);
            addr2 += hi_frag;  // will be page aligned
        }

        if (xi && phdr->p_flags & PF_X) {
            char *hatch = make_hatch(phdr, xo.buf, page_mask);
            if (0!=hatch) {
                // Always update AT_NULL, especially for compressed PT_INTERP.
                // Clearing lo bit of av is for i386 only; else is superfluous.
                auxv_up((ElfW(auxv_t) *)(~1 & (size_t)av), AT_NULL, (size_t)hatch);
            }

            // SELinux: Map the contents of mfd as per *phdr.
            DPRINTF("hatch protect addr=%%p  mlen=%%p\\n", addr, mlen);
            Psync(addr, mlen, MS_SYNC);  // be sure file gets de-compressed bytes
            munmap(addr, mlen);  // toss the VMA that has PROT_WRITE
            if (addr != mmap(addr, mlen, prot, MAP_FIXED|MAP_SHARED, mfd, 0)) {
                err_exit(9);
            }
            close(mfd);
        }
        else if ((PROT_WRITE|PROT_READ) != prot
        &&  0!=Pprotect(addr, mlen, prot)) {
            err_exit(10);
ERR_LAB
        }
        if (addr2 < hi_addr) { // pages for .bss beyond last page for p_filesz
            DPRINTF("zmap addr2=%%p  len=%%p\\n", addr2, hi_addr - addr2);
            if (addr2 != mmap_privanon(addr2, hi_addr - addr2, prot, MAP_FIXED)) {
                err_exit(10);
            }
        }
    }
    if (xi && ET_DYN!=ehdr->e_type) {
        // Needed only if compressed shell script invokes compressed shell.
        // Besides, fold.S needs _Ehdr that is tossed
        // do_brk((void *)v_brk);
    }
    if (p_reloc) {
        *p_reloc = reloc;
    }
    return ehdr->e_entry + reloc;
}


/*************************************************************************
// upx_main - called by our entry code
//
// This function is optimized for size.
**************************************************************************/

void *
upx_main(  // returns entry address
/*arg1*/    struct b_info const *const bi  // 1st block header
/*arg2*/    , size_t const sz_compressed  // total length
/*arg3*/    , ElfW(Addr) elfaddr  // In: &ElfW(Ehdr) for stub
/*arg4*/    , ElfW(auxv_t) *const av
/*arg5*/    , ElfW(Ehdr) *const ehdr  // temp char[sz_ehdr] for decompressing
)
{
    DPRINTF("upx_main  b_info=%%p  sz_compressed=%%p  ehdr=%%p  av=%%p\\n",
        bi, sz_compressed, ehdr, av);
    Extent xo, xi1, xi2;
    xo.buf  = (char *)ehdr;
    xo.size = bi->sz_unc;  // can require bi aligned(4)
    xi2.buf = CONST_CAST(char *, bi); xi2.size = bi->sz_cpr + sizeof(*bi);
    xi1.buf = CONST_CAST(char *, bi); xi1.size = sz_compressed;

    // ehdr = Uncompress Ehdr and Phdrs
    unpackExtent(&xi2, &xo);  // never filtered?

    ElfW(Addr) *const p_reloc = &elfaddr;
    ElfW(Addr) page_mask = get_page_mask(); (void)page_mask;
    DPRINTF("upx_main1  .e_entry=%%p  p_reloc=%%p  *p_reloc=%%p  page_mask=%%p\\n",
        ehdr->e_entry, p_reloc, *p_reloc, page_mask);
    ElfW(Phdr) *phdr = (ElfW(Phdr) *)(1+ ehdr);

    // De-compress Ehdr again into actual position, then de-compress the rest.
    ElfW(Addr) entry = do_xmap(ehdr, &xi1, 0, av, p_reloc);
    DPRINTF("upx_main2  entry=%%p  *p_reloc=%%p\\n", entry, *p_reloc);
    auxv_up(av, AT_ENTRY , entry);

  { // Map PT_INTERP program interpreter
    phdr = (ElfW(Phdr) *)(1+ ehdr);
    unsigned j;
    for (j=0; j < ehdr->e_phnum; ++phdr, ++j) if (PT_INTERP==phdr->p_type) {
        char const *const iname = *p_reloc + (char const *)phdr->p_vaddr;
        int const fdi = open(iname, O_RDONLY, 0);
        if (0 > fdi) {
            err_exit(18);
        }
        if (MAX_ELF_HDR_32!=read(fdi, (void *)ehdr, MAX_ELF_HDR_32)) {
ERR_LAB
            err_exit(19);
        }
        // We expect PT_INTERP to be ET_DYN at 0.
        // Thus do_xmap will set *p_reloc = slide.
        *p_reloc = 0;  // kernel picks where PT_INTERP goes
        entry = do_xmap(ehdr, 0, fdi, 0, p_reloc);
        DPRINTF("interp p_reloc=%%p  reloc=%%p\\n", p_reloc, *p_reloc);
        auxv_up(av, AT_BASE, *p_reloc);  // musl
        close(fdi);
    }
  }

    return (void *)entry;
}

#if DEBUG  //{

static int
unsimal(unsigned x, char *ptr, int n)
{
    unsigned m = 10;
    while (10 <= (x / m)) m *= 10;
    while (10 <= x) {
        unsigned d = x / m;
    x -= m * d;
        m /= 10;
        ptr[n++] = '0' + d;
    }
    ptr[n++] = '0' + x;
    return n;
}

static int
decimal(int x, char *ptr, int n)
{
    if (x < 0) {
        x = -x;
        ptr[n++] = '-';
    }
    return unsimal(x, ptr, n);
}

static int
heximal(unsigned long x, char *ptr, int n)
{
    unsigned j = -1+ 2*sizeof(unsigned long);
    unsigned long m = 0xful << (4 * j);
    for (; j; --j, m >>= 4) { // omit leading 0 digits
        if (m & x) break;
    }
    for (; m; --j, m >>= 4) {
        unsigned d = 0xf & (x >> (4 * j));
        ptr[n++] = ((10<=d) ? ('a' - 10) : '0') + d;
    }
    return n;
}

#define va_arg      __builtin_va_arg
#define va_end      __builtin_va_end
#define va_list     __builtin_va_list
#define va_start    __builtin_va_start

static int
dprintf(char const *fmt, ...)
{
    int n= 0;
    char const *literal = 0;  // NULL
    char buf[24];  // ~0ull == 18446744073709551615 ==> 20 chars
    va_list va; va_start(va, fmt);
    for (;;) {
        char c = *fmt++;
        if (!c) { // end of fmt
            if (literal) {
                goto finish;
            }
            break;  // goto done
        }
        if ('%'!=c) {
            if (!literal) {
                literal = fmt;  // 1 beyond start of literal
            }
            continue;
        }
        // '%' == c
        if (literal) {
finish:
            n += write(2, -1+ literal, fmt - literal);
            literal = 0;  // NULL
            if (!c) { // fmt already ended
               break;  // goto done
            }
        }
        switch (c= *fmt++) { // deficiency: does not handle _long_
        default: { // un-implemented conversion
            n+= write(2, -1+ fmt, 1);
        } break;
        case 0: { // fmt ends with "%\0" ==> ignore
            goto done;
        } break;
        case 'u': {
            n+= write(2, buf, unsimal(va_arg(va, unsigned), buf, 0));
        } break;
        case 'd': {
            n+= write(2, buf, decimal(va_arg(va, int), buf, 0));
        } break;
        case 'p': {
            buf[0] = '0';
            buf[1] = 'x';
            n+= write(2, buf, heximal((unsigned long)va_arg(va, void *), buf, 2));
        } break;
        case 'x': {
            buf[0] = '0';
            buf[1] = 'x';
            n+= write(2, buf, heximal(va_arg(va, int), buf, 2));
        } break;
        } // 'switch'
    }
done:
    va_end(va);
    return n;
 }
#endif  //}

/* vim:set ts=4 sw=4 et: */
