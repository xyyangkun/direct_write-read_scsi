/**************************************************************************
 * 文件名: mshmpool.h
 * 功能      : 用共享内存实现的缓冲池  ,可以由不同进程
 			  实现生产者和消费者
 *************************************************************************/
#ifndef MSHMPOOL_H
#define MSHMPOOL_H
#ifndef _WIN32

#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/sem.h>
#else
#define size_t unsigned long
#define key_t  unsigned long
#endif

////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////
//当修改共享缓冲区中的数据时请注意，修改后如果
//在设备上运行使用新的gtipc的应用程序有可能会出现
//段错误的情况，这主要是由于在gtvs1000/3000设备上已
//经存在一个值为key的共享缓冲区，但是结构不一样
//了，所以会出现段错误，如果在gtvs3000设备上可以用
//ipcrm  和 ipcs 关闭已经申请的共享缓冲区，然后再运行
//新的应用程序。由于gtvs1000上没有这两个应用程序建
//议重新启动设备并确保首次运行新版的应用程序.
//zw-add 2011-11-08
////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////

#include "mshm.h"
#define 	SHPOOL_BLOCK_SIZE		512		//缓冲池中的块容量
#define	SHPOOL_MAX_USERS		8		//该缓冲池支持的最大用户数(信号量集中最大的信号量数目)
										//也就是本缓冲池库最大支持的用户数
//#define SHPOOL_MAX_ELE			400	//100//缓冲池可以容纳的最大元素个数
//SHPOOL_MAX_ELE 这个值最好大于共享缓冲池所能存放的最多元素 的个数，
//这样设置的好处是可以避免在缓冲池内重复出现多次相同元素的序号
#define SHPOOL_MAX_ELE				5000	//增大元素个数是为了保存更多的帧数，供录像回放使用 zw-modified-20110720
								
typedef struct{
//用来描述元素的结构
	int num;					//序号 0~0xffffffff循环
	long userflag;			//用户自定义的标记
	long datalen;				//元素有效数据长度
	int blocks;				//元素数据占用的总块数
	int bblock;				//元素数据占用的起始块
	int eblock;				//元素数据占用的结束块
}SHPOOL_ELE;

typedef union{
	unsigned char data[SHPOOL_BLOCK_SIZE];
}SHPOOL_BLOCK;

#define  	MSHMPOOL_LOCAL_USR	0	//本地用户
#define	MSHMPOOL_NET_USR		1	//网络用户
typedef struct{
	//缓冲池用户信息结构
	int	 	valid;			//用户有效标志0代表空闲1代表已使用
	int		curele;			//当前可以访问的元素序号
	pid_t 	upid;			//用户的进程号
	time_t	stime;			//用户连接开始时间//秒为单位
	char 	name[32];		//用户名最大31字节(使用缓冲池的进程描述)
	int		type;			//用户类型 MSHMPOOL_LOCAL_USR:MSHMPOOL_NET_USR
	char  	info[128];		//用户描述信息,每个用户信息最长64字节
}SHPOOL_USR;

typedef struct{
	int maxsem;								//缓冲池允许的用户缓存信号量数
											//如果信号量超过这个值则认为
											//用户可能已经意外退出
	int num;									//缓冲池中最新元素的序号
	int tblocks;								//缓冲池中的总块数
	int fblocks;								//当前的剩余块数
	int fbstart;								//空闲块的起始编号
//	int maxele;								//对多可容纳的元素个数
	int boffset;								//元素数据相对于对ph首地址的偏移
	int count;								//缓冲池当前有效元素个数
	int head;									//第一个有效元素(最老的元素)
	int tail;									//可以填充元素的空白空间
	key_t semkey;							//用于通讯的信号量的key,这个key是共享内存
											//用到的信号量的key|0x50000000
	int semid;								//用于通讯的信号量
	int last_usr_no;							//最后一个用户的序号负值表示还从来没有过用户
//用于统计数据流量用到的变量
	unsigned long		send_bytes;						//总共发送的字节数
///////
	char	pinfo[512];							//缓冲池用到的数据信息，具体结构提供给上层应用自己定义
											//缓冲池提供给应用程序存放信息的空间
	
	SHPOOL_ELE	eles[SHPOOL_MAX_ELE];		//元素头存储区
	SHPOOL_USR	users[SHPOOL_MAX_USERS];	//存放用户信息
}SHPOOL_HEAD;

