// ----------------------------------------------------------------
// geraLances.c
// Funções de geração de lances
#include "ice.h"
#include <stdlib.h>
#include <stdio.h>

// Variáveis globais
// locais
TBitBoard mskBitBoardLancesPeao[2][64];
TBitBoard mskBitBoardAtaquesPeao[2][64];
TBitBoard mskBitBoardLancesCavalo[64];
TBitBoard mskBitBoardLancesRei[64];
TBitBoard mskBitBoardAcima[64];
TBitBoard mskBitBoardAbaixo[64];
TBitBoard mskBitBoardDesliz[8][64];
TBitBoard mskBitBoardBordas[8];
TBitBoard mskBitBoardInitPeao[2];

// externas
extern    TBoard    tabPrincipal;
extern    TLance    listaLances[2000];
extern    int       indListaLances[40];
extern    TLance    listaLancesJogados[200];
extern    int       indListaLancesJogados;
extern    int       valorPecas[8];
extern    TBitBoard mskBitBoardUnitario[64];
extern    TLance    killerMoves[MAX_DEPTH][2];


// protótipos locais
void        GeraLance(int, TByte, TByte, TByte, TByte, TByte, TByte, int);
int         GeraListaLances(int, int, int, int);
int         IdentificaLances(TBitBoard, int, TByte, TByte, int, int, int);
int         ObtemMelhorLance(int, TLance*);
void        InicializaMaskLances();
int         VerificaRoque(int);
int         VerificaXeque(int);

// protótipos externos
extern void Fim(int, char*);
extern void IniciaListaLances(int*, int);
extern int  historyHeur[2][7][64];
extern int  BitMenosSignificativo(const TBitBoard);
extern int  BitMaisSignificativo(const TBitBoard);
extern int  ComparaLance(TLance*, TLance*);
// debug
extern void MostraBitBoard(TBitBoard);

