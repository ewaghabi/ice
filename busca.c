// ----------------------------------------------------------------
// busca.c
// Funcoes de busca
#include "ice.h"
#include <stdio.h>
#include <time.h>

// globais externas
extern TLance           listaLances[2000];
extern int              indListaLances[40];
extern TBoard           tabPrincipal;
extern int              post;
extern int              ladoMotor;

// globais locais
clock_t               ticks1, ticks2;            // variaveis para medicao do tempo com funcao clock() (time.h)
int                   qtdCentesimos;             // total de tempo destinado a busca 
long                  qtdNos;                    // numero de nos pesquisados
long                  qtdNos2;                   // qtd nos para calculo de branch factor
int                   flagRetorno;               // flag para comunicacao entre nos
int                   flagLanceImpossivel;       // flag para comunicacao entre nos
int                   flagTimeOut;               // flag para comunicacao entre nos
int                   maxProfundidade;           // profundidade original 
int                   ultimaEval = 0;            // retorno do AlfaBeta: valor da posicao
int                   ultimoKiller = 0;          // ultimo killer move substituido 
TLance                killerMoves[MAX_DEPTH][2]; // dois slots para killer moves (por profundidade)

// variaveis para debugacao
long                    noDebug = -1; //16990; //-1; //= 18050; //-1; // 3898;
int                     numLanceDebug = -1;
extern int              Debug;
extern int              mostraNo;

// prototipos externos
// geraLances.c
extern int  GeraListaLances(int, int, int, int);
extern void InicializaMaskLances();
extern int  ObtemMelhorLance(int, TLance*);
// make.c
extern void Make(TLance*, TBoard*);
extern void UnMake(TLance*, TBoard*);
// eval.c
extern int  Eval(TBoard*);
extern int  VerificaXeque(int);
extern int  popCount(TBitBoard);

// Debug
extern void MostraTabuleiro(TBoard*);
extern void MostraBitBoard(TBitBoard);
extern void ImprimeListaLances(int, int, TByte);
extern void ImprimeLance(TLance*, int, TByte, int);

// prototipos locais
int   Busca(int, TLance*);
int   AlphaBeta(int, int, int, TPv*);
int   Quiescence(int, int, int);
long  Bench(int, int, long*);
void  InicializaKillerMoves(void);
int   AtualizaKillerMoves(TLance*, int);
void  CopiaLance(TLance*, TLance*);
int   ComparaLance(TLance*, TLance*);
void  ImprimePV(TPv*, int, int);
void  InicializaTempoBusca(int);
int   VerificaFimTempoBusca();

// ----------------------------------------------------
// Busca()
int Busca(int tempoBusca, TLance* lance) {
  int     i, profundidade, retorno, eval;
  TPv     pv;
  
  qtdNos = qtdNos2 = 0;                  // inicializa o contador de nos e o tempo
  InicializaTempoBusca(tempoBusca);      // inicializa tempo
  InicializaKillerMoves();               // inicializa vetor de killer moves
  ticks1 = clock();                      // inicializa contador de tempo

  // iterative deepening
  for (profundidade = 2; profundidade < MAX_DEPTH; profundidade++) {
    maxProfundidade = profundidade;        // marca a maior profundidade a ser pesquisada, para indexar a lista de lances
    flagRetorno = flagLanceImpossivel = 0; // flags de comunicacao entre os nos
    flagTimeOut = 0;
    
    // realiza a busca com janela inicial maior possivel
    // note que a variante principal e re-enviada a busca para auxiliar a ordenacao de lances
    eval = AlphaBeta(profundidade, -INFINITO, +INFINITO, &pv);  
    // eval = AlphaBeta_debug(profundidade, -INFINITO, +INFINITO, &pv);  

    // verifica se a linha possui empate ou mate
    retorno = 0;
    if (!(flagRetorno & (MSK_MATE | MSK_AFOGADO)))     
      for (i = 0; i< pv.numLances; i++) 
        if (pv.lances[i].especial & (MSK_MATE | MSK_AFOGADO)) retorno = pv.lances[i].especial;
    
    // verifica se tempo se esgotou ou a linha possui empate ou mate,
    // ou tempo restante e menor do que o utilizado para a ultima profundidade <- isto deve ser revisto em caso de null-move
    if (flagTimeOut || ((retorno | flagRetorno) & (MSK_MATE | MSK_AFOGADO))) {
        
        if (!flagTimeOut)
          ultimaEval = eval;
          
        if (!(flagRetorno & (MSK_MATE | MSK_AFOGADO))) { 
          CopiaLance(lance, &pv.lances[0]);                              // obtem lance da PV para retorno        
          ImprimePV(&pv, ultimaEval, maxProfundidade);                   // imprime PV final
        }
      
      // retorna possivel resultado 
      return flagRetorno;
    }
    ultimaEval = eval;
    ImprimePV(&pv, ultimaEval, profundidade);                         // imprime PV ao final de cada profundidade
  }
  return flagRetorno;
}

