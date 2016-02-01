#include "bus/controller.h"
#include "vr4300/cpu.h"
#include "mips.h"
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <termios.h>
#include <unistd.h>

pthread_mutex_t emu_mutex;

int ttyraw()
{
	int fd = STDIN_FILENO;
	struct termios oldtermios;
	/* Set terminal mode as follows:
	   Noncanonical mode - turn off ICANON.
	   Turn off signal-generation (ISIG)
	   	including BREAK character (BRKINT).
	   Turn off any possible preprocessing of input (IEXTEN).
	   Turn ECHO mode off.
	   Disable CR-to-NL mapping on input.
	   Disable input parity detection (INPCK).
	   Disable stripping of eighth bit on input (ISTRIP).
	   Disable flow control (IXON).
	   Use eight bit characters (CS8).
	   Disable parity checking (PARENB).
	   Disable any implementation-dependent output processing (OPOST).
	   One byte at a time input (MIN=1, TIME=0).
	*/
	struct termios newtermios;
	if(tcgetattr(fd, &oldtermios) < 0)
		return 1;
	newtermios = oldtermios;

	newtermios.c_lflag &= ~(ECHO | ICANON | IEXTEN | ISIG);
	/* OK, why IEXTEN? If IEXTEN is on, the DISCARD character
	   is recognized and is not passed to the process. This 
	   character causes output to be suspended until another
	   DISCARD is received. The DSUSP character for job control,
	   the LNEXT character that removes any special meaning of
	   the following character, the REPRINT character, and some
	   others are also in this category.
	*/

	newtermios.c_iflag &= ~(BRKINT | ICRNL | INPCK | ISTRIP | IXON);
	/* If an input character arrives with the wrong parity, then INPCK
	   is checked. If this flag is set, then IGNPAR is checked
	   to see if input bytes with parity errors should be ignored.
	   If it shouldn't be ignored, then PARMRK determines what
	   character sequence the process will actually see.
	   
	   When we turn off IXON, the start and stop characters can be read.
	*/

	newtermios.c_cflag &= ~(CSIZE | PARENB);
	/* CSIZE is a mask that determines the number of bits per byte.
	   PARENB enables parity checking on input and parity generation
	   on output.
	*/

	newtermios.c_cflag |= CS8;
	/* Set 8 bits per character. */

	newtermios.c_oflag &= ~(OPOST);
	/* This includes things like expanding tabs to spaces. */

	newtermios.c_cc[VMIN] = 1;
	newtermios.c_cc[VTIME] = 0;

	/* You tell me why TCSAFLUSH. */
	if(tcsetattr(fd, TCSAFLUSH, &newtermios) < 0)
		return -1;
	return 0;
}

static void printstate(Mips * emu,uint64_t n) {
    int i;
    fprintf(stderr,"State: %lu\n",n);
    for(i = 0; i < 32; i++){
        fprintf(stderr,"%s%d: %08x\n","gr",i,emu->regs[i]);
    }
    
    #define PRFIELD(X) fprintf(stderr, #X ": %08x\n",emu->X); 
    PRFIELD(hi);
    PRFIELD(lo);
    PRFIELD(pc);
    PRFIELD(delaypc);
    PRFIELD(CP0_Index);
    PRFIELD(CP0_EntryHi);
    PRFIELD(CP0_EntryLo0);
    PRFIELD(CP0_EntryLo1);
    PRFIELD(CP0_Context);
    PRFIELD(CP0_Wired);
    PRFIELD(CP0_Status);
    PRFIELD(CP0_Epc);
    PRFIELD(CP0_BadVAddr);
    PRFIELD(CP0_ErrorEpc);
    PRFIELD(CP0_Cause);
    PRFIELD(CP0_PageMask);
    PRFIELD(CP0_Count);
    PRFIELD(CP0_Compare);
    #undef PRFIELD
    
}

