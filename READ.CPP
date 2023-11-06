#include "def.h"
#include "key.h"

dword _baud=1200;
word  _stop=1;
word  _size=8;
word  _portIn=0;
word  _portOut=1;
word  _parity=0;
word  _timer_baud=0x3E2;
volatile word  head_r,tail_r,sizeMsg,head_w,tail_w;
byte  *ReadBuffer;
byte  *WriteBuffer;
byte  *SendBuffer;
Batch *OutputBuffer;
word debug_flag=FALSE;
byte*FAILED = "failed\n\r";
byte*SUCCESS = "successful\n\r";
bool invert=FALSE;
static color=0;
bool file_flag;
char file_name[100];

/* FUNCTION */
extern "C++"{
void AnalizeIniString(byte*);
void ReadIniFile(void);
void PrintIniFile(void);
void RestartComputer(int);

int SetReadHook(void);
int SetWriteHook(void);
int SendMessage(int,long,char *,bool,char);
int ResetAllHook(void);
long allMsg=0,okMsg=0;
word _fastcall CRC(byte*,word);
void ClearBuffer(void);
byte _pascal getByteW(void);
void _pascal addByteW(byte);
bool _pascal isEmptyW(void);
byte _pascal getByteR(void);
void _pascal addByteR(byte);
bool _pascal isEmptyR(void);
extern SetTimerHook(void);
extern ResetTimerHook(void);
}

void _pascal __Print(char *frm,...)
{
  char buf[200];
  va_list arg;
  va_start(arg,frm);
  vsprintf(buf,frm,arg);
  if(file_flag){
   FILE *in;
   in = fopen(file_name,"at");
   if(in){
    fprintf(in,buf);
    fclose(in);
   }
  }
  cputs(buf);
  va_end(arg);
}

static word OutToPort(byte*msg)
{
 char cap[9],*ptr,func;
 bool status,inv;
 for(ptr=msg;*ptr&&*ptr!='\x2';ptr++);
 if(!*ptr)return BAD;
 if(strlen(ptr)==1)return OK; // void msg
 inv  = (ptr[1]!='A');
 func=0;
 switch(ptr[3]){
  case 'B': func=1; break;
  case 'C': func=2; break;
  case 'D': func=3; break;
 }
 ptr = ptr+8; // 0000000
 strncpy(cap,ptr,7);
 cap[7]=0;
 __Print("Func (%01d) Invert(%01d)\n\r",func,inv);
 if(debug_flag){
  __Print("Capcode (%07d)\n\r"
	  "#MESSAGE#\n\r",atoi(cap));
  cprintf("%s",ptr+7);
  cprintf("\n\r#END#\n\r");
 }
 textattr(0x10|color++);
 okMsg+=((status=SendMessage(0,atol(cap),ptr+7,inv,func))==OK);
 ++allMsg;
 int _x=wherex(),_y=wherey();
 window(1,4,80,5);
 textattr(0x1F);
 cprintf("Msg:%04lu Ok:%04lu Err:%04lu",allMsg,okMsg,allMsg-okMsg);
 window(1,5,80,25);
 gotoxy(_x,_y);
 return status;
}

word CheckCRC(byte*msg,word crc)
{
 word crc1=CRC(msg,strlen(msg));
 __Print("CRC: Terminal(%u)   Message(%u) : %s\n\r",crc,crc1,
	 (crc1==crc)?"OK":"BAD");
 return (crc1==crc)?OK:BAD;
}

bool ReadMsg(word *crc,dword*start,dword*end)
{
 word bytes=1;
 *start = head_r;
 *crc=0;
 while(bytes!=ETX){
  while(isEmptyR());
  bytes = getByteR();
 }
 *end = head_r;
 for(int i=0;i<2;i++){
  while(isEmptyR());
  *crc|=(getByteR()<<(8*i));
 }
 return OK;
}

void StopTransmmiting(void);

