/**************************************************************************
 * �ļ���: mshmpool.c
 * ����      : �ù����ڴ�ʵ�ֵĻ����  ,�����ɲ�ͬ����
 			  ʵ�������ߺ�������
 *************************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <unistd.h>
#include <string.h>
#include <pthread.h>
#include "mshm.h"
#include "mshmpool.h"
#include "msem.h"
#include <syslog.h>

static int newmaxsem=-1;
static int oldmaxsem;
static int places=0;

/**************************************************************************
  *������	:NewerEle
  *����	:���ر�cur���µ�Ԫ�����
  *����	: pool:ָ�򻺳�ص�ָ�� 
  			  cur  :��ǰԪ�����
  *����ֵ	: ��ǰԪ�ص���һ��Ԫ����� ,��ֵ��Զ�Ϸ�
  *ע		:pool�ĺϷ����Լ�cur�ĺϷ����ɵ����߱�֤
  *************************************************************************/
static __inline__ int NewerEle(MSHM_POOL *pool,int cur)
{
	if(++cur>=SHPOOL_MAX_ELE)
		cur=0;
	return cur;
}

/**************************************************************************
  *������	:OlderEle
  *����	:���ر�cur���ϵ�Ԫ�����
  *����	: pool:ָ�򻺳�ص�ָ�� 
  			  cur  :��ǰԪ�����
  *����ֵ	: ��ǰԪ�ص���һ��Ԫ����� ,��ֵ��Զ�Ϸ�
  *ע		:pool�ĺϷ����Լ�cur�ĺϷ����ɵ����߱�֤
  *************************************************************************/
static __inline__  int OlderEle(MSHM_POOL *pool,int cur)
{
	if(--cur<0)
		cur=SHPOOL_MAX_ELE-1;
	return cur;
}

/**************************************************************************
  *������	:MShmDistanceEle
  *����	:������Ԫ�صľ���
  *����	: pool:ָ�򻺳�ص�ָ�� 
  			  a     :��һ��Ԫ�����
  			  b     :�ڶ���Ԫ�����
  *����ֵ	: ����Ԫ�صľ���,�߼����൱��b-a ,0��ʾ�����ֵ��ʾb>a
  *ע		:pool�ĺϷ����Լ�a,b�ĺϷ����ɵ����߱�֤
  *************************************************************************/
static __inline__ int MShmDistanceEle(MSHM_POOL *pool,int a,int b)
{
	SHPOOL_HEAD	*ph=pool->ph;
	int head=ph->head;
	//int tail=ph->tail;
	int distb=0;//����
	int dista=0;
	if(a==b)
		return 0;
	if(b>=head)
		distb=b-head;
	else
		distb=SHPOOL_MAX_ELE-b+head;

	if(a>=head)
		dista=a-head;
	else
		dista=SHPOOL_MAX_ELE-a+head;
	return (distb-dista);
}
#if 0
/**************************************************************************
  *������	:MShmEleRemain
  *����	:���ػ��ж���Ԫ��û�ж�ȡ
  *����	: pool:ָ�򻺳�ص�ָ�� 
  *����ֵ	: ��ǰ�û����ж�������û�ж�ȡ
  *ע		:pool�ĺϷ����ɵ����߱�֤
  *************************************************************************/
static __inline__ int MShmEleRemain(MSHM_POOL *pool)
{
	SHPOOL_HEAD     	*ph=pool->ph;
	SHPOOL_USR	 	*usr=&ph->users[pool->userno];
	SHPOOL_ELE	      	*ele=&ph->eles[usr->curele];
	if(usr->curele==ph->tail)
		return 0;		//FIXME ��num����unsigned long ��ʾ�ķ�ΧʱӦ���⴦��
	return (ph->num-ele->num);	
}

#endif
/**************************************************************************
  *������	:SemAddAll
  *����	:�򻺳���������û�������Դ�ź���
  *����	: pool:ָ�򻺳�ص�ָ��
  *����ֵ	:��
  *ע:Ҫ����һ�㺯���������ڴ�,�����Ϸ�������һ�㺯
  		��������
  *************************************************************************/
