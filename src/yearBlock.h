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

class yearBlock
{
public:
	yearBlock(){};
	virtual ~yearBlock(){};
	/*把年块的数据写入硬盘*/
	virtual void year_block_save();

private:
	static unsigned char buf_head[YEAR_HEAD_BLOCK_SIZE*BLOCKSIZE];
	/*年块初始化*/
	virtual int year_init();
};





#endif /* YEARBLOCK_H_ */