void * runCen64(void * p) {
  struct bus_controller *bus = (struct bus_controller *) p;
  struct vr4300 vr4300;

  vr4300_init(&vr4300, bus);
  vr4300.pipeline.icrf_latch.pc = 0xFFFFFFFF801E4B10ULL;

  // Prime the pipeline...
  while (vr4300.pipeline.dcwb_latch.common.pc != 0xFFFFFFFF801E4B10ULL ||
        (vr4300.pipeline.dcwb_latch.common.fault ||
        vr4300.pipeline.dcwb_latch.common.killed))
    vr4300_cycle(&vr4300);

  //printf("cmips starts at 0x%.8X... PRIMED!!\n",bus->emu->pc);
  unsigned steps_compld = 0;

  uint8_t *mem_at_wb = malloc(64 * 1024 * 1024);
  uint8_t *mem_at_commit = malloc(64 * 1024 * 1024);

  if (mem_at_wb == NULL || mem_at_commit == NULL) {
    printf("MOAR mammaries required!!\n");
    abort();
  }

  memcpy(mem_at_wb, bus->emu->mem, 64 * 1024 * 1024);
  memcpy(mem_at_commit, bus->emu->mem, 64 * 1024 *1024);

  while (1) {
        int i;

        if(pthread_mutex_lock(&emu_mutex)) {
            puts("mutex failed lock, exiting");
            exit(1);
        }

        for (i = 0; i < 10000; i++) {
#if 0
          //printf(".");
          //fflush(stdout);

          // printf("step_mips, pc: 0x%.8X\n", bus->emu->pc);
          uint32_t cmp_pc = bus->emu->pc;
          step_mips(bus->emu);

          do {
            vr4300_cycle(&vr4300);
          } while(vr4300.pipeline.last_pipe_result.fault ||
                  vr4300.pipeline.last_pipe_result.killed);

          vr4300.pipeline.last_pipe_result.fault = ~0;

          if (cmp_pc != (uint32_t) vr4300.pipeline.last_pipe_result.pc) {
            printf("PC mismatch detected @ %u steps!\n", steps_compld);
            printf("cmips: 0x%.8X, cen64: 0x%.8X\n",
              bus->emu->pc, vr4300.pipeline.last_pipe_result.pc);
            abort();
          }

          size_t ri;
          for (ri = 1; ri < 32; ri++) {
            if ((uint32_t) vr4300.regs[ri] != bus->emu->regs[ri]) {
              printf("GPR[%u] mismatch detected @ 0x%.8X/%u steps!\n", ri, cmp_pc, steps_compld);
              printf("cmips: 0x%.8X, cen64: 0x%.8X\n", bus->emu->regs[ri], vr4300.regs[ri]);
              abort();
            }
          }

#if 0
          else {
            printf("cmips: 0x%.8X, cen64: 0x%.8X\n",
              bus->emu->pc, vr4300.pipeline.last_pipe_result.pc);
          }
#endif

// this is too slow
#if 1
        if (steps_compld > 242180) {
          //memcpy(mem_at_commit, bus->mem, 64 * 1024 * 1024);
          if (steps_compld > 242182 && memcmp(mem_at_commit, bus->emu->mem, 64 * 1024 * 1024)) {
            size_t k;
            bool false_alarm;
            for (k = 0; k < 64 * 1024 * 1024 / 4; k++) {
              uint32_t cm, cm1, cm2;
              memcpy(&cm, mem_at_commit + k * 4, sizeof(cm));
              memcpy(&cm1, mem_at_wb + k * 4, sizeof(cm1));
              memcpy(&cm2, bus->mem + k * 4, sizeof(cm2));
              if (cm != bus->emu->mem[k] && cm1 != bus->emu->mem[k] && cm2 != bus->emu->mem[k]) {
                printf("Memory mismatch detected @ %u steps!\n", steps_compld);
                printf("   -> @addr=0x%.8X ... cmips=0x%.8X, cen64=0x%.8X\n",
                  (unsigned) (k * 4), bus->emu->mem[k], cm);
                false_alarm = false;
                break;
              } else {
                false_alarm = true;
                break;
              }
            }
            if (!false_alarm)
            abort();
          }

          memcpy(mem_at_commit, mem_at_wb, 64 * 1024 * 1024);
          memcpy(mem_at_wb, bus->mem, 64 * 1024 * 1024);
        }
#endif
        steps_compld++;
#else
            vr4300_cycle(&vr4300);
#endif
        }

        if(pthread_mutex_unlock(&emu_mutex)) {
            puts("mutex failed unlock, exiting");
            exit(1);
        }
  }

  return NULL;
}

void * runEmulator(void * p) {
    Mips * emu = (Mips *)p;

    while(emu->shutdown != 1) {
        int i;
        
        if(pthread_mutex_lock(&emu_mutex)) {
            puts("mutex failed lock, exiting");
            exit(1);
        }

        for(i = 0; i < 1000 ; i++)
            step_mips(emu);
        
        if(pthread_mutex_unlock(&emu_mutex)) {
            puts("mutex failed unlock, exiting");
            exit(1);
        }
        
    }
    free_mips(emu);
    exit(0);
    
}




int main(int argc,char * argv[]) {
    struct bus_controller bus;
    Mips * emu;

    pthread_t emu_thread;
    
    if(pthread_mutex_init(&emu_mutex,NULL)) {
        puts("failed to create mutex");
        return 1;
    }
    
    if (argc < 3 || (strcmp(argv[2], "cmips") && strcmp(argv[2], "cen64"))) {
        printf("Usage: %s image.srec <emutype>\n",argv[0]);
        printf("<emutype> can either be cmips or cen64\n");
        return 1;
    }
 
    emu = new_mips(64 * 1024 * 1024);
    
    if (!emu) {
        puts("allocating emu failed.");
        return 1;
    }
    
    if (loadSrecFromFile_mips(emu,argv[1]) != 0) {
        puts("failed loading srec");
        return 1;
    }

  if (!strcmp(argv[2], "cen64")) {   
    uint8_t *mem = malloc(64 * 1024 * 1024);

    if (mem == NULL) {
      puts("allocated mem failed.");
      return 1;
    }

    bus_init(&bus, mem, 64 * 1024 * 1024, emu);
    memcpy(mem, emu->mem, emu->pmemsz);
  }
    
#if 0
	if(ttyraw()) {
		puts("failed to configure raw mode");
		return 1;
	}
#endif

  if (!strcmp(argv[2], "cmips")) {
    if(pthread_create(&emu_thread,NULL,runEmulator,emu)) {
        puts("creating emulator thread failed!");
        return 1;
    }
  } else {
    if (pthread_create(&emu_thread,NULL,runCen64,&bus)) {
        puts("creating emulator thread failed!");
        return 1;
    }
  }
	
    while(1) {
        int c = getchar();
        if(c == EOF) {
            exit(1);
        }
        
        if(pthread_mutex_lock(&emu_mutex)) {
            puts("mutex failed lock, exiting");
            exit(1);
        }
        uart_RecieveChar(emu,c);
        if(pthread_mutex_unlock(&emu_mutex)) {
            puts("mutex failed unlock, exiting");
            exit(1);
        }
    }
    
    
    return 0;
}