// --------------------------------------------------------------------------
// GeraListaLances()
// Os parâmetros "tipoGeracao", "valorQuiesc" e "casaUltCaptura" norteiam o tipo de lances gerados:
// se (tipoGeracao | MSK_GERA_TODOS), todos os lances são gerados
// se (tipoGeracao | MSK_GERA_CAPTURAS), apenas as capturas são geradas
// se (tipoGeracao | MSK_QUIESCENCE), apenas as capturas com pecaCapturada valendo mais que "valorQuiesc", e
//                                    promoções para peças com valor maior de "valorQuiesc", ou ainda
//                                    capturas realizadas na "casaUltCaptura" (recapturas) são geradas.
int GeraListaLances(int iindListaLances, int tipoGeracao, int valorQuiesc, int casaUltCaptura) {

  int       casaOrigem = 0;
  int       primBit = 0, permissaoRoque = 0, casaDestino = 0;
  int       vezOponente = (tabPrincipal.vez)^1;
  TBitBoard bbPeca, bbLance, bbCapturas;
  TBitBoard pecasVez      = tabPrincipal.pecas[tabPrincipal.vez];
  TBitBoard pecasOponente = tabPrincipal.pecas[vezOponente];
  TBitBoard todasPecas    = pecasVez | pecasOponente;
  int      dirFrente      = tabPrincipal.vez ? 8 : -8;
  int      dirDuplo       = dirFrente << 1;
  

  // Inicializa lista de lances a partir do indice informado
  IniciaListaLances(&indListaLances[0], iindListaLances + 1);
  // Lances de peao
  if (tabPrincipal.peoes[tabPrincipal.vez]) {
    bbPeca = tabPrincipal.peoes[tabPrincipal.vez];
    while (bbPeca) {
      casaOrigem = __builtin_ctzll(bbPeca);
      TBitBoard maskOrig = mskBitBoardUnitario[casaOrigem];
      // capturas
      bbLance = mskBitBoardAtaquesPeao[tabPrincipal.vez][casaOrigem] & pecasOponente;
      if (bbLance && IdentificaLances(bbLance, iindListaLances, PEAO, casaOrigem, tipoGeracao, valorQuiesc, casaUltCaptura))
        return 1; // avisa captura de rei

      // demais lances (apenas para geração completa)
      if (tipoGeracao & MSK_GERA_TODOS) {
        bbLance = mskBitBoardLancesPeao[tabPrincipal.vez][casaOrigem] & ~todasPecas;
        if (bbLance)
          IdentificaLances(bbLance, iindListaLances, PEAO, casaOrigem, tipoGeracao, valorQuiesc, casaUltCaptura);
        
        // verifica se peao pode andar duas casas
        if (maskOrig & mskBitBoardInitPeao[tabPrincipal.vez]) {
          int casaFrente = casaOrigem + dirFrente;
          int casaDupla  = casaOrigem + dirDuplo;
          TBitBoard maskFrente = mskBitBoardUnitario[casaFrente];
          if (!(maskFrente & todasPecas)) {
            TBitBoard maskDupla = mskBitBoardUnitario[casaDupla];
            if (!(maskDupla & todasPecas)) {
              bbLance = maskDupla;
              IdentificaLances(bbLance, iindListaLances, PEAO, casaOrigem, tipoGeracao, valorQuiesc, casaUltCaptura);                
            }
          }
        }
      }

      // verifica captura en passant
      if (indListaLancesJogados) {
        TLance *ultimo = &listaLancesJogados[indListaLancesJogados-1];
        int ultOrigem  = ultimo->casaOrigem;
        int ultDestino = ultimo->casaDestino;
        int delta      = ultDestino - ultOrigem;
        if (ultimo->peca == PEAO && delta > 9 && (ultDestino == casaOrigem + 1 || ultDestino == casaOrigem - 1))
          if (!(tipoGeracao & MSK_QUIESCENCE) || ((tipoGeracao & MSK_QUIESCENCE) && (PEAO >= valorQuiesc) )) {
            casaDestino = ultDestino + dirFrente;
            GeraLance(iindListaLances, PEAO, casaOrigem, casaDestino, MSK_ENPASSANT, 0, PEAO, valorPecas[PEAO]);      
          }
      }
          
      // retira casaOrigem do bitboard de peças
      bbPeca &= bbPeca - 1;
    }
  }

  // Lances de Cavalo
  if (tabPrincipal.cavalos[tabPrincipal.vez]) {
    bbPeca = tabPrincipal.cavalos[tabPrincipal.vez];
    while (bbPeca) {
      casaOrigem = __builtin_ctzll(bbPeca);
      // capturas
      bbLance = mskBitBoardLancesCavalo[casaOrigem] & ~pecasVez & pecasOponente;
      if (bbLance && IdentificaLances(bbLance, iindListaLances, CAVALO, casaOrigem, tipoGeracao, valorQuiesc, casaUltCaptura))
        return 1; // avisa captura de rei

      // demais lances (apenas para geração completa)
      if (tipoGeracao & MSK_GERA_TODOS) {
        bbLance = mskBitBoardLancesCavalo[casaOrigem] & ~pecasVez & ~pecasOponente;
        if (bbLance)
          IdentificaLances(bbLance, iindListaLances, CAVALO, casaOrigem, tipoGeracao, valorQuiesc, casaUltCaptura);
      }
      // retira casaOrigem do bitboard de peças
      bbPeca &= bbPeca - 1;
    }
  }
    
  // Lances de Rei
  casaOrigem = BitMenosSignificativo(tabPrincipal.rei[tabPrincipal.vez]);
  // capturas
  bbLance = mskBitBoardLancesRei[casaOrigem] & ~pecasVez & pecasOponente;
  if (bbLance && IdentificaLances(bbLance, iindListaLances, REI, casaOrigem, tipoGeracao, valorQuiesc, casaUltCaptura))
    return 1; // identifica captura do rei
  
  // demais lances do rei (apenas para geração completa)
  if (tipoGeracao & MSK_GERA_TODOS) {
    bbLance = mskBitBoardLancesRei[casaOrigem] & ~pecasVez & ~pecasOponente;
    if (bbLance)
      IdentificaLances(bbLance, iindListaLances, REI, casaOrigem, tipoGeracao, valorQuiesc, casaUltCaptura);
    
    // roques
    if (!tabPrincipal.mskRoque[tabPrincipal.vez]) { // se ainda nao rocou
      if ((permissaoRoque = VerificaRoque(tabPrincipal.vez))) {
        if (permissaoRoque & MSK_ROQUEP) {
          GeraLance(iindListaLances, REI, casaOrigem, casaOrigem + 2, MSK_ROQUEP, 0, 0, 400); // gera roque pequeno
        } else {
          GeraLance(iindListaLances, REI, casaOrigem, casaOrigem - 2, MSK_ROQUEG, 0, 0, 300); // gera roque grande
        }
      }
    }
  }

  // Pecas deslizantes
  // Lances de bispo
  if (tabPrincipal.bispos[tabPrincipal.vez]) {
    bbPeca = tabPrincipal.bispos[tabPrincipal.vez];
    while (bbPeca) {
      casaOrigem = __builtin_ctzll(bbPeca);
      bbCapturas = bbLance = 0x00;
      // raio 1: diagonal NE 
      if (mskBitBoardDesliz[1][casaOrigem]) {
        primBit = BitMaisSignificativo(mskBitBoardDesliz[1][casaOrigem] & (todasPecas | mskBitBoardBordas[1]));
        bbLance |= mskBitBoardDesliz[1][casaOrigem] & mskBitBoardAbaixo[primBit];
        bbCapturas |= bbLance & pecasOponente;
      }
      // raio 7: diagonal NO
      if (mskBitBoardDesliz[7][casaOrigem]) {
        primBit = BitMaisSignificativo(mskBitBoardDesliz[7][casaOrigem] & (todasPecas | mskBitBoardBordas[7]));
        bbLance |= mskBitBoardDesliz[7][casaOrigem] & mskBitBoardAbaixo[primBit];
        bbCapturas |= bbLance & pecasOponente;
      }
      // raio 3: diagonal SE
      if (mskBitBoardDesliz[3][casaOrigem]) {
        primBit = BitMenosSignificativo(mskBitBoardDesliz[3][casaOrigem] & (todasPecas | mskBitBoardBordas[3]));
        bbLance |= mskBitBoardDesliz[3][casaOrigem] & mskBitBoardAcima[primBit];
        bbCapturas |= bbLance & pecasOponente;
      }
      // raio 5: diagonal SO
      if (mskBitBoardDesliz[5][casaOrigem]) {
        primBit = BitMenosSignificativo(mskBitBoardDesliz[5][casaOrigem] & (todasPecas | mskBitBoardBordas[5]));
        bbLance |= mskBitBoardDesliz[5][casaOrigem] & mskBitBoardAcima[primBit];
        bbCapturas |= bbLance & pecasOponente;
      }
      // capturas
      if (bbCapturas && IdentificaLances(bbCapturas, iindListaLances, BISPO, casaOrigem, tipoGeracao, valorQuiesc, casaUltCaptura)) 
        return 1; // captura de rei
      // demais lances
      if (tipoGeracao & MSK_GERA_TODOS) {
        // retirando as capturas dos lances
        bbLance &= ~todasPecas;
        if (bbLance && IdentificaLances(bbLance, iindListaLances, BISPO, casaOrigem, tipoGeracao, valorQuiesc, casaUltCaptura)) 
          return 1; // captura de rei
      }
      // retira casaOrigem do bitboard de peças
      bbPeca &= bbPeca - 1;
    }
  }


  // Lances de Torre
  if (tabPrincipal.torres[tabPrincipal.vez]) {
    bbPeca = tabPrincipal.torres[tabPrincipal.vez];
    while (bbPeca) {
      casaOrigem = __builtin_ctzll(bbPeca);
      bbCapturas = bbLance = 0x00;
      // raio 0: coluna N 
      if (mskBitBoardDesliz[0][casaOrigem]) {
        primBit = BitMaisSignificativo(mskBitBoardDesliz[0][casaOrigem] & (todasPecas | mskBitBoardBordas[0]));
        bbLance |= mskBitBoardDesliz[0][casaOrigem] & mskBitBoardAbaixo[primBit];
        bbCapturas |= bbLance & pecasOponente;
      }
      // raio 6: linha O
      if (mskBitBoardDesliz[6][casaOrigem]) {
        primBit = BitMaisSignificativo(mskBitBoardDesliz[6][casaOrigem] & (todasPecas | mskBitBoardBordas[6]));
        bbLance |= mskBitBoardDesliz[6][casaOrigem] & mskBitBoardAbaixo[primBit];
        bbCapturas |= bbLance & pecasOponente;
      }
      // raio 4: coluna S
      if (mskBitBoardDesliz[4][casaOrigem]) {
        primBit = BitMenosSignificativo(mskBitBoardDesliz[4][casaOrigem] & (todasPecas | mskBitBoardBordas[4]));
        bbLance = bbLance | (mskBitBoardDesliz[4][casaOrigem] & mskBitBoardAcima[primBit]);
        bbCapturas |= bbLance & pecasOponente;
      }
      // raio 2: linha L
      if (mskBitBoardDesliz[2][casaOrigem]) {
        primBit = BitMenosSignificativo(mskBitBoardDesliz[2][casaOrigem] & (todasPecas | mskBitBoardBordas[2]));
        bbLance |= mskBitBoardDesliz[2][casaOrigem] & mskBitBoardAcima[primBit];
        bbCapturas |= bbLance & pecasOponente;
      }
      // capturas
      if (bbCapturas && IdentificaLances(bbCapturas, iindListaLances, TORRE, casaOrigem, tipoGeracao, valorQuiesc, casaUltCaptura)) 
        return 1; // captura de rei
      // demais lances
      if (tipoGeracao & MSK_GERA_TODOS) {
        // retirando as capturas dos lances
        bbLance &= ~todasPecas;
        if (bbLance && IdentificaLances(bbLance, iindListaLances, TORRE, casaOrigem, tipoGeracao, valorQuiesc, casaUltCaptura)) 
          return 1; // captura de rei
      }
      // retira casaOrigem do bitboard de peças
      bbPeca &= bbPeca - 1;
    }
  }
    
  // Lances de Dama
  if (tabPrincipal.damas[tabPrincipal.vez]) {
    bbPeca = tabPrincipal.damas[tabPrincipal.vez];
    while (bbPeca) {
      casaOrigem = __builtin_ctzll(bbPeca);
      bbCapturas = bbLance = 0x00;
      // raio 0: coluna N 
      if (mskBitBoardDesliz[0][casaOrigem]) {
        primBit = BitMaisSignificativo(mskBitBoardDesliz[0][casaOrigem] & (todasPecas | mskBitBoardBordas[0]));
        bbLance |= mskBitBoardDesliz[0][casaOrigem] & mskBitBoardAbaixo[primBit];
        bbCapturas |= bbLance & pecasOponente;
      }
      // raio 1: diagonal NE 
      if (mskBitBoardDesliz[1][casaOrigem]) {
        primBit = BitMaisSignificativo(mskBitBoardDesliz[1][casaOrigem] & (todasPecas | mskBitBoardBordas[1]));
        bbLance |= mskBitBoardDesliz[1][casaOrigem] & mskBitBoardAbaixo[primBit];
        bbCapturas |= bbLance & pecasOponente;
      }
      // raio 6: linha O
      if (mskBitBoardDesliz[6][casaOrigem]) {
        primBit = BitMaisSignificativo(mskBitBoardDesliz[6][casaOrigem] & (todasPecas | mskBitBoardBordas[6]));
        bbLance |= mskBitBoardDesliz[6][casaOrigem] & mskBitBoardAbaixo[primBit];
        bbCapturas |= bbLance & pecasOponente;
      }
      // raio 7: diagonal NO
      if (mskBitBoardDesliz[7][casaOrigem]) {
        primBit = BitMaisSignificativo(mskBitBoardDesliz[7][casaOrigem] & (todasPecas | mskBitBoardBordas[7]));
        bbLance |= mskBitBoardDesliz[7][casaOrigem] & mskBitBoardAbaixo[primBit];
        bbCapturas |= bbLance & pecasOponente;
      }
      // raio 2: linha L
      if (mskBitBoardDesliz[2][casaOrigem]) {
        primBit = BitMenosSignificativo(mskBitBoardDesliz[2][casaOrigem] & (todasPecas | mskBitBoardBordas[2]));
        bbLance |= mskBitBoardDesliz[2][casaOrigem] & mskBitBoardAcima[primBit];
        bbCapturas |= bbLance & pecasOponente;
      }
      // raio 3: diagonal SE
      if (mskBitBoardDesliz[3][casaOrigem]) {
        primBit = BitMenosSignificativo(mskBitBoardDesliz[3][casaOrigem] & (todasPecas | mskBitBoardBordas[3]));
        bbLance |= mskBitBoardDesliz[3][casaOrigem] & mskBitBoardAcima[primBit];
        bbCapturas |= bbLance & pecasOponente;
      }
      // raio 4: coluna S
      if (mskBitBoardDesliz[4][casaOrigem]) {
        primBit = BitMenosSignificativo(mskBitBoardDesliz[4][casaOrigem] & (todasPecas | mskBitBoardBordas[4]));
        bbLance = bbLance | (mskBitBoardDesliz[4][casaOrigem] & mskBitBoardAcima[primBit]);
        bbCapturas |= bbLance & pecasOponente;
      }
      // raio 5: diagonal SO
      if (mskBitBoardDesliz[5][casaOrigem]) {
        primBit = BitMenosSignificativo(mskBitBoardDesliz[5][casaOrigem] & (todasPecas | mskBitBoardBordas[5]));
        bbLance |= mskBitBoardDesliz[5][casaOrigem] & mskBitBoardAcima[primBit];
        bbCapturas |= bbLance & pecasOponente;
      }
      // capturas
      if (bbCapturas && IdentificaLances(bbCapturas, iindListaLances, DAMA, casaOrigem, tipoGeracao, valorQuiesc, casaUltCaptura)) 
        return 1; // captura de rei
      // demais lances
      if (tipoGeracao & MSK_GERA_TODOS) {
        // retirando as capturas dos lances
        bbLance &= ~todasPecas;
        if (bbLance && IdentificaLances(bbLance, iindListaLances, DAMA, casaOrigem, tipoGeracao, valorQuiesc, casaUltCaptura)) 
          return 1; // captura de rei
      }
      // retira casaOrigem do bitboard de peças
      bbPeca &= bbPeca - 1;
    }
  }

  return 0;
}