#define	MSHM_POOL_NORMAL_USR	0	//普通用户
#define	MSHM_POOL_CREATER		1	//创建者
#define	MSHM_POOL_LOOKER			2	//察看者
typedef struct{
	int				userflag;	//用户标志 0:普通用户  1:创建者 2:察看者(不能从池中接收信号量)
	int				userno;		//用户编号
	MEM_CORE		*mc;		//存放缓冲池的内存
	SHPOOL_HEAD 	*ph;		//存放缓冲池的头的地址
	SHPOOL_BLOCK 	*eledata;	//存放元素数据的地址
}MSHM_POOL;
/**************************************************************************
  *函数名	:MShmPoolMvUsrPlace
  *功能	:将用户当前读取数据的指针按要求移动
  *参数	: pool:指向缓冲池的指针 
  			  place:要移动的数量
  			  	负值表示向前移动|place|格
  			  	正值表示向后移动place格
  			  	0表示不用移动
  *返回值	:返回0表示成功 负值表示失败错误是负的errno
  *************************************************************************/
int MShmPoolMvUsrPlace(MSHM_POOL *pool,int place);

/**************************************************************************
  *函数名	:MShmPoolGetResource
  *功能	:从缓冲池中取出一个元素放入缓冲区
  *参数	: pool:指向缓冲池的指针 
  			  buf:目标数据缓冲区的首地址
  			  buflen:缓冲区的长度
	输出   :eleseq:元素的序号
			 flag:元素的标志
  *返回值	:成功则返回拷贝的字节数(元素的有效长度)
  			 若用户缓冲区大小不够则返回 -ENOMEM
  *************************************************************************/
int MShmPoolGetResource(MSHM_POOL *pool,void *buf,int buflen,int *eleseq,int *flag);

/**************************************************************************
  *函数名	:MShmPoolAddResource
  *功能	:将指定缓冲区的数据作为元素放入缓冲池尾部
  *参数	: pool:指向缓冲池的指针 
  			  buf:源数据缓冲区的首地址
  			  buflen:缓冲区的有效数据长度
			 flag:要加入的元素的标志
  *返回值	:返回0表示成功 负值表示出错
  *
  *************************************************************************/
 int MShmPoolAddResource(MSHM_POOL *pool,void *buf,int buflen,int flag);

/**************************************************************************
  *函数名	:MShmPoolCreate
  *功能	:按照给定的pkey创建指定大小的共享缓冲池
  *参数	: name:缓冲池的名字
  			  pkey: 缓冲池的key(本函数将以这个key创建共享内存
  			           互斥锁信号量,并以(key|0x50000000)创建资源信号量)
  			  bytes:希望创建 的缓冲区中的实际数据容量
  *输出	:本函数将初始化pool结构中的字段
  *返回值	:返回0表示成功 负值表示失败错误是负的errno
  *
  *************************************************************************/
int MShmPoolCreate(char *name,MSHM_POOL *pool,key_t pkey,int bytes);

/**************************************************************************
  *函数名	:MShmPoolSetMaxNum
  *功能	:设置信号缓冲池中判断用户超时的元素数
  *参数	: pool:指向缓冲池的指针 
  			  num: 判断超时的元素个数
  *返回值	:返回0表示成功 负值表示失败错误是负的errno
  *                   本函数由调用MShmPoolCreate的进程调用,当用户有超过
  *                   num个元素没有读取时将会认为用户已经退出，踢出用户
  *************************************************************************/
int MShmPoolSetMaxNum(MSHM_POOL *pool,int num);


/**************************************************************************
  *函数名	:MShmPoolSetMaxNum_t
  *功能	:设置信号缓冲池中判断用户超时的元素数
  *参数	: num: 判断超时的元素个数
  *返回值	:返回0表示成功 负值表示失败错误是负的errno
  *                   本函数由调用MShmPoolCreate的进程调用,当用户有超过
  *                   num个元素没有读取时将会认为用户已经退出，踢出用户
  *************************************************************************/
void MSHmPoolSetMaxNum_t(int num);

