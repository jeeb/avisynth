// Cpulib.h : cpu-specific libraries by Richard Berg
//
// Many ideas & (heavily modified) algorithms derived from the AMD & Intel 
// Optimization Manuals, hacked into generality with copious 
// ugly-macro-action-stuff


class CPULIB : public Fastlib
{
public:
  CPULIB (void) {};
  virtual ~CPULIB (void) {};

  virtual void fMemcpy (void * dst, const void * src, UL n);
  virtual void fMemset (void * dst, int val, UL n);
};


/*** #define the specifics of each CPU's cache architecture & instruction support ***/

#if CPUF == CPU_P2
#define CACHE_LINE       32          // size of an L1 cache line
#define NUM_CACHE_LINES  512         // total capacity of L1 data cache
#define PREF(x)          ;           // ASM instruction to soft-prefetch x bytes
#define MOVNT(x,y)       movq x, y   // ASM instruction for non-temporal quadword move
#define FENCE            ;           // ASM instruction to flush write cache
#endif

#if CPUF == CPU_P3
#define CACHE_LINE       32
#define NUM_CACHE_LINES  512
#define PREF(x)          prefetchnta x
#define MOVNT(x,y)       movntq   x, y
#define FENCE            sfence
#endif

#if CPUF == CPU_P4
#define CACHE_LINE       64
#define NUM_CACHE_LINES  128
#define PREF(x)          prefetchnta x 
#define MOVNT(x,y)       movntq x, y
#define FENCE            sfence
#endif

#if CPUF == CPU_K6
#define CACHE_LINE       64
#define NUM_CACHE_LINES  512
#define PREF(x)          prefetch x
#define MOVNT(x,y)       movq  x, y
#define FENCE            ;
#endif

#if CPUF == CPU_K7
#define CACHE_LINE       64
#define NUM_CACHE_LINES  1024
#define PREF(x)          prefetchnta x
#define MOVNT(x,y)       movntq x, y
#define FENCE            sfence
#endif



/*** #define the max memory each method should process before deferring to a more
 *** optimized technique 
 ***/

// The smallest copy uses the X86 "movsd" instruction, in an optimized
// form which is an "unrolled loop".  
#define TINY_BLOCK_COPY 64

// Next is a copy that uses the MMX registers to copy 8 bytes at a time,
// also using the "unrolled loop" optimization. This code uses
// the software prefetch instruction to get the data into the cache.
#define IN_CACHE_COPY (CACHE_LINE * NUM_CACHE_LINES)


// For larger blocks, which will spill beyond the cache, it's faster to
// use the Streaming Store instruction MOVNTQ. This write instruction
// bypasses the cache and writes straight to main memory. This code also
// uses the software prefetch instruction to pre-read the data.
#define UNCACHED_COPY (197 * NUM_CACHE_LINES)  // dunno why AMD chose 197, but I've fiddled with
                                               // it and it seems to be a good value


// For the largest size blocks, a special technique called Block Prefetch
// can be used to accelerate the read operations. Block Prefetch reads
// one address per cache line, for a series of cache lines, in a short loop.
// This is faster than using software prefetch. The technique is great for
// getting maximum read bandwidth, especially in DDR memory systems.
#define BLOCK_PREFETCH_COPY infinity 
#define CACHEBLOCK (NUM_CACHE_LINES / 2)  // # of cache lines for block prefetch
                                          // AMD used NCL/8 but I find /2 is about 5% faster 
                                          // on 1MB copies and doesn't hurt anything else









/*********************************************************************************
 block copy: copy a number of bytes from one memory block to another.  alignment
             handled internally
*********************************************************************************/

void CPULIB :: fMemcpy(void * dst, const void * src, UL n)
{
#if CPUF == CPU_P1
#include <stdlib.h>
    memcpy(dst, src, n);
    return;
}