// -----------------------------------------------------------------------
// IdentificaLances()
int IdentificaLances(TBitBoard bbLance, int iindListaLances, TByte peca, TByte casaOrigem, int tipoGeracao, int valorQuiesc, int casaUltCaptura) {
  int   valorLance, casaDestino;
  int   vezOponente = (tabPrincipal.vez)^1;
  TBitBoard oppAll     = tabPrincipal.pecas[vezOponente];
  TBitBoard oppPawns   = tabPrincipal.peoes[vezOponente];
  TBitBoard oppKnights = tabPrincipal.cavalos[vezOponente];
  TBitBoard oppBishops = tabPrincipal.bispos[vezOponente];
  TBitBoard oppRooks   = tabPrincipal.torres[vezOponente];
  TBitBoard oppQueens  = tabPrincipal.damas[vezOponente];
  TBitBoard oppKing    = tabPrincipal.rei[vezOponente];
  TByte pecaCapturada, pecaPromovida;

  while (bbLance) {
    casaDestino = __builtin_ctzll(bbLance);
    pecaCapturada = 0;
    valorLance = 0;
    TBitBoard maskCasa = mskBitBoardUnitario[casaDestino];
    // identifica peça capturada       
    if (oppAll & maskCasa) {
      if (oppPawns & maskCasa) 
        pecaCapturada = PEAO;
      else if (oppKnights & maskCasa) 
        pecaCapturada = CAVALO;
      else if (oppBishops & maskCasa) 
        pecaCapturada = BISPO;
      else if (oppRooks & maskCasa) 
        pecaCapturada = TORRE;
      else if (oppQueens & maskCasa) 
        pecaCapturada = DAMA;
      else if (oppKing & maskCasa) 
        return 1; // avisa captura do rei - posição não é legal
    }

    // Calcula o valor da captura
    valorLance = (valorPecas[pecaCapturada] * 100) / valorPecas[peca];

    // Gera promocoes ou lances
    if (peca == PEAO && (mskBitBoardUnitario[casaDestino] & (mskBitBoardBordas[0] | mskBitBoardBordas[4]))) {
      for (pecaPromovida = CAVALO;pecaPromovida<REI;pecaPromovida++)
        if ((tipoGeracao & MSK_GERA_TODOS) || ((tipoGeracao & MSK_QUIESCENCE) && valorPecas[pecaPromovida] >= valorQuiesc)) 
          GeraLance(iindListaLances, PEAO, casaOrigem, casaDestino, 0, pecaPromovida, pecaCapturada, (pecaCapturada ? valorPecas[pecaCapturada] + valorPecas[pecaPromovida]: valorPecas[pecaPromovida] + PEAO));          
    } else if ((tipoGeracao != MSK_QUIESCENCE) || (tipoGeracao == MSK_QUIESCENCE && (valorPecas[pecaCapturada] >= valorQuiesc || casaDestino == casaUltCaptura))) {
      GeraLance(iindListaLances, peca, casaOrigem, casaDestino, 0, 0, pecaCapturada, valorLance);
    }

    // Retira casaDestino do bitboard de lances
    bbLance &= bbLance - 1;
  }
  return 0;
}

 
// -------------------------------------------------------------------------------------------
// GeraLance()
void GeraLance(int iindListaLances, TByte peca, TByte casaOrigem, TByte casaDestino, TByte especial, TByte pecaPromovida, TByte pecaCapturada, int valorLance) {

  int iListaLances;
  // Tabelas para valoração do lance
  int  tabPeao[64] = {  900,  900,  900,  900,  900,  900,  900,  900,
  		                  300,  300,  300,  300,  300,  300,  300,  300,
  		                   20,   25,   30,   35,   35,   30,   25,   20,
			                    5,    5,    6,   10,   10,    6,    5,    5,
			                    3,    3,    4,    7,    7,    1,    1,    2,
				                  3,    3,    0,    3,    3,    0,    3,    3};

  int  tabCavalo[64] = {-10,   -5,    6,    7,    7,    6,   -5,  -10,
			                   -5,    7,   10,   11,    11,  10,    7,   -5,
				                  6,   10,   14,   14,    14,  14,   10,    6,
				                  7,   11,   14,   20,    20,  14,   11,    7,
				                  7,   11,   14,   20,    20,  14,   11,    7,
				                  6,   10,   14,   14,    14,  14,   10,    6,
				                 -5,    7,   10,   11,    11,  10,    7,   -5,
				                -10,   -5,    6,    7,    7,    6,   -5,  -10};

  int  tabBispo[64] =  {  5,    3,    3,    3,     3,   3,    3,    5,
			                    3,   10,    9,    9,     9,   9,   10,    3,
				                  3,    9,   15,   14,    14,  15,    9,    3,
				                  3,    9,   14,   20,    20,  14,    9,    3,
				                  3,    9,   14,   20,    20,  14,    9,    3,
				                  3,    9,   15,   14,    14,  15,    9,    3,
				                  3,   10,    9,    9,     9,   9,   10,    3,
				                  5,    3,    3,    3,     3,   3,    3,    5};
				                
  int  tabTorre[64] =  { 25,   25,   25,   25,    25,  25,   25,   25,
			                   50,   50,   50,   50,    50,  50,   50,   50,
				                 15,   10,   10,   10,    10,  10,   10,   15,
				                 15,   10,   05,   05,    05,  05,   10,   15,
				                 15,   10,   05,   05,    05,  05,   10,   15,
				                 15,   10,   05,   05,    05,  05,   10,   15,
				                 05,   10,   10,   10,    10,  10,   10,   05,
				                 05,   05,   10,   20,    20,  10,   05,   05};
				                
  int  tabRei[64] =    {  5,    3,    3,    3,     3,   3,    3,    5,
			                    3,   10,    9,    9,     9,   9,   10,    3,
				                  3,    9,   15,   14,    14,  15,    9,    3,
				                  3,    9,   14,   20,    20,  14,    9,    3,
				                  3,    9,   14,   20,    20,  14,    9,    3,
				                  3,    9,   15,   14,    14,  15,    9,    3,
				                  3,   10,    9,    9,     9,   9,   10,    3,
				                  5,   15,   15,    3,     3,   3,   15,   15};		                			                				                

  // Descobre o proximo espaço vazio
  if (indListaLances[iindListaLances] == indListaLances[iindListaLances + 1])
    iListaLances = 0; // primeiro lance de lista 0!
  else if (indListaLances[iindListaLances + 1])
    iListaLances = indListaLances[iindListaLances + 1];  // primeiro lance da lista
  else
    iListaLances = indListaLances[iindListaLances];

  // prepara a próxima lista de lances
  indListaLances[iindListaLances + 1] = iListaLances + 1;

	// valorando o lance de acordo com as tabelas de casa de destino acima 
	if (peca == PEAO) 
		valorLance += tabPeao[(tabPrincipal.vez ? (63 - casaDestino) : casaDestino)];
	else if (peca == CAVALO)
		valorLance += tabCavalo[casaDestino];
  else if (peca == BISPO)
    valorLance += tabBispo[casaDestino];
  else if (peca == TORRE)
    valorLance += tabTorre[(tabPrincipal.vez ? (63 - casaDestino) : casaDestino)];
  else if (peca == REI)
    valorLance += tabRei[(tabPrincipal.vez ? (63 - casaDestino) : casaDestino)];
  else if (peca == DAMA)
    valorLance += (tabTorre[(tabPrincipal.vez ? (63 - casaDestino) : casaDestino)] + tabBispo[casaDestino]);
		
  // Gera o lance
  listaLances[iListaLances].casaDestino    = casaDestino;
  listaLances[iListaLances].casaOrigem     = casaOrigem;
  listaLances[iListaLances].peca           = peca;
  listaLances[iListaLances].especial       = especial;
  listaLances[iListaLances].pecaPromovida  = pecaPromovida;
  listaLances[iListaLances].pecaCapturada  = pecaCapturada;
  listaLances[iListaLances].valorLance     = valorLance;
		
}

