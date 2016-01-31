.PHONY: all clean

all: emu

emu: common/debug.c common/one_hot.c arch/tlb/tlb.c bus/controller.c bus/memorymap.c vr4300/cp0.c vr4300/cp1.c vr4300/cpu.c vr4300/dcache.c vr4300/decoder.c vr4300/fault.c vr4300/functions.c vr4300/icache.c vr4300/opcodes.c vr4300/pipeline.c vr4300/segment.c src/emu.c src/main.c src/srec.c src/uart.c
	gcc -ggdb3 -g3 -fdata-sections -ffunction-sections -I. -Iarch -Icommon -Iinclude $^ -pthread -lpthread -o emu

#./src/gen/doop.gen.c: ./disgen/*.py ./disgen/mips.json
#	mkdir -p ./src/gen/
#	python ./disgen/disgen.py ./disgen/cdisgen.py ./disgen/mips.json > ./src/gen/doop.gen.c

clean:
	rm -vrf ./src/gen/
	rm -fv ./emu