static __inline__ void SemAddAll(MSHM_POOL *pool)
{
	int i;
	int rc;
	int SemVal;
	MEM_CORE 		*mc=pool->mc;
	SHPOOL_HEAD 	*ph=pool->ph;
	SHPOOL_USR   	*usr=ph->users;
	int			  	semid=ph->semid;

	
	if(newmaxsem!=-1)
	{
		ph->maxsem=newmaxsem;
	}
	else
	{
		ph->maxsem=oldmaxsem;
	}


	for(i=0;i<SHPOOL_MAX_USERS;i++)
	{
		if(usr->valid)
		{
			rc=SemAdd(semid,i);
			SemVal=SemGetVal(semid,i);
			//yk del 20130417 printf("Semgetval[%d]=====[%d]\n",i,SemVal);
			if(SemVal>ph->maxsem)
			{
				//yk del 20130417 printf("%s user:%d(%s) maybe exited(%d) ,delete it!\n",mc->name,i,usr->name,SemVal);
				if((SemVal)==(ph->maxsem+1))
				{
					syslog(LOG_INFO,"%s user:%d(%s) maybe exited(%d) ,delete it!\n",mc->name,i,usr->name,SemVal);
				}
				usr->valid=0;//�û������Ѿ���������,ɾ���û�
			}
			if(rc<0)
			{
				printf("SemAddAll:SemAdd %d:%d --errno=%d %s  cur cnt=%d err\n",semid,i,rc,strerror(-rc),SemGetVal(semid,i));
				if(rc==-ERANGE)
				{
					//yk del 20130417printf("%s user:%d maybe exited ,delete it!\n",mc->name,i);
					usr->valid=0;//�û������Ѿ���������,ɾ���û�
					
				}
				
			}
		}
		usr++;
			
	}
	return ;
}

/**************************************************************************
  *������	:GetAFreeUsrNo
  *����	:��ȡһ�����е��û����
  *����	: pool:ָ�򻺳�ص�ָ�� 
  *����ֵ	: ��ֵ��ʾ���� ������ʾ���õ��û����
  *ע		:pool�ĺϷ����ɵ����߱�֤
  *************************************************************************/
 static int GetAFreeUsrNo(MSHM_POOL *pool)
{//�û���ÿ�ζ���ݼ�
	int i;
	SHPOOL_HEAD *ph=pool->ph;
	SHPOOL_USR   *usr=NULL;
	if(ph->last_usr_no<0)
	{
		ph->last_usr_no=0;		
	}
	for(i=(ph->last_usr_no+1);i<SHPOOL_MAX_USERS;i++)
	{
		usr=&ph->users[i];
		if(!usr->valid)
		{
			ph->last_usr_no=i;
			return i;
		}
		//usr++;
	}
	for(i=0;i<=ph->last_usr_no;i++)
	{
		usr=&ph->users[i];
		if(!usr->valid)
		{
			ph->last_usr_no=i;
			return i;
		}
		//usr++;	
	}
	return -EBUSY;	
}
#if 0
//�ͷ�һ���û���ָ�����û�
//����һ�㸺���������ڴ�
static void FreeAUsrNo(MSHM_POOL *pool,int no)
{
	SHPOOL_HEAD *ph=pool->ph;
	SHPOOL_USR   *usr=ph->users;
	usr+=no;
	usr->valid=1;
	usr->upid=0;
}
#endif


int MShmPoolSetUserName(MSHM_POOL *pool,int userno,char *name)
{
	SHPOOL_HEAD	*ph;
	SHPOOL_USR   	*usr;
	if((pool==NULL)||(userno<0)||(userno>=SHPOOL_MAX_USERS)||(name==NULL))
		return -EINVAL;
	ph=pool->ph;
	if(ph==NULL)
		return -EINVAL;
	usr=&ph->users[userno];
	strncpy(usr->name,name,(sizeof(usr->name)-1));
	usr->name[sizeof(usr->name)-1]='\0';
	return 0;
}
//��ȡָ���û��ŵ��û��� ������ַ����name��
int MShmPoolGetUsrName(MSHM_POOL *pool,int userno,char **name)
{
	SHPOOL_HEAD	*ph;
	SHPOOL_USR   	*usr;
	if((pool==NULL)||(userno<0)||(userno>=SHPOOL_MAX_USERS)||(name==NULL))
		return -EINVAL;	
	if(ph==NULL)
		return -EINVAL;
	usr=&ph->users[userno];
	*name=usr->name;
	return 0;
}

/**************************************************************************
  *������	:FreeBlocks
  *����	:�ͷ����ϵ�ָ�������Ŀ�
  *����	: pool:ָ�򻺳�ص�ָ�� 
  			  needfree:��Ҫ�ͷŵĿ���
  *����ֵ	: �����ͷŵĿ���
  *ע		:pool�ĺϷ������ɵ����߱�֤
  *************************************************************************/
