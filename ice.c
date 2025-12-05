#include "ice.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>

// variaveis globais
TBoard           tabPrincipal;
char             simboloPecas[7] = {0, 0, 'N', 'B', 'R', 'Q', 'K'};
char             simboloPecasPromocao[7] = {' ', 0, 'n', 'b', 'r', 'q'};
int              valorPecas[7]  = {0, VAL_PEAO, VAL_CAVALO, VAL_BISPO, VAL_TORRE, VAL_DAMA, VAL_REI};
TLance           listaLances[2000];
TLance           listaLancesJogados[200];
int              indListaLances[40];
int              indListaLancesJogados = 0;
TBitBoard        mskBitBoardUnitario[64];
int              ladoMotor = NEGRAS; // lado que o ICE esta jogando
int              post = 0;           // flag utilizada para imprimir pv
int              controleTempo = 0;  // numero de lances ate o proximo controle de tempo
int              incrementoTempo = 0;// quantidade de segundos recebidos por lance
long             tempoMotor = 0;     // tempo restante (em centesimos) ate o proximo controle de tempo
long             tempoOponente;      // tempo restante do oponente (util em caso de troca de lados)
extern int       ultimaEval;         // ultima avaliacao retornada por Alfabeta
// flag de debug
int     Debug = 0;
int     mostraNo = 0;


// versao
float   VERSAO = 1.40;

// prototipos externos
// init.c
extern void IniciaTabuleiro(char*, TBoard*, TByte, TByte, TByte);
extern void IniciaMaskBitBoardUnitario(TBitBoard*);
// geraLances.c
extern void GeraListaLances(int, int, int, int);
extern void InicializaMaskLances();
extern int  ObtemMelhorLance(int);
extern int  VerificaRoque(int);
extern int  VerificaXeque(int);
extern int  Eval(TBoard*);
// make.c
extern void Make(TLance*, TBoard*);
extern void UnMake(TLance*, TBoard*);
// busca.c
extern int  Busca(int, TLance*);
extern int  Quiescence(int, int, int);
extern int  Quiescence_debug(int, int, int);
extern long Bench(int, int, long*);
// bitBoardFunc.c
extern void setOnebits(void);

// prototipos locais
void        Saudacao(void);
void        Fim(int, char*);
void        LeInput(char*,FILE*);
int         parseLance(char*,TLance*);
void        TrocaTempos(void);
int         AlocaTempo(void);
// funcoes para Debug
void        ImprimeListaLances(int, int, TByte);
void        ImprimeLance(TLance*, int, TByte, int);
extern void MostraTabuleiro(TBoard*);
extern void MostraBitBoard(TBitBoard);
extern int  BitMenosSignificativo(TBitBoard); // inlineGccAsm8086.c
extern int  BitMaisSignificativo(TBitBoard);
//extern int  BitMenosSigPreench(TBitBoard);
//extern int  BitMaisSigPreench(TBitBoard);


