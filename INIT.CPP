#include "def.h"

/* EXTERN */
extern  dword _baud;
extern  word  _stop;
extern  word  _size;
extern  word  _portIn;
extern  word  _portOut;
extern  word  _parity;
extern  word _timer_baud;
extern  byte *ReadBuffer;
extern  byte *WriteBuffer;
extern  Batch *OutputBuffer;
extern  volatile word head_r,tail_r,sizeMsg,head_w,tail_w;
extern  byte*FAILED;
extern  byte*SUCCESS;

extern  void _pascal __Print(char *frm,...);


/* STATIC*/
static void interrupt (*oldTimer)(...);
static void interrupt newTimer(...);
static void interrupt (*oldTimerGlobal)(...);
static void interrupt newTimerGlobal(...);
static void interrupt (*oldDriver)(...);
static void interrupt newDriver(...);
static bool IniReadPort(void);
static bool CheckOutPort(void);
static void SetZeroParametr(void);
void ClearBuffer(void);

static intnumber[]={0xC,0xB};
static intmask[]={0x10,0x8};
static masksize[]={0x3,0x2,0x1,0x0};
static maskstop[]={0x4,0x0};
static maskparity[]={0x18,0x8,0};
static volatile dword  _NeedSay=0;

/* PUBLIC */
volatile dword __tick,__tickGlobal;
word __portIn;



static word getBaud(dword a)
{
 switch(a){
  case 1200:  a = 96; break;
  case 2400:  a = 48; break;
  case 4800:  a = 24; break;
  case 9600:  a = 12; break;
  case 19200: a = 6;  break;
  case 38400: a = 3;  break;
  case 57600: a = 2;  break;
  case 115200:a = 1;  break;
  default: a = 96; break;
 }
 return (word)a;
}

static bool IniReadPort(void)
{
 word port = __portIn,
 baud,
 mask = masksize[8-_size]|maskstop[2-_stop]|maskparity[2-_parity];
 outp(0x20,0x20); // END INTERRUPT (Может быть был сбой при вызове прерывания)
 __Print("Device terminal port set...");
 disable();
 outp(REI,0); // check port
 if(inp(REI)!=0){
  __Print(FAILED);
  return BAD;
 }
 oldDriver = getvect(intnumber[_portIn]);
 setvect(intnumber[_portIn],newDriver);
 outp(REM,15);// set DTR|CTS|OUT1|OUT2
 outp(0x21,inp(0x21)&(~intmask[_portIn])); //set interrupt
 outp(RCNTRL,mask);                        // set parametr for transsmit
 outp(RCNTRL,inp(RCNTRL)|0x80);            // set baud mode
 baud = getBaud((dword)_baud);
 delay(5);  //
 outp(REI,(baud>>8)&0xFF);                 // high baud
 outp(RWRT,baud&0xFF);                     // low  baud
 outp(RCNTRL,inp(RCNTRL)&(~0x80));         // set normal mode
 outp(REI,3); // only read&write
 enable();
/* read ( i'm not found error,
   but if clear this line "Window 95||Window NT" no read)
*/
 __Print(SUCCESS);
 return OK;
}



byte _pascal getByteW(void)
{
 byte bytes;
 bytes = WriteBuffer[head_w++];
 if(head_w>MAX_BUFFER)head_w=MAX_BUFFER;
 return bytes;
}
void _pascal addByteW(byte a)
{
#ifndef FROMSTDIN
 word port = __portIn;
 WriteBuffer[tail_w++]=a;
 if(tail_w>MAX_BUFFER) tail_w=MAX_BUFFER;
 outp(REI,inp(REI)|(2));
#else
 fwrite(&a,1,1,stderr);
#endif
}