// ------------------------------------------------------------
// Bench(): roda AlphaBeta em profundidade fixa N vezes para medir NPS
long Bench(int profundidade, int repeticoes, long *tempoCentesimos) {
  int rep, depth;
  int eval = 0;
  TPv pv;

  // desativa time-out e zera contadores
  qtdNos = 0;
  flagTimeOut = 0;
  qtdCentesimos = 100000000; // muito grande para nao cortar por tempo
  InicializaKillerMoves();

  ticks1 = clock();
  for (rep = 0; rep < repeticoes; rep++) {
    for (depth = 2; depth <= profundidade; depth++) {
      maxProfundidade = depth;
      flagRetorno = flagLanceImpossivel = 0;
      eval = AlphaBeta(depth, -INFINITO, +INFINITO, &pv);
      ultimaEval = eval;
      if (post) ImprimePV(&pv, eval, depth); // mostra PV se post estiver ativo
    }
  }
  ticks2 = clock();

  *tempoCentesimos = (ticks2 - ticks1)/(CLOCKS_PER_SEC/100);
  return qtdNos;
}

// ------------------------------------------------------
// AlphaBeta()
int AlphaBeta(int profundidade, int alpha, int beta, TPv *pv) {
    int indLanceLista, i, flagXeque = 0;
    int valor, ply, qtdLancesPossiveis;
    TPv pvTemp;
    static int emNullMove = 0; // evita dois null-move seguidos

    // estabelece o ply
    ply = maxProfundidade - profundidade;
    // conta nos
    qtdNos++;
    
    // verifica final de busca, e chama Quiescence
    if (profundidade == 0) {
      pv->numLances = 0;
      return Quiescence(alpha, beta, ply);
    }

    // verifica time-out
    if (qtdNos & NO_VERIF_TIMEOUT)
      if (VerificaFimTempoBusca()) 
        return alpha;

    // verifica se esta em xeque
    flagXeque = VerificaXeque(tabPrincipal.vez);
    // extensao de busca para xeque = 1 ply
//    if (flagXeque) profundidade++;

    // Null-move pruning desabilitado temporariamente (precisa revisao)
    
    // gera lances, verificando se houve captura do rei (a posicao eh ilegal)
    if (GeraListaLances(ply, MSK_GERA_TODOS, 0, 0)) {
      flagLanceImpossivel = 1;
      return 0;
    }
      
    qtdLancesPossiveis = 0;
    while ((indLanceLista = ObtemMelhorLance(ply, &pv->lances[ply])) != -1) { // obtem lances um a um, o pv primeiro
      Make(&listaLances[indLanceLista], &tabPrincipal);                    // faz o lance no tabuleiro principal
      flagLanceImpossivel = flagRetorno = 0;                               // inicializa flags de comunicacao
      valor = -AlphaBeta(profundidade - 1, -beta, -alpha, &pvTemp);        // pesquisa
      UnMake(&listaLances[indLanceLista], &tabPrincipal);                  // desfaz o lance no tabuleiro principal
      
      // testa flags de comunicacao
      if (flagTimeOut)            // verifica time-out
        return alpha;      
      if (flagLanceImpossivel) { // verifica lance impossivel
        flagLanceImpossivel = 0;
        continue;
      }
      
      listaLances[indLanceLista].especial |= flagRetorno;
      flagRetorno = 0;

      // se o lance da mate ou afoga, atualiza PV e retorna valor       
      if (listaLances[indLanceLista].especial & (MSK_MATE | MSK_AFOGADO)) {
        CopiaLance(&pv->lances[0],&listaLances[indLanceLista]);                            
        pv->numLances = 1;
        return valor;
      }
      
      qtdLancesPossiveis++;       // conta lance possivel

      // verifica cut-off
      if (valor > alpha) {
        alpha = valor;

        if (valor >= beta) {                                  
          AtualizaKillerMoves(&listaLances[indLanceLista], ply); // atualiza KillerMoves so em cortes
          return beta;                      
        }
        // atualiza PV - cria nova pv com lance feito, seguido da linha (pv) recebida
        CopiaLance(&pv->lances[0],&listaLances[indLanceLista]);                            
        for (i=0;i<(pvTemp.numLances);i++) CopiaLance(&pv->lances[i + 1],&pvTemp.lances[i]);
        pv->numLances = pvTemp.numLances + 1;
        
        if (!ply) ImprimePV(pv, valor, maxProfundidade);     // imprime PV quando ply for 0
      }
    }
    
    if (flagXeque) flagRetorno |= MSK_XEQUE;   // marca flag de xeque para lance anterior saber que eh xeque
    
    // testa possiveis retornos: normal, mate ou afogado
    if (qtdLancesPossiveis) return alpha; 
    else {
      if (flagXeque) {
        flagRetorno |= MSK_MATE;
        return -9999;
      } else {
        flagRetorno |= MSK_AFOGADO;
        return 0;
      }
    }
} 
// -----------------------------------------------------------
// CopiaLance(destino, origem)
void CopiaLance(TLance* a, TLance* b) {
  a->peca          = b->peca;
  a->casaOrigem    = b->casaOrigem;
  a->casaDestino   = b->casaDestino;
  a->pecaCapturada = b->pecaCapturada;
  a->pecaPromovida = b->pecaPromovida;
  a->especial      = b->especial;
  a->valorLance    = b->valorLance;
}

