// ------------------------------------------------------------------------------------
// make.c
// --
// funções make() e unmake()
#include "ice.h"
#include <stdio.h>

// globais externas
extern TBitBoard        mskBitBoardUnitario[64];
extern TLance           listaLancesJogados[200];
extern int              indListaLancesJogados;
extern int              Debug;
extern long             qtdNos;
extern char             simboloPecas[];

// funções externas
extern void AtualizaBitBoardPecas(TBoard*);
extern void MostraBitBoard(TBitBoard);
extern void MostraTabuleiro(TBoard*);
extern void ImprimeLance(TLance*, int, TByte, int);
// protótipos locais
void Make(TLance*, TBoard*);
void UnMake(TLance*, TBoard*);

void Make(TLance* lance, TBoard* tab) {
  
  TByte pecaDest;
  int   i, casaEnPassant;
 
  // removendo peça da casa de origem
  switch (lance->peca) {
    case PEAO:   tab->peoes[tab->vez]   ^= mskBitBoardUnitario[lance->casaOrigem]; break;   
    case CAVALO: tab->cavalos[tab->vez] ^= mskBitBoardUnitario[lance->casaOrigem]; break;   
    case BISPO:  tab->bispos[tab->vez]  ^= mskBitBoardUnitario[lance->casaOrigem]; break;   
    case TORRE:  tab->torres[tab->vez]  ^= mskBitBoardUnitario[lance->casaOrigem]; break;   
    case DAMA:   tab->damas[tab->vez]   ^= mskBitBoardUnitario[lance->casaOrigem]; break;   
    default    : tab->rei[tab->vez]     ^= mskBitBoardUnitario[lance->casaOrigem]; break;   
  }
  // colocando peça na casa de destino
  // (se for uma promoção, o peão deve ser substituido por pecaPromovida)
  switch (lance->pecaPromovida ? lance->pecaPromovida : lance->peca) {
    case PEAO:   tab->peoes[tab->vez]   |= mskBitBoardUnitario[lance->casaDestino]; break;   
    case CAVALO: tab->cavalos[tab->vez] |= mskBitBoardUnitario[lance->casaDestino]; break;   
    case BISPO:  tab->bispos[tab->vez]  |= mskBitBoardUnitario[lance->casaDestino]; break;   
    case TORRE:  tab->torres[tab->vez]  |= mskBitBoardUnitario[lance->casaDestino]; break;   
    case DAMA:   tab->damas[tab->vez]   |= mskBitBoardUnitario[lance->casaDestino]; break;   
    default    : tab->rei[tab->vez]     |= mskBitBoardUnitario[lance->casaDestino]; break;   
  }


  // trata situações do roque
  if (lance->peca == REI) {
    if (lance->casaDestino == lance->casaOrigem + 2) {      // move a torre do roque pequeno
      if (tab->torres[tab->vez] & mskBitBoardUnitario[(lance->casaDestino) + 1]) {
        tab->torres[tab->vez]   ^= mskBitBoardUnitario[(lance->casaDestino) + 1];
        tab->torres[tab->vez]   |= mskBitBoardUnitario[(lance->casaDestino) - 1];
        tab->mskRoque[tab->vez] |= MSK_ROQUEP;
      }
    } else if (lance->casaDestino == lance->casaOrigem - 2) { // move a torre do roque grande
      if (tab->torres[tab->vez] & mskBitBoardUnitario[lance->casaDestino - 2]) {
        tab->torres[tab->vez]   ^= mskBitBoardUnitario[lance->casaDestino - 2]; 
        tab->torres[tab->vez]   |= mskBitBoardUnitario[lance->casaDestino + 1];
        tab->mskRoque[tab->vez] |= MSK_ROQUEG;
      }
    } 
  }

  // muda a vez 
  tab->vez ^= 1;
  
  // se for uma captura, remove a peça do adversário
  switch (lance->pecaCapturada) {
    case PEAO:   if (lance->especial & MSK_ENPASSANT) 
                   casaEnPassant = lance->casaDestino + (tab->vez ? 8 : -8);
                 else 
                   casaEnPassant = lance->casaDestino;
                 tab->peoes[tab->vez]   ^= mskBitBoardUnitario[casaEnPassant];      break;   
    case CAVALO: tab->cavalos[tab->vez] ^= mskBitBoardUnitario[lance->casaDestino]; break;   
    case BISPO:  tab->bispos[tab->vez]  ^= mskBitBoardUnitario[lance->casaDestino]; break;   
    case TORRE:  tab->torres[tab->vez]  ^= mskBitBoardUnitario[lance->casaDestino]; break;   
    case DAMA:   tab->damas[tab->vez]   ^= mskBitBoardUnitario[lance->casaDestino]; break;   
    case REI:    tab->rei[tab->vez]     ^= mskBitBoardUnitario[lance->casaDestino]; break;   
    default:     break;
  }
  
  // incrementa lance se agora são as brancas a jogar
  if (tab->vez == BRANCAS)
    tab->numLance++;
  
  // inclui o lance na lista de lances jogados
	listaLancesJogados[indListaLancesJogados].peca          = lance->peca;
	listaLancesJogados[indListaLancesJogados].casaOrigem    = lance->casaOrigem;
	listaLancesJogados[indListaLancesJogados].casaDestino   = lance->casaDestino;
	listaLancesJogados[indListaLancesJogados].especial      = lance->especial;
	listaLancesJogados[indListaLancesJogados].pecaPromovida = lance->pecaPromovida;
	listaLancesJogados[indListaLancesJogados].pecaCapturada = lance->pecaCapturada;
	listaLancesJogados[indListaLancesJogados].valorLance    = lance->valorLance;
	indListaLancesJogados++;
  
  // Atualiza o bitboard de peças
  AtualizaBitBoardPecas(tab);        
  
}