static __inline__  int FreeBlocks(MSHM_POOL *pool,int needfree)
{
	SHPOOL_HEAD	*ph=pool->ph;
	SHPOOL_ELE		*ele;
	int head;
	int freecnt=ph->fblocks;

	while(freecnt<needfree)
	{
		if(ph->count>0)
		{			
			head=ph->head;
			ele=&ph->eles[head];
			freecnt+=ele->blocks;
			ph->fblocks+=ele->blocks;
			ph->count--;
			ele->datalen=0;
			ele->blocks=0;
			ph->head=NewerEle(pool,ph->head);
		}
		else
		{//û���㹻�Ŀռ䣬�����������ͷŵĿ�̫����
			return -ENOMEM;
		}
	}
	return freecnt;
	
}
//�ͷŻ����ͷԪ��
//�����ͷŵĿ���
static __inline__  int FreeHeadEle(MSHM_POOL *pool)
{
	SHPOOL_HEAD	*ph=pool->ph;
	SHPOOL_ELE		*ele;
	int head;
	int freecnt;
	
	head=ph->head;
	ele=&ph->eles[head];
	freecnt=ele->blocks;
	ph->fblocks+=ele->blocks;
	ph->count--;
	ele->datalen=0;
	ele->blocks=0;
	ph->head=NewerEle(pool,ph->head);
	return freecnt;
	
}
/**************************************************************************
  *������	:MShmPoolReqBlocks
  *����	:������е��ڴ�����
  *����	: pool:ָ�򻺳�ص�ָ�� 
  			  need:��Ҫ�Ŀ���
  *����ֵ	: ���ؿ��п����ʼ���
  *ע		:Ϊ���Ч�� pool��need�����ĺϷ���Ӧ���ϲ㺯����֤
  			 Ҫ����һ�㺯���������ڴ�
  *************************************************************************/
static __inline__ int MShmPoolReqBlocks(MSHM_POOL *pool,int need)
{
	SHPOOL_HEAD	*ph;
	int ret;
	ph=pool->ph;

	do
	{		
		if(ph->fblocks>need)
		{
			break;
		}
		ret=FreeBlocks(pool,need*2);//�ͷ����������Ŀ�
		if(ret>0)
			break;
		else
		{
			return -ENOMEM;	
		}
	}while(0);
	ret=ph->fbstart;
	ph->fbstart=(ph->fbstart+need)%(ph->tblocks);	
	return ret;
}

/**************************************************************************
  *������	:InsertData2Ele
  *����	:�����ݷ��뻺��ص�ָ��Ԫ����(Ԫ�ر����Ѿ������)
  *����	: pool:ָ�򻺳�ص�ָ�� 
  			  ele  :ָ��Ҫ�������ݵ�Ԫ��
  			  buf:���ݻ��������׵�ַ
  			  len:buf����Ч���ݵĳ���
  *����ֵ	:����0��ʾ�ɹ�
  *ע		:������Ч������һ������߱�֤
  			 Ҫ����һ�㺯���������ڴ�
  *************************************************************************/
static __inline__ int InsertData2Ele(MSHM_POOL *pool,SHPOOL_ELE *ele,void *buf,int len)
{
	SHPOOL_HEAD	*ph        = pool->ph;
	SHPOOL_BLOCK 	*dblock = pool->eledata;
	int 				startblock=ele->bblock;
	int 				blocks=ele->blocks;
	int				tailblocks=ph->tblocks-startblock;	//β��ʣ��Ŀ��п�
	int				tailbytes=tailblocks*SHPOOL_BLOCK_SIZE;
	if(tailbytes>=len)
	{
		memcpy((void*)&dblock[startblock],buf,len);
	}
	else
	{
		memcpy((void*)&dblock[startblock],buf,tailbytes);
		memcpy((void*)&dblock[0],(buf+tailbytes),(len-tailbytes));
	}
	ele->datalen=len;
	ph->count++;
	ph->fblocks-=blocks;

	ph->send_bytes+=len;	//��������

	return 0;
}

/**************************************************************************
  *������	:SaveEle2Buf
  *����	:������ص�ָ��Ԫ���������ݿ�����������
  *����	: pool:ָ�򻺳�ص�ָ�� 
  			  buf:Ŀ�����ݻ��������׵�ַ
  			  buflen:�������ĳ���
  			  eleno:Ҫ������Ԫ�����
	���   :eleseq:Ԫ�ص����
			 flag:Ԫ�صı�־
  *����ֵ	:�ɹ��򷵻ؿ������ֽ���(Ԫ�ص���Ч����)
  			 ���û���������С�����򷵻� -ENOMEM
  *ע		:������Ч������һ������߱�֤
  			 Ҫ����һ�㺯���������ڴ�
  *************************************************************************/
static __inline__ int SaveEle2Buf(MSHM_POOL *pool,void *buf,int buflen,int eleno,int *eleseq,int *flag)
{
	SHPOOL_HEAD	*ph=pool->ph;	
	SHPOOL_ELE		*ele=&ph->eles[eleno];
	int 				 startblock=ele->bblock;
	int				tailblocks=ph->tblocks-startblock;	//β��ʣ��Ŀ��п�
	int				tailbytes=tailblocks*SHPOOL_BLOCK_SIZE;
	int				elelen=ele->datalen;//Ԫ��ʵ�ʳ���
	void				*cptr=NULL;			//�������ݵ���ʼ��ַָ��
	if(buflen<elelen)
	{
		printf("buflen=%d elen=%d!!!\n",buflen,elelen);
		return -ENOMEM;
	}
	cptr=&pool->eledata[startblock];
	if(elelen<=tailbytes)
	{
		memcpy(buf,cptr,elelen);
	}
	else
	{
		memcpy(buf,cptr,tailbytes);
		memcpy(buf+tailbytes,(void*)pool->eledata,(elelen-tailbytes));
	}
	*eleseq=ele->num;
	*flag=ele->userflag;
	return elelen;
}











