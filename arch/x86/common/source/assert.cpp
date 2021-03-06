/**
 * @file assert.cpp
 *
 */

#include "assert.h"
#include "kprintf.h"
#include "panic.h"
#include "debug_bochs.h"
#include "Thread.h"
#include "ArchInterrupts.h"

extern Thread* currentThread;

void pre_new_sweb_assert(uint32 condition, uint32 line, const char* file)
{
  char const *error_string = "KERNEL PANIC: Assertion Failed in File:  on Line:      ";
  char line_string[5];
  if (!condition)
  {
    ArchInterrupts::disableInterrupts();
    extern uint32 boot_completed;
    boot_completed = 0;
    uint8 * fb = (uint8*)0xC00B8000;
    uint32 s=0;
    uint32 i=0;
    for (s=0; s<40; ++s)
    {
      fb[i++] = error_string[s];
      fb[i++] = 0x9f;
    }

    writeLine2Bochs(error_string);
    writeChar2Bochs('\n');
    writeLine2Bochs(file);
    writeChar2Bochs('\n');
    while (file && *file)
    {
      fb[i++] = *file++;
      fb[i++] = 0x9f;
    }

    for (s=40; s<54; ++s)
    {
      fb[i++] = error_string[s];
      fb[i++] = 0x9f;
    }

    i-=4;
    for (s=0; s < (sizeof(line_string) - 1); ++s)
    {
      line_string[s]=' ';
    }
    line_string[s]='\0';
    while (line>0)
    {
      fb[i++] = (uint8) ( 0x30 + (line%10) );
      fb[i] = 0x9f;
      line_string[--s] = ( 0x30 + (line%10) );
      i-=3;
      line /= 10;
    }
    writeLine2Bochs(line_string);
    writeChar2Bochs('\n');
    if (currentThread != 0)
      currentThread->printBacktrace(false);
    for ( ; ;) ;
  }
}


void sweb_assert(const char *condition, uint32 line, const char* file)
{
  ArchInterrupts::disableInterrupts();
  extern uint32 boot_completed;
  boot_completed = 0;
  kprintfd("KERNEL PANIC: Assertion %s failed in File %s on Line %d\n",condition, file, line);
  kprintf("KERNEL PANIC: Assertion %s failed in File %s on Line %d\n",condition, file, line);
  kpanict((uint8*) "Halting System\n");
}
