#ifndef __DRIVER_H__
#define __DRIVER_H__

#include "common.h"
#include "array.h"

enum {
    DRV_RNG,
    DRV_BLOCK,
    DRV_GPU,
    DRV_INPUT
};

struct Driver;

typedef struct {
    struct Driver *driver;
    u32            active_device_id;
    u32            irq_number;
    void          *platform_info;
    void          *data;
} Driver_State;

#define DRV_INIT_FN(name, arg1_name) \
    s64 name(Driver_State *arg1_name)
#define DRV_IRQ_FN(name, arg1_name) \
    s64 name(Driver_State *arg1_name)
#define DRV_RNG_SERVICE_FN(name, arg1_name, arg2_name, arg3_name) \
    s64 name(Driver_State *arg1_name, u8 *arg2_name, u64 arg3_name)
#define DRV_BLK_READ_FN(name, arg1_name, arg2_name, arg3_name, arg4_name) \
    s64 name(Driver_State *arg1_name, u64 arg2_name, u8 *arg3_name, u64 arg4_name)
#define DRV_BLK_WRITE_FN(name, arg1_name, arg2_name, arg3_name, arg4_name) \
    s64 name(Driver_State *arg1_name, u64 arg2_name, const u8 *arg3_name, u64 arg4_name)
#define DRV_GPU_RESET_DISPLAY_FN(name, arg1_name) \
    s64 name(Driver_State *arg1_name)
#define DRV_GPU_CLEAR_FN(name, arg1_name, arg2_name) \
    s64 name(Driver_State *arg1_name, u32 arg2_name)
#define DRV_GPU_GET_RECT_FN(name, arg1_name, arg2_name, arg3_name, arg4_name, arg5_name) \
    s64 name(Driver_State *arg1_name, u32 *arg2_name, u32 *arg3_name, u32 *arg4_name, u32 *arg5_name)
#define DRV_GPU_PIXELS_FN(name, arg1_name, arg2_name, arg3_name, arg4_name, arg5_name, arg6_name) \
    s64 name(Driver_State *arg1_name, u32 arg2_name, u32 arg3_name, u32 arg4_name, u32 arg5_name, u32 *arg6_name)
#define DRV_GPU_RECT_FN(name, arg1_name, arg2_name, arg3_name, arg4_name, arg5_name, arg6_name) \
    s64 name(Driver_State *arg1_name, u32 arg2_name, u32 arg3_name, u32 arg4_name, u32 arg5_name, u32 arg6_name)
#define DRV_GPU_COMMIT_FN(name, arg1_name) \
    s64 name(Driver_State *arg1_name)

typedef s64 (*Driver_Init_Fn)(Driver_State*);
typedef s64 (*Driver_IRQ_Fn)(Driver_State*);
typedef s64 (*Driver_RNG_Service_Fn)(Driver_State*, u8*, u64);
typedef s64 (*Driver_BLOCK_Read_Fn)(Driver_State*, u64, u8*, u64);
typedef s64 (*Driver_BLOCK_Write_Fn)(Driver_State*, u64, const u8*, u64);
typedef s64 (*Driver_GPU_Reset_Display_Fn)(Driver_State*);
typedef s64 (*Driver_GPU_Clear_Fn)(Driver_State*, u32);
typedef s64 (*Driver_GPU_Get_Rect_Fn)(Driver_State*, u32*, u32*, u32*, u32*);
typedef s64 (*Driver_GPU_Pixels_Fn)(Driver_State*, u32, u32, u32, u32, u32*);
typedef s64 (*Driver_GPU_Rect_Fn)(Driver_State*, u32, u32, u32, u32, u32);
typedef s64 (*Driver_GPU_Commit_Fn)(Driver_State*);

typedef struct Driver {
    const char     *name;
    u32             device_id;
    u32             type;
    Driver_Init_Fn  init;
    Driver_IRQ_Fn   irq;
    union {
        struct {
            Driver_RNG_Service_Fn service;
        } rng;
        struct {
            Driver_BLOCK_Read_Fn  read;
            Driver_BLOCK_Write_Fn write;
        } block;
        struct {
            Driver_GPU_Reset_Display_Fn reset_display;
            Driver_GPU_Clear_Fn         clear;
            Driver_GPU_Get_Rect_Fn      get_rect;
            Driver_GPU_Pixels_Fn        pixels;
            Driver_GPU_Rect_Fn          rect;
            Driver_GPU_Commit_Fn        commit;
        } gpu;
        struct {
        } input;
    };
    struct Driver *next;
} Driver;



extern Driver  *driver_head;
extern array_t  driver_states;

void          init_drivers(void);
Driver       *find_driver(u32 device_id);
s64           first_driver_of_type(u32 type, Driver **driver, Driver_State **driver_state);
Driver_State *driver_for_adid(u32 adid);
s64           start_driver(Driver *driver, u32 active_device_id, u32 irq_number, void *platform_info);
void          register_driver(Driver *driver);


#define FOR_DRIVER(drv_ptr) \
    for ((drv_ptr) = driver_head; (drv_ptr) != NULL; (drv_ptr) = (drv_ptr)->next)

#define FOR_DRIVER_STATE(state_ptr_ptr) array_traverse(driver_states, (state_ptr_ptr))

#define PCI_TO_DEVICE_ID(vendor, device)   ((vendor << 16) + device)
#define PCI_TO_ACTIVE_DEVICE_ID(bus, slot) ((bus    << 16) + slot)

#endif