int main(int argc, char* argv[])
{

  TBoard* ptrTabPrincipal = &tabPrincipal;
  TLance  lance, melhorLance;
  int     retParseLance, retBusca;
  char    strInput[BUFFER_LEITURA];
  char    initFEN[70];
  int     posFEN, ladoFEN;
  int     permissoesRoque;

                               
  clock_t ticks1, ticks2;      
  long i;                 
  
  // Verificando parametros da linha de comando
  if(argc > 1)                 
    if(!strcmp(argv[1], "-d")) Debug = 1;
                               
  // Imprimindo saudacao       
  Saudacao();                  
                               
  // Desabilitando buffer de entrada e saida, para interface com Winboard
  setbuf (stdout, NULL);       
  setbuf (stdin, NULL);        
  // Inicializando tabuleiro com posicao FEN
  
  IniciaTabuleiro("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR",ptrTabPrincipal, BRANCAS, (MSK_ROQUEP | MSK_ROQUEG), (MSK_ROQUEP | MSK_ROQUEG));
 
  // Inicializando mascaras
  IniciaMaskBitBoardUnitario(&mskBitBoardUnitario[0]);
  InicializaMaskLances();
  setOnebits();  
   
  // loop de leitura dos comandos do xboard (winboard)
  while (1) {
    // verifica se e a vez do motor jogar
    if (tabPrincipal.vez == ladoMotor) {
      retBusca = Busca(AlocaTempo(), &melhorLance);
      // testa retorno de mate ou afogado decorrente do lance realizado pelo oponente
      if (retBusca & (MSK_MATE | MSK_AFOGADO)) {
        if (retBusca & MSK_MATE) {
          if (!ladoMotor)
            printf("0-1 {Black mates}\n");
          else
            printf("1-0 {White mates}\n");
        } else printf("1/2-1/2 {Stalemate}\n");
        ladoMotor = -1;
        continue;
      }
      // verificando se e hora de abandonar
      if (ultimaEval < RESIGN_VALUE) {
        if (!ladoMotor)
          printf("0-1 {White resigns}\n");
        else
          printf("1-0 {Black resigns}\n");
        ladoMotor = -1;
        continue;
      }
      // faz o lance
      Make(&melhorLance, ptrTabPrincipal);  
      // envia o lance para a interface
      printf("move %c%d%c%d%c\n",melhorLance.casaOrigem%8 + 97, (8 - melhorLance.casaOrigem/8),
                                 melhorLance.casaDestino%8 + 97,(8 - melhorLance.casaDestino/8),
                                 simboloPecasPromocao[melhorLance.pecaPromovida]);
      // verifica possivel resultado decorrente do lance realizado pelo motor
      if (melhorLance.especial & MSK_MATE) {
        if (!ladoMotor)
          printf("1-0 {White mates}\n");
        else
          printf("0-1 {Black mates}\n");
        ladoMotor = -1;
      }
      if (melhorLance.especial & MSK_AFOGADO) {
        printf("1/2-1/2 {Stalemate}\n");
        ladoMotor = -1;
      }
    }
    
    // obtendo e tratando entrada
    LeInput(strInput, stdin);
		if (!strcmp(strInput, "xboard")) {          // comando de inicializacao enviado pelo Winboard ao inicio
		  continue;
		} else if (!strncmp(strInput, "protover", 8)) { // comando indicando versao do protocolo e solicitando features
		  printf("feature ping=0 setboard=1 san=0 usermove=1 time=1 draw=1 sigint=0 sigterm=0 reuse=0 analyze=0 myname=\"ICE v%01.02f\" variants=\"normal\" colors=0 name=0\n",VERSAO);
      printf("feature done=1\n");
		} else if (!strncmp(strInput, "accepted", 8) || // features aceitas com sucesso 
		           strInput[0] == 0                  || // enviado eventualmente - deve ser ignorado
		          !strcmp(strInput, "random")        || // comando especifico para o GNUChess 4 - ignorado
              !strcmp(strInput, "computer")      || // winboard avisando que o jogo sera contra outra engine
		          !strcmp(strInput, "?")    ) {         // comando "move now", ignorado nesta parte   
			continue;    
		} else if (!strcmp(strInput, "quit") ||
		           !strcmp(strInput, "exit")) {     // fim de programa
			Fim(0, "Programa finalizado com sucesso");;
		} else if (!strcmp(strInput, "new")) {      // inicializa tabuleiro e lista de lances
			IniciaTabuleiro("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR",ptrTabPrincipal, BRANCAS, (MSK_ROQUEP | MSK_ROQUEG), (MSK_ROQUEP | MSK_ROQUEG));
			ladoMotor = NEGRAS;
			indListaLancesJogados = 0;
			ultimaEval=0;
		} else if (!strncmp(strInput, "level", 5)) { // recebe controle de tempo
			sscanf(strInput+6,"%d",&controleTempo);
			if (!controleTempo)                        // recebe incremento
			  sscanf(strInput+8,"%ld %d",&tempoMotor,&incrementoTempo);
		} else if (!strncmp(strInput, "time", 4)) { // recebe tempo total restante do motor (em centesimos)
			sscanf(strInput+5,"%ld",&tempoMotor);
		} else if (!strncmp(strInput, "otim", 4)) { // recebe tempo total restante do motor (em centesimos)
			sscanf(strInput+5,"%ld",&tempoOponente);
		} else if (!strcmp(strInput, "force")) {    // entra em modo "espectador", isto e, nao joga para nenhum lado
			ladoMotor = -1; 
		} else if (!strncmp(strInput, "result", 6)) { // a partida acabou o winboard esta informando o resultado
			ladoMotor = -1;
		} else if (!strcmp(strInput, "post")) {     // imprime pv
			post = 1; 
		} else if (!strcmp(strInput, "nopost")) {   // nao imprime pv
			post = 0; 
		} else if (!strcmp(strInput, "undo")) {     // volta um lance (winboard envia force antes)
			UnMake(&listaLancesJogados[indListaLancesJogados-1], ptrTabPrincipal);
		} else if (!strcmp(strInput, "remove")) {   // volta dois lances 
      UnMake(&listaLancesJogados[indListaLancesJogados-1], ptrTabPrincipal);
      UnMake(&listaLancesJogados[indListaLancesJogados-1], ptrTabPrincipal);      
		} else if (!strncmp(strInput, "setboard", 8)) {  // inicializa tabuleiro com string FEN
		  // obtem string com posicao
		  for (posFEN=0;strInput[posFEN+9] != 32;posFEN++);
		  strncpy(initFEN, strInput+9, posFEN); initFEN[posFEN] = '\0';
		  // obtem lado a jogar
		  if (strInput[posFEN+10] == 98) ladoFEN = NEGRAS; else ladoFEN = BRANCAS;
			IniciaTabuleiro(initFEN, ptrTabPrincipal, ladoFEN, (MSK_ROQUEP | MSK_ROQUEG), (MSK_ROQUEP | MSK_ROQUEG));
			ladoMotor = ladoFEN^1;
		} else if (!strcmp(strInput, "playother")) { // define que o motor jogara pelo lado que nao esta na vez
		  ladoMotor = (tabPrincipal.vez ? NEGRAS : BRANCAS);
		  TrocaTempos();
		} else if (!strcmp(strInput, "white")) {     // define que o jogador (oponente) jogara pelas brancas
		  if (ladoMotor != NEGRAS) {
			  ladoMotor = NEGRAS;
			  TrocaTempos();
			}
		} else if (!strcmp(strInput, "black")) {    // define que o jogador (oponente) jogara pelas brancas
		  if (ladoMotor != BRANCAS) {
			  ladoMotor = BRANCAS;
			  TrocaTempos();
			}
    } else if (!strncmp(strInput, "bench", 5)) { // mede desempenho: bench <profundidade> <repeticoes>
      int depth = 5, reps = 1;
      long tempoCentesimos = 0;
      long nos;
      sscanf(strInput+6, "%d %d", &depth, &reps);
      if (depth < 1) depth = 1;
      if (reps < 1) reps = 1;
      nos = Bench(depth, reps, &tempoCentesimos);
      long nps = tempoCentesimos ? (nos * 100) / tempoCentesimos : nos;
      printf("bench depth=%d reps=%d nos=%ld tempo=%ldcs nps=%ld\n", depth, reps, nos, tempoCentesimos, nps);
		} else if (!strcmp(strInput, "go")) {       // faz o motor comecar a pensar pelo lado da vez
			ladoMotor = ptrTabPrincipal->vez;
		} else if (!strcmp(strInput, "d") || 
		           !strcmp(strInput, "diagrama")) { // imprime o tabuleiro atual na tela
			MostraTabuleiro(ptrTabPrincipal);
/*			printf("Teste BitMenosSignificativo pecas negras : %d\n",BitMenosSignificativo(tabPrincipal.pecas[NEGRAS]));
//			printf("Teste BitMenosSignificativo pecas brancas: %d\n",BitMenosSignificativo(tabPrincipal.pecas[BRANCAS]));
			printf("Teste BitMenosSigPreench pecas negras    : %d\n",BitMenosSigPreench(tabPrincipal.pecas[NEGRAS]));
			printf("Teste BitMenosSigPreench pecas brancas   : %d\n",BitMenosSigPreench(tabPrincipal.pecas[BRANCAS]));
//			printf("Teste BitMaisSignificativo pecas negras  : %d\n",BitMaisSignificativo(tabPrincipal.pecas[NEGRAS]));
			printf("Teste BitMaisSignificativo pecas brancas : %d\n",BitMaisSignificativo(tabPrincipal.pecas[BRANCAS]));
			printf("Teste BitMaisSigPreench pecas negras     : %d\n",BitMaisSigPreench(tabPrincipal.pecas[NEGRAS]));
			printf("Teste BitMaisSigPreench pecas brancas    : %d\n",BitMaisSigPreench(tabPrincipal.pecas[BRANCAS]));*/
		} else if (!strcmp(strInput, "gen")) {           // -------------------------- DEBUG --------------------------
		  GeraListaLances(0, MSK_GERA_TODOS, 0, 0);
      ImprimeListaLances(0,tabPrincipal.numLance,tabPrincipal.vez);
    } else if (!strcmp(strInput, "testegen")) {     
      printf("Testando geracao de lances (10k iteracoes):\n");
      ticks1 = clock();
      for (i=0;i<10000;i++) 
        GeraListaLances(0, MSK_GERA_TODOS, 0, 0);
      ticks2 = clock();      
      printf("Todos os lances: %ld clock ticks.\n",(long)(ticks2 - ticks1));
      ticks1 = clock();
      for (i=0;i<10000;i++) 
        GeraListaLances(0, MSK_QUIESCENCE, 200, 14);
      ticks2 = clock();      
      printf("Quiescence: %ld clock ticks.\n",(long)(ticks2 - ticks1));      
		} else if (!strcmp(strInput, "genQ")) {
		  GeraListaLances(0, MSK_QUIESCENCE, 200, 14);
      ImprimeListaLances(0,tabPrincipal.numLance,tabPrincipal.vez);
		} else if (!strcmp(strInput, "genC")) {
		  GeraListaLances(0, MSK_GERA_CAPTURAS, 0, 0);
      ImprimeListaLances(0,tabPrincipal.numLance,tabPrincipal.vez);
		} else if (!strcmp(strInput, "roque")) {
		  permissoesRoque = VerificaRoque(ptrTabPrincipal->vez);
		  if (permissoesRoque & MSK_ROQUEP) printf("Roque pequeno permitido\n");
		  if (permissoesRoque & MSK_ROQUEG) printf("Roque grande permitido\n");
		} else if (!strcmp(strInput, "xeque")) {
		  if (VerificaXeque(ptrTabPrincipal->vez))
		    printf("Lado em xeque!\n");
		  else
		    printf("Lado nao esta em xeque\n");
		} else if (!strcmp(strInput, "quiesc")) {
		  printf("Quiescence: %d\n",Quiescence_debug(-INFINITO, +INFINITO, 0));
		} else if (!strcmp(strInput, "eval")) {
		  printf("Eval: %d (ultima: %d)\n",Eval(ptrTabPrincipal),ultimaEval);
    } else if (!strcmp(strInput,"debug")) {
      if (Debug) Debug = 0; else Debug = 1;
    } else if (!strcmp(strInput,"mostraNo")) {
      if (mostraNo) mostraNo = 0; else mostraNo = 1; // ------------------------- DEBUG --------------------------
		} else if (!strncmp(strInput,"usermove",8)) {    // jogador informou um lance
		      if (parseLance(strInput+9, &lance)) 
            printf("Illegal move: %s (%d)\n",strInput,retParseLance);
		      else 
		        Make(&lance, ptrTabPrincipal);		    
		} else if ((retParseLance = parseLance(strInput, &lance)) != 2) {    // jogador informou um lance (protocolo 1)
		      if (retParseLance)
            printf("Illegal move: %s (%d)\n",strInput,retParseLance);
		      else 
		        Make(&lance, ptrTabPrincipal);		    
		} else printf("Error (unknown command): %s\n",strInput);
	}

  Fim(0, "Programa finalizado com sucesso");
  return 0;
}

