//---------------------------------------------------
//- bitBoardFunc.c
// Funções/Utilitários para bitboards
#include "ice.h"

// globais
unsigned char onebits[0x10000]; 
int           lsbRS[256] = { -120, 0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0, 
                                4, 0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0, 
                                5, 0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0, 
                                4, 0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0, 
                                6, 0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0, 
                                4, 0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0, 
                                5, 0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0, 
                                4, 0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0, 
                                7, 0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0, 
                                4, 0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0, 
                                5, 0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0, 
                                4, 0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0, 
                                6, 0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0, 
                                4, 0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0, 
                                5, 0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0, 
                                4, 0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0 
                            }; 
                  

// protótipos
void setOnebits();  // deve ser chamado durante a inicialização do engine
int  popCount(TBitBoard); 
int  BitMenosSignificativo(const TBitBoard); 
int  BitMaisSignificativo(const TBitBoard);

// ------------------------------------------------------------------
// setOnebits
// inicializa tabela onebits[] para a função popCount
// original de Leen Ammeraal, baseado em código de Volker
// obtido no Winboard Fórum http://wbforum.volker-pittlik.name
void setOnebits() // used during initialization 
{  onebits[0] = 0; 
   int i;
   for (i=1; i<0x10000; ++i) 
      onebits[i] = onebits[i >> 1] + (i & 1); 
} 

// ------------------------------------------------------------------
// popCount 
// original de Leen Ammeraal, baseado em código de Volker
// obtido com permissão do autor no Winboard Fórum http://wbforum.volker-pittlik.name
int popCount(TBitBoard x) { 
 
   unsigned int u1 = (unsigned int)x;
   unsigned int u2 = (unsigned int)(x>>32); 
   
   return    onebits[(unsigned short)u1] + onebits[u1>>16] 
           + onebits[(unsigned short)u2] + onebits[u2>>16]; 
}

// BitMenosSignificativo - retorna o índice do primeiro bit ligado em um bitboard
// quando o bitboard está vazio, retorna -64
// original de Reinhard Scharnagl
// obtido com permissão do autor no Winboard Fórum http://wbforum.volker-pittlik.name
// FIND LSB(b) = FIND MSB(b ^ (b - 1))
int BitMenosSignificativo(const TBitBoard b) { 
  unsigned buf; 
  int acc = 0; 

  if ((buf = (unsigned)b) == 0) { 
    buf = (unsigned)(b >> 32); 
    acc = 32; 
  } 
  if ((unsigned short)buf == 0) { 
    buf >>= 16; 
    acc += 16; 
  } 
  if ((unsigned char)buf == 0) { 
    buf >>= 8; 
    acc += 8; 
  } 
  return acc + lsbRS[buf & 0xff]; 
} 

// BitMaisSignificativo - não funciona com bitboards vazios
int BitMaisSignificativo(TBitBoard bitboard) { 
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

/* main para teste
main() {
  TBitBoard b[4] = { 0xFF00000000000000ULL, 0x0000FF0000000000ULL, 0x00000000FF000000ULL, 0x000000000000FF00ULL };
  int i;
  
  setOnebits();
  
  for (i=0;i<4;i++) {
    printf("%d - %0X\n",i,(long long)b[i]);
    printf("  BitMenosSignificativo: %d\n",BitMenosSignificativo(b[i]));
    printf("  BitMaisSignificativo.: %d\n",BitMaisSignificativo(b[i]));
    printf("  Numero de bits.......: %d\n",popCount(b[i]));
    printf(" -------------------------------------\n");
  }
}
*/
