#define FROMSTDIN
#define _DEBUG
#include <stdio.h>
#include <dos.h>
#include <conio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <alloc.h>


#define TRUE bool(1)
#define FALSE bool(0)
#define MAX_BUFFER  10300
/* RETURN VALUE */
#define OK  bool(1)
#define BAD bool(0)
/****************/
#define LEVEL_0  0x10
#define LEVEL_1  0x20
#define LEVEL_2  0x40
#define RRWD   port+0
#define RWRT   port+0
#define REI    port+1
#define RII    port+2
#define RCNTRL port+3
#define REM    port+4
#define RSL    port+5
#define RSM    port+6
#define SOH 0x1
#define STX 0x2
#define ETX 0x3
#define EOT 0x4
#define ENQ 0x5
#define ACK 0x6
#define HLT 0x9
#define ERR 0x10
#define SCW (unsigned long)0x7CD215D8
#define ICW (unsigned long)0x7A89C197
#define FCW    0xAA
#define SIZE   576/8
#define ENABLE_RTS 0x2
#define MAX_BATCH (MAX_BUFFER/40+14)

typedef unsigned char byte;
typedef unsigned int  word;
typedef unsigned long  dword;
typedef unsigned char bool;
typedef struct __Batch{
 unsigned long scw;
 unsigned long frame[8][2];
}Batch;