#else
  __asm {
    mov ecx, n    ; number of bytes to copy
    mov edi, dst  ; destination
    mov esi, src  ; source
    mov ebx, ecx  ; keep a copy of count

    cld
    cmp ecx, TINY_BLOCK_COPY
    jb $memcpy_ic_3 ; tiny? skip mmx copy

    cmp ecx, 32*1024      ; don't align between 32k-64k because
    jbe $memcpy_do_align  ; it appears to be slower
    cmp ecx, 64*1024
    jbe $memcpy_align_done

  $memcpy_do_align:
    mov ecx, 8      ; a trick that's faster than rep movsb...
    sub ecx, edi    ; align destination to qword
    and ecx, 111b   ; get the low bits
    sub ebx, ecx    ; update copy count
    neg ecx         ; set up to jump into the array
    add ecx, offset $memcpy_align_done
    jmp ecx         ; jump to array of movsb's

  align 4
    movsb
    movsb
    movsb
    movsb
    movsb
    movsb
    movsb
    movsb

  $memcpy_align_done: ; destination is dword aligned
#if CPUF == CPU_P4  // still need to DQWORD align if P4
    test edi, 7       ; qword aligned?
    jz $qw_aligned

    movd mm0, [esi]   ; already dword aligned, so moving a dword makes you qword aligned
    movd [edi], mm0
    
  $qw_aligned:
    test edi, 15      ; dqword aligned?
    jz $dqw_aligned

    movq mm0, [esi]   ; already qword aligned, so moving a qword makes you dqword aligned
    movq [edi], mm0

  $dqw_aligned:                      
#endif

    mov ecx, ebx      ; number of bytes left to copy
#if CACHE_LINE == 64
    shr ecx, 6        ; get 64-byte block count
#else
    shr ecx, 5        ; get 32-byte block count
#endif
    jz $memcpy_ic_2   ; finish the last few bytes

    cmp ecx, IN_CACHE_COPY / CACHE_LINE ; too big 4 cache? use uncached copy
    jae $memcpy_uc_test


// This is small block copy that uses the MMX registers to copy 8 bytes
// at a time. It uses the "unrolled loop" optimization, and also uses
// the software prefetch instruction to get the data into the cache.

  align 16
  $memcpy_ic_1:         ; cache-line block copies, in-cache copy
    PREF([esi + (200* CACHE_LINE /34+192)]) ; start reading ahead  // tweak math for other CPUs?
#if CACHE_LINE == 64
#if CPUF == CPU_P4
    movdqa xmm0, [esi+0]    ; read 128 bits
    movdqa xmm1, [esi+16]
    movdqa [edi+0], xmm0    ; write 128 bits
    movdqa [edi+16], xmm1
    movdqa xmm0, [esi+32]  
    movdqa xmm1, [esi+48] 
    movdqa [edi+32], xmm0  
    movdqa [edi+48], xmm1
#else // no P4
    movq mm0, [esi+0]   ; read 64 bits
    movq mm1, [esi+8]
    movq [edi+0], mm0   ; write 64 bits
    movq [edi+8], mm1   ; note: the normal movq writes the
    movq mm2, [esi+16]  ; data to cache; a cache line will be
    movq mm3, [esi+24]  ; allocated as needed, to store the data
    movq [edi+16], mm2
    movq [edi+24], mm3
    movq mm0, [esi+32]
    movq mm1, [esi+40]
    movq [edi+32], mm0
    movq [edi+40], mm1
    movq mm2, [esi+48]
    movq mm3, [esi+56]
    movq [edi+48], mm2
    movq [edi+56], mm3
#endif
#else // 32-bit cache_line
    movq mm0, [esi+0]   
    movq mm1, [esi+8]
    movq [edi+0], mm0   
    movq [edi+8], mm1   
    movq mm2, [esi+16]  
    movq mm3, [esi+24]  
    movq [edi+16], mm2
    movq [edi+24], mm3    
#endif
    add esi, CACHE_LINE   ; update source pointer
    add edi, CACHE_LINE   ; update destination pointer
    dec ecx               ; count down
    jnz $memcpy_ic_1      ; last cache-line block?

  $memcpy_ic_2:
    mov ecx, ebx        ; has valid low 6 bits of the byte count
  $memcpy_ic_3:
    shr ecx, 2          ; dword count
    and ecx, 1111b      ; only look at the "remainder" bits
    neg ecx             ; set up to jump into the array
    add ecx, offset $memcpy_last_few
    jmp ecx             ; jump to array of movsd's
  
  $memcpy_uc_test:
    cmp ecx, UNCACHED_COPY / CACHE_LINE ; big enough? use block prefetch copy
    jae $memcpy_bp_1

  $memcpy_64_test:
    or ecx, ecx       ; tail end of block prefetch will jump here
    jz $memcpy_ic_2   ; no more cache-line blocks left


// For larger blocks, which will spill beyond the cache, it's faster to
// use the SSE instruction MOVNTQ. This write instruction
// bypasses the cache and writes straight to main memory. This code also
// uses the software prefetch instruction to pre-read the data.
  align 16
  $memcpy_uc_1:           ; cache-line blocks, uncached copy
    PREF([esi + (200* CACHE_LINE /34+192)]) ; start reading ahead     // tweak math for other CPUs?
#if CPUF == CPU_P4
    movdqa xmm0,[esi+0]     ; read 128 bits
    add edi, CACHE_LINE     ; update destination pointer
    movdqa xmm1,[esi+16]  
    add esi, CACHE_LINE
    movdqa xmm2,[esi-32]
    movntdq [edi-64], xmm0  ; write 128 bits
    movdqa xmm0,[esi-16]
    movntdq [edi-48], xmm1
    movntdq [edi-32], xmm2
#else // no P4
    movq mm0,[esi+0]      ; read 64 bits
    add edi, CACHE_LINE   
    movq mm1,[esi+8]
    add esi, CACHE_LINE   
#if CACHE_LINE == 64
    movq mm2,[esi-48]
    MOVNT([edi-64], mm0)  ; write 64 bits, bypassing the cache
    movq mm0,[esi-40]     ; note: movntq also prevents the CPU
    MOVNT([edi-56], mm1)  ; from READING the destination address
    movq mm1,[esi-32]     ; into the cache, only to be over-written
    MOVNT([edi-48], mm2)  ; so that also helps performance
    movq mm2,[esi-24]
    MOVNT([edi-40], mm0)
    movq mm0,[esi-16]
    MOVNT([edi-32], mm1)
    movq mm1,[esi-8]
    MOVNT([edi-24], mm2)
    MOVNT([edi-16], mm0)    
    MOVNT([edi-8], mm1)
#else // 32-bit cache-line         
    movq mm2,[esi-48]
    MOVNT([edi-64], mm0) 
    movq mm0,[esi-40]    
    MOVNT([edi-56], mm1) 
    movq mm1,[esi-32]    
    MOVNT([edi-48], mm2)     
    MOVNT([edi-40], mm0)    
    MOVNT([edi-32], mm1)     
#endif // cache_line
#endif // P4
    dec ecx
    jnz $memcpy_uc_1      ; last cache-line block?

    jmp $memcpy_ic_2      ; almost done


// For the largest size blocks, a special technique called Block Prefetch
// can be used to accelerate the read operations. Block Prefetch reads
// one address per cache line, for a series of cache lines, in a short loop.
// This is faster than using software prefetch. The technique is great for
// getting maximum read bandwidth, especially in DDR memory systems.
  $memcpy_bp_1:           ; large blocks, block prefetch copy
    cmp ecx, CACHEBLOCK   ; big enough to run another prefetch loop?
    jl $memcpy_64_test    ; no, back to regular uncached copy

    mov eax, CACHEBLOCK / 2           ; block prefetch loop, unrolled 2X
    add esi, CACHEBLOCK * CACHE_LINE  ; move to the top of the block

  align 16
  $memcpy_bp_2:
    mov edx, [esi - CACHE_LINE]     ; grab one address per cache line
    mov edx, [esi - CACHE_LINE * 2] ; grab one address per cache line
    sub esi, CACHE_LINE * 2         ; go reverse order
    dec eax                         ; count down the cache lines
    jnz $memcpy_bp_2                ; keep grabbing more lines into cache

    mov eax, CACHEBLOCK   ; now that it's in cache, do the copy
  align 16
  $memcpy_bp_3:
#if CACHE_LINE == 64
#if CPUF == CPU_P4
    movdqa xmm0, [esi]      ; read 128 bits
    movdqa xmm1, [esi+16]   
    movdqa xmm2, [esi+32]   
    movdqa xmm3, [esi+48]   
    movntdq [edi], xmm0     ; write 128 bits
    movntdq [edi+16], xmm1  
    movntdq [edi+16], xmm1  
    movntdq [edi+16], xmm1  
#else // no P4
    movq mm0, [esi ]      ; read 64 bits
    movq mm1, [esi+ 8]
    movq mm2, [esi+16]
    movq mm3, [esi+24]
    movq mm4, [esi+32]
    movq mm5, [esi+40]
    movq mm6, [esi+48]
    movq mm7, [esi+56]    
    MOVNT([edi ], mm0)    ; write 64 bits, bypassing cache
    MOVNT([edi+ 8], mm1)  ; note: movntq also prevents the CPU
    MOVNT([edi+16], mm2)  ; from READING the destination address
    MOVNT([edi+24], mm3)  ; into the cache, only to be over-written,
    MOVNT([edi+32], mm4)  ; so that also helps performance
    MOVNT([edi+40], mm5)
    MOVNT([edi+48], mm6)
    MOVNT([edi+56], mm7)
#endif // P4
#else  // 32-bit cache line
    movq mm0, [esi ]      
    movq mm1, [esi+ 8]
    movq mm2, [esi+16]
    movq mm3, [esi+24]    
    MOVNT([edi ], mm0)    
    MOVNT([edi+ 8], mm1)  
    MOVNT([edi+16], mm2)  
    MOVNT([edi+24], mm3)  
#endif // cache_line
    add esi, CACHE_LINE   ; update source pointer
    add edi, CACHE_LINE   ; update dest pointer

    dec eax               ; count down

    jnz $memcpy_bp_3      ; keep copying
    sub ecx, CACHEBLOCK   ; update the 64-byte block count
    jmp $memcpy_bp_1      ; keep processing blocks

    
// The smallest copy uses the X86 "movsd" instruction, in an optimized
// form which is an "unrolled loop". Then it handles the last few bytes.

  align 4
    movsd
    movsd ; perform last 1-15 dword copies
    movsd
    movsd
    movsd
    movsd
    movsd
    movsd
    movsd
    movsd ; perform last 1-7 dword copies
    movsd
    movsd
    movsd
    movsd
    movsd
    movsd

  $memcpy_last_few:   ; dword aligned from before movsd's
    mov ecx, ebx      ; has valid low 2 bits of the byte count
    and ecx, 11b      ; the last few cows must come home
    jz $memcpy_final  ; no more, let's leave

    rep movsb         ; the last 1, 2, or 3 bytes
  $memcpy_final:
    emms              ; clean up the MMX state
    FENCE             ; flush the write buffer
    mov eax, dst      ; ret value = destination pointer
  }
  return;
}
#endif











