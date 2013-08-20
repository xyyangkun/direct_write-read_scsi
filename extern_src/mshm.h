/**************************************************************************
 * 文件名 : mshm.h
 * 功能	: 将共享内存的调用接口进行包装，提供易于使
 			   用的接口   
 *************************************************************************/
#ifndef MSHM_H
#define MSHM_H
#include <stdlib.h>

/*
 * 描述共享内存的结构
 */
typedef struct 
{
//本结构定义的所有字段都是所有放在共享内存开头
//供所有进程共享的
   unsigned long        magic;          //共享内存标志,创建时付值0x5500aa00表示正常，否则表示出现异常
   size_t      	tsize;		//共享内存总的大小
   size_t       	usize;		//用户程序需要的内存大小
   key_t		mkey;		//共享内存的key
   int			memid;		//共享内存的句柄
   int			semid;		//互斥锁的句柄
   long		uoffset;		//用户可用的空间偏移地址
   char		name[64];	//共享内存的名字
}MEM_CORE;









/**************************************************************************
  *函数名	:MShmCoreCreate
  *功能	:以给定的key和usersize创建共享内存
  *参数	: mkey:共享内存的key 
  *			  usersize:用户程序需要的空间
  *			  name:要创建的共享内存的名字
  *                   u_ptr:输出值,填充用户可用的内存区域起始地址,NULL表示不需要填充
  *返回值	: 返回NULL表示有错误 错误码存在errno
  *			  返回连接好的内存地址MemCore指针,
  *			  以后用这个指针作为参数调用其它函数
  *注:本函数并不会把分配到的内存初始化为0
  *************************************************************************/
MEM_CORE * MShmCoreCreate(char *name,key_t mkey,size_t usersize,void **u_ptr);

/**************************************************************************
  *函数名	:MShmCoreAttach
  *功能	:按照给定的key打开并连接共享内存
  *参数	: mkey:共享内存的key 
  *                   u_ptr:输出值,填充用户可用的内存区域起始地址,NULL表示不需要填充
  *返回值	: 返回NULL表示有错误 错误码存在errno ENOENT表示该
  				key的共享内存还没有被创建
  			  返回连接好的内存地址MemCore指针,
  			  以后用这个指针作为参数调用其它函数
  *************************************************************************/
MEM_CORE * MShmCoreAttach(key_t mkey,void **u_ptr);
/**************************************************************************
  *函数名	:MShmCoreDetach
  *功能	:退出到指定共享内存的连接
  *参数	: mc:共享内存的首地址
  *返回值	: 0表示成功
  *************************************************************************/
int MShmCoreDetach(MEM_CORE *mc);

/**************************************************************************
  *函数名	:MShmLock
  *功能	:取得对指定共享内存的访问权
  *参数	: mc :共享内存的起始地址(由MShmCoreCreate或MShmCoreAttach返回)
  *返回值	: 0表示成功 负值表示错误 错误码在errno中
  *************************************************************************/
int MShmLock(MEM_CORE *mc);

/**************************************************************************
  *函数名	:MShmLock
  *功能	:释放对指定共享内存的访问权
  *参数	: mc :共享内存的起始地址(由MShmCoreCreate或MShmCoreAttach返回)
  *返回值	: 0表示成功 负值表示错误 错误码在errno中
  *************************************************************************/
int MShmUnLock(MEM_CORE *mc);




#endif

