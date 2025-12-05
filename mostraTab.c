#include "ice.h"
#include <stdio.h>

// prototipos externos
extern int Eval(TBoard*);

// prototipos locais
void MostraTabuleiro(TBoard*);
void MostraBitBoard(TBitBoard);

// ---------------------------------------------------------------------
// MostaTabuleiro()
void MostraTabuleiro(TBoard* tab) {
   // imprime um tabuleiro 8x8 em ASCII com coordenadas
   int rank, file;
   TBitBoard mask;
   char peca;

   printf("   +---+---+---+---+---+---+---+---+\n");
   for (rank = 0; rank < 8; rank++) {           // rank 0 = 8a fileira (a8)
      printf(" %d |", 8 - rank);
      for (file = 0; file < 8; file++) {
         int idx = rank * 8 + file;
         mask = (1ULL << idx);

         if (tab->peoes[BRANCAS] & mask)      peca = 'P';
         else if (tab->cavalos[BRANCAS] & mask) peca = 'N';
         else if (tab->bispos[BRANCAS] & mask)  peca = 'B';
         else if (tab->torres[BRANCAS] & mask)  peca = 'R';
         else if (tab->damas[BRANCAS] & mask)   peca = 'Q';
         else if (tab->rei[BRANCAS] & mask)     peca = 'K';
         else if (tab->peoes[NEGRAS] & mask)    peca = 'p';
         else if (tab->cavalos[NEGRAS] & mask)  peca = 'n';
         else if (tab->bispos[NEGRAS] & mask)   peca = 'b';
         else if (tab->torres[NEGRAS] & mask)   peca = 'r';
         else if (tab->damas[NEGRAS] & mask)    peca = 'q';
         else if (tab->rei[NEGRAS] & mask)      peca = 'k';
         else                                   peca = ' ';

         printf(" %c |", peca);
      }
      printf("\n   +---+---+---+---+---+---+---+---+\n");
   }
   printf("     a   b   c   d   e   f   g   h\n");
   printf("Lado a jogar: %s\n", tab->vez ? "Negras" : "Brancas");
}

// ----------------------------------------------------------
// MostraBitBoard
void MostraBitBoard(TBitBoard BitBoard) {
   int i, qtdPecas = 0;
   TBitBoard BBTemp = 0x01;
   
   printf("   +0|+1|+2|+3|+4|+5|+6|+7|\n");     
   printf("---------------------------");        
    
   for (i=0; i<=63; i++) {
      if (!(i%8)) printf("\n%2d| ",i);
      
      if (BitBoard & (BBTemp << i)) {
        qtdPecas++;
        printf("X| ");
      } else 
        printf("-| ");
      
   }
   printf("\n---------------------------\n");  
   printf("Total pecas: %d\n",qtdPecas);         
   
}