// -----------------------------------------------------------------------------------
// InicializaMaskLances()
void InicializaMaskLances() {
  int       casaOrigem, casaDestino, raio;
  int       raios[8]   = {-8, -7, 1, 9, 8, 7, -1, -9};   
  int       lancesCavalo[8] = {-17, -15, -10, -6, 6, 10, 15, 17};  
  TBitBoard msk = 0x01;

  for (casaOrigem = 0; casaOrigem < 64; casaOrigem++) {
    // inicializando mascaras cheias acima e abaixo da casa de origem
    msk |= mskBitBoardUnitario[casaOrigem];    
    mskBitBoardAcima[casaOrigem] = msk;
    mskBitBoardAbaixo[casaOrigem] = ~mskBitBoardAcima[casaOrigem];
    mskBitBoardAbaixo[casaOrigem] ^= mskBitBoardUnitario[casaOrigem];
    
    mskBitBoardLancesCavalo[casaOrigem]  = 0x00;
    mskBitBoardLancesRei[casaOrigem]     = 0x00;    
    mskBitBoardLancesPeao[0][casaOrigem] = 0x00;
    mskBitBoardLancesPeao[1][casaOrigem] = 0x00;
    mskBitBoardAtaquesPeao[0][casaOrigem]= 0x00;
    mskBitBoardAtaquesPeao[1][casaOrigem]= 0x00;

    // lances de peão
    if (casaOrigem > 7 && casaOrigem < 56) {
      mskBitBoardLancesPeao[1][casaOrigem] = mskBitBoardUnitario[casaOrigem + 8];
      mskBitBoardLancesPeao[0][casaOrigem] = mskBitBoardUnitario[casaOrigem - 8];
      // capturas
      if ((casaOrigem&7) == 0) {
        mskBitBoardAtaquesPeao[1][casaOrigem]  = mskBitBoardUnitario[casaOrigem + 9];
        mskBitBoardAtaquesPeao[0][casaOrigem]  = mskBitBoardUnitario[casaOrigem - 7];
      } else {
        if ((casaOrigem&7) == 7) {
          mskBitBoardAtaquesPeao[1][casaOrigem]  = mskBitBoardUnitario[casaOrigem + 7];
          mskBitBoardAtaquesPeao[0][casaOrigem]  = mskBitBoardUnitario[casaOrigem - 9];
        } else {
          mskBitBoardAtaquesPeao[1][casaOrigem]  = mskBitBoardUnitario[casaOrigem + 7];
          mskBitBoardAtaquesPeao[0][casaOrigem]  = mskBitBoardUnitario[casaOrigem - 7];
          mskBitBoardAtaquesPeao[1][casaOrigem] |= mskBitBoardUnitario[casaOrigem + 9];
          mskBitBoardAtaquesPeao[0][casaOrigem] |= mskBitBoardUnitario[casaOrigem - 9];
        }
      }
    } 

    for (raio = 0; raio < 8; raio++) {
      // lances de cavalo 
      casaDestino = casaOrigem + lancesCavalo[raio];                                            // calcula casa de destino
      if (!(casaDestino < 0 || casaDestino > 63 || abs((casaDestino&7) - (casaOrigem&7)) > 2))  // valida limites do tabuleiro
        mskBitBoardLancesCavalo[casaOrigem] |= mskBitBoardUnitario[casaDestino];

      // lances de rei
      casaDestino = casaOrigem + raios[raio];                                                   // calcula casa de destino
      if (!(casaDestino < 0 || casaDestino > 63 || abs((casaDestino&7) - (casaOrigem&7)) > 2))  // valida limites do tabuleiro
        mskBitBoardLancesRei[casaOrigem] |= mskBitBoardUnitario[casaDestino];
        
      // lances de peças deslizantes
      mskBitBoardDesliz[raio][casaOrigem] = 0x00;
      casaDestino = casaOrigem;
      while(1) {
        casaDestino += raios[raio];                             // calcula casa de destino
        if ( casaDestino < 0 || casaDestino > 63 || 
           ( raio != 0 && raio < 4 && (casaDestino&7) == 0 ) ||
           ( raio > 4 && (casaDestino&7) == 7 ))                  // valida limites do tabuleiro
          break;
        mskBitBoardDesliz[raio][casaOrigem] |= mskBitBoardUnitario[casaDestino];
      }
    }
  }
  
  // inicializando bordas para cada raio
  mskBitBoardBordas[0] = 0x00000000000000FFULL;
  mskBitBoardBordas[1] = 0x80808080808080FFULL;
  mskBitBoardBordas[2] = 0x8080808080808080ULL;
  mskBitBoardBordas[3] = 0xFF80808080808080ULL;
  mskBitBoardBordas[4] = 0xFF00000000000000ULL;
  mskBitBoardBordas[5] = 0xFF01010101010101ULL;
  mskBitBoardBordas[6] = 0x0101010101010101ULL;
  mskBitBoardBordas[7] = 0x01010101010101FFULL;
  // máscaras para verificação se o peão pode andar duas casas
  mskBitBoardInitPeao[1] = 0x000000000000FF00ULL;
  mskBitBoardInitPeao[0] = 0x00FF000000000000ULL;  
 
}

