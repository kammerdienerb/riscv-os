#include "driver.h"
#include "array.h"
#include "kmalloc.h"

Driver  *driver_head;
array_t  driver_states;

void register_driver(Driver *driver) {
    Driver *old_head;

    driver->next      = NULL;
    old_head          = driver_head;
    driver_head       = driver;
    driver_head->next = old_head;
}



#include "drv_rng.h"
#include "drv_blk.h"
#include "drv_gpu.h"
#include "drv_input.h"

void init_drivers(void) {
    driver_states = array_make(Driver_State*);
    register_driver(&DRIVER_RNG);
    register_driver(&DRIVER_BLK);
    register_driver(&DRIVER_GPU);
    register_driver(&DRIVER_INPUT);
}

Driver *find_driver(u32 device_id) {
    Driver *driver;

    driver = driver_head;

    while (driver != NULL) {
        if (driver->device_id == device_id) {
            return driver;
        }
        driver = driver->next;
    }

    return NULL;
}

s64 first_driver_of_type(u32 type, Driver **out_driver, Driver_State **out_driver_state) {
    Driver_State **statep;
    Driver_State  *state;

    FOR_DRIVER_STATE(statep) {
        state = *statep;
        if (state->driver->type == type) {
            *out_driver       = state->driver;
            *out_driver_state = state;
            return 0;
        }
    }

    *out_driver       = NULL;
    *out_driver_state = NULL;

    return -1;
}

Driver_State *driver_for_adid(u32 adid) {
    Driver_State **statep;
    Driver_State  *state;

    FOR_DRIVER_STATE(statep) {
        state = *statep;
        if (state->active_device_id == adid) {
            return state;
        }
    }

    return NULL;
}

s64 start_driver(Driver *driver, u32 active_device_id, u32 irq_number, void *platform_info) {
    Driver_State *state;

    state = kmalloc(sizeof(*state));

    state->driver           = driver;
    state->active_device_id = active_device_id;
    state->irq_number       = irq_number;
    state->platform_info    = platform_info;

    array_push(driver_states, state);

    return driver->init(state);
}
