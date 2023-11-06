#include "def.h"

extern  dword _baud;
extern  word  _stop;
extern  word  _size;
extern  word  _portIn;
extern  word  _portOut;
extern  word  _parity;
extern  word  _timer_baud;//=DELAY;
extern void _pascal __Print(char *frm,...);



const byte* NameIniFile="terminal.ini";


/**********************************************
***********************************************

Function load parameter from ini files

***********************************************
***********************************************/
void AnalizeIniString(byte*string)
{
 byte cmd[20],option[30];
 word i;

 i=0;
 while(*string&&*string!='='&&i!=20) cmd[i++]=*string++;
 i=cmd[i]=0;
 while(*string&&i!=30)option[i++]=*(++string);
 option[i]=0;
 if(!strcmp(cmd,"baud")){
  _baud = atol(option);
 }else if(!strcmp(cmd,"stop")){
  _stop = atoi(option);
 }else if(!strcmp(cmd,"size")){
  _size = atoi(option);
 }else if(!strcmp(cmd,"in")){
  strlwr(option);
  _portIn = (!strcmp(option,"com1"))?0:1;
 }else if(!strcmp(cmd,"out")){
  strlwr(option);
  _portOut = (!strcmp(option,"com1"))?0:1;
  _portOut = (_portIn==_portOut)?(1-_portIn):_portOut;
 }else if(!strcmp(cmd,"parity")){
  _parity = atoi(option);
 }else if(!strcmp(cmd,"baudOut")){
  i = atoi(option);
  if(!i)i=1200;
  _timer_baud = 1193181/(long)i;
 }
}
//

void ReadIniFile(void)
{
 word bytes;
 byte string[255];
 FILE *in = fopen(NameIniFile,"rt");

 if(!in){
  printf("---------------Error----------------\n"
	 "No found \"terminal.ini\"\nSet default value\n"
	 "------------------------------------\n");
  return;
 }
 while(!feof(in)){
  bytes=getc(in);
  switch(bytes){
   case '#': while(!feof(in)&&getc(in)!='\n');break;
   default:  string[0]=bytes;
	     fscanf(in,"%200s",string+1);
	     AnalizeIniString(string);
	     while(!feof(in)&&getc(in)!='\n');break;
  }
 }
 fclose(in);
}

const byte *_par[]={"none","1","2","3"};
void PrintIniFile(void)
{
 clrscr();
 window(1,1,80,4);
 textattr(0x4F);
 clrscr();
 printf( "InPort(%s),"
	 "SpeedInPort(%lu),"
	 "Stop(%d),"
	 "Size(%d),"
	 "Parity(%s)\n\r"
	 "OutPorts(%s(DTR,RTS-ON TRS),LPT(pin1-data0,pin14-ON TRS)),"
	 "SpeedOutPorts(%u)\n\r",
	 (_portIn)?"COM2":"COM1",
	 _baud,_stop,_size,_par[_parity],
	 (_portOut)?"COM2":"COM1",unsigned(1193181L/_timer_baud));
 textattr(0x4E);
 cprintf("                          TERMINAL PAGER SYSTEM\r\n");
 textattr(0x1F);
 for(int i=0;i<79;i++) cputs("=");
}