// --------------------------------------------------------------------------------------
// ObtemMelhorLance()
// Esta rotina recebe o indice de indListaLances e retorna o índice do lance de listaLances
// com o maior valor. Utiliza a máscara MSK_SELECIONADO para marcar os lances já retornados
// Recebe o primeiro lance da PV, e se estiver pesquisando ply = 0, recupera este lance primeiro
int ObtemMelhorLance(int iindListaLance, TLance* lancePV) {
  int i;  
  int melhorLance = -1;
  int valMelhorLance = -9999;
  
  for (i=indListaLances[iindListaLance];i<indListaLances[iindListaLance+1];i++) {
    // verifica o lance com maior valorLance
    if (!(listaLances[i].especial & MSK_SELECIONADO)) {

      // verifica o lance da pv
//      if (iindListaLance == 0 && ComparaLance(&listaLances[i],lancePV)) {
      if (ComparaLance(&listaLances[i],lancePV)) {
        melhorLance = i;
        break;
      }
      // monta score temporario sem alterar valorLance base
      int score = listaLances[i].valorLance;
      if (ComparaLance(&killerMoves[iindListaLance][0],&listaLances[i]) ||
          ComparaLance(&killerMoves[iindListaLance][1],&listaLances[i])) {
        score += 900;
      }
      
      if (score > valMelhorLance) {
        valMelhorLance = score;
        melhorLance = i;
      }
    }
  }
  
  if (melhorLance > -1)
    listaLances[melhorLance].especial |= MSK_SELECIONADO;

  return melhorLance;
  
}