// ----------------------------------------------------------------------
// Saudacao()
// Imprime Saudacao
void Saudacao(void) {
  printf("ICE v%01.02f\n",VERSAO);
  printf("Motor de Xadrez por Eduardo Waghabi\n");
  if (Debug) printf("Debug ON\n");
}

// ----------------------------------------------------------------------
// Fim()
// Finaliza programa com codigo de retorno especificado, apos imprimir mensagem
void Fim(int RC, char* msg) {
  printf("%s\n",msg);
  exit(RC);
}

// ------------------------------------------------------------------------
// LeInput()
void LeInput(char *strInput, FILE *fd) {
  int charLido, i = 0;
  
  for (i=0;i<BUFFER_LEITURA;i++) {
    charLido = getc(fd);
    if (charLido == (int)'\n' || charLido == EOF)
      break;
    strInput[i] = charLido;
  }           
  strInput[i] = '\0'; // finalizando a string
}

// ------------------------------------------------------------------------
// parseLance()
int parseLance(char *strInput, TLance *lance) {
    
  int  linhaOrigem, colunaOrigem, linhaDestino, colunaDestino;
  char pecaPromovida;

  // verificando se string parece-se com um lance
  if (strInput[0] < 97 || strInput[0] > 104 ||
      strInput[1] < 49 || strInput[1] > 56  ||
      strInput[2] < 97 || strInput[2] > 104 ||
      strInput[3] < 49 || strInput[3] > 56)
    return 2; // nao e um lance

  // inicializando lance
  lance->peca = lance->casaOrigem = lance->casaDestino = lance->pecaPromovida = lance->pecaCapturada = lance->valorLance = lance->especial = 0;

  // obtendo informacoes do lance 
  colunaOrigem  = strInput[0];
  linhaOrigem   = strInput[1];
  colunaDestino = strInput[2];
  linhaDestino  = strInput[3];
  pecaPromovida = strInput[4];
  
  // convertendo coordenadas para o sistema do motor
  lance->casaOrigem  = 63 - ((linhaOrigem  - 49)* 8) + colunaOrigem  - 104;
  lance->casaDestino = 63 - ((linhaDestino - 49)* 8) + colunaDestino - 104;
  
  // identifica se uma peca esta sendo promovida
  switch (pecaPromovida) {
      case 113 : lance->pecaPromovida = DAMA;   break;
      case 114 : lance->pecaPromovida = TORRE;  break;
      case 98  : lance->pecaPromovida = BISPO;  break;
      case 110 : lance->pecaPromovida = CAVALO; break;
      default  : lance->pecaPromovida = 0;      break;
  }
  
  // verifica se a casa de destino esta ocupada por uma peca do mesmo bando (lance ilegal)
  if (tabPrincipal.pecas[tabPrincipal.vez]     & mskBitBoardUnitario[lance->casaDestino])
    return 1;

  // identifica qual peca esta sendo movida
  if (tabPrincipal.pecas[tabPrincipal.vez]     & mskBitBoardUnitario[lance->casaOrigem]) {
    if (tabPrincipal.peoes[tabPrincipal.vez]   & mskBitBoardUnitario[lance->casaOrigem])
       lance->peca = PEAO;
    if (tabPrincipal.cavalos[tabPrincipal.vez] & mskBitBoardUnitario[lance->casaOrigem])
       lance->peca = CAVALO;
    if (tabPrincipal.bispos[tabPrincipal.vez]  & mskBitBoardUnitario[lance->casaOrigem])
       lance->peca = BISPO;
    if (tabPrincipal.torres[tabPrincipal.vez]  & mskBitBoardUnitario[lance->casaOrigem])
       lance->peca = TORRE;
    if (tabPrincipal.damas[tabPrincipal.vez]   & mskBitBoardUnitario[lance->casaOrigem])
       lance->peca = DAMA;
    if (tabPrincipal.rei[tabPrincipal.vez]     & mskBitBoardUnitario[lance->casaOrigem])
       lance->peca = REI;
  } else return 3;   // casa de origem nao possui pecas do bando da vez (lance ilegal)
  
  // identifica se promocao foi correta, ou se era necessaria
  if (tabPrincipal.vez == BRANCAS) {
    if ((lance->pecaPromovida && lance->peca != PEAO) || (lance->pecaPromovida && lance->casaDestino > 7))
      return 4; // promocao incorreta
    if (!lance->pecaPromovida && lance->casaDestino < 7 && lance->peca == PEAO) 
      return 5; // promocao era necessaria
  } else {
    if ((lance->pecaPromovida && lance->peca != PEAO) || (lance->pecaPromovida && lance->casaDestino < 56))
      return 4;
    if (!lance->pecaPromovida && lance->casaDestino > 55 && lance->peca == PEAO) 
      return 5;
  }
    
  // identifica se uma captura esta sendo feita
  if (tabPrincipal.pecas[tabPrincipal.vez^1]     & mskBitBoardUnitario[lance->casaDestino]) {
    if (tabPrincipal.peoes[tabPrincipal.vez^1]   & mskBitBoardUnitario[lance->casaDestino]) 
       lance->pecaCapturada = PEAO;
    if (tabPrincipal.cavalos[tabPrincipal.vez^1] & mskBitBoardUnitario[lance->casaDestino])
       lance->pecaCapturada = CAVALO;
    if (tabPrincipal.bispos[tabPrincipal.vez^1]  & mskBitBoardUnitario[lance->casaDestino])
       lance->pecaCapturada = BISPO;
    if (tabPrincipal.torres[tabPrincipal.vez^1]  & mskBitBoardUnitario[lance->casaDestino])
       lance->pecaCapturada = TORRE;
    if (tabPrincipal.damas[tabPrincipal.vez^1]   & mskBitBoardUnitario[lance->casaDestino])
       lance->pecaCapturada = DAMA;
  } else if ((lance->peca == PEAO) &&
             (lance->casaDestino >= (tabPrincipal.vez ? 40 : 16) || lance->casaDestino <= (tabPrincipal.vez ? 47 : 23)) &&
             (tabPrincipal.peoes[tabPrincipal.vez^1]  & mskBitBoardUnitario[lance->casaDestino + (tabPrincipal.vez ? -8 : 8)])) {
    lance->pecaCapturada = PEAO;
    lance->especial |= MSK_ENPASSANT;
  } else lance->pecaCapturada = 0;

  if (Debug) {  
    printf("Lance recebido: ");
    ImprimeLance(lance, tabPrincipal.numLance, tabPrincipal.vez, 1);
    printf("\n");
  }
  
  // lance identificado com sucesso
  return 0;
}



