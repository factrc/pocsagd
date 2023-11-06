#define OUT2FILE
#include "def.h"
#include "key.h"

#define OUT_FILE_DEBUG "c:\\pocsagd.log"

extern volatile dword __tick;
extern word _portOut;
extern Batch *OutputBuffer;
extern bool invert;
extern SetTimerHook(void);
extern ResetTimerHook(void);
extern void _pascal __Print(char *frm,...);
extern RestartComputer(int);

static word OutModemFromDOS(byte*ptr,dword size)
{
#ifndef OUT2FILE
 byte s,mask=ENABLE_RTS,out;
 word port=0x3F8 - 0x100*_portOut;
 word LPTport = 0x378;

 outp(0x37A,ENABLE_RTS); // Output to LPT Enable transsmit
                      //0x2
 outp(REM,inp(REM)|ENABLE_RTS); // 0xFD
 __tick=0;  do{}while(__tick<61);
 for(dword i=0;i<size;i++,ptr++){
   s = *ptr;
  for(word j=0;j<8;j++){
   __tick=0;  do{}while(!__tick);
    out = 1-((s>>7)&1);
    if(invert)out=1-out;
    out |= mask;
    outport(REM,out);
    outp(LPTport,out);
    s<<=1;
  }
 }
 __tick=0;  do{}while(__tick<61);
 outp(0x37A,~ENABLE_RTS); // 0 = On, 1 = Off
 __tick=0;  do{}while(__tick<61);
 outp(REM,inp(REM)&(~ENABLE_RTS)); //clear all flag
#else
 FILE *outf;
 dword i;
 word j;
 byte s,out,d;
 outf = fopen(OUT_FILE_DEBUG,"ab");
 if(!out) return OK;
 for(i=0;i<size;i++,ptr++){
  s = *ptr;
  d=0;
  for(j=0;j<8;j++){
    out = 1-((s>>7)&1);
    if(invert) out=1-out;
/**************/
    d|=out;
    d<<=1;
/**************/
    s<<=1;
  }
  fwrite(&d,1,1,outf);
 }
 fclose(outf);
#endif
 return OK;
}


static dword CalcSizeBatch(byte*msg,dword num)
{
 word s=strlen((byte*)msg)/sizeof(Batch)+1;
 s+=(num&3);
 s+=4;
 return s;
}

static void AddBatch(long sizeBatch)
{
// Batch*buffer = (Batch*)calloc(sizeBatch+1,sizeof(Batch));
// if(!buffer) return NULL;
 for(long j=0;j<=sizeBatch;j++){ // fill batch's synhronize codeword and idle codeword (empty batch)
   OutputBuffer[j].scw = SCW;
  for(long i=0;i<8;i++){
   OutputBuffer[j].frame[i][0] = ICW;
   OutputBuffer[j].frame[i][1] = ICW;
  }
 }
// return buffer;
}

static dword calc_cw(dword cw)
{
 int bit,parity;
 dword local_cw;
/* parity = 0;*/
 local_cw=cw;  /* bch */
/* for(bit=1; bit<=21; bit++, cw <<= 1) if(cw & 0x80000000L) cw ^= 0xED200000L;*/
 for(bit=21; --bit>=0; cw <<= 1) if(cw & 0x80000000L) cw ^= 0xED200000L;
 local_cw |= (cw >> 21);
 cw =local_cw;  /* parity */
/*  for(bit=1; bit<=32; bit++,cw<<=1)   if(cw & 0x80000000L) parity++;*/
 for(parity=0;cw;cw>>=1) parity+=(cw&1);
/* for(bit=32; --bit>=0; cw<<=1)   if(cw & 0x80000000L) parity++;*/
 return local_cw|(parity&1);
}

/*
static long CreateMsg1(dword *msg,int fun)
{
 *msg = 0;
 *msg|=((fun&3)<<11);
}
*/

static long CreateMsg(Batch*msg,byte*str,dword num,char fun)
{
 word c_cw=0,batch1=0,i,frame_adr=num&0x7;
 dword local_cw=0,mask = 0x40000000L;
 byte a;
 msg[batch1].frame[frame_adr][c_cw] = (num>>3)<<13;

/* 0000 0000 0000 0000 000X X000 0000 0000*/
/* 0000 0000 0000 0000 0000 1000 0000 0000*/
/* 0000 0000 0000 0000 0001 0000 0000 0000*/
/* 0000 0000 0000 0000 0001 1000 0000 0000*/
/* 0000 0000 0000 0000 0000 0000 0000 0000*/
 switch(fun){
  case 1: msg[batch1].frame[frame_adr][c_cw] |= 0x00000800L; break;
  case 2: msg[batch1].frame[frame_adr][c_cw] |= 0x00001000L; break;
  case 3: msg[batch1].frame[frame_adr][c_cw] |= 0x00001800L; break;
  default: msg[batch1].frame[frame_adr][c_cw] |= 0x00000000L;
 }



 msg[batch1].frame[frame_adr][c_cw] = calc_cw(
                                      msg[batch1].frame[frame_adr][c_cw]);
 c_cw++;// next codeword
 for(;*str;str++){
  a = *str;//<<1;
  for(i=0;i<7;i++){
   if(a&1)   local_cw|=mask;
   mask>>=1;
   a>>=1;
   if(mask==0x400){
    mask=0x40000000;
    local_cw |=0x80000000;
    msg[batch1].frame[frame_adr][c_cw] = calc_cw(local_cw);
    local_cw = 0;
    if(++c_cw>=2)c_cw=0; /* FIX to   (++c_cw)&=1;  */
    if(!(c_cw&1)){
     if(++frame_adr==8){
      frame_adr=0;
      batch1++;
     }//if(++frame_adr==8){
    }//if(!(c_cw&1)){
   }//if(mask==0x400){
  }//for(i=0;i<7;i++){
 }//for(;*str;str++){
 return batch1+1;
}

/*
FIXED need size+SIZE
*/
                                                // 72*8
static byte*CreateHeaderAddMsg(dword*msg,long size)
{
 unsigned char *head = (unsigned char*)calloc(size,sizeof(unsigned char));
 unsigned char *ptr = head;
 if(!head) return NULL;
 for(long i=0;i<SIZE;i++) *ptr++=0xAA;
 size-=SIZE;
 size>>=2;
 for(i=0;i<size;i++){
  *ptr++= (char)(*msg>>24);
  *ptr++= (char)(*msg>>16);
  *ptr++= (char)(*msg>>8);
  *ptr++= (char)*msg;
  msg++;
 }
 return head;
}

int SendMessage(int cmd,long num,char *str,bool inv,char func)
{
 byte *header;
 dword size,tmp;
 strcat(str,"\x4\x4\x4");
 size = CalcSizeBatch(str,num);
 if(size>MAX_BATCH){
  __Print("Error: calculate batch block\n\r");
  return BAD;
 }
 AddBatch(size);
 size = CreateMsg(OutputBuffer,str,num,func);
 num = size*sizeof(Batch); size += (num+SIZE);
 header = (byte*)CreateHeaderAddMsg((dword*)OutputBuffer,(dword)size);

 if(!header) return BAD;
 __Print("Transmmiting - On\n\rTransfer...wait\n\r");
 invert=inv;
 SetTimerHook();
 size = OutModemFromDOS(header,(long)(size));
 ResetTimerHook();
 invert=FALSE;
 free(header);
 __Print("Transmmiting - Off\n\rTransfer(%s)\n\r",(size==OK)?"OK":"FAILED");
 return size;
}
