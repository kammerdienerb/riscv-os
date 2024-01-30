#include "rng.h"
#include "driver.h"
#include "kprint.h"

s64 rng_fill(u8 *buff, u64 len) {
    Driver       *driver;
    Driver_State *state;

    if (first_driver_of_type(DRV_RNG, &driver, &state) != 0) {
        kprint("no viable RNG driver or device found\n");
        return -1;
    }

    return driver->rng.service(state, buff, len);
}