/*************************���º�������Ҫ������****************************/

/**************************************************************************
  *������	:MShmPoolGetResource
  *����	:�ӻ������ȡ��һ��Ԫ�ط��뻺����
  *����	: pool:ָ�򻺳�ص�ָ�� 
  			  buf:Ŀ�����ݻ��������׵�ַ
  			  buflen:�������ĳ���
	���   :eleseq:Ԫ�ص����
			 flag:Ԫ�صı�־
  *����ֵ	:�ɹ��򷵻ؿ������ֽ���(Ԫ�ص���Ч����)
  			 ���û���������С�����򷵻� -ENOMEM
  *ָ������Ŀ�ָ�����Ӧ����һ�㱣֤,�������ΪNULL�����δ���
  *************************************************************************/
int MShmPoolGetResource(MSHM_POOL *pool,void *buf,int buflen,int *eleseq,int *flag)
{
	SHPOOL_HEAD	*ph;
	MEM_CORE		*mc;
	SHPOOL_USR		*usr;
	int cur;	//ͷ��β����ǰλ�õ����
	int rc;
	int cnt=0;

	if(buflen<0)	//����ָ������Ŀ�ָ�����Ӧ����һ�㱣֤,�������ΪNULL�����δ���
		return -EINVAL;
	mc=pool->mc;
	ph=pool->ph;
	usr=&ph->users[pool->userno];
	cur=usr->curele;
	if(mc==NULL)
		return -EINVAL;

	
	//while((ph->tail==ph->head)||(cur==ph->tail)||(abs(MShmDistanceEle(pool,cur,ph->tail))==places))
	{		
		rc=SemReq(ph->semid,pool->userno);	
		if(rc<0)
		{
			printf(" SemReq rc=%d ph->semid=%d userno=%d\n",rc,ph->semid,pool->userno);
			return rc;
		}
		// zsk fixbug  �������û����д�Ķ������
		//rc = SemSetVal(ph->semid,pool->userno,0);
		if(ph->head==0)
		{
			if(abs(MShmDistanceEle(pool,ph->head,ph->tail))<5)
			{//������Ѿ������������³�ʼ��
				if(usr->curele>=ph->tail)
				{
					do
					{
						
						pthread_testcancel();
						rc=MShmLock(mc);
						if(rc!=0)
						{
							printf("MShmLock rc=%d!!\n",rc);
						}
					}while(rc!=0);
					usr->curele=ph->tail-1;
					MShmUnLock(mc);
					SemSetVal(ph->semid,pool->userno,0);
					
				}
			}
		}
		cur=usr->curele;

	}	
	do
	{
		pthread_testcancel();

		rc=MShmLock(mc);
		if(rc!=0)
		{
			printf("MShmLock rc=%d!!\n",rc);
		}
	}while(rc!=0);

	rc=SaveEle2Buf(pool,buf,buflen,cur,eleseq,flag);
	if(rc>=0)
	{
		usr->curele=NewerEle(pool,cur);	
	}
	MShmUnLock(mc);

	return rc;	
}

/**************************************************************************
  *������	:MShmPoolAddResource
  *����	:��ָ����������������ΪԪ�ط��뻺���β��
  *����	: pool:ָ�򻺳�ص�ָ�� 
  			  buf:Դ���ݻ��������׵�ַ
  			  buflen:����������Ч���ݳ���
			 flag:Ҫ�����Ԫ�صı�־
  *����ֵ	:����0��ʾ�ɹ� ��ֵ��ʾ����
  *
  *************************************************************************/
 int MShmPoolAddResource(MSHM_POOL *pool,void *buf,int buflen,int flag)
{
	int needblock;
	int fstartb;//���п���ʼ���
	SHPOOL_HEAD	*ph;
	SHPOOL_ELE		*tail;
	MEM_CORE		*mc;
	int 				 retval=0;
	int				rc;
	if((pool==NULL)||(buf==NULL)||(buflen<0))
		return -EINVAL;       
	mc=pool->mc;
	do
	{
		pthread_testcancel();
		rc=MShmLock(mc);
		if(rc!=0)
		{
			printf("MShmLock rc=%d!!\n",rc);
		}
	}while(rc!=0);
	do
	{
		ph=pool->ph;
		tail=&ph->eles[ph->tail];
		needblock=(buflen-1)/SHPOOL_BLOCK_SIZE+1;	//������Ҫ�Ŀ���
		fstartb=MShmPoolReqBlocks(pool,needblock);
		if(fstartb<0)
		{
			retval=-ENOMEM;
			break;
		}
		ph->num++;


		tail->num=ph->num;
		tail->userflag=flag;
		tail->bblock=fstartb;
		tail->blocks=needblock;
		if((fstartb+needblock)>=ph->tblocks)
		{
			tail->eblock=(fstartb+needblock-1)-ph->tblocks;
		}
		else
		{
			tail->eblock=fstartb+needblock-1;
		}
		InsertData2Ele(pool,tail,buf,buflen);	//����������β��
		ph->tail=NewerEle(pool,ph->tail);
		if(ph->tail==ph->head)
		{
			FreeHeadEle(pool);	//�������ʱ�ͷ����ϵ�Ԫ��
		}
	}while(0);

	if(retval==0)
		SemAddAll(pool);
	MShmUnLock(mc);

	return retval;
	
}




