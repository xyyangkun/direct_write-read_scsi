/*
 * dayBlock.h
 *
 *  Created on: 2013-8-20
 *      Author: yangkun
 */

#ifndef DAYBLOCK_H_
#define DAYBLOCK_H_
#include "blocks.h"

static  char day_data_head[8]={0x4d,0x4f,0x4e,0x54,0x5a,0xa5,0xa5,0x5a};
class dayBlock:public blocks
{
public:
	//dayBlock();
	dayBlock(long long seek=YEAR_SEEK+YEAR_HEAD_BLOCK_SIZE,int size=DATE_HEAD_BLOCK_SIZE*BLOCKSIZE,int data_head_size=sizeof(day_data_head),char *data_head_buf=day_data_head) \
			:blocks(seek,size,data_head_size,data_head_buf){};
	virtual ~dayBlock();

};

#endif /* DAYBLOCK_H_ */