/**************************************************************************
  *函数名	:MSHmPoolAttach
  *功能	:按照给定的pkey连接到缓冲池
  *参数	: pkey:要连接的 缓冲池的key
  *输出	:本函数将初始化pool结构中的字段
  *返回值	:返回0表示成功 负值表示失败错误是负的errno
  *注		:本函数主要用于监控缓冲池的运行状态的程序
  *************************************************************************/
int MSHmPoolAttach(MSHM_POOL *pool,key_t pkey);

/**************************************************************************
  *函数名	:MShmPoolReq
  *功能	:按照给定的pkey请求一个缓冲池,将结果填充到pool
  *参数	: pkey:要请求的 缓冲池的key
  			  name:请求内存池的用户名
  			  type:用户类型由上层函数传下来
  *输出	:本函数将初始化pool结构中的字段
  *返回值	:返回0表示成功 负值表示失败错误是负的errno
  *************************************************************************/
int MShmPoolReq(MSHM_POOL *pool,key_t pkey,char *name,int type);

/**************************************************************************
  *函数名	:MShmPoolReqFree
  *功能	:释放已经申请共享缓冲池
  *参数	: pool:缓冲池结构指针
  *返回值	:返回0表示成功 负值表示失败错误是负的errno
  *************************************************************************/
int MShmPoolReqFree(MSHM_POOL *pool);

/**************************************************************************
  *函数名	:MShmPoolGetUsrInfo
  *功能	:获取指定用户号的用户信息并将地址放入info中
  *参数	: pool:缓冲池结构指针
  			: userno 用户号
  *输出	: info 返回时将info中填入缓冲区的地址
  *返回值	:返回负值表示出错，正值表示info缓冲区的大小
  *注		:info缓冲区中的信息内容及结构是由上层应用程
  			 序定义的
  			 在需要时由上层函数负责锁内存
  *************************************************************************/
int MShmPoolGetUsrInfo(MSHM_POOL *pool,int userno,void **info);

/**************************************************************************
  *函数名	:MshmPoolGetTotalUser
  *功能	:获取当前使用缓冲池的用户总数
  *参数	: pool:缓冲池结构指针
  *返回值	:返回缓冲池的用户总数,负值表示出错
  *************************************************************************/
int MShmPoolGetTotalUser(MSHM_POOL *pool);
/**************************************************************************
  *函数名	:MShmPoolGetInfo
  *功能	:获取当前使用缓冲池的应用自定义信息
  *参数	: pool:缓冲池结构指针
  *返回值	:返回应用自定义的信息地址
  *			如果需要则上层调用应负责将内存加锁
  *************************************************************************/
void *MShmPoolGetInfo(MSHM_POOL *pool);
/**************************************************************************
  *函数名	:GetUsrValid
  *功能	:获取当前用户缓冲池是否仍然有效标志
  *参数	: pool:缓冲池结构指针
  *返回值	:返回负值表示出错，0表示无效 1表示有效

  *************************************************************************/
int GetUsrValid(MSHM_POOL *pool);
/**************************************************************************
  *函数名	:MShmPoolReActiveUsr
  *功能	:重新激活用户,如果用户长时间不能接收数据
  			:则缓冲池发送程序会把认为它退出了,所以接收者应
  			 定期调用此函数,重新激活用户(防止意外)
  *参数	: pool:缓冲池结构指针
  *返回值	:0表示正确 负值表示错误 1表示用户已经被删除，
  			现在重新激活
  *注		:本函数不会锁共享内存			
  *************************************************************************/
int	MShmPoolReActiveUsr(MSHM_POOL *pool);

/**************************************************************************
  *函数名	:MShmEleRemain
  *功能	:返回还有多少元素没有读取
  *参数	: pool:指向缓冲池的指针 
  *返回值	: 当前用户还有多少数据没有读取
  *注		:pool的合法性由调用者保证
  *************************************************************************/
static __inline__ int MShmEleRemain(MSHM_POOL *pool)
{
	SHPOOL_HEAD     	*ph=pool->ph;
	SHPOOL_USR	 	*usr=&ph->users[pool->userno];
	SHPOOL_ELE	      	*ele=&ph->eles[usr->curele];
	if(usr->curele==ph->tail)
		return 0;		//FIXME 当num超过unsigned long 表示的范围时应特殊处理
        if(ph->num>=ele->num)
            return (ph->num-ele->num);	
        else
            return (ph->num);//FIXME
}


#endif





