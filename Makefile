PROJECT=eelectric
LD65_FLAGS=
CA65_FLAGS=
EMULATOR=/mnt/c/NESDev/Mesen.exe
VERSION := $(shell git describe --exact-match --tags 2> /dev/null || git rev-parse --short HEAD)

TARGET=${PROJECT}.nes

.PHONY: debug run usage release FORCE

default: ${TARGET}

debug: LD65_FLAGS += --dbgfile ${PROJECT}.dbg
debug: CA65_FLAGS += -g -DDEBUG=1
debug: ${TARGET}

${TARGET}: src/main.o src/crt0.o src/lib/unrle.o \
           assets/nametables.o
	ld65 $^ -C MMC3.cfg nes.lib -m map.txt -o ${TARGET} ${LD65_FLAGS}

%.o: %.s
	ca65 $< ${CA65_FLAGS}

src/main.s: src/main.c assets/nametables.h
	cc65 -Oirs src/main.c --add-source ${CA65_FLAGS}

src/crt0.o: src/crt0.s src/mmc3/mmc3_code.asm src/lib/neslib.s src/lib/nesdoug.s assets/*.chr
	ca65 $< ${CA65_FLAGS}

assets/nametables.o: assets/nametables.s assets/nametables.h \
                     assets/nametables/main.rle \
                     assets/nametables/title.rle
	ca65 $< ${CA65_FLAGS}

clean:
	rm src/*.o src/*/*.o assets/*.o *.nes *.dbg map.txt -f

run: debug
	${EMULATOR} ${TARGET}

usage: tools/ld65-map.json

tools/ld65-map.json: map.txt tools/ld65-map.rb
	ruby tools/ld65-map.rb map.txt 2 1 tools/ld65-map.json

release: ${TARGET}
	cp ${TARGET} ${PROJECT}-${VERSION}.nes

FORCE:
