#include "blk.h"
#include "driver.h"
#include "kprint.h"

s64 blk_read(u32 adid, u64 offset, u8 *buff, u64 len) {
    Driver_State *state;

    if ((state = driver_for_adid(adid)) == NULL) {
        kprint("no viable block driver or device found\n");
        return -1;
    }

    return state->driver->block.read(state, offset, buff, len);
}

s64 blk_write(u32 adid, u64 offset, const u8 *buff, u64 len) {
    Driver_State *state;

    if ((state = driver_for_adid(adid)) == NULL) {
        kprint("no viable block driver or device found\n");
        return -1;
    }

    return state->driver->block.write(state, offset, buff, len);
}