/**************************************************************************
  *������	:MShmPoolSetMaxNum
  *����	:�����źŻ�������ж��û���ʱ��Ԫ����
  *����	: pool:ָ�򻺳�ص�ָ�� 
  			  num: �жϳ�ʱ��Ԫ�ظ���
  *����ֵ	:����0��ʾ�ɹ� ��ֵ��ʾʧ�ܴ����Ǹ���errno
  *                   �������ɵ���MShmPoolCreate�Ľ��̵���,���û��г���
  *                   num��Ԫ��û�ж�ȡʱ������Ϊ�û��Ѿ��˳����߳��û�
  *************************************************************************/
int MShmPoolSetMaxNum(MSHM_POOL *pool,int num)
{
       int                         rc;
	MEM_CORE		*mc=NULL;
	SHPOOL_HEAD	*ph=NULL;
	if((mc==NULL)||(ph==NULL))
		return -EINVAL;
       mc=pool->mc;
       ph=pool->ph;

       rc=MShmLock(mc);
       ph->maxsem=num;
	  oldmaxsem=ph->maxsem;
       rc=MShmUnLock(mc);

       return 0;
}


void MSHmPoolSetMaxNum_t(int num)
{
	newmaxsem=num;
}
/**************************************************************************
  *������	:MShmPoolCreate
  *����	:���ո�����pkey����ָ����С�Ĺ������
  *����	: name:����ص�����
  			  pkey: ����ص�key(�������������key���������ڴ�
  			           �������ź���,����(key|0x50000000)������Դ�ź���)
  			  bytes:ϣ������ �Ļ������е�ʵ����������
  *���	:����������ʼ��pool�ṹ�е��ֶ�
  *����ֵ	:����0��ʾ�ɹ� ��ֵ��ʾʧ�ܴ����Ǹ���errno
  *
  *************************************************************************/
int MShmPoolCreate(char *name,MSHM_POOL *pool,key_t pkey,int bytes)
{
	int retval=0;
	int i;
	union semun semopts;
	MEM_CORE		*mc;
	SHPOOL_HEAD	*ph;
	long realsize;		//ʵ����Ҫ������ֽ���
	long pagesize;		//ҳ���С
	long offset;			//���ݿ��ƫ��
	void *p;
	int	rc;
	if((pool==NULL)||(pkey<0)||(bytes<=0))
	{
		return -EINVAL;
	}
	pagesize = sysconf(_SC_PAGESIZE);
	realsize=((sizeof(SHPOOL_HEAD)-1)/pagesize +1)+((bytes+1)/pagesize +1);//����ص�ͷռ�õ�ҳ����+��Ҫ��������ݿռ���
	realsize*=pagesize;
	mc=MShmCoreCreate(name,pkey,realsize,NULL);
	if(mc==NULL)
		return -errno;//errno�Ѿ���MShmCoreCreate����������

	pool->userno=0;
	do
	{
		pthread_testcancel();
		rc=MShmLock(mc);
		if(rc!=0)
		{
			printf("MShmLock rc=%d!!\n",rc);
		}
	}while(rc!=0);
	do{
		//��ʼ�� MSHM_POOL�ṹ�е�ָ��
		p=(void *)mc;
		p+=mc->uoffset;
		pool->mc=mc;
		pool->ph=(SHPOOL_HEAD*)p;
		ph=(SHPOOL_HEAD*)pool->ph;
		offset=((sizeof(SHPOOL_HEAD)-1)/pagesize +1)*pagesize;//���ݿ鰴��ҳ�����
		ph->boffset=offset;
		p=(void*)(ph);
		p+=ph->boffset;
		pool->eledata=(SHPOOL_BLOCK *)p;
		//��ʼ��ph�еı���
		ph=(SHPOOL_HEAD*)pool->ph;
		ph->maxsem=100;//
		oldmaxsem=ph->maxsem;
		ph->num=0;
            ///20070509 shixin fix
		ph->tblocks=(((bytes-1)/pagesize + 1)*pagesize)/SHPOOL_BLOCK_SIZE;	//ʵ�ʷ��䵽���ڴ��ǰ���ҳ������

              ph->fblocks=ph->tblocks;
		ph->fbstart=0;
		//boffset�ֶ��Ѿ����������ú���
		ph->count=0;
		ph->head=0;
		ph->tail=0;
		ph->last_usr_no=-1;//��û���û�
		ph->semkey=mc->mkey|0x50000000;
		ph->semid   = semget(ph->semkey, SHPOOL_MAX_USERS, IPC_CREAT|IPC_EXCL|0600);
		if (ph->semid <0)
		{
			if(errno == EEXIST)
			{
				ph->semid = semget(ph->semkey, 0, IPC_EXCL|0600);
			}
			if(ph->semid<0)
			{
				printf("failed create shar pool sem: 0x%x\n",ph->semkey);
				retval=-ENAVAIL;
				break;
			}
		}
		else
		{
			memset((void*)ph->pinfo,0,sizeof(ph->pinfo));
                    
		}
		for(i=0;i<SHPOOL_MAX_USERS;i++)
		{
			semopts.val=0;						//�������û�����Դ������Ϊ0
			semctl(ph->semid , 0, SETVAL, semopts); 
		}
		memset((void*)ph->eles,0,sizeof(ph->eles));//���Ԫ�ػ�����
		memset((void*)pool->eledata,0,(SHPOOL_BLOCK_SIZE*ph->tblocks));//��ʼ��������
		ph->send_bytes=0;
		pool->userflag=1;

	}while(0);
	MShmUnLock(mc);

	return retval;
}

