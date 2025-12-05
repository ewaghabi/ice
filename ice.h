#ifndef __ICE_H__
#define __ICE_H__

// constantes

// pecas
#define PEAO     1
#define CAVALO   2
#define BISPO    3
#define TORRE    4
#define DAMA     5
#define REI      6

// valor das peças
#define VAL_PEAO     100
#define VAL_CAVALO   300
#define VAL_BISPO    300
#define VAL_TORRE    500
#define VAL_DAMA     900
#define VAL_REI     9900
#define VAL_REI_PC   250

// jogadores (vez)
#define BRANCAS 0
#define NEGRAS  1

// máscaras especiais
#define MSK_XEQUE       0x01
#define MSK_MATE        0x02
#define MSK_ROQUEP      0x04
#define MSK_ROQUEG      0x08
#define MSK_ENPASSANT   0x10
#define MSK_DESEMPATE   0x20    // indica se lance necessita de desempate na hora da impressao
#define MSK_SELECIONADO 0x40    // indica se lance ja foi selecionado para busca
#define MSK_AFOGADO     0x80    // indica se lance afoga rei adversario

// máscaras para geração de lances
#define MSK_GERA_TODOS     0x01 
#define MSK_GERA_CAPTURAS  0x02
#define MSK_QUIESCENCE     0x04

// tamanho do buffer de leitura
#define BUFFER_LEITURA 256   // em bytes

// constantes de busca
#define INFINITO         99999
#define MAX              0
#define MIN              1
#define MAX_DEPTH        30 
#define RESIGN_VALUE     -500 // valor abaixo do qual ICE abandonará
#define NO_VERIF_TIMEOUT 15   // deve ser (2^n - 1)

// tipos e estruturas
typedef unsigned char      TByte;
typedef unsigned long long TBitBoard;

// TBoard - estrutura do tabuleiro principal
typedef struct {
			TBitBoard   rei[2];
			TBitBoard   damas[2];
			TBitBoard   torres[2];
			TBitBoard   bispos[2];
			TBitBoard   cavalos[2];
			TBitBoard   peoes[2];
			TBitBoard   pecas[2];

			TBitBoard   enPassant;
			TByte       mskRoque[2];
			TByte       vez;
			int         numLance;
		 } TBoard;

// TLance - estrutura para manter os lances
typedef struct {
			TByte     peca;
			TByte	    casaOrigem;
			TByte     casaDestino;
			TByte	    especial;
			TByte	    pecaPromovida;
			TByte	    pecaCapturada;
			int       valorLance;
            } TLance;

// PV = Principal Variation            
typedef struct {
    int    numLances;              // Number of moves in the line.
    TLance lances[MAX_DEPTH];      // The line.
              }   TPv;
              
#endif
