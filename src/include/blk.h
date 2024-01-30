#ifndef __BLK_H__
#define __BLK_H__

#include "common.h"

s64 blk_read(u32 adid, u64 offset, u8 *buff, u64 len);
s64 blk_write(u32 adid, u64 offset, const u8 *buff, u64 len);

#endif
