// ----------------------------------------------------------------
// eval.c
// Funções de avaliação da posição
#include "ice.h"

// globais externas
extern    int       valorPecas[8];
extern    TBitBoard mskBitBoardUnitario[64];
extern    TLance    listaLancesJogados[200];
extern    int       indListaLancesJogados;


// globais locais

// protótipos locais
int Eval(TBoard*);
int ValorPosicao(TByte, int, int, int);

// ----------------------------------------------------------------
// Eval()
int Eval(TBoard* tab) {
  int i, j, bando, flagRoque, oldValorPosicao, valorPosicao[2] = {0, 0};
  int numBispos[2], numTorres[2], numDamas[2], numPeoes[2], numCavalos[2];
  int ocupacaoColunasPeoes[2][8]  = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
  int ocupacaoColunasTorres[2][8] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};  
  int ocupacaoColunasRei[2][8]    = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};  
 
  for (bando=(tab->vez)^1;;bando^=1) {
    numPeoes[bando] = 0;
    numCavalos[bando] = 0;
    numBispos[bando] = 0;
    numTorres[bando] = 0;
    numDamas[bando] = 0;
    
    for (i=0;i<64;i++) {
      if (tab->pecas[bando] & mskBitBoardUnitario[i]) {
        if (tab->peoes[bando] & mskBitBoardUnitario[i]) {
          ocupacaoColunasPeoes[bando][i&7]++;
          valorPosicao[bando] += valorPecas[PEAO] + ValorPosicao(PEAO, i, bando, tab->numLance);
          numPeoes[bando]++;
        }
        if (tab->cavalos[bando] & mskBitBoardUnitario[i]) {
          valorPosicao[bando] += valorPecas[CAVALO] + ValorPosicao(CAVALO, i, bando, tab->numLance);
          numCavalos[bando]++;
        }
        if (tab->bispos[bando] & mskBitBoardUnitario[i]) {
          valorPosicao[bando] += valorPecas[BISPO] + ValorPosicao(BISPO, i, bando, tab->numLance);
          numBispos[bando]++;
        }
        if (tab->torres[bando] & mskBitBoardUnitario[i]) {
          valorPosicao[bando] += valorPecas[TORRE] + ValorPosicao(TORRE, i, bando, tab->numLance);
          ocupacaoColunasTorres[bando][i&7]++;
          numTorres[bando]++;
        }
        if (tab->damas[bando] & mskBitBoardUnitario[i]) {
          valorPosicao[bando] += valorPecas[DAMA] + ValorPosicao(DAMA, i, bando, tab->numLance);
          numDamas[bando]++;
        }
        if (tab->rei[bando] & mskBitBoardUnitario[i]) {
          valorPosicao[bando] += valorPecas[REI] + ValorPosicao(REI, i, bando, tab->numLance);
          ocupacaoColunasRei[bando][i&7]++;
        }
      }
    }
    if (bando==tab->vez) break;
  }
  
  for (bando=(tab->vez)^1;;bando^=1) {
    if (numBispos[bando] > 1) valorPosicao[bando] +=20;                          // par de bispos  = +15 centesimos
    
    // bônus para final de rei e peões com peão a mais - para induzir à simplificação quando possível
    if (numPeoes[bando] > numPeoes[bando^1] && !numBispos[bando] && !numCavalos[bando] && !numTorres[bando] && !numDamas[bando])
      valorPosicao[bando] +=75; 
    // penaliza o oposto...
    if (numPeoes[bando] < numPeoes[bando^1] && !numBispos[bando] && !numCavalos[bando] && !numTorres[bando] && !numDamas[bando])
      valorPosicao[bando] -=75; 
    
    // analise de colunas
    for (i=0;i<8;i++) {
      // posição das torres
      if (ocupacaoColunasTorres[bando][i] && !ocupacaoColunasPeoes[bando][i]) {
        valorPosicao[bando] += 20;                                               // torres em colunas semi-abertas
        if (ocupacaoColunasTorres[bando][i] > 1) valorPosicao[bando] += 5;       // torres dobradas!
        if (!ocupacaoColunasPeoes[bando^1][i]) valorPosicao[bando] += 10;    // torres em colunas abertas
      }
      
      // posição do rei
      if (ocupacaoColunasRei[bando][i]) {
        if (!ocupacaoColunasPeoes[bando^1][i] &&     
           (numTorres[bando^1] > 1 || numDamas[bando^1]))                        // rei em coluna semi-aberta (para o outro bando)
          valorPosicao[bando] -= 20;                                             // quando oponente ainda tem peças pesadas
        if (ocupacaoColunasPeoes[bando][i])         valorPosicao[bando] += 5;    // escudo de peões
        if (i==0 && ocupacaoColunasPeoes[bando][1]) valorPosicao[bando] += 5;    // proteção do rei
        else if (i==7 && ocupacaoColunasPeoes[bando][6]) valorPosicao[bando] += 5;    // proteção do rei
        else {
          if (ocupacaoColunasPeoes[bando][i-1]) valorPosicao[bando] += 5;        // proteção do rei
          if (ocupacaoColunasPeoes[bando][i+1]) valorPosicao[bando] += 5;        // proteção do rei
        }
      }
      
      // posição peões      
      if (ocupacaoColunasPeoes[bando][i])  {        
        if (ocupacaoColunasPeoes[bando][i] > 1) 
          valorPosicao[bando] -= ((i > 1 && i < 6) ? 10 : 15);                       // peoes dobrados = -10 ou 15 centesimos cada dependendo da coluna
        oldValorPosicao = valorPosicao[bando];                                       // guarda valorPosicao para verificar peoes isolados
        if (i==0 && !ocupacaoColunasPeoes[bando][1]) valorPosicao[bando] -= 15;      // peoes isolados na coluna a = -15 centesimos
        else if (i==7 && !ocupacaoColunasPeoes[bando][6]) valorPosicao[bando] -= 15; // peoes isolados na coluna h = -15 centesimos      
        else if (!ocupacaoColunasPeoes[bando][i-1] && !ocupacaoColunasPeoes[bando][i+1])     
          valorPosicao[bando] -= 15;                                                 // outros peoes isolados = -15 centesimos cada
        if (oldValorPosicao != valorPosicao[bando] && !ocupacaoColunasPeoes[bando^1][i])   
          valorPosicao[bando] -= 30;                                                 // peoes isolados em colunas abertas
        // rudimentos de peões passados          
        if (!ocupacaoColunasPeoes[bando^1][i]) {
          if (i==0 && !ocupacaoColunasPeoes[bando^1][0]) valorPosicao[bando] += 15;  // peao passado na coluna a 
          else if (i==7 && !ocupacaoColunasPeoes[bando^1][6]) valorPosicao[bando] += 15; // peao passado na coluna h
          else if (!ocupacaoColunasPeoes[bando^1][i-1] && !ocupacaoColunasPeoes[bando^1][i+1])
             valorPosicao[bando] += 20;                                              // outros peoes isolados = -15 centesimos cada
        }
      }
    }
    // bonus de roque
    if (tab->mskRoque[bando] && (numTorres[bando^1] > 1 || numDamas[bando^1])) 
      valorPosicao[bando] += 50;                                               // houve roque e ainda existem peças pesadas
      
    if (bando==tab->vez) break;
  }
  
  // analise de abertura
  flagRoque = 0;
  if (tab->numLance < 11) {
    for (i=0; i<indListaLancesJogados;i++) {
      if (listaLancesJogados[i].especial & (MSK_ROQUEG | MSK_ROQUEP)) {
        flagRoque = 1;                                                
        valorPosicao[i&1] += 15;                                      // bônus por rocar cedo        
      }
      if (listaLancesJogados[i].casaOrigem < 48 && (i&1) == BRANCAS)
        valorPosicao[BRANCAS] -= 15;                                  // penalidade por mover a mesma peça duas vezes na abertura
      if (listaLancesJogados[i].casaOrigem > 15 && (i&1) == NEGRAS)
        valorPosicao[NEGRAS] -= 15;
      if (listaLancesJogados[i].casaOrigem == ((i&1) ? 3 : 59))
        valorPosicao[i&1] -= 15;                                      // penalidade por mover a dama muito cedo
      if (listaLancesJogados[i].peca == REI && !flagRoque)            
        valorPosicao[i&1] -= 15;                                      // penalidade por mover o rei (antes do roque)
      if (listaLancesJogados[i].peca == PEAO && 
          listaLancesJogados[i].casaOrigem == ((i&1) ? 13 : 53))   
        valorPosicao[i&1] -= 15;                                      // penalidade por mover o peão de f antes de rocar 
      if (listaLancesJogados[i].casaOrigem == ((i&1) ? 12 : 52))
        valorPosicao[i&1] += 15;                                      // bônus por mover o peão central e
      if (listaLancesJogados[i].casaOrigem == ((i&1) ? 11 : 51))
        valorPosicao[i&1] += 15;                                      // bônus por mover o peão central d
    }
  }

  // retorna diferença
  return valorPosicao[tab->vez] - valorPosicao[(tab->vez)^1];
}

