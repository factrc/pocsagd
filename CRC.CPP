#include "def.h"
#include "crc.h"

static void _fastcall add_CRC(byte k)
{
 k^=_CRC;
 _CRC>>=8;
 _CRC^=_table_CRC[k];
}

word _fastcall CRC(byte* str,word size)
{
 _CRC= 0;
 while(size-->0) add_CRC(*str++);
 return _CRC;
}