/*********************************************************************************
 block fill: fill a number of DWORDs (*not* chars) at DWORD aligned destination
 with DWORD initializer using cacheable stores
*********************************************************************************/

void CPULIB :: fMemset (void * dst, int val, UL n)
{
#if CPUF == CPU_P1
    memset(dst, val, n);
    return;
}

#else
__asm {
    mov edi, [dst]      ; pointer to dst
    mov ecx, [n]        ; number of bytes to copy  // changed for compatibility with std::memset
    movd mm0, [val]     ; initialization data
    punpckldq mm0, mm0  ; extend fill data to QWORD
    shr ecx, 2          ; convert to DWORD counting
    // possibility: support unaligned dst addresses, i.e. if val is 
    // really a char as in std::memset
    jb $filldone2_fc    ; yes, must be no DWORDs to fill, done
    test edi, 7         ; dst QWORD aligned?
    jz $dstqaligned2_fc ; yes

    movd [edi], mm0     ; store one DWORD to dst
    add edi, 4          ; dst++
    dec ecx             ; number of DWORDs to fill

  $dstqaligned2_fc:
#if CPUF == CPU_P4
    movq2dq xmm0, mm0     ; put into xmm0
    punpcklqdq xmm0, xmm0 ; and extend to DQWORD
    test edi, 15          ; dst DQWORD aligned?
    jz $dstdqaligned2_fc

    movq [edi], mm0       ; store one QWORD to dst
    add edi, 8            ; dst++
    dec ecx               ; number of DWORDS to fill
    dec ecx

  $dstdqaligned2_fc:
#endif
    mov ebx, ecx          ; number of DWORDs to fill
    cmp ecx, UNCACHED_COPY/4 ; big enough? 
    jae $memset_uc_1      ; if so use streaming (uncached) copy

#if CACHE_LINE == 64      // downside of unrolling loop: can't generalize # of iterations
    shr ecx, 4            ; number of cache lines to fill
    jz $fillqwords2_fc    ; no whole cache lines to fill, maybe QWORDs

  align 16                ; align loop for optimal performance
  $cloop2_fc:
    add edi, 64           ; dst++
#if CPUF == CPU_P4
    movdqa [edi-64], xmm0 ; store 1st DQWORD in cache line to dst
    movdqa [edi-48], xmm0 ; 
    movdqa [edi-32], xmm0 ; 
    movdqa [edi-16], xmm0 ; ...4th DQWORD in cache line
#else // no P4
    movq [edi-64], mm0    ; store 1st QWORD in cache line to dst
    movq [edi-56], mm0    ; store 2nd QWORD in cache line to dst
    movq [edi-48], mm0    ; etc.
    movq [edi-40], mm0    ; 
    movq [edi-32], mm0    ; 
    movq [edi-24], mm0    ; 
    movq [edi-16], mm0    ;       
    movq [edi -8], mm0    ; ...8th QWORD in cache line
#endif // P4
#elif CACHE_LINE == 32    // do unrolled loop for other CPUs
    shr ecx, 3            ; number of cache lines to fill
    jz $fillqwords2_fc    ; no whole cache lines to fill, maybe QWORDs

  align 16                ; align loop for optimal performance
  $cloop2_fc:
    add edi, 32           ; dst++    
    movq [edi-32], mm0    ; store 1st QWORD in cache line to dst
    movq [edi-24], mm0    ; 
    movq [edi-16], mm0    ;       
    movq [edi -8], mm0    ; ...4th QWORD in cache line
#endif // Cache_Line
    dec ecx               ; count--
    jnz $cloop2_fc        ; until no more cache lines to copy
    jmp $fillqwords2_fc   ; skip over streaming version
    

  $memset_uc_1:           ; same as last block, using movntq/movntdq when possible
#if CACHE_LINE == 64
    shr ecx, 4            ; 
    jz $fillqwords2_fc    ; 

  align 16                ; 
  $cloop2_fc_nt:
    add edi, 64           ; 
#if CPUF == CPU_P4
    movntdq [edi-64], xmm0 ;
    movntdq [edi-48], xmm0 ;
    movntdq [edi-32], xmm0 ;
    movntdq [edi-16], xmm0 ;
#else // no P4
    MOVNT([edi-64], mm0)  ; 
    MOVNT([edi-56], mm0)  ; 
    MOVNT([edi-48], mm0)  ; 
    MOVNT([edi-40], mm0)  ; 
    MOVNT([edi-32], mm0)  ; 
    MOVNT([edi-24], mm0)  ; 
    MOVNT([edi-16], mm0)  ;        
    MOVNT([edi -8], mm0)  ; 
#endif // P4
#elif CACHE_LINE == 32
    shr ecx, 3            ; 
    jz $fillqwords2_fc    ; 

  align 16                ; 
  $cloop2_fc_nt:
    add edi, 32           ; 
    MOVNT([edi-32], mm0)  ; 
    MOVNT([edi-24], mm0)  ; 
    MOVNT([edi-16], mm0)  ;  
    MOVNT([edi -8], mm0)  ; 
#endif // Cache_Line
    dec ecx               ; 
    jnz $cloop2_fc_nt     ; 

  $fillqwords2_fc:
    mov ecx, ebx        ; number of DWORDs to fill
    and ebx, 0xE        ; number of QWORDS left to fill * 2
    jz $filldword2_fc   ; no QWORDs left, maybe DWORD left

  align 16 ;align loop for optimal performance
  $qloop2_fc:
    MOVNT([edi], mm0)   ; store QWORD to dst
    add edi, 8          ; dst++
    sub ebx, 2          ; count--
    jnz $qloop2_fc      ; until no more QWORDs left to copy

  $filldword2_fc:
    test ecx, 1         ; DWORD left to fill?
    jz $filldone2_fc    ; nope, we're done

    movd [edi], mm0     ; store last DWORD to dst
  $filldone2_fc:  
  emms                  ; clear MMX state
  FENCE
  }
  return;
}
#endif


#undef CACHE_LINE       
#undef NUM_CACHE_LINES  
#undef PREF         
#undef MOVNT 
#undef FENCE           
#undef TINY_BLOCK_COPY
#undef IN_CACHE_COPY 
#undef UNCACHED_COPY
#undef BLOCK_PREFETCH_COPY 
#undef CACHEBLOCK 