// -----------------------------------------------------------------------
// VerificaRoque()
// Esta função retorna MSK_ROQUEP e/ou MSK_ROQUEG se for possível para o lado da vez rocar
// Basicamente ela faz uma pseudo-geração de lances para o lado contrário 
// para saber se alguma das casas envolvidas está atacada, além de verificar as posições do rei e torres,
// e se já houve algum movimento de rei e torre
int VerificaRoque(int vez) {

  int       lancesCavalo[8] = {-17, -15, -10, -6, 6, 10, 15, 17};
  int       diagonais[4]    = {-9, 7, -7, 9};
  int       linhas[4]       = {-1, -8, 8, 1};
  int       casaDestino, casaOrigem;
  int       casaOrigemRoque[2]       = {60, 4};
  int       casaOrigemTorreRoqueP[2] = {63, 7};
  int       casaOrigemTorreRoqueG[2] = {56, 0};  
  int       lance, diagonal, linha, i;
  int       bandoOposto     = vez^1;
  int       permissoesRoque = (MSK_ROQUEP | MSK_ROQUEG);
  TBitBoard todasPecas;

  // verifica se existem peças entre rei e torres
  todasPecas = tabPrincipal.pecas[BRANCAS] | tabPrincipal.pecas[NEGRAS];
  if (todasPecas & (mskBitBoardUnitario[casaOrigemRoque[tabPrincipal.vez] - 3] | mskBitBoardUnitario[casaOrigemRoque[tabPrincipal.vez] - 2] | mskBitBoardUnitario[casaOrigemRoque[tabPrincipal.vez] - 1]))
    permissoesRoque &= MSK_ROQUEP;
  if (todasPecas & (mskBitBoardUnitario[casaOrigemRoque[tabPrincipal.vez] + 2] | mskBitBoardUnitario[casaOrigemRoque[tabPrincipal.vez] + 1]))
    permissoesRoque &= MSK_ROQUEG;
  if (!permissoesRoque) return 0; 

  // verifica se rei ou torres já moveram
  for (i=0;i<indListaLancesJogados;i++) {
    if (listaLancesJogados[i].casaOrigem == casaOrigemRoque[tabPrincipal.vez])       permissoesRoque = 0x00;
    if (listaLancesJogados[i].casaOrigem == casaOrigemTorreRoqueP[tabPrincipal.vez]) permissoesRoque &= MSK_ROQUEG;
    if (listaLancesJogados[i].casaOrigem == casaOrigemTorreRoqueG[tabPrincipal.vez]) permissoesRoque &= MSK_ROQUEP;
    if (!permissoesRoque) return 0; // ou rei ou ambas as torres já foram movidas
  } 
  
  // verifica e rei e torres estão na posicão original (é possível que não, se uma posição FEN foi informada e não há histórico
  if (!(tabPrincipal.rei[vez] & mskBitBoardUnitario[casaOrigemRoque[tabPrincipal.vez]]))          permissoesRoque = 0x00;
  if (!(tabPrincipal.torres[vez] & mskBitBoardUnitario[casaOrigemTorreRoqueP[tabPrincipal.vez]])) permissoesRoque &= MSK_ROQUEG;
  if (!(tabPrincipal.torres[vez] & mskBitBoardUnitario[casaOrigemTorreRoqueG[tabPrincipal.vez]])) permissoesRoque &= MSK_ROQUEP;
  if (!permissoesRoque) return 0; // ou rei ou ambas as torres não estão em suas posições originais

  // verifica ataques às casas  
  for (casaOrigem = 0; casaOrigem < 64; casaOrigem++) {
    // Lances de peão 
//    if (tabPrincipal.peoes[vezOponente] & (mskBitBoardUnitario[casaOrigem])) {      
    if (tabPrincipal.peoes[bandoOposto] & (mskBitBoardUnitario[casaOrigem])) {            
      casaDestino = (bandoOposto ? casaOrigem + 7 : casaOrigem - 9);        // Calcula a casa de destino
      if ((casaDestino >= casaOrigemRoque[tabPrincipal.vez]   - 3 && casaDestino   <= casaOrigemRoque[tabPrincipal.vez]) ||
          (casaDestino+2 >= casaOrigemRoque[tabPrincipal.vez] - 3 && casaDestino+2 <= casaOrigemRoque[tabPrincipal.vez]))
        permissoesRoque &= MSK_ROQUEP;
      if ((casaDestino >= casaOrigemRoque[tabPrincipal.vez]   && casaDestino   <= casaOrigemRoque[tabPrincipal.vez]+2) ||
          (casaDestino+2 >= casaOrigemRoque[tabPrincipal.vez] && casaDestino+2 <= casaOrigemRoque[tabPrincipal.vez]+2))
        permissoesRoque &= MSK_ROQUEG; 
      if (!permissoesRoque) return 0; 
    } 

    // Lances de Cavalo
    if (tabPrincipal.cavalos[bandoOposto] & (mskBitBoardUnitario[casaOrigem])) {
      for (lance = 0; lance < 8; lance++) {
        casaDestino = casaOrigem + lancesCavalo[lance];
        if (casaDestino >= casaOrigemRoque[tabPrincipal.vez] - 3 && casaDestino <= casaOrigemRoque[tabPrincipal.vez])
          permissoesRoque &= MSK_ROQUEP;
        if (casaDestino >= casaOrigemRoque[tabPrincipal.vez] && casaDestino <= casaOrigemRoque[tabPrincipal.vez] + 2)
          permissoesRoque &= MSK_ROQUEG;
        if (!permissoesRoque) return 0; 
      }
    }

    // Pecas deslizantes
    // Lances diagonais (bispo ou Dama)
    if ((tabPrincipal.bispos[bandoOposto] | tabPrincipal.damas[bandoOposto])& (mskBitBoardUnitario[casaOrigem])) {
      for (diagonal = 0; diagonal < 4; diagonal++) {
        for (casaDestino = casaOrigem + diagonais[diagonal];; casaDestino = casaDestino + diagonais[diagonal]) {
          // Verifica fim de tabuleiro
          if (casaDestino < 0 || casaDestino > 63 || 
             (diagonal < 2 && (casaDestino&7) == 7) ||
             (diagonal > 1 && (casaDestino&7) == 0))
            break;
    
          if (casaDestino >= casaOrigemRoque[tabPrincipal.vez] - 3 && casaDestino <= casaOrigemRoque[tabPrincipal.vez])
            permissoesRoque &= MSK_ROQUEP;
          if (casaDestino >= casaOrigemRoque[tabPrincipal.vez] && casaDestino <= casaOrigemRoque[tabPrincipal.vez] + 2)
            permissoesRoque &= MSK_ROQUEG;
          if (!permissoesRoque) return 0; 
            
          // Verifica se casa esta ocupada (fim da diagonal)
    	    if (todasPecas & (mskBitBoardUnitario[casaDestino])) break;
        }
      }
    }
    
    // Lances horizontais (torre ou Dama)
    if ((tabPrincipal.torres[bandoOposto] | tabPrincipal.damas[bandoOposto])& (mskBitBoardUnitario[casaOrigem])) {
      for (linha = 0; linha < 4; linha++) {
        for (casaDestino = casaOrigem + linhas[linha];; casaDestino = casaDestino + linhas[linha]) {
          // Verifica final de tabuleiro
          if (casaDestino < 0 || casaDestino > 63 || 
    	       (linha == 0 && (casaDestino&7) == 7) ||
    	       (linha == 3 && (casaDestino&7) == 0)) {
    	        break;
    	      }
    	      
          if (casaDestino >= casaOrigemRoque[tabPrincipal.vez] - 3 && casaDestino <= casaOrigemRoque[tabPrincipal.vez])
            permissoesRoque &= MSK_ROQUEP;
          if (casaDestino >= casaOrigemRoque[tabPrincipal.vez] && casaDestino <= casaOrigemRoque[tabPrincipal.vez] + 2)
            permissoesRoque &= MSK_ROQUEG;
          if (!permissoesRoque) return 0; 
    
          // Verifica se casa esta ocupada (fim da linha)
          if (todasPecas & (mskBitBoardUnitario[casaDestino])) break;
        }
      }
    } 
  }  
  return permissoesRoque;
}