/**************************************************************************
  *������	:MSHmPoolAttach
  *����	:���ո�����pkey���ӵ������
  *����	: pkey:Ҫ���ӵ� ����ص�key
  *���	:����������ʼ��pool�ṹ�е��ֶ�
  *����ֵ	:����0��ʾ�ɹ� ��ֵ��ʾʧ�ܴ����Ǹ���errno
  *ע		:��������Ҫ���ڼ�ػ���ص�����״̬�ĳ���
  *************************************************************************/
int MSHmPoolAttach(MSHM_POOL *pool,key_t pkey)
{
	MEM_CORE		*mc=NULL;
	SHPOOL_HEAD	*ph=NULL;
	void 			*p=NULL;
	
	if((pool==NULL)||((int)pkey<0))
	{
		return -EINVAL;
	}
	errno=0;
	mc=MShmCoreAttach(pkey,NULL);
	if(mc==NULL)
		return -errno;//errno�Ѿ���MShmCoreCreate����������
	if(mc->uoffset<=0)
	{
		//�����߻�û�г�ʼ����
		MShmCoreDetach(mc);
		return -EPERM;
	}

	//��ʼ�� MSHM_POOL�ṹ�е�ָ��
	p=(void *)mc;
	p+=mc->uoffset;
	pool->mc=mc;
	pool->ph=(SHPOOL_HEAD*)p;
	ph=(SHPOOL_HEAD*)pool->ph;
	if(ph->boffset<=0)
	{
		//�����߻�û�г�ʼ����
		MShmCoreDetach(mc);
		return -EPERM;		
	}
	p=(void*)(ph);
	p+=ph->boffset;
	pool->eledata=(SHPOOL_BLOCK *)p;


	pool->userflag=2;
	return 0;
}
/**************************************************************************
  *������	:MShmPoolMvUsrPlace
  *����	:���û���ǰ��ȡ���ݵ�ָ�밴Ҫ���ƶ�
  *����	: pool:ָ�򻺳�ص�ָ�� 
  			  place:Ҫ�ƶ�������
  			  	��ֵ��ʾ��ǰ�ƶ�|place|��
  			  	��ֵ��ʾ����ƶ�place��
  			  	0��ʾ�����ƶ�
  *����ֵ	:����>=0��ʾ�ƶ��ĵ�Ԫ��, ��ֵ��ʾʧ�ܴ����Ǹ���errno
  *************************************************************************/