// ------------------------------------------------------------------------
// ImprimeListaLances()
void ImprimeListaLances(int iindListaLances, int numLance, TByte vez) {

  int iListaLances;
  int qtdLances = 0;
  
  for (iListaLances = indListaLances[iindListaLances]; iListaLances < indListaLances[iindListaLances + 1]; iListaLances++) {
      ImprimeLance(&listaLances[iListaLances], numLance, vez, 1);
	    qtdLances++;
  }
  
  printf(" %d\n",qtdLances);
  // Total: %d lances\n",qtdLances);
}

// ------------------------------------------------------------------
// ImprimeLance()
void ImprimeLance(TLance *lance, int numLance, TByte vez, int primeiroLance) {

   if (vez == BRANCAS)
     printf("%d. ",numLance);
   
   if (vez == NEGRAS && primeiroLance)
     printf("%d... ",numLance);

   if (lance->especial & (MSK_ROQUEG | MSK_ROQUEP)) {
     if (lance->especial & MSK_ROQUEP) printf("O-O");
     if (lance->especial & MSK_ROQUEG) printf("O-O-O");
   } else {
     if (simboloPecas[lance->peca])                   printf("%c",simboloPecas[lance->peca]);
     if (lance->peca == PEAO && lance->pecaCapturada) printf("%c",lance->casaOrigem%8 + 97);
     if (lance->especial & MSK_DESEMPATE)             printf("(%c%d)",lance->casaOrigem%8 + 97,(8 - lance->casaOrigem/8));
     if (lance->pecaCapturada)                        printf("x");
    
     printf("%c%d",lance->casaDestino%8 + 97,(8 - lance->casaDestino/8));
	 
  	 if (lance->pecaPromovida)                        printf("=%c",simboloPecas[lance->pecaPromovida]);
  	 if (lance->especial & MSK_ENPASSANT)             printf("e.p.");  	 
	 }
	 
	 if (lance->especial & MSK_MATE)                    printf("#");
	 else if (lance->especial & MSK_XEQUE)              printf("+");  
   	   
	 printf(" ");
}

// ------------------------------------------------------------------
// TrocaTempos()
void TrocaTempos() {
  long tempoTemp = tempoMotor;
	tempoMotor = tempoOponente;
	tempoOponente = tempoTemp;
}

// -----------------------------------------------------------------
// AlocaTempo()
int AlocaTempo(void) {
  int tempoAlocado = 0;
  int lancesAteControle = (controleTempo - (tabPrincipal.numLance - 1)%controleTempo);

  if (controleTempo) {
    tempoAlocado = labs((tempoMotor - 50) / lancesAteControle);
  } else {
    if (incrementoTempo)
      tempoAlocado += (incrementoTempo * 100) - 50;
    else 
      tempoAlocado = tempoMotor / 30;
  }
  
  if (tempoAlocado <= 0)
    tempoAlocado = 100; // garante algum tempo minimo para evitar time-out imediato

  return tempoAlocado;
}
