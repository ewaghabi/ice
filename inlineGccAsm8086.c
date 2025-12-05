// ----------------------------------------------------------------
// inlineGccAsm8086
// Funções com Assembly inline para compilador GNU C (gcc) - versão para 8086
#include "ice.h"

// protótipos
int BitMenosSignificativo(TBitBoard);  // obtém o bit menos significativo de um bitboard
int BitMenosSigPreench(TBitBoard);     // versão mais rápida quando se tem certeza que o bitboard não está vazio
int BitMaisSignificativo(TBitBoard);   // obtém o bit mais significativo de um bitboard
int BitMaisSigPreench(TBitBoard);      // versão mais rápida quando se tem certeza que o bitboard não está vazio
int IterativePopCount(TBitBoard);      // conta quantos bits ligados há em um bitboard
int PopCount(TBitBoard);               // versão mais rápida se o bitboard contém mais do que 5 bits ligados
int NumBits(TBitBoard bitboard);       // minha versão asm para iterativePopCount

// --------------------------------------------------------------------------
int NumBits(TBitBoard bitboard) { 
  int qtdBits = 0; 
  TBitBoard idxbit;

  asm("       movl  $0, %%ecx     \n\t"
      "lnb1:  bsfl  %%eax, %%ebx  \n\t" 
      "       jz    lnb2          \n\t" 
      "       inc   %%ecx         \n\t" 
      "       btrl  %%ebx, %%eax  \n\t" 
      "       jmp   lnb1          \n\t"
      "lnb2:  bsfl  %%edx, %%ebx  \n\t" 
      "       jz    enb           \n\t" 
      "       inc   %%ecx         \n\t" 
      "       btrl  %%ebx, %%edx  \n\t" 
      "       jmp   lnb2          \n\t"      
      "enb:   movl  %%ecx, %0     \n\t" 
      :"=r" (qtdBits)
      :"A"  (bitboard)
      : "ecx", "ebx", "cc"); 
  return qtdBits; 
} 

// -------------------------------------------------------------------------------
int IterativePopCount(TBitBoard b) { 
    int n; 
    for (n = 0; b != 0; n++, b &= (b - 1)); 
    return n; 
} 

// -------------------------------------------------------------------------------
int PopCount(const TBitBoard b) { 
    int n; 
    const TBitBoard a = b - ((b >> 1) & 0x5555555555555555ULL); 
    const TBitBoard c = (a & 0x3333333333333333ULL ) + ((a >> 2) & 0x3333333333333333ULL ); 
    n = ((int) c) + ((int) (c >> 32)); 
    n = (n & 0x0F0F0F0F) + ((n >> 4) & 0x0F0F0F0F); 
    n = (n & 0xFFFF) + (n >> 16); 
    n = (n & 0xFF) + (n >> 8); 
    return n; 
} 

// --------------------------------------------------------------------------
int BitMenosSignificativo(TBitBoard bitboard) { 
  int idxBit = -1; 

  asm("       bsfl  (%1), %0          \n\t" 
      "       jnz   ebsf              \n\t" 
      "       movl  $-0x01, %0        \n\t" 
      "       orl   $0,4(%1)          \n\t" 
      "       jz    ebsf              \n\t"       
      "       bsfl  4(%1), %0         \n\t" 
      "       addl  $0x20, %0         \n\t" 
      "ebsf:  nop                     \n\t" 
      :"=r" (idxBit):"r" (&bitboard)); 
  return idxBit; 
} 

// --------------------------------------------------------------------------
int BitMaisSignificativo(TBitBoard bitboard) { 
  int idxBit = -1; 

  asm("       bsrl  4(%1), %0     \n\t" 
      "       jnz   ebsr1         \n\t" 
      "       movl  $-0x01, %0    \n\t" 
      "       orl   $0,(%1)       \n\t" 
      "       jz    ebsr          \n\t" 
      "       bsrl  (%1), %0      \n\t" 
      "       jnz   ebsr          \n\t" 
      "ebsr1: addl  $0x20, %0     \n\t" 
      "ebsr:  nop                 \n\t" 
      :"=r" (idxBit):"r" (&bitboard)); 
  return idxBit; 
} 

// --------------------------------------------------------------------------
int BitMenosSigPreench(TBitBoard bitboard) { 
  int idxBit = -1; 

  asm("       bsfl  (%1), %0      \n\t" 
      "       jnz   efbsf         \n\t" 
      "       bsfl  4(%1), %0     \n\t" 
      "       addl  $0x20, %0     \n\t" 
      "efbsf: nop                 \n\t" 
      :"=r" (idxBit):"r" (&bitboard)); 
  return idxBit; 
} 

// --------------------------------------------------------------------------
int BitMaisSigPreench(TBitBoard bitboard) { 
  int idxBit = -1; 

  asm("        bsrl  4(%1), %0     \n\t" 
      "        jnz   efbsr1        \n\t" 
      "        bsrl  (%1), %0      \n\t" 
      "        jnz   efbsr         \n\t" 
      "efbsr1: addl  $0x20, %0     \n\t" 
      "efbsr:  nop                 \n\t" 
      :"=r" (idxBit):"r" (&bitboard)); 
  return idxBit; 
}