int MShmPoolMvUsrPlace(MSHM_POOL *pool,int place)
{
	int i;
	int num=0;	//
	int temp;
	int moved=0;
	MEM_CORE		*mc=pool->mc;
	SHPOOL_HEAD	*ph=pool->ph;
	SHPOOL_USR   	*usr=NULL;
	if((mc==NULL)||(ph==NULL))
		return -EINVAL;
	usr=&ph->users[pool->userno];
	if(place<0)
	{//�ӵ�ǰλ����ǰ
		num=-place;
		temp=usr->curele;
		for(i=0;i<num;i++)
		{	//��ǰ����older_nums���ϵ�Ԫ��
			temp=OlderEle(pool, temp);
			moved++;
			if(temp==ph->head)		//��ͷ��
					break;	
		}
		//MShmLock(mc);
		usr->curele=temp;				//�ҵ���		
		//MShmUnLock(mc);
	}
	else if(place>0)
	{
		num=place;
		temp=usr->curele;
		for(i=0;i<num;i++)
		{	//��ǰ����older_nums���ϵ�Ԫ��
			temp=NewerEle(pool, temp);
			moved++;
			if(temp==ph->tail)		//��β��
					break;	
		}
		//MShmLock(mc);
		usr->curele=temp;				//�ҵ���		
		//MShmUnLock(mc);
	}
	places=moved;
	return moved;
		
}
/**************************************************************************
  *������	:MShmPoolReq
  *����	:���ո�����pkey����һ�������,�������䵽pool
  *����	: pkey:Ҫ����� ����ص�key
  			  name:�����ڴ�ص��û���
  			  type:�û��������ϲ㺯��������
  *���	:����������ʼ��pool�ṹ�е��ֶ�
  *����ֵ	:����0��ʾ�ɹ� ��ֵ��ʾʧ�ܴ����Ǹ���errno
  *************************************************************************/
int MShmPoolReq(MSHM_POOL *pool,key_t pkey,char *name,int type)
{
	
	int retval=0;
	
	SHPOOL_HEAD	*ph=NULL;
	SHPOOL_USR   	*usr=NULL;
	MEM_CORE		*mc=NULL;
	int rc;
	retval=MSHmPoolAttach(pool,pkey);
	if(retval<0)
		return retval;
	mc=pool->mc;
	ph=pool->ph;
	do
	{
		pthread_testcancel();
		rc=MShmLock(mc);
		if(rc!=0)
		{
			printf("MShmLock rc=%d!!\n",rc);
		}
	}while(rc!=0);
	do{
		//��ʼ�� MSHM_POOL�ṹ�е�ָ��
		retval=GetAFreeUsrNo(pool);
		if(retval<0)
		{
			pool->ph=NULL;
			pool->mc=NULL;
			pool->eledata=NULL;			
			break;
		}
		pool->userno=retval;
		SemSetVal(ph->semid,pool->userno,0);
		retval=0;
		usr=&ph->users[pool->userno];
		usr->valid=1;
		usr->curele=ph->tail;	//ʹ����Ԫ�س�Ϊ��ǰԪ��
		usr->type=type;
		usr->upid=getpid();
		usr->stime=time(NULL);
		if(name==NULL)
			name="normal user";
		MShmPoolSetUserName(pool,pool->userno,name);
		pool->userflag=0;

	}while(0);
	MShmUnLock(mc);	

	if(retval!=0)
		MShmCoreDetach(mc);

	return retval;
}

/**************************************************************************
  *������	:MShmPoolReqFree
  *����	:�ͷ��Ѿ����빲�����
  *����	: pool:����ؽṹָ��
  *����ֵ	:����0��ʾ�ɹ� ��ֵ��ʾʧ�ܴ����Ǹ���errno
  *************************************************************************/
int MShmPoolReqFree(MSHM_POOL *pool)
{
	SHPOOL_HEAD	*ph;
	SHPOOL_USR   	*usr;
	int rc;
	if(pool==NULL)
		return -EINVAL;
	ph=pool->ph;
	if(ph==NULL)
		return -EINVAL;
	if(pool->userno>=SHPOOL_MAX_USERS)
		return -EINVAL;
	if(pool->userflag==MSHM_POOL_NORMAL_USR)
	{
		do
		{
			pthread_testcancel();

			rc=MShmLock(pool->mc);
			if(rc!=0)
			{
				printf("MShmLock rc=%d!!\n",rc);
			}
		}while(rc!=0);
		usr=&ph->users[pool->userno];
		usr->valid=0;
		MShmUnLock(pool->mc);

	}
	if(pool->mc!=NULL)
	{
		MShmCoreDetach(pool->mc);
		pool->mc=NULL;
	}
	return 0;
}
//

/**************************************************************************
  *������	:MShmPoolDelUsrByPid
  *����	:ɾ��ָ�����̺ŵ��û�
  *����	: pool:����ؽṹָ��
  *����ֵ	:����1 ��ʾɾ�����û�
  *			 ����0��ʾ�û�ԭ���Ͳ�����
  *			 ��ֵ��ʾ����
  *************************************************************************/
int MShmPoolDelUsrByPid(MSHM_POOL *pool,pid_t pid)
{
	int i,Ret=0;
	SHPOOL_HEAD	*ph;
	SHPOOL_USR   	*usr;
	if(pool==NULL)
		return -EINVAL;
	ph=pool->ph;
	if(ph==NULL)
		return -EINVAL;

	usr=ph->users;
	for(i=0;i<SHPOOL_MAX_USERS;i++)
	{
		if(usr->upid==pid)
		{
			if(usr->valid)
			{				
				Ret=1;
			}
			usr->valid=0;
			break;			
		}
		usr++;
	}
	return 0;
}


