# ICE – Motor de Xadrez (versão original)

Motor UCI/XBoard escrito em C durante a graduação, usando bitboards, busca alfa‑beta com iterative deepening, quiescence search e killer moves. A interface é via protocolo **xboard/winboard** (não UCI).

## Estrutura rápida
- `ice.c`: laço principal do protocolo xboard (comandos `xboard`, `new`, `go`, `usermove`, etc.) e controle de tempo.
- `busca.c`: busca alfa‑beta, quiescence, PV e killer moves.
- `geraLances.c`: geração e ordenação de lances.
- `make.c` / `init.c`: fazer/desfazer lances e inicialização de bitboards.
- `eval.c`: função de avaliação simples (material + heurísticas de posição/abertura).
- `bitBoardFunc.c` / `inlineGccAsm8086.c`: funções de bit twiddling; contêm assembly x86 antigo.
- `mostraTab.c`: impressão de tabuleiro/bitboard para debug.

## Status e limitações atuais
- O código é pré‑C99 “estrito” e usa **assembly inline x86** (`bsfl/bsrl` etc.). Em Macs Apple Silicon isso não compila; mesmo em x86_64 moderno pode exigir ajustes. As funções críticas são `BitMenosSignificativo`, `BitMaisSignificativo`, `popCount`/`NumBits`.
- Cabeçalhos padrão faltam em alguns arquivos (ex.: `<string.h>`, `<stdlib.h>`, `<stdio.h>`), então o clang atual emite erros de “implicit declaration”.
- Strings com acentos estão em Latin‑1, podendo gerar avisos de encoding.

## Como compilar
### Caminho rápido para Apple Silicon ou x86_64 moderno
1. Substitua as funções de bit scan/popcount por built‑ins portáveis (clang/gcc):
   ```c
   int BitMenosSignificativo(TBitBoard b) { return b ? __builtin_ctzll(b) : -64; }
   int BitMaisSignificativo(TBitBoard b) { return b ? 63 - __builtin_clzll(b) : -64; }
   int popCount(TBitBoard b) { return __builtin_popcountll(b); }
   ```
   Depois disso, remova `inlineGccAsm8086.c` do build.
2. Adicione os cabeçalhos que faltam (`<string.h>`, `<stdlib.h>`, `<stdio.h>` onde necessário).
3. Compile com:
   ```sh
   clang -std=gnu99 -O2 -Wall -Wextra -o ice \
     ice.c init.c make.c busca.c eval.c geraLances.c mostraTab.c bitBoardFunc.c
   ```

### Caminho “nostálgico” (x86 com assembly antigo)
- Tente manter o `-std=gnu99` e compile em uma máquina x86_64 com GCC/clang que aceite o asm inline AT&T 32‑bit. No mac ARM não vai rodar.

## Como usar (xboard/winboard)
1. Inicie o binário: `./ice`.
2. Com xboard/winboard, configure o motor como First Chess Program (fcp). Exemplos de comandos aceitos:
   - `xboard` (handshake), `protover 2`, `new`, `go`.
   - Lances: `usermove e2e4` ou no protocolo antigo `e2e4`.
   - `force`, `undo`, `remove`, `setboard <FEN>`, `post/nopost` para PV, `level/time/otim` para tempo.
3. Para debug manual: rode `./ice`, digite `xboard`, depois `d` para imprimir o tabuleiro ou `gen`/`genC`/`genQ` para listar lances.

## Próximos passos sugeridos
- Tornar o código portável (substituir asm por built‑ins) e limpar includes.
- Adicionar um modo de build com `make`/`CMake` simples.
- Converter strings para UTF‑8 e usar `setlocale` se desejar preservar acentos.
- Opcional: implementar UCI ou manter apenas xboard.

## Crédito original
Motor de xadrez ICE por Eduardo Waghabi. Versão indicada no código: 1.40.
