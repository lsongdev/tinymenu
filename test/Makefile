# Host render check: runs a scripted session of the demo UI at several screen
# sizes and compares every screen with the committed snapshots.
#
#   make           render to out/<w>x<h>/ (PBM + PNG, sheet.png) and compare
#   make update    accept the current renders as the new snapshots
#   make clean

CC ?= clang
SIZES ?= 128x64 160x80 160x128 128x128 96x64 128x32
LIB := $(shell find ../src -name '*.c')
SRC := render.c $(LIB) ../examples/demo/tm_demo.c
CFLAGS ?= -std=gnu11 -O1 -g -Wall -Wextra -Wno-unused-parameter \
          -fsanitize=address,undefined -fno-sanitize-recover=undefined -fno-omit-frame-pointer
INC := -I../src -I../examples/demo

check: unit render
	@./unit
	@fail=0; for s in $(SIZES); do \
	  for f in out/$$s/*.pbm; do \
	    cmp -s $$f snapshots/$$s/$$(basename $$f) || { echo "DIFF $$f"; fail=1; }; \
	  done; \
	done; \
	if [ $$fail = 0 ]; then echo "all screens match the snapshots"; else echo "see out/<size>/sheet.png; 'make update' accepts the changes"; exit 1; fi

render: $(SRC)
	@rm -rf out
	@$(CC) $(CFLAGS) $(INC) $(SRC) -lm -o render
	@for s in $(SIZES); do \
	  mkdir -p out/$$s && ./render $$s out/$$s && python3 -I png.py out/$$s || exit 1; \
	done

unit: unit.c $(LIB)
	@$(CC) $(CFLAGS) $(INC) unit.c $(LIB) -lm -o unit

update: render
	@for s in $(SIZES); do mkdir -p snapshots/$$s && cp out/$$s/*.pbm snapshots/$$s/; done
	@echo "snapshots updated"

clean:
	rm -rf render render.dSYM unit unit.dSYM out

.PHONY: check render update clean
