// ----------------------------------------------------------------
// inlineGccAsm8086.c (legacy)
// Versão original com asm 8086 desabilitada por padrão.
// Mantido apenas como referência histórica; as versões portáveis
// estão em bitBoardFunc.c usando built-ins do compilador.

#if 0
#include "ice.h"

int BitMenosSignificativo(TBitBoard);  // obtém o bit menos significativo de um bitboard
int BitMenosSigPreench(TBitBoard);     // versão mais rápida quando se tem certeza que o bitboard não está vazio
int BitMaisSignificativo(TBitBoard);   // obtém o bit mais significativo de um bitboard
int BitMaisSigPreench(TBitBoard);      // versão mais rápida quando se tem certeza que o bitboard não está vazio
int IterativePopCount(TBitBoard);      // conta quantos bits ligados há em um bitboard
int PopCount(TBitBoard);               // versão mais rápida se o bitboard contém mais do que 5 bits ligados
int NumBits(TBitBoard bitboard);       // minha versão asm para iterativePopCount

// Código original com asm inline (desabilitado)
#endif
