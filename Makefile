CC      ?= clang
CFLAGS  ?= -std=gnu99 -O2 -Wall -Wextra
SOURCES := bitBoardFunc.c busca.c eval.c geraLances.c ice.c init.c make.c mostraTab.c

ice: $(SOURCES)
	$(CC) $(CFLAGS) -o $@ $(SOURCES)

.PHONY: clean
clean:
	rm -f ice
