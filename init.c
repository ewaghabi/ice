// ----------------------------------------------------------------
// init.c
// Funções de inicialização
#include "ice.h"

// protótipos
void IniciaTabuleiro(char*, TBoard*, TByte, TByte, TByte);
void AtualizaBitBoardPecas(TBoard*);
void IniciaMaskBitBoardUnitario(TBitBoard*);
void IniciaListaLances(int*, int);
extern void MostraTabuleiro(TBoard*);

// global externa
extern int indListaLancesJogados;
extern int indListaLances[40];

// ----------------------------------------------------------------
// IniciaTabuleiro()
// Inicializa uma estrutura de tabuleiro com uma posição a partir de uma string FEN
// String FEN exemplo para a posição inicial:
// "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR"
// ****************************************************************
void IniciaTabuleiro(char* posicao, TBoard* tab, TByte vez, TByte roqueB, TByte roqueN ) {

	int casa       = 0;
	int i          = 0;
	TBitBoard peca = 0x01;

  tab->peoes[NEGRAS]   = tab->peoes[BRANCAS]   = 0x00;
  tab->bispos[NEGRAS]  = tab->bispos[BRANCAS]  = 0x00;
  tab->cavalos[NEGRAS] = tab->cavalos[BRANCAS] = 0x00;
  tab->torres[NEGRAS]  = tab->torres[BRANCAS]  = 0x00;
  tab->damas[NEGRAS]   = tab->damas[BRANCAS]   = 0x00;
  tab->rei[NEGRAS]     = tab->rei[BRANCAS]     = 0x00;          

	while(*posicao) {
	  switch (*posicao) {
		  case 112 /*"p"*/: tab->peoes[NEGRAS]    |= (peca << casa); break;
		  case 98  /*"b"*/: tab->bispos[NEGRAS]   |= (peca << casa); break;
		  case 110 /*"n"*/: tab->cavalos[NEGRAS]  |= (peca << casa); break;
		  case 114 /*"r"*/: tab->torres[NEGRAS]   |= (peca << casa); break;
		  case 113 /*"q"*/: tab->damas[NEGRAS]    |= (peca << casa); break;
		  case 107 /*"k"*/: tab->rei[NEGRAS]      |= (peca << casa); break;
		  case 80  /*"P"*/: tab->peoes[BRANCAS]   |= (peca << casa); break;
		  case 66  /*"B"*/: tab->bispos[BRANCAS]  |= (peca << casa); break;
		  case 78  /*"N"*/: tab->cavalos[BRANCAS] |= (peca << casa); break;
		  case 82  /*"R"*/: tab->torres[BRANCAS]  |= (peca << casa); break;
		  case 81  /*"Q"*/: tab->damas[BRANCAS]   |= (peca << casa); break;
		  case 75  /*"K"*/: tab->rei[BRANCAS]     |= (peca << casa); break;
		  case 47  /*"/"*/: casa--; break;
		  default : casa += *posicao - 49;
	  }
	  posicao++;
	  casa++;
	}

  AtualizaBitBoardPecas(tab);

	tab->vez      = vez;
	tab->numLance = 1;
	tab->mskRoque[BRANCAS] = 0; //= roqueB;
	tab->mskRoque[NEGRAS]  = 0; //= roqueN;

	// reinicializa lista de lances gerados e jogados
	IniciaListaLances(&indListaLances[0],0);  
	indListaLancesJogados = 0;

}

// ----------------------------------------------------------------
// AtualizaBitBoardPecas()
void AtualizaBitBoardPecas(TBoard *tab) {
  tab->pecas[BRANCAS] = tab->peoes[BRANCAS] | tab->bispos[BRANCAS] | tab->cavalos[BRANCAS] | tab->torres[BRANCAS] | tab->damas[BRANCAS] | tab->rei[BRANCAS];
  tab->pecas[NEGRAS] = tab->peoes[NEGRAS] | tab->bispos[NEGRAS] | tab->cavalos[NEGRAS] | tab->torres[NEGRAS] | tab->damas[NEGRAS] | tab->rei[NEGRAS];  
}


// ----------------------------------------------------------------
// IniciaMaskBitBoardUnitario()
void IniciaMaskBitBoardUnitario(TBitBoard *bboard) {
  TBitBoard peca = 0x01;
  int       i;
  
  for (i=0;i<64;i++) 
    bboard[i] = peca << i;
  
}

// ----------------------------------------------------------------
// IniciaListaLances()
void IniciaListaLances(int *indListaLances, int indInicio) {
  int i;
  
  for (i=indInicio;i<40;i++)
    indListaLances[i]=0;
}