/*******************��Ϣά��************************************/

/**************************************************************************
  *������	:MShmPoolGetUsrInfo
  *����	:��ȡָ���û��ŵ��û���Ϣ������ַ����info��
  *����	: pool:����ؽṹָ��
  			: userno �û���
  *���	: info ����ʱ��info�����뻺�����ĵ�ַ
  *����ֵ	:���ظ�ֵ��ʾ������ֵ��ʾinfo�������Ĵ�С
  *ע		:info�������е���Ϣ���ݼ��ṹ�����ϲ�Ӧ�ó�
  			 �����
  			 ����Ҫʱ���ϲ㺯���������ڴ�
  *************************************************************************/
int MShmPoolGetUsrInfo(MSHM_POOL *pool,int userno,void **info)
{
	SHPOOL_HEAD	*ph;
	SHPOOL_USR   	*usr;
	if((pool==NULL)||(userno<0)||(userno>=SHPOOL_MAX_USERS)||(info==NULL))
		return -EINVAL;	
	ph=pool->ph;
	if(ph==NULL)
		return -EINVAL;
	usr=&ph->users[userno];
	*info=usr->info;
	return sizeof(usr->info);
}

/**************************************************************************
  *������	:GetUsrValid
  *����	:��ȡ��ǰ�û�������Ƿ���Ȼ��Ч��־
  *����	: pool:����ؽṹָ��
  *����ֵ	:���ظ�ֵ��ʾ����0��ʾ��Ч 1��ʾ��Ч

  *************************************************************************/
int GetUsrValid(MSHM_POOL *pool)
{
	SHPOOL_HEAD	*ph;
	SHPOOL_USR   	*usr;

	if(pool==NULL)
		return -EINVAL;	
	if(pool->userflag!=MSHM_POOL_NORMAL_USR)
		return -EINVAL;
	ph=pool->ph;
	if(ph==NULL)
		return -EINVAL;	
	usr=&ph->users[pool->userno];
	return usr->valid;
}
/**************************************************************************
  *������	:MshmPoolGetTotalUser
  *����	:��ȡ��ǰʹ�û���ص��û�����
  *����	: pool:����ؽṹָ��
  *����ֵ	:���ػ���ص��û�����,��ֵ��ʾ����
  *************************************************************************/
int MShmPoolGetTotalUser(MSHM_POOL *pool)
{
	int i,total=0;
	SHPOOL_HEAD	*ph;
	SHPOOL_USR   	*usr;
	if(pool==NULL)
		return -EINVAL;
	
	ph=pool->ph;
	usr=ph->users;
	for(i=0;i<SHPOOL_MAX_USERS;i++)
	{
		if(usr->valid)
		{
			total++;
		}
		usr++;
	}
	return total;
}

/**************************************************************************
  *������	:MShmPoolGetInfo
  *����	:��ȡ��ǰʹ�û���ص�Ӧ���Զ�����Ϣ
  *����	: pool:����ؽṹָ��
  *����ֵ	:����Ӧ���Զ������Ϣ��ַ
  *			�����Ҫ���ϲ����Ӧ�����ڴ����
  *************************************************************************/
void *MShmPoolGetInfo(MSHM_POOL *pool)
{
	SHPOOL_HEAD	*ph;
	if(pool==NULL)
		return NULL;
	if(pool->ph==NULL)
		return NULL;
	ph=pool->ph;
	return (void*)ph->pinfo;
}

/**************************************************************************
  *������	:MShmPoolReActiveUsr
  *����	:���¼����û�,����û���ʱ�䲻�ܽ�������
  			:�򻺳�ط��ͳ�������Ϊ���˳���,���Խ�����Ӧ
  			 ���ڵ��ô˺���,���¼����û�(��ֹ����)
  *����	: pool:����ؽṹָ��
  *����ֵ	:0��ʾ��ȷ ��ֵ��ʾ���� 1��ʾ�û��Ѿ���ɾ����
  			�������¼���
  *ע		:�����������������ڴ�			
  *************************************************************************/
int	MShmPoolReActiveUsr(MSHM_POOL *pool)
{
	SHPOOL_USR   	*usr;
	int				rc;
	if(pool==NULL)
		return -EINVAL;
	if(pool->mc==NULL)
		return -EINVAL;
	
	if(pool->userflag!=MSHM_POOL_NORMAL_USR)
		return -EINVAL;
	if(pool->userno>=SHPOOL_MAX_USERS)
		return -EINVAL;
	usr=pool->ph->users;
	usr+=pool->userno;
	if(!usr->valid)
	{
		do
		{
			pthread_testcancel();

			rc=MShmLock(pool->mc);
			if(rc!=0)
			{
				printf("MShmLock rc=%d!!\n",rc);
			}
		}while(rc!=0);
		usr->valid=1;
		MShmUnLock(pool->mc);

		return 1;
	}
	return 0;
}

