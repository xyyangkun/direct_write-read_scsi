/**************************************************************************
 * �ļ��� : mshm.c
 * ����	: �������ڴ�ĵ��ýӿڽ��а�װ���ṩ����ʹ
 			   �õĽӿ�   
 *************************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/sem.h>
#include <string.h>
#include <pthread.h>
#include "mshm.h"
#include "msem.h"


#define MSHM_MAGIC      0x5500aa00              ///�����ڴ��־
static const struct sembuf s_DoLock={//lock���ź����ṹ��ʼ��
			sem_num:0,
			sem_op:-1,
	#ifndef VENC_TEST
			sem_flg:SEM_UNDO,		
	#else
			sem_flg:(IPC_NOWAIT |SEM_UNDO),
	#endif
};

static const struct sembuf s_DoUnLock={//unlock���ź����ṹ��ʼ��
			sem_num:0,
			sem_op:1,
	#ifndef VENC_TEST
			sem_flg:SEM_UNDO,
	#else
			sem_flg:(IPC_NOWAIT |SEM_UNDO),
	#endif
};


/**************************************************************************
  *������	:MShmCoreCreate
  *����	:�Ը�����key��usersize���������ڴ�
  *����	: mkey:�����ڴ��key 
  *			  usersize:�û�������Ҫ�Ŀռ�
  *			  name:Ҫ�����Ĺ����ڴ������
  *                   u_ptr:���ֵ,����û����õ��ڴ�������ʼ��ַ,NULL��ʾ����Ҫ���
  *����ֵ	: ����NULL��ʾ�д��� ���������errno
  *			  �������Ӻõ��ڴ��ַMemCoreָ��,
  *			  �Ժ������ָ����Ϊ����������������
  *ע:������������ѷ��䵽���ڴ��ʼ��Ϊ0
  *************************************************************************/
MEM_CORE * MShmCoreCreate(char *name,key_t mkey,size_t usersize,void **u_ptr)
{
	int err;
	int FirstCreateFlag=0;	//�״δ�����key�Ĺ����ڴ��־
	MEM_CORE *mc=NULL;
	key_t semkey;
	size_t tsize;				//ʵ�ʷ���Ĺ����ڴ��С
	size_t pagesize;
	void *area = ((void *)-1);	//���Ӻõ��ڴ��ַ
	int fdmem=-1;			//�����ڴ���
	int fdsem=-1;			//�ź������
	struct shmid_ds shmbuf;
//	union semun semopts;
	if ((usersize <= 0)||(mkey<0))
	{
		errno=EINVAL;
		return NULL;
	} 	

#ifdef FOR_3022
while(1)
{
#endif
/******************���乲���ڴ�***************************/
	pagesize = sysconf(_SC_PAGESIZE);					//ϵͳҳ��С
    	tsize = (((usersize-1)/pagesize+1)+1)*pagesize;		//ʵ�ʷ���ʱ����һҳ��Ϊ�����ڴ�������ͷ
	//���������ڴ�
	fdmem=shmget(mkey, tsize, (SHM_R|SHM_W|IPC_CREAT|IPC_EXCL));
	if(fdmem<0)
	{
		err=errno;
		if(err==EEXIST)
		{	//�Ѿ�����
			fdmem=shmget(mkey, 0, 0);
		
#ifdef FOR_3022
			if(fdmem<0)
			{
				printf("failed create share memory 0x%x\n",mkey);
				return NULL;
			}
			shmctl(fdmem, IPC_STAT, &shmbuf);
			if(shmbuf.shm_segsz < tsize)
			{
				///��ɾ��ԭ���ģ������½����µĹ����ڴ�
				err=shmctl(fdmem, IPC_RMID, 0);
				if(err<0)
				{
					printf("�н�����ʹ�ù����ڴ�\n");
				}
				continue;	
			}
#endif
		}
		if(fdmem<0)
		{
			printf("failed create share memory 0x%x,err=%d\n",mkey,err);
			return NULL;
		}
	}
	else
	{
		FirstCreateFlag=1;
	}
	//���ӹ����ڴ�
	area = (void *)shmat(fdmem, NULL, 0);
	if(area==((void*)-1))
	{
		printf("failed attach share memory 0x%x\n",mkey);
		return NULL;
	}
	if(FirstCreateFlag)
	{//����ǵ�һ�δ����򽫹����ڴ��ʼ��Ϊ0
		memset((void*)area,0,tsize);
	}
	//���ù����ڴ�	
     	shmctl(fdmem, IPC_STAT, &shmbuf);
    	shmbuf.shm_perm.uid = getuid();
    	shmbuf.shm_perm.gid = getgid();
    	shmctl(fdmem, IPC_SET, &shmbuf);
 //   if (shmctl(fdmem, IPC_RMID, NULL) == -1)
   //     FAIL(MM_ERR_CORE|MM_ERR_SYSTEM, "failed to remove shared memory in advance");



	semkey=mkey;	//���ź�����key�빲���ڴ��key��ͬ

	fdsem=SemCreate(semkey,1);
	if(fdsem<0)
	{
		printf("failed create share memory sem: 0x%x\n",semkey);
              shmdt(area);
		return NULL;
	}
	//semopts.val=1;
	//semctl(fdsem, 0, SETVAL, semopts); 
	SemSetVal(fdsem,0,1);
	/*
	 * Configure the memory core parameters
	 */
	mc = (MEM_CORE *)area;
	mc->mkey=mkey;
	mc->tsize     = tsize;
	mc->usize    = usersize;	
	mc->memid  = fdmem;
	mc->semid	   = fdsem;
	mc->uoffset  = pagesize;
	if(name!=NULL)
	{
		sprintf(mc->name,"%s",name);
	}
	else
		sprintf(mc->name,"%s","NONAME");
       if(u_ptr!=NULL)
       {
               *u_ptr=(void*)((void*)mc+mc->uoffset);
       }
       mc->magic=MSHM_MAGIC;
	return mc;
#ifdef FOR_3022
}
#endif
}

