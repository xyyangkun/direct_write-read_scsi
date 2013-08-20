#include <stdio.h>
#include <string.h>
#include <errno.h>
#include "typedefine.h"
#include <media_api.h>
#include <pthread.h>

//int posix_memalign(void **memptr, size_t alignment, size_t size);
int posix_memalign(void **memptr, size_t alignment, size_t size)
{
	*memptr=(void *)malloc(size*alignment);
	if(*memptr==NULL)
	{
		perror("error in malloc and it will exit\n");
		return -1;
		exit(1);
	}
	return 0;
}


/****************************************************************************/
/*******************************��д���õ��Ĳ����ӿ�**************************/
/****************************************************************************/

/**********************************************************************************************
 * ������	:init_media_rw()
 * ����	:��ʼ����дý����Ϣ�õĽṹ
 * ����	:type:ý���ʽ,��Ƶ������Ƶ MEDIA_TYPE_VIDEO,MEDIA_TYPE_AUDIO
 *			 no:��Դ���
 *			 buflen:Ҫ����������ʱ��д�Ļ�����(media->temp_buf)�Ĵ�С
 * ���	 media:����ʱ������Ӧ����Ϣ
 * ����ֵ	:0��ʾ�ɹ���ֵ��ʾʧ��
 **********************************************************************************************/
int init_media_rw(OUT media_source_t *media,IN int type,IN int no,IN int buflen)
{
	int ret;
	if(media==NULL)
	{
		return -EINVAL;
	}
	pthread_mutex_init(&media->mutex,NULL);
	media->media_type=type;
	media->no=no;
	media->attrib=NULL;
	media->thread_id=-1;
	media->dev_stat=-1;
	media->temp_buf=NULL;
	media->buflen=buflen;
	if(buflen>0)
	{
		ret=posix_memalign((void**)&media->temp_buf,sizeof(DWORD ),buflen);
		if(ret==0)
		{
			media->buflen=buflen;
		}
		else
		{
			media->buflen=0;//�ڴ治��
			return -ENOMEM;
		}
	}
	return 0;
}

/**********************************************************************************************
 * ������	:get_media_attrib()
 * ����	:��ȡ����ý�����ԵĽṹָ��
 * ����	:media:����ý����Ϣ�Ľṹָ��
 * ����ֵ	:����ý�����Խṹ��ָ��,NULL��ʾ����ΪNULL,���߻�δ����
 **********************************************************************************************/
void *get_media_attrib(IN media_source_t *media)
{
	if((media==NULL))
		return NULL;
	else
		return media->attrib;
}

/****************************************************************************/
/*******************************�������Ľӿ�**************************/
/****************************************************************************/

/**************************************************************************
  *������	:AttachVEncDevice
  *����	:���ӵ�һ���Ѿ��򿪵Ļ����
  *����	:name: �����豸�ĳ�����
  *			 Enc:�Ѿ���ʼ���õ�������������Ϣ�Ľṹ
  *			 Pool:һ��û�г�ʼ���������������Ϣ�Ľṹ
  *			 bytes:ϣ������Ĺ���������С 0��ʾ�Զ�ѡ���С
  *����ֵ	:0��ʾ���������豸������
  *			 -EINVAL:��������
  *			 -EAGAIN:�豸���ڳ�ʼ������
  *			 --ENODEV:�豸����
  *			-
  *ע:������Ӧ���ɴӻ������ж�ȡ���ݵĳ������
  *************************************************************************/
static int attach_media_device(int key,media_source_t *media,char *usr_name,int type)
{
	int Ret;
	MSHM_POOL *pool=NULL;
	key_t EncKey=(key_t)key;
	if((media==NULL)||(usr_name==NULL))
		return -EINVAL;
	pool=&media->mpool;
	if(key<0)
	{
//		showbug();
		return -EINVAL;
	}
	if((type!=MSHMPOOL_LOCAL_USR)&&(type!=MSHMPOOL_NET_USR))
		return -EINVAL;
	
	Ret=MShmPoolReq(pool,EncKey,usr_name,type);
	if(Ret<0)
		return Ret;
	return 0;
	
}
/**********************************************************************************************
 * ������	:move_media_place()
 * ����	:����ǰ�û��Ķ�ȡλ���ƶ�place��λ��
 *			
 * ����	:media:����ý���ָ��
 *			 place:Ҫ�ƶ�������,
 *				��ֵ��ʾ��ǰ�ƶ�,��ֵ��ʾ����ƶ�
 * ���	 media:����ʱ������Ӧ����Ϣ
 * ����ֵ	:0��ʾ�ɹ���ֵ��ʾʧ��
 **********************************************************************************************/
