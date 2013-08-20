#ifndef MSEM_H
#define MSEM_H

#include <sys/sem.h>
#include <errno.h>
#include <syslog.h>
//���ź�����ֵʱ��Ҫ�õ���������

union semun {
         int val;                  		/* value for SETVAL */
         struct semid_ds *buf;     /* buffer for IPC_STAT, IPC_SET */
         unsigned short *array;    /* array for GETALL, SETALL */
                                   		/* Linux specific part: */
         struct seminfo *__buf;    /* buffer for IPC_INFO */
};
//��ָ�����ź������е�ָ���ź���������Դ
static __inline__ int SemAdd(int semid,int sem_num)
{
	int rc;
	struct sembuf sem={sem_num,1,IPC_NOWAIT};
	while ((rc = semop(semid, (struct sembuf *)&sem, 1)) < 0)
	{
		rc=-errno;
		if(rc==-EINTR)
		{
			//pthread_testcancel();
			continue;
		}
		else
			break;
	}
	return rc;
}
//����ָ�����ź������е�ָ���ź�����Դ
static __inline__ int SemReq(int semid,int sem_num)
{
	int rc=0;
	struct sembuf sem={sem_num,-1,0};
	while ((rc = semop(semid, (struct sembuf *)&sem, 1)) < 0) 
	{
		rc=-errno;
		if(errno==-EINTR)
		{
//			pthread_testcancel();
			continue;
		}
		else 
			break;
	}
	return rc;
}
//����ָ���ź������е��ź�����ֵ
static __inline__ int SemSetVal(int semid,int sem_num,int val)
{
	union semun semopts;
	semopts.val=val;
	return semctl(semid,sem_num,SETVAL,semopts);
}
//����ָ���ź������е��ź�����ֵ
static __inline__ int SemGetVal(int semid,int sem_num)
{
	return semctl(semid,sem_num,GETVAL);
}
//�����ź������������ź�����id
//semnumҪ�������ź������е��ź�����Ŀ
static __inline__ int SemCreate(key_t SemKey,int SemNum)
{
	int SemFd=-1;
	int err;
	SemFd = semget(SemKey, SemNum, IPC_CREAT|IPC_EXCL|0600);
	if (SemFd <0)
	{
		err=errno;
		if(err == EEXIST)
		{
				SemFd = semget(SemKey, SemNum, IPC_EXCL|0600);
		}
		if(SemFd<0)
		{
			printf("failed create sem key=0x%x\n",SemKey);
			syslog(LOG_INFO,"failed create sem key=0x%x\n",SemKey);
			return -err;
		}
	}
       else
       {
             //SemSetVal(SemFd,0,SemNum);       
       }
	return SemFd;
}

#endif