// --------------------------------------------------
// ValorPosicao())
int ValorPosicao(TByte tipoPeca, int casa, int bando, int numLance) {

  int  tabPeao[64] = {    0,    0,    0,    0,    0,    0,    0,    0,
  		                   30,   30,   35,   40,   40,   35,   30,   30,
  		                   10,   10,   12,   20,   20,   12,   10,   10,
			                    5,    5,    6,   18,   18,    6,    5,    5,
			                    3,    3,    4,   17,   17,    1,    1,    2,
				                  3,    5,    0,   10,   10,    0,    5,    3,
				                  2,    5,    5,    0,    0,    5,    5,    2};

  int  tabCavalo[64] = {  0,    2,    6,    7,    7,    6,    2,    0,
			                    9,   10,   15,   17,    17,  15,   10,    9,
				                  8,   15,   20,   30,    30,  20,   15,    8,
				                  7,   11,   14,   20,    20,  14,   11,    7,
				                  7,   11,   14,   20,    20,  14,   11,    7,
				                  6,   10,   14,   14,    14,  14,   10,    6,
				                 -5,    7,   10,   11,    11,  10,    7,   -5,
				                -10,    0,   10,    7,    7,   10,    0,  -10};

  int  tabBispo[64] =  {  5,    3,    3,    3,     3,   3,    3,    5,
			                    3,   10,    9,    9,     9,   9,   10,    3,
				                  3,    9,   15,   14,    14,  15,    9,    3,
				                  3,    9,   14,   20,    20,  14,    9,    3,
				                  3,    9,   14,   20,    20,  14,    9,    3,
				                  3,    9,   15,   14,    14,  15,    9,    3,
				                  3,   10,    9,    9,     9,   9,   10,    3,
				                  5,    3,    0,    3,     3,   0,    3,    5};
				                
  int  tabTorre[64] =  { 25,   25,   25,   25,    25,  25,   25,   25,
			                   50,   50,   50,   50,    50,  50,   50,   50,
				                 15,   10,   10,   10,    10,  10,   10,   15,
				                 15,   10,   05,   05,    05,  05,   10,   15,
				                 15,   10,   05,   05,    05,  05,   10,   15,
				                 15,   10,   05,   05,    05,  05,   10,   15,
				                 05,   10,   10,   10,    10,  10,   10,   05,
				                 05,   05,   10,   20,    20,  10,   05,   05};

  int  tabRei[64] =    { -5,   -3,   -3,   -3,    -3,  -3,   -3,   -5,
			                   -3,  -10,   -9,   -9,    -9,  -9,  -10,   -3,
				                 -5,   -9,  -15,  -14,   -14, -15,   -9,   -5,
				                 -1,   -5,  -14,  -20,   -20, -14,   -5,   -1,
				                  2,   -1,   -5,  -20,   -20,  -5,   -1,    2,
				                  3,    2,   -1,  -10,   -10,  -1,    2,    3,
				                  15,  12,    2,    4,     4,   2,   12,   15,
				                  15,  15,   12,    8,     8,  12,   15,   15};

  int  tabReiFinal[64]={  5,    3,    3,    3,     3,   3,    3,    5,
			                    3,    9,    9,    9,     9,   9,    9,    3,
				                  3,    9,   15,   14,    14,  15,    9,    3,
				                  3,    9,   14,   20,    20,  14,    9,    3,
				                  3,    9,   14,   20,    20,  14,    9,    3,
				                  3,    9,   15,   14,    14,  15,    9,    3,
				                  3,    9,    9,    9,     9,   9,    9,    3,
				                  5,    3,    3,    3,     3,   3,    3,    5};
				                  
  switch (tipoPeca) {
    case PEAO:   return tabPeao[bando ? (63 - casa) : casa]; break;
    case CAVALO: return tabCavalo[bando ? (63 - casa) : casa]; break;
    case BISPO:  return tabBispo[bando ? (63 - casa) : casa]; break;
    case TORRE:  return tabTorre[bando ? (63 - casa) : casa]; break;
    case DAMA:   return tabBispo[bando ? (63 - casa) : casa] + tabTorre[bando ? (63 - casa) : casa]; break;
    default:     if (numLance < 35)
                   return tabRei[bando ? (63 - casa) : casa]; 
                 else
                   return tabReiFinal[bando ? (63 - casa) : casa]; 
                 break;
  }
}

    

				                  


              
                  