int move_media_place(IO media_source_t *media,IN int place)
{
	if(media==NULL)
		return -EINVAL;
	return MShmPoolMvUsrPlace(&media->mpool,place);
}
/**********************************************************************************************
 * ������	:connect_media_read()
 * ����	:���ӵ�ָ��key��ý�建���(��ȡ���ݵĳ���ʹ��)
 * ����	:key:Ҫ���ӵ�ý�建��ص�key
 *			 name:�����ߵ������ַ���
 *			 usr_type:�����ߵ��û����� MSHMPOOL_LOCAL_USR��MSHMPOOL_NET_USR
 * ���	 media:����ʱ������Ӧ����Ϣ
 * ����ֵ	:0��ʾ�ɹ���ֵ��ʾʧ��
 **********************************************************************************************/
int connect_media_read(OUT media_source_t *media,IN int key,IN char *name,IN int usr_type)
{
	int ret=0;
	if((media==NULL)||(name==NULL))
		return -EINVAL;
	if(media->dev_stat<0)
	{			
			
			ret=attach_media_device(key,media,name,usr_type);			
			if(ret>=0)
			{
				printf("ConnectVEncDev call AttachEncDevice%d  Ret=%d:%s usrno=%d name=%s\n",media->no,ret,strerror(-ret),media->mpool.userno,name);
				pthread_mutex_lock(&media->mutex);			
				media->attrib=MShmPoolGetInfo(&media->mpool);
				media->dev_stat=0;
				pthread_mutex_unlock(&media->mutex);	
				ret=0;
			}
			
			return ret;
	}
	return ret;	
}

/**********************************************************************************************
 * ������	:disconnect_media_read()
 * ����	:�������ӵ�ý�建��ضϿ�(��ȡ���ݵĳ���ʹ��)
 * ����	:meida:���������ӵ�ý�建��ؽṹָ��
 * ���	 media:����ʱ������Ӧ����Ϣ
 * ����ֵ	:0��ʾ�ɹ���ֵ��ʾʧ��
 **********************************************************************************************/
int disconnect_media_read(IO media_source_t *media)
{
	int ret;
	if(media==NULL)
		return -EINVAL;
	pthread_mutex_lock(&media->mutex);	
	media->dev_stat=-1;
	media->attrib=NULL;
	pthread_mutex_unlock(&media->mutex);	
	ret=MShmPoolReqFree(&media->mpool);
	
	return ret;	
}

/**********************************************************************************************
 * ������	:reactive_media_usr()
 * ����	:���¼���ý�����(��ȡ���ݵĳ���ʹ��)
 * ����	:meida:���������ӵ�ý�建��ؽṹָ��
 * ���	 media:����ʱ������Ӧ����Ϣ
 * ����ֵ	:0��ʾ��������, 1��ʾ�Ѿ����¼���� 
 **********************************************************************************************/
int reactive_media_usr(IO media_source_t *media)
{
	return MShmPoolReActiveUsr(&media->mpool);
}




/****************************************************************************/
/*******************************д�����Ľӿ�**************************/
/****************************************************************************/

/**********************************************************************************************
 * ������	:create_media_write()
 * ����	:�����������(�Ի���ؽ���д�����ĳ������)
 * ����	:key:�����ڴ滺��ص�key
 *			 name:�����ߵ�����
 *			 size:׼�������Ļ���ش�С(byte)
 * ���	 media:����ʱ������Ӧ����Ϣ
 * ����ֵ	:0��ʾ�ɹ�,��ֵ��ʾʧ��
 **********************************************************************************************/
int create_media_write(OUT media_source_t *media,IN int key,char *name,int size)
{
	int ret=0;
	if((name==NULL)||(key<0)||(size<=0))
		return -EINVAL;

	ret=MShmPoolCreate(name,&media->mpool,key,size);
	printf("call MShmPoolCreate(key=%x) ret=%d\n",key,ret);	
	if(ret!=0)
	{
		printf("create_media_write key:0x%x err ret=%d ",key,ret);
	}
	else
	{
		media->attrib=MShmPoolGetInfo(&media->mpool);
		media->dev_stat=0;
		media->attrib->media_type=media->media_type;
		media->attrib->stat=ENC_NO_INIT;
		ret=0;
	}
	return ret;
}

/**********************************************************************************************
 * ������	:set_media_attrib()
 * ����	:����ý����Դ����
 * ����	:media:����ý����Ϣ�Ľṹָ��
 *			 attrib:Ҫ���õ�������Ϣ������
 *			 att_len:attrib�е���Ч�ֽ���
 * ���	 media:����ʱ������Ӧ����Ϣ
 * ����ֵ	:att_len��ʾ�ɹ�,��ֵ��ʾʧ��
 **********************************************************************************************/
int set_media_attrib(IO media_source_t *media,IN void *attrib,IN int att_len)
{
	if((media==NULL)||(attrib==NULL)||(att_len<=0))
		return -EINVAL;
	memcpy(media->attrib,attrib,att_len);
	return att_len;
}