// -----------------------------------------------------------
// ComparaLance()
int ComparaLance(TLance* a, TLance* b) {
  if (a->peca == 0 || b->peca == 0) 
    return 0;
    
  if (a->peca          == b->peca       &&
      a->casaOrigem    == b->casaOrigem &&
      a->casaDestino   == b->casaDestino &&
      a->pecaPromovida == b->pecaPromovida)
    return 1;
  else return 0;
}


// ----------------------------------------------------------
// ImprimePV()
void ImprimePV(TPv* pv, int score, int ply) {
  int i, numLance, vez, primLance;
  long tempo;
  
  // obtendo o tempo decorrido ate agora
  if (post) {
    ticks2 = clock();
    tempo = (ticks2 - ticks1)/(CLOCKS_PER_SEC/100);
    printf("%2d %4d %5ld %7ld ",ply,score,tempo,qtdNos); 
    
    vez       = tabPrincipal.vez;
    numLance  = tabPrincipal.numLance;
    primLance = 1;
    
    for (i=0;i<pv->numLances;i++) { 
      ImprimeLance(&pv->lances[i], numLance, vez, primLance);
      vez ^= 1; 
      if (vez == BRANCAS) numLance++;
      primLance = 0;
    }
    
    if (ladoMotor) score *= -1;
    
    if (score > 200)       printf("+-");
    else if (score > 125)  printf("+/-");
    else if (score > 60)   printf("+/=");
    else if (score < -200) printf("-+");
    else if (score < -125) printf("-/+");
    else if (score < -60)  printf("-/=");
    else printf("=");
    printf("\n");
  }
}