// -----------------------------------------------------------------------
// VerificaXeque()
// Esta função retorna 1 caso o lado solicitado esteja em xeque na posição
// Muito similar à VerificaRoque, mas desta vez obtendo a casa onde está o rei antes
int VerificaXeque(int vez) {

  int       lancesCavalo[8] = {-17, -15, -10, -6, 6, 10, 15, 17};
  int       diagonais[4]    = {-9, 7, -7, 9};
  int       linhas[4]       = {-1, -8, 8, 1};
  int       casaDestino, casaOrigem, casaRei;
  int       lance, diagonal, linha, i;
  int       bandoOposto     = vez^1;
  TBitBoard todasPecas = tabPrincipal.pecas[BRANCAS] | tabPrincipal.pecas[NEGRAS];

  // obtem casa do rei
  for (i=0;i<64;i++) {
    if (tabPrincipal.rei[vez] & mskBitBoardUnitario[i]) {
       casaRei = i;
       break;
    }
  }
  // verifica ataques à casaRei
  for (casaOrigem = 0; casaOrigem < 64; casaOrigem++) {
    // Lances de peão 
    if (tabPrincipal.peoes[bandoOposto] & (mskBitBoardUnitario[casaOrigem])) {            
      casaDestino = (bandoOposto ? casaOrigem + 7 : casaOrigem - 9);        // Calcula a casa de destino
      if (casaDestino == casaRei || (casaDestino+2) == casaRei) return 1; 
    } 

    // Lances de Cavalo
    if (tabPrincipal.cavalos[bandoOposto] & (mskBitBoardUnitario[casaOrigem])) {
      for (lance = 0; lance < 8; lance++) {
        casaDestino = casaOrigem + lancesCavalo[lance];
        if (casaDestino == casaRei) return 1; 
      }
    }

    // Pecas deslizantes
    // Lances diagonais (bispo ou Dama)
    if ((tabPrincipal.bispos[bandoOposto] | tabPrincipal.damas[bandoOposto])& (mskBitBoardUnitario[casaOrigem])) {
      for (diagonal = 0; diagonal < 4; diagonal++) {
        for (casaDestino = casaOrigem + diagonais[diagonal];; casaDestino = casaDestino + diagonais[diagonal]) {
          // Verifica fim de tabuleiro
          if (casaDestino < 0 || casaDestino > 63 || 
             (diagonal < 2 && (casaDestino&7) == 7) ||
             (diagonal > 1 && (casaDestino&7) == 0)) break;

          if (casaDestino == casaRei) return 1; 
            
          // Verifica se casa esta ocupada (fim da diagonal)
    	    if (todasPecas & (mskBitBoardUnitario[casaDestino])) break;
        }
      }
    }
    
    // Lances horizontais (torre ou Dama)
    if ((tabPrincipal.torres[bandoOposto] | tabPrincipal.damas[bandoOposto])& (mskBitBoardUnitario[casaOrigem])) {
      for (linha = 0; linha < 4; linha++) {
        for (casaDestino = casaOrigem + linhas[linha];; casaDestino = casaDestino + linhas[linha]) {
          // Verifica final de tabuleiro
          if (casaDestino < 0 || casaDestino > 63 || 
    	       (linha == 0 && (casaDestino&7) == 7) ||
      	       (linha == 3 && (casaDestino&7) == 0)) {
    	        break;
    	      }
    	      
          if (casaDestino == casaRei) return 1; 
    
          // Verifica se casa esta ocupada (fim da linha)
          if (todasPecas & (mskBitBoardUnitario[casaDestino])) break;
        }
      }
    } 
  }  
  return 0;
}