static void EngineWriteOutput(void)
{
 dword start=0,end=0;
 byte  cap[10];
 word  bytes,crc;
 bool read_msg;
 bool exit_from_while=FALSE;
 static countHLT=0;


 while(!isEmptyR()&&!exit_from_while){
  bytes = getByteR();
  switch(bytes){
    case SOH:
	     textattr(0x10|color++);
	     if(color==1)++color;
	     color&=0xF;
	     __Print("Message read...wait\n\r");
	     if((read_msg=ReadMsg(&crc,&start,&end))==BAD){
	      addByteW(ERR);
	      ++allMsg;
	      __Print("Message read(FAILED)\n\r");
	      return;
	     }
	    __Print("Message read(OK)\n\r");
	    exit_from_while=TRUE;
	    countHLT=0;
	    break;
    case HLT:
      if(++countHLT>=10){
       RestartComputer(LEVEL_0);
       countHLT=0;
      }                            break;
    case ENQ: __Print("Command: \"Ready\"\r\n");  addByteW(EOT);
	     countHLT=0;
	     break;
   }//switch
 }//while

 if(read_msg==OK){
  long size=end-start;
  if(!size){
   read_msg=BAD;
   addByteW(ACK);//say ok
   return;
  }
  if(size<0)  size=(long)MAX_BUFFER+size; // carry buffer
  byte *msg = (byte*)calloc(size+7,sizeof(byte));
  if(!msg){
    __Print("msg(%p)\n\r",msg);
    return;
  }
  msg[0]=1;
  for(word i=1;start!=end;i++,start++){
     if(start>MAX_BUFFER)start=0;
     msg[i]= ReadBuffer[start];
  }
  if(CheckCRC(msg,crc)==OK){
    msg[i-1]=0;
    switch(OutToPort(msg)){
     case OK:  addByteW(ACK); break;// say terminal "Ok"
     case ERR: RestartComputer(LEVEL_0);
     case BAD: addByteW(ERR); break;// say terminal "Bad"
    }
   }else addByteW(ERR);
   StopTransmmiting();
   free(msg);
   read_msg=BAD;
 } //if
}


word CheckKeyboard(void)
{
 word bytes,status=0;
#ifdef _DEBUG
 if(!(bytes=getche())) bytes|=(getche()<<8);
 switch(bytes){
  case K_ALT1: addByteW(0x1);break;
  case K_ALT2: addByteW(0x2);break;
  case K_ALT3: addByteW(0x3);break;
  case K_ALT4: addByteW(0x4);break;
  case K_ALT5: addByteW(0x9);break;
  case K_ESC: status = 1; break;
  default : addByteW(bytes);
 }
#endif
return status;
}

void _cdecl main(int argc,char *argv[])
{
 word status;

 for(status=1;status<argc;status++){
  if(argv[status][0]!='-')continue;

  if(!strcmp(argv[status],"-debug")) debug_flag=TRUE;
   else
    if(!strcmp(argv[status],"-invert"))invert=TRUE;
     else
       if(!strcmp(argv[status],"-file")){
	file_flag=TRUE;
	if(argv[status+1]) strcpy(file_name,argv[status+1]);
	else strcpy(file_name,"debug.log");
       }//if
 }

 ReadIniFile();
 PrintIniFile();
 window(1,5,80,25);
 textattr(0x17);
 clrscr();
#ifndef FROMSTDIN
 status = SetReadHook();
#else
 status = OK;
#endif
 __Print("Open terminal port...%s",
	 (status==BAD)?FAILED:SUCCESS);
 if(status==BAD) return;
#ifndef FROMSTDIN
 status = SetWriteHook();
#else
 status = OK;
#endif
 __Print("Open pager port...%s",
	 (status==BAD)?FAILED:SUCCESS);
 if(status==BAD)return;
 __Print("------------ Main loop -----\n\r");
 while(
#ifndef FROMSTDIN
 1
#else
 !feof(stdin)
#endif
 ){
  if(!isEmptyR()){
   textattr(0x17);
   EngineWriteOutput();
  }
#ifdef _DEBUG
  if(kbhit()) if(CheckKeyboard())break;
#endif
 }//while
#ifndef FROMSTDIN
 ResetAllHook();
#else
 status = OK;
#endif
 __Print("\n----------- Exit -----------\n\r");
}