// ----------------------------------------------------------------
// Quiescence()
int Quiescence(int alpha, int beta, int ply) {
    TPv pv;
    int valor, indLanceLista;

    valor = Eval(&tabPrincipal);
    // conta nos    
    qtdNos++;

    if (valor >= beta) return beta;
    if (valor > alpha) alpha = valor;
    
    // gera lances para Quiescence
    if (GeraListaLances(ply, MSK_QUIESCENCE, valor, 0)) {
      flagLanceImpossivel = 1;
      return 0;
    }
    
    pv.lances[0].casaDestino = pv.lances[0].casaOrigem = 0;                // inutilizando pv, para evitar escolha em ObtemMelhorLance
    while ((indLanceLista = ObtemMelhorLance(ply, &pv.lances[0])) != -1) { // obtem lances um a um
      Make(&listaLances[indLanceLista], &tabPrincipal);                    // faz o lance no tabuleiro principal
      flagLanceImpossivel = flagRetorno = 0;                               // inicializa flags de comunicacao
      valor = -Quiescence(-beta, -alpha, ply+1);                           // pesquisa
      UnMake(&listaLances[indLanceLista], &tabPrincipal);                  // desfaz o lance no tabuleiro principal

      // testa flags de comunicacao
      if (flagLanceImpossivel) {
        flagLanceImpossivel = 0;
        continue;
      }
      if (valor >= beta) return beta;
      if (valor > alpha) alpha = valor;

    }  

    return alpha; 
} 

// ------------------------------------------------------------
// AtualizaKillerMoves
// retorno 1: lance ja existente, valor incrementado
// retorno 0: lance nao existente, incluido no primeiro slot vazio ou de menor valor
int  AtualizaKillerMoves(TLance* lance, int ply) {
  int slot = 0;
  
  if (ComparaLance(&killerMoves[ply][0],lance)) {
    killerMoves[ply][0].valorLance++;
    return 1;
  }
  if (ComparaLance(&killerMoves[ply][1],lance)) {
    killerMoves[ply][1].valorLance++;
    return 1;
  }
  if (killerMoves[ply][0].peca == 0) {
    CopiaLance(&killerMoves[ply][0],lance);
    killerMoves[ply][0].valorLance = 0;
    return 0;
  } 
  if (killerMoves[ply][1].peca == 0) {
    CopiaLance(&killerMoves[ply][1],lance);
    killerMoves[ply][1].valorLance = 0;
    return 0;
  }
  if (killerMoves[ply][1].valorLance == killerMoves[ply][0].valorLance) {
    CopiaLance(&killerMoves[ply][ultimoKiller^1],lance);
    killerMoves[ply][ultimoKiller^1].valorLance = 0;
    ultimoKiller^=1;
    return 0;
  }
  if (killerMoves[ply][1].valorLance < killerMoves[ply][0].valorLance) 
    slot++;
  CopiaLance(&killerMoves[ply][slot],lance);
  killerMoves[ply][slot].valorLance = 0;
  ultimoKiller=slot;
  return 0;
}

// ------------------------------------------------------------
// InicializaTempoBusca()
void  InicializaTempoBusca(int tempoMax) {
  ticks1 = clock();
  qtdCentesimos = tempoMax;
}

// ------------------------------------------------------------
// InicializaKillerMoves()
void  InicializaKillerMoves() {
  int i;
  for (i=0;i<MAX_DEPTH;i++)
    killerMoves[i][0].peca = killerMoves[i][1].peca = 0;
  ultimoKiller = 0;
}


// --------------------------------------------------------------
// VerificaFimTempoBusca()
int   VerificaFimTempoBusca() {
  flagTimeOut = 0;
  if ((long)((clock() - ticks1)/(CLOCKS_PER_SEC/100)) >= (long)qtdCentesimos) 
    flagTimeOut = 1;

  return flagTimeOut;
}

// ***********************************************************************************

