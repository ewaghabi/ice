#include "ice.h"
#include <stdio.h>

// protótipos externos
extern int Eval(TBoard*);

// protótipos locais
void MostraTabuleiro(TBoard*);
void MostraBitBoard(TBitBoard);

// ---------------------------------------------------------------------
// MostaTabuleiro()
void MostraTabuleiro(TBoard* tab) {
   int i, qtdPecas[2] = {0 , 0};
   TBitBoard BBTemp = 0x01;
   
   printf("   +0|+1|+2|+3|+4|+5|+6|+7|\n");     
   printf("---------------------------");        
    
   for (i=0; i<=63; i++) {
      if (!(i%8)) printf("\n%2d| ",i);
      
      if (tab->peoes[BRANCAS] & (BBTemp << i)) {
        qtdPecas[BRANCAS]++;
        printf("P| ");
      } else if (tab->bispos[BRANCAS] & (BBTemp << i)) {
        qtdPecas[BRANCAS]++;
        printf("B| ");
      } else if (tab->cavalos[BRANCAS] & (BBTemp << i)) {
        qtdPecas[BRANCAS]++;
        printf("N| ");        
      } else if (tab->torres[BRANCAS] & (BBTemp << i)) {
        qtdPecas[BRANCAS]++;
        printf("R| ");        
      } else if (tab->damas[BRANCAS] & (BBTemp << i)) {
        qtdPecas[BRANCAS]++;
        printf("Q| "); 
      }else if (tab->rei[BRANCAS] & (BBTemp << i)) {
        qtdPecas[BRANCAS]++;
        printf("K| ");    
      } else if (tab->peoes[NEGRAS] & (BBTemp << i)) {
        qtdPecas[NEGRAS]++;
        printf("p| ");
      } else if (tab->bispos[NEGRAS] & (BBTemp << i)) {
        qtdPecas[NEGRAS]++;
        printf("b| ");
      } else if (tab->cavalos[NEGRAS] & (BBTemp << i)) {
        qtdPecas[NEGRAS]++;
        printf("n| ");        
      } else if (tab->torres[NEGRAS] & (BBTemp << i)) {
        qtdPecas[NEGRAS]++;
        printf("r| ");        
      } else if (tab->damas[NEGRAS] & (BBTemp << i)) {
        qtdPecas[NEGRAS]++;
        printf("q| "); 
      }else if (tab->rei[NEGRAS] & (BBTemp << i)) {
        qtdPecas[NEGRAS]++;
        printf("k| ");                     
      } else 
        printf("-| ");

   }
   printf("\n---------------------------\n");  
   printf("Lado a jogar: ");
   if (tab->vez) printf("Negras\n"); else printf("Brancas\n");
   
   // imprime a avaliação da posição atual
//   printf("Eval: %02.02f\n",((float)Eval(tab))/100);  
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