byte _pascal getByteR(void)
{
#ifndef FROMSTDIN
 byte bytes = ReadBuffer[head_r++];
 if(head_r>MAX_BUFFER)head_r=MAX_BUFFER;
#else
 byte bytes;
 fread(&bytes,1,1,stdin);
#endif
 return bytes;
}
void _pascal addByteR(byte a)
{
 ReadBuffer[tail_r++]=a;
 if(tail_r>MAX_BUFFER)tail_r=MAX_BUFFER;
}
#ifndef FROMSTDIN
bool _pascal isFullR(void){ return (tail_r==MAX_BUFFER);}
bool _pascal isFullW(void){ return (tail_w==MAX_BUFFER);}
bool _pascal isEmptyR(void){ return (head_r==tail_r);}
bool _pascal isEmptyW(void){ return (head_w==tail_w);}
#else
bool _pascal isFullR(void){ return (tail_r==MAX_BUFFER);}
bool _pascal isFullW(void){ return (tail_w==MAX_BUFFER);}
bool _pascal isEmptyR(void){ return feof(stdin);}
bool _pascal isEmptyW(void){ return 1;}
#endif
/********************************
This hardware interrupt asynchrone port
*********************************/
static void interrupt newDriver(...)
{
 word port=__portIn;
 int i;
 disable();
 while((i=inp(RII)&0x7)!=1)
 {
  switch(i){
   case 2: if(isFullW()) ClearBuffer();
	   if(!isEmptyW()) outp(RWRT,getByteW());
	   else outp(REI,inp(REI)&(~2)); // disable write symbol
	   break;
   case 4:  i = inp(RRWD);
	    if(isFullR()){
	     ClearBuffer();
	    }
	    addByteR(i);
	    break;
  }
  i = inp(RII);
 }
 outp(0x20,0x20); // END INTERRUPT
}
/*********  END ********************/



/*
SetReadHook   - set port for read and write (terminal work)
SetWriteHook  - check and alloced memory for write
ResetAllHook
SetTimerHook
ResetTimerHook
newTimer
RestartComputer
*/
int SetReadHook(void)
{
 int status = OK;
 __portIn = 0x3F8-0x100*_portIn;
 if(!(ReadBuffer=(byte*)malloc(MAX_BUFFER+1))) status=BAD;
 else
  if(!(WriteBuffer=(byte*)malloc(MAX_BUFFER/2))) status = BAD;
   else if(!IniReadPort()) status=BAD;
	  else SetZeroParametr();
 return status;
}
static void SetZeroParametr(void){ head_r=tail_r=head_w=tail_w=sizeMsg=0;}
void ClearBuffer(void){ head_r=tail_r=head_w=tail_w=0;}

void StopTransmmiting(void)
{
 word port=0x3F8 - 0x100*_portOut;
 outp(REM,inp(REM)&(~ENABLE_RTS)); // 1 = On, 0 = Off
 outp(0x37A,~ENABLE_RTS); // 0 = On, 1 = Off
}

int SetWriteHook(void)
{
 word status = OK,port = 0x3F8-0x100*_portOut;
 outp(REI,0);
 outp(REM,inp(REM)&(~ENABLE_RTS)); // 1 = On, 0 = Off
 outp(0x37A,~ENABLE_RTS); // 0 = On, 1 = Off
 if(inp(REI)!=0) return BAD;
 OutputBuffer = (Batch *)calloc(MAX_BATCH,sizeof(Batch));
 return status;
}

int ResetAllHook(void)
{
 if(ReadBuffer)free(ReadBuffer);
 if(WriteBuffer)free(WriteBuffer);
 setvect(intnumber[_portIn],oldDriver);
 return OK;
}

void SetTimerHook(void)
{
 disable();
 oldTimer = getvect(0x08);
 setvect(0x08,newTimer);
 asm mov al,0x36;
 asm out 0x43,al;
 asm mov cl,100
L1: asm  loop L1
 asm mov al,byte ptr _timer_baud;
 asm out 0x40,al;
 asm mov al,byte ptr _timer_baud+1;
 asm out 0x40,al;
  __tick=0;
 enable();
}
void ResetTimerHook(void)
{
 disable();
 asm mov al,0x36;
 asm out 0x43,al;
 asm mov cl,100
L2: asm  loop L2
 asm mov ax,0xFF //set timer 0xFFFF
 asm out dx,al;
 asm out dx,al;
 setvect(0x08,oldTimer);
 enable();
}
void interrupt newTimer(...)
{
 ++__tick;
 if(++_NeedSay>66){
  oldTimer();
  _NeedSay=0;
 }
 outp(0x20,0x20);
}
void RestartComputer(int status)
{
 void (far*fun)(void)=(void(far*)(void))MK_FP(0xFFFF,0);;
 if(status==LEVEL_1){}
 *(word*)MK_FP(0,0x472)=1234;
 fun(); /* this reset */
}
