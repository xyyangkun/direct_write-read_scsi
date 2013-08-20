/*
	һЩ�����������Ͷ���
*/
#ifndef _TYPE_DEFINE_H
#define _TYPE_DEFINE_H

#ifndef _WIN32			//add by scott
#define BYTE    unsigned char
#define WORD    unsigned short
#define DWORD   unsigned long
#endif //_WIN32
#define U32     unsigned long
#define S8      char
#define S16     short
#define S32     long

#if 0 
//#define int8_t   char
#define uint8_t  unsigned char
#define int16_t  short
#define uint16_t unsigned short
//#define int32_t  int
#define uint32_t unsigned int
#define int64_t  long long 
//__int64
#define uint64_t unsigned long long
#endif
//__int64

#define CACHE_LINE  16
#define ptr_t uint32_t



//����һλ
#define 	set_bit(val,bit) 		val|=(1<<(bit))
//���һλ
#define	clr_bit(val,bit)		val&=~(1<<(bit))

#define	SetBit		set_bit
#define	ClrBit		clr_bit
//�ж��Ƿ���λ
#define   IfBitSet(val,bit)		if(val&(1<<bit))
//�ж��Ƿ����λ
#define	IfBitClr(val,bit)		if(!(val&(1<<bit)))

#endif

