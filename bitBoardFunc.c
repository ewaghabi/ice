//---------------------------------------------------
//- bitBoardFunc.c
// Funções/Utilitários para bitboards
#include "ice.h"

// globais
unsigned char onebits[0x10000]; 

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
int popCount(TBitBoard x) { 
   return __builtin_popcountll(x);
}

// BitMenosSignificativo - retorna o índice do primeiro bit ligado em um bitboard
// quando o bitboard está vazio, retorna -64
int BitMenosSignificativo(const TBitBoard b) { 
  return b ? __builtin_ctzll(b) : -64;
} 

// BitMaisSignificativo - retorna -64 quando vazio
int BitMaisSignificativo(const TBitBoard bitboard) { 
  return bitboard ? 63 - __builtin_clzll(bitboard) : -64;
}