// -----------------------------------------------------------
// Alfabeta com instrucoes de debugacao
int AlphaBeta_debug(int profundidade, int alpha, int beta, TPv *pv) {
    int indLanceLista, i, flagXeque = 0;
    int valor, ply, qtdLancesPossiveis;
    TPv pvTemp;

    // estabelece o ply
    ply = maxProfundidade - profundidade;
    // conta nos
    qtdNos++;

   
    if (profundidade == 0) {
/*      if (Debug) {
        for(i=0;i<=ply;i++) printf(" * ");
        printf(" Saindo do AlphaBeta - avaliacao: %02.02f\n",(float)Quiescence(-INFINITO, +INFINITO, ply)/100);
      }       */
      pv->numLances = 0;
      return Quiescence(-INFINITO, +INFINITO, ply);      
    }
    if (Debug) {
      for(i=0;i<=ply;i++) printf(" * ");
      printf(" Entrando no AlphaBeta: profund.:%d, alpha:%d, beta:%d\n",profundidade, alpha, beta);
      printf(" pv recebida: "); ImprimePV(pv, 0, maxProfundidade);
      getchar();
    } 
    
    
    // liga-desliga Debug para o no especificado
    if (qtdNos == noDebug) Debug = 1; // else Debug = 0;

    // verifica se esta em xeque
    flagXeque = VerificaXeque(tabPrincipal.vez);
    if (Debug && flagXeque) {
      for(i=0;i<=ply;i++) printf(" * ");
      printf(" Lado a jogar esta em xeque!\n");
    }

    // gera lances, verificando se houve captura do rei (a posicao eh ilegal)
    if (GeraListaLances(ply, MSK_GERA_TODOS, 0, 0)) {
      if (Debug) {
        for(i=0;i<=ply;i++) printf(" * ");
        printf(" Detectado Lance Impossivel!\n");
      } 
      flagLanceImpossivel = 1;
      return 0;
    }
    
/*    if (Debug) {
      for(i=0;i<=ply;i++) printf(" * ");
      printf(" Lista de lances: ");
      ImprimeListaLances(ply,tabPrincipal.numLance,tabPrincipal.vez);
    }    */
    
    qtdLancesPossiveis = 0;
    while ((indLanceLista = ObtemMelhorLance(ply, &pv->lances[0])) != -1) {          // obtem lances um a um
      if (Debug) {
        for(i=0;i<=ply;i++) printf(" * ");
        printf(" Realizando lance:  ");
        ImprimeLance(&listaLances[indLanceLista],tabPrincipal.numLance,tabPrincipal.vez,1);
        printf("\n");
      }
      
      Make(&listaLances[indLanceLista], &tabPrincipal);              // faz o lance no tabuleiro principal
      flagLanceImpossivel = flagRetorno = 0;                         // inicializa flags de comunicacao
      valor = -AlphaBeta_debug(profundidade - 1, -beta, -alpha, &pvTemp);  // pesquisa
      UnMake(&listaLances[indLanceLista], &tabPrincipal);            // desfaz o lance no tabuleiro principal
      
      // testa flags de comunicacao
      if (flagLanceImpossivel) { 
        if (Debug) {
          for(i=0;i<=ply;i++) printf(" * ");
          printf(" ");
          ImprimeLance(&listaLances[indLanceLista],tabPrincipal.numLance,tabPrincipal.vez,1);
          printf(" e Impossivel. Obtendo proximo lance...\n");
        } 
        flagLanceImpossivel = 0;
        continue;
      }
      
      if (Debug) {
        for(i=0;i<=ply;i++) printf(" * ");
        printf(" Retorno da busca. Valor: %d Xeque: %d Mate: %d Afogado: %d\n",valor,((flagRetorno & MSK_XEQUE)?1:0),((flagRetorno & MSK_MATE)?1:0),((flagRetorno & MSK_AFOGADO)?1:0));
      } 
      
      listaLances[indLanceLista].especial |= flagRetorno;
      flagRetorno = 0;

      // se o lance da mate ou afoga, atualiza PV e retorna valor       
      if (listaLances[indLanceLista].especial & (MSK_MATE | MSK_AFOGADO)) {
        CopiaLance(&pv->lances[0],&listaLances[indLanceLista]);                            
        pv->numLances = 1;
        return valor;
      }
      
      qtdLancesPossiveis++;       // conta lance possivel
        
      // verifica cut-off
      if (valor >= beta) { 
        if (Debug) {
          for(i=0;i<=ply;i++) printf(" * ");
          printf(" Saindo do AlphaBeta - alpha >= beta - retornando beta %d\n",beta);
        } 
        return beta;
      }         
      if (valor > alpha) {
        alpha = valor;
        // atualiza PV
        // cria nova pv com lance feito seguido da linha (pv) recebida
        CopiaLance(&pv->lances[0],&listaLances[indLanceLista]);                            
        for (i=0;i<(pvTemp.numLances);i++) CopiaLance(&pv->lances[i + 1],&pvTemp.lances[i]);
        pv->numLances = pvTemp.numLances + 1;
        if (Debug) {
          for(i=0;i<=ply;i++) printf(" * ");
          printf(" Atualizando PV: ");
          ImprimePV(pv, valor, maxProfundidade);
        } 
        if (!ply) ImprimePV(pv, valor, maxProfundidade);     // imprime PV quando ply for 0        
        AtualizaKillerMoves(&listaLances[indLanceLista], ply); // atualiza KillerMoves
      }
    }
    
    if (flagXeque) { 
      flagRetorno |= MSK_XEQUE;   // marca flag de xeque para lance anterior saber que eh xeque
      if (Debug) {
        for(i=0;i<=ply;i++) printf(" * ");
        printf(" Marcando flagRetorno com XEQUE\n");
      } 
    }
    
    // testa possiveis retornos: normal, mate ou afogado
    if (qtdLancesPossiveis) {
      if (Debug) {
        for(i=0;i<=ply;i++) printf(" * ");
        printf(" Saindo do AlphaBeta - retornando alpha: %d\n",alpha);
      } 
      return alpha; 
    } else {
      if (flagXeque) {
        flagRetorno |= MSK_MATE;
        if (Debug) {
          for(i=0;i<=ply;i++) printf(" * ");
          printf(" Sem lances possiveis, e em xeque: MATE\n");
        } 
        return -9999;
      } else {
        flagRetorno |= MSK_AFOGADO;
        if (Debug) {
          for(i=0;i<=ply;i++) printf(" * ");
          printf(" Sem lances possiveis, e nao esta em xeque: AFOGADO\n");
        } 
        return 0;
      }
    }
} 

