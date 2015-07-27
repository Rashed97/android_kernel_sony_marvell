/*
 * YAFFS: Yet Another Flash File System. A NAND-flash specific file system.
 *
 * Copyright (C) 2002-2010 Aleph One Ltd.
 *   for Toby Churchill Ltd and Brightstar Engineering
 *
 * Created by Charles Manning <charles@aleph1.co.uk>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include "yaffs_nand.h"
#include "yaffs_tagscompat.h"

#include "yaffs_getblockinfo.h"
#include "yaffs_summary.h"

int yaffs_ReadChunkWithTagsFromNAND(yaffs_Device *dev, int chunkInNAND,
					   __u8 *buffer,
					   yaffs_ExtendedTags *tags)
{
	int result;
	yaffs_ExtendedTags localTags;

	int flash_chunk = chunkInNAND - dev->chunkOffset;

	dev->nPageReads++;

	/* If there are no tags provided, use local tags to get prioritised gc working */
	if (!tags)
		tags = &localTags;

	if (dev->param.readChunkWithTagsFromNAND)
		result = dev->param.readChunkWithTagsFromNAND(dev, flash_chunk, buffer,
						      tags);
	else
		result = yaffs_TagsCompatabilityReadChunkWithTagsFromNAND(dev,
									flash_chunk,
									buffer,
									tags);
	if (tags &&
	   tags->eccResult > YAFFS_ECC_RESULT_NO_ERROR) {

		yaffs_BlockInfo *bi;
		bi = yaffs_GetBlockInfo(dev, chunkInNAND/dev->param.nChunksPerBlock);
		if (tags->eccResult == YAFFS_ECC_RESULT_UNFIXED) {
			yaffs_HandleUncorrectableChunkError(dev, bi);
		} else {
			yaffs_HandleChunkError(dev, bi);
		}
	}

	return result;
}

int yaffs_WriteChunkWithTagsToNAND(yaffs_Device *dev,
						   int chunkInNAND,
						   const __u8 *buffer,
						   yaffs_ExtendedTags *tags)
{
	int result;
	int flash_chunk = chunkInNAND - dev->chunkOffset;

	dev->nPageWrites++;



	if (tags) {
		tags->sequenceNumber = dev->sequenceNumber;
		tags->chunkUsed = 1;
		T(YAFFS_TRACE_WRITE,
		  (TSTR("Writing chunk %d tags %d %d" TENDSTR), chunkInNAND,
		   tags->objectId, tags->chunkId));
	} else {
		T(YAFFS_TRACE_ERROR, (TSTR("Writing with no tags" TENDSTR)));
		YBUG();
	}

	if (dev->param.writeChunkWithTagsToNAND)
		result = dev->param.writeChunkWithTagsToNAND(dev, flash_chunk, buffer,
						     tags);
	else
		result = yaffs_TagsCompatabilityWriteChunkWithTagsToNAND(dev,
								       flash_chunk,
								       buffer,
								       tags);

	yaffs_summary_add(dev, tags, chunkInNAND);

	return result;
}

int yaffs_MarkBlockBad(yaffs_Device *dev, int blockNo)
{
	blockNo -= dev->blockOffset;


	if (dev->param.markNANDBlockBad)
		return dev->param.markNANDBlockBad(dev, blockNo);
	else
		return yaffs_TagsCompatabilityMarkNANDBlockBad(dev, blockNo);
}

int yaffs_QueryInitialBlockState(yaffs_Device *dev,
						 int blockNo,
						 yaffs_BlockState *state,
						 __u32 *sequenceNumber)
{
	blockNo -= dev->blockOffset;

	if (dev->param.queryNANDBlock)
		return dev->param.queryNANDBlock(dev, blockNo, state, sequenceNumber);
	else
		return yaffs_TagsCompatabilityQueryNANDBlock(dev, blockNo,
							     state,
							     sequenceNumber);
}


int yaffs_EraseBlockInNAND(struct yaffs_DeviceStruct *dev,
				  int blockInNAND)
{
	int result;

	blockInNAND -= dev->blockOffset;

	dev->nBlockErasures++;

	result = dev->param.eraseBlockInNAND(dev, blockInNAND);

	return result;
}

int yaffs_InitialiseNAND(struct yaffs_DeviceStruct *dev)
{
	if(dev->param.initialiseNAND)
		return dev->param.initialiseNAND(dev);
	return YAFFS_OK;
}



