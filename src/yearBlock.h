/*
 * yearBlock.h
 *
 *  Created on: 2013-8-19
 *      Author: xy
 */

#ifndef YEARBLOCK_H_
#define YEARBLOCK_H_
#include "blocks.h"


struct year_queue_data
{
	unsigned int queue_size ,queue_head,queue_tail;
};
static char year_data_head[8]={0x59,0x45,0x41,0x52,0xa5,0xa5,0xa5,0xa5};
class yearBlock:private  blocks
{

public:
	yearBlock():blocks(YEAR_SEEK,YEAR_HEAD_BLOCK_SIZE*BLOCKSIZE,sizeof(year_data_head),year_data_head)
	{
		printf("%s\n",__FUNCTION__);


	};
	yearBlock(int a,int b):blocks(a,b){};
	~yearBlock(){printf("%s\n",__FUNCTION__);};




private:
	/*年块数据头*/


};





#endif /* YEARBLOCK_H_ */