int Quiescence_debug(int alpha, int beta, int ply) {
    TPv pv;
    int valor, indLanceLista, i;

    if (Debug) {
      for(i=0;i<=ply;i++) printf(" * ");
      printf(" Entrando no Quiescence profund.:%d, alpha:%d, beta:%d\n",ply, alpha, beta);
    } 

    valor = Eval(&tabPrincipal);
    if (Debug) {
      for(i=0;i<=ply;i++) printf(" * ");
      printf(" Eval.:%d\n",valor);
    } 

    if (valor >= beta) return beta;
    if (valor > alpha) alpha = valor;
    
    // verifica time-out
//    if (VerificaFimTempoBusca()) {
//      if (Debug) {
//        for(i=0;i<=ply;i++) printf(" * ");
//        printf(" Saindo por time-out\n",valor);
//      } 
//      return alpha;
//    }
    
    // gera lances para Quiescence
    if (GeraListaLances(ply, MSK_QUIESCENCE, valor, 0)) {
      if (Debug) {
        for(i=0;i<=ply;i++) printf(" * ");
        printf(" Detectado lance impossivel!\n");
      } 
      flagLanceImpossivel = 1;
      return 0;
    }
    
    if (Debug) {
      for(i=0;i<=ply;i++) printf(" * ");
      printf(" Lista de lances: ");
      ImprimeListaLances(ply,tabPrincipal.numLance,tabPrincipal.vez);
    }    
    
    pv.lances[0].casaDestino = pv.lances[0].casaOrigem = 0;                // inutilizando pv, para evitar escolha em ObtemMelhorLance
    while ((indLanceLista = ObtemMelhorLance(ply, &pv.lances[0])) != -1) { // obtem lances um a um
      if (Debug) {
        for(i=0;i<=ply;i++) printf(" * ");
        printf(" Realizando lance: ");
        ImprimeLance(&listaLances[indLanceLista],tabPrincipal.numLance,tabPrincipal.vez,1);
        printf("\n");
      }
      Make(&listaLances[indLanceLista], &tabPrincipal);                    // faz o lance no tabuleiro principal
      flagLanceImpossivel = flagRetorno = 0;                               // inicializa flags de comunicacao
      valor = -Quiescence(-beta, -alpha, ply+1);                           // pesquisa
      UnMake(&listaLances[indLanceLista], &tabPrincipal);                  // desfaz o lance no tabuleiro principal

      // testa flags de comunicacao
      if (flagLanceImpossivel) {
        if (Debug) {
          for(i=0;i<=ply;i++) printf(" * ");
          printf(" Lance e impossivel!\n");
        } 
        flagLanceImpossivel = 0;
        continue;
      }
      if (valor >= beta) return beta;
      if (valor > alpha) alpha = valor;

    }  

    return alpha; 
} 