//
/**************************************************************************
  *������	:MShmCoreAttach
  *����	:���ո�����key�򿪲����ӹ����ڴ�
  *����	: mkey:�����ڴ��key 
  *                   u_ptr:���ֵ,����û����õ��ڴ�������ʼ��ַ,NULL��ʾ����Ҫ���
  *����ֵ	: ����NULL��ʾ�д��� ���������errno ENOENT��ʾ��
  				key�Ĺ����ڴ滹û�б�����
  			  �������Ӻõ��ڴ��ַMemCoreָ��,
  			  �Ժ������ָ����Ϊ����������������
  *************************************************************************/
MEM_CORE * MShmCoreAttach(key_t mkey,void **u_ptr)
{
	MEM_CORE *mc=NULL;
	void *area = ((void *)-1);	//���Ӻõ��ڴ��ַ
	int fdmem=-1;			//�����ڴ���
	int pagesize = sysconf(_SC_PAGESIZE);					//ϵͳҳ��С

	if (mkey<0)
	{
		errno=EINVAL;
		return NULL;
	} 	

	//���Ѿ��������Ĺ����ڴ�
	fdmem=shmget(mkey, 0, 0);
	if(fdmem<0)
	{
		//printf("share memory 0x%x not be created\n",mkey);
		return NULL;
	}

	//���ӹ����ڴ�
	area = (void *)shmat(fdmem, NULL, 0);
	if(area==((void*)-1))
	{
		printf("failed attach share memory 0x%x\n",mkey);
		return NULL;
	}
	mc = (MEM_CORE *)area;
       if(mc->uoffset!=pagesize)
       {///˵�����ܻ�û��������ʼ��
            errno=EAGAIN;
            printf("mc->uoffset=%d pagesize=%d!!\n",(int)mc->uoffset,(int)pagesize);
            MShmCoreDetach(mc);
            return NULL;
       }
       if(u_ptr!=NULL)
       {
               *u_ptr=(void*)((void*)mc+mc->uoffset);
       }
	return mc;
}
/**************************************************************************
  *������	:MShmCoreDetach
  *����	:�˳���ָ�������ڴ������
  *����	: mc:�����ڴ���׵�ַ
  *����ֵ	: 0��ʾ�ɹ�
  *************************************************************************/
int MShmCoreDetach(MEM_CORE *mc)
{
		if(mc==NULL)
			return -EINVAL;
		return shmdt(mc);
}

/**************************************************************************
  *������	:MShmLock
  *����	:ȡ�ö�ָ�������ڴ�ķ���Ȩ
  *����	: mc :�����ڴ����ʼ��ַ(��MShmCoreCreate��MShmCoreAttach����)
  *����ֵ	: 0��ʾ�ɹ� ��ֵ��ʾ���� ��������errno��
  *************************************************************************/
#include <syslog.h>
int MShmLock(MEM_CORE *mc)
{
	int rc;
	if(mc==NULL)
	{
		errno=EINVAL;
		return -1;	
	}
       if(mc->magic!=MSHM_MAGIC)
       {///�����ڴ������ƻ�,�����˳�
                printf("MEM_CORE bad magic:0x%x exit!!\n",(int)mc->magic);
                syslog(LOG_ERR,"MEM_CORE bad magic:0x%x exit!!\n",(int)mc->magic);
                exit(1);
       }
	//printf("lock sem val=%d\n",SemGetVal(mc->semid,0));

	while (((rc = semop(mc->semid, (struct sembuf *)&s_DoLock, 1)) < 0))
	{
		rc=-errno;

#ifdef VENC_TEST
		//printf("MShmLock��ס��rc=[%d]...\n",rc);
		pthread_testcancel();
		usleep(50);
#endif

		if(rc==-EINTR)
		{
			//pthread_testcancel();			
			continue;
		}
		else
		{
#ifdef VENC_TEST
			if(rc!=-EAGAIN)	////zw-test
#endif
				break;
		}
	}
	return rc;

}
/**************************************************************************
  *������	:MShmLock
  *����	:�ͷŶ�ָ�������ڴ�ķ���Ȩ
  *����	: mc :�����ڴ����ʼ��ַ(��MShmCoreCreate��MShmCoreAttach����)
  *����ֵ	: 0��ʾ�ɹ� ��ֵ��ʾ���� ��������errno��
  *************************************************************************/
int MShmUnLock(MEM_CORE *mc)
{
	int rc;
	if(mc==NULL)
	{
		errno=EINVAL;
		return -1;	
	}
       if(mc->magic!=MSHM_MAGIC)
       {///�����ڴ������ƻ�,�����˳�
                printf("MEM_CORE bad magic:0x%x exit!!\n",(int)mc->magic);
                syslog(LOG_ERR,"MEM_CORE bad magic:0x%x exit!!\n",(int)mc->magic);
                exit(1);
       }    
	while (((rc = semop(mc->semid, (struct sembuf *)&s_DoUnLock, 1)) < 0))
	{
		rc=-errno;

#ifdef VENC_TEST
		//printf("MShmUnLock ��ס��rc=[%d]...\n",rc);
		pthread_testcancel();
		usleep(50);
#endif

		if(rc==-EINTR)
		{
//			pthread_testcancel();
			continue;
		}
		else
		{
#ifdef VENC_TEST
			if(rc!=-EAGAIN)	////zw-test
#endif
				break;
		}
	}
	return rc;
}