void UnMake(TLance* lance, TBoard* tab) {
  
  TByte pecaDest;
  int   i, casaEnPassant;

  // se foi uma captura, recoloca a peca do adversário
  switch (lance->pecaCapturada) {
    case PEAO:   if (lance->especial & MSK_ENPASSANT) 
                   casaEnPassant = lance->casaDestino + (tab->vez ? 8 : -8);
                 else 
                   casaEnPassant = lance->casaDestino;
                 tab->peoes[tab->vez]   |= mskBitBoardUnitario[casaEnPassant];      break;      
    case CAVALO: tab->cavalos[tab->vez] |= mskBitBoardUnitario[lance->casaDestino]; break;
    case BISPO:  tab->bispos[tab->vez]  |= mskBitBoardUnitario[lance->casaDestino]; break;
    case TORRE:  tab->torres[tab->vez]  |= mskBitBoardUnitario[lance->casaDestino]; break;
    case DAMA:   tab->damas[tab->vez]   |= mskBitBoardUnitario[lance->casaDestino]; break;
    case REI:    tab->rei[tab->vez]     |= mskBitBoardUnitario[lance->casaDestino]; break;    
    default:     break;
  }

  // volta a vez 
  tab->vez ^= 1;
  
  // removendo peça da casa de destino
  // (se foi uma promoção, a peça promovida eh que deve ser retirada)
  switch (lance->pecaPromovida ? lance->pecaPromovida : lance->peca) {
    case PEAO:   tab->peoes[tab->vez]   ^= mskBitBoardUnitario[lance->casaDestino]; break;
    case CAVALO: tab->cavalos[tab->vez] ^= mskBitBoardUnitario[lance->casaDestino]; break;
    case BISPO:  tab->bispos[tab->vez]  ^= mskBitBoardUnitario[lance->casaDestino]; break;
    case TORRE:  tab->torres[tab->vez]  ^= mskBitBoardUnitario[lance->casaDestino]; break;
    case DAMA:   tab->damas[tab->vez]   ^= mskBitBoardUnitario[lance->casaDestino]; break;
    default    : tab->rei[tab->vez]     ^= mskBitBoardUnitario[lance->casaDestino]; break;
  }
  
  // colocando peça na casa de origem
  switch (lance->peca) {
    case PEAO:   tab->peoes[tab->vez]   |= mskBitBoardUnitario[lance->casaOrigem]; break; 
    case CAVALO: tab->cavalos[tab->vez] |= mskBitBoardUnitario[lance->casaOrigem]; break; 
    case BISPO:  tab->bispos[tab->vez]  |= mskBitBoardUnitario[lance->casaOrigem]; break; 
    case TORRE:  tab->torres[tab->vez]  |= mskBitBoardUnitario[lance->casaOrigem]; break; 
    case DAMA:   tab->damas[tab->vez]   |= mskBitBoardUnitario[lance->casaOrigem]; break; 
    default    : tab->rei[tab->vez]     |= mskBitBoardUnitario[lance->casaOrigem]; break; 
  }
  
  // trata situações do roque
  if (lance->peca == REI) {
    if (lance->casaDestino == lance->casaOrigem + 2) {      // move a torre do roque pequeno
      tab->torres[tab->vez]  ^= mskBitBoardUnitario[(lance->casaDestino) - 1];
      tab->torres[tab->vez]  |= mskBitBoardUnitario[(lance->casaDestino) + 1];
      tab->mskRoque[tab->vez] = 0;      
    } else if (lance->casaDestino == lance->casaOrigem - 2) { // move a torre do roque grande
      tab->torres[tab->vez]  ^= mskBitBoardUnitario[(lance->casaDestino) + 1];
      tab->torres[tab->vez]  |= mskBitBoardUnitario[(lance->casaDestino) - 2];
      tab->mskRoque[tab->vez] = 0;
    }
  }
  
  // decrementa lance se agora são as negras a jogar
  if (tab->vez == NEGRAS)
    tab->numLance--;

  // retira o lance na lista de lances jogados
	indListaLancesJogados--;
	
  // Atualiza o bitboard de peças
  AtualizaBitBoardPecas(tab);  

}
