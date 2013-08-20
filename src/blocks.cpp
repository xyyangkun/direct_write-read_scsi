/*
 * blocks.cpp
 *
 *  Created on: 2013-8-20
 *      Author: yangkun
 */
#include "blocks.h"
/*比如：年块，天块,秒块读取，写入*/
blocks::blocks(long long seek,int size,int data_head_size,char *data_head_buf,bool is_new_mem,char *data)
{
	printf("%s3\n",__FUNCTION__);
	std::cout<<data_head_buf<<std::endl;
	this->is_new_mem=is_new_mem;
	if(init_sda()<0)
	{
		printf("disk init error\n");
		exit(1);
	}

	this->seek=seek;
	block_num=get_block_num_from_data(size);
	data_size =size;
	this->data_head_size=data_head_size;
	if(is_new_mem)
	{
		this->data= new char[size];
		memset(this->data, 0, size);
		memcpy(this->data,data_head_buf,data_head_size);
		/*data中的实际数据*/
		data_buf = this->data + data_head_size;
		this->data_head_buf=data_head_buf;
		if(read_hd_data()<0)
		{
			printf("disk read error\n");
			exit(1);
		}
	}
}

int blocks::get_data_head_dize()
{
	return data_head_size;
}
char *blocks::get_data_head()
{
	return data_head_buf;
}
