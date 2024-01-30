#include "common.h"
#include "vfs.h"
#include "driver.h"
#include "utils.h"
#include "kmalloc.h"
#include "array.h"
#include "blk.h"

static s64 identify(u32 adid);
static s64 mount(u32 adid, File *mount_point);
static s64 create(File *dir, const char *name, u32 kind);
static s64 read(File *file, u8 *dst, u64 offset, u64 n_bytes);
static s64 write(File *file, u8 *src, u64 offset, u64 n_bytes);
static s64 size(File *file);

static FS_Impl impl = {
    .name     = "ext4",
    .identify = identify,
    .mount    = mount,
    .create   = create,
    .read     = read,
    .write    = write,
    .size     = size,
};


#define __nonstring __attribute__((nonstring))

#define EXT2_LABEL_LEN (16)


typedef struct {
/*000*/
    u32    s_inodes_count;                               /* Inodes count                       */
    u32    s_blocks_count;                               /* Blocks count                       */
    u32    s_r_blocks_count;                             /* Reserved blocks count              */
    u32    s_free_blocks_count;                          /* Free blocks count                  */
/*010*/
    u32    s_free_inodes_count;                          /* Free inodes count                  */
    u32    s_first_data_block;                           /* First Data Block                   */
    u32    s_log_block_size;                             /* Block size                         */
    u32    s_log_cluster_size;                           /* Allocation cluster size            */
/*020*/
    u32    s_blocks_per_group;                           /* # Blocks per group                 */
    u32    s_clusters_per_group;                         /* # Fragments per group              */
    u32    s_inodes_per_group;                           /* # Inodes per group                 */
    u32    s_mtime;                                      /* Mount time                         */
/*030*/
    u32    s_wtime;                                      /* Write time                         */
    u16    s_mnt_count;                                  /* Mount count                        */
    s16    s_max_mnt_count;                              /* Maximal mount count                */
    u16    s_magic;                                      /* Magic signature                    */
    u16    s_state;                                      /* File system state                  */
    u16    s_errors;                                     /* Behaviour when detecting errors    */
    u16    s_minor_rev_level;                            /* minor revision level               */
/*040*/
    u32    s_lastcheck;                                  /* time of last check                 */
    u32    s_checkinterval;                              /* max. time between checks           */
    u32    s_creator_os;                                 /* OS                                 */
    u32    s_rev_level;                                  /* Revision level                     */
/*050*/
    u16    s_def_resuid;                                 /* Default uid for reserved blocks    */
    u16    s_def_resgid;                                 /* Default gid for reserved blocks    */
    /*
     * These fields are for EXT2_DYNAMIC_REV superblocks only.
     *
     * Note: the difference between the compatible feature set and
     * the incompatible feature set is that if there is a bit set
     * in the incompatible feature set that the kernel doesn't
     * know about, it should refuse to mount the filesystem.
     *
     * e2fsck's requirements are more strict; if it doesn't know
     * about a feature in either the compatible or incompatible
     * feature set, it must abort and not try to meddle with
     * things it doesn't understand...
     */
    u32    s_first_ino;                                  /* First non-reserved inode           */
    u16    s_inode_size;                                 /* size of inode structure            */
    u16    s_block_group_nr;                             /* block group # of this superblock   */
    u32    s_feature_compat;                             /* compatible feature set             */
/*060*/
    u32    s_feature_incompat;                           /* incompatible feature set           */
    u32    s_feature_ro_compat;                          /* readonly-compatible feature set    */
/*068*/
    u8     s_uuid[16] __nonstring;                       /* 128-bit uuid for volume            */
/*078*/
    u8     s_volume_name[EXT2_LABEL_LEN] __nonstring;    /* volume name, no NUL?               */
/*088*/
    u8     s_last_mounted[64] __nonstring;               /* directory last mounted on, no NUL? */
/*0c8*/
    u32    s_algorithm_usage_bitmap;                     /* For compression                    */
    /*
     * Performance hints.  Directory preallocation should only
     * happen if the EXT2_FEATURE_COMPAT_DIR_PREALLOC flag is on.
     */
    u8     s_prealloc_blocks;                            /* Nr of blocks to try to preallocate*/
    u8     s_prealloc_dir_blocks;                        /* Nr to preallocate for dirs         */
    u16    s_reserved_gdt_blocks;                        /* Per group table for online growth  */
    /*
     * Journaling support valid if EXT2_FEATURE_COMPAT_HAS_JOURNAL set.
     */
/*0d0*/
    u8     s_journal_uuid[16] __nonstring;               /* uuid of journal superblock         */
/*0e0*/
    u32    s_journal_inum;                               /* inode number of journal file       */
    u32    s_journal_dev;                                /* device number of journal file      */
    u32    s_last_orphan;                                /* start of list of inodes to delete  */
/*0ec*/
    u32    s_hash_seed[4];                               /* HTREE hash seed                    */
/*0fc*/
    u8     s_def_hash_version;                           /* Default hash version to use        */
    u8     s_jnl_backup_type;                            /* Default type of journal backup     */
    u16    s_desc_size;                                  /* Group desc. size: INCOMPAT_64BIT   */
/*100*/
    u32    s_default_mount_opts;                         /* default EXT2_MOUNT_* flags used    */
    u32    s_first_meta_bg;                              /* First metablock group              */
    u32    s_mkfs_time;                                  /* When the filesystem was created    */
/*10c*/
    u32    s_jnl_blocks[17];                             /* Backup of the journal inode        */
/*150*/
    u32    s_blocks_count_hi;                            /* Blocks count high 32bits           */
    u32    s_r_blocks_count_hi;                          /* Reserved blocks count high 32 bits*/
    u32    s_free_blocks_hi;                             /* Free blocks count                  */
    u16    s_min_extra_isize;                            /* All inodes have at least # bytes   */
    u16    s_want_extra_isize;                           /* New inodes should reserve # bytes  */
/*160*/
    u32    s_flags;                                      /* Miscellaneous flags                */
    u16    s_raid_stride;                                /* RAID stride in blocks              */
    u16    s_mmp_update_interval;                        /* # seconds to wait in MMP checking  */
    u64    s_mmp_block;                                  /* Block for multi-mount protection   */
/*170*/
    u32    s_raid_stripe_width;                          /* blocks on all data disks (N*stride)*/
    u8     s_log_groups_per_flex;                        /* FLEX_BG group size                 */
    u8     s_checksum_type;                              /* metadata checksum algorithm        */
    u8     s_encryption_level;                           /* versioning level for encryption    */
    u8     s_reserved_pad;                               /* Padding to next 32bits             */
    u64    s_kbytes_written;                             /* nr of lifetime kilobytes written   */
/*180*/
    u32    s_snapshot_inum;                              /* Inode number of active snapshot    */
    u32    s_snapshot_id;                                /* sequential ID of active snapshot   */
    u64    s_snapshot_r_blocks_count;                    /* active snapshot reserved blocks    */
/*190*/
    u32    s_snapshot_list;                              /* inode number of disk snapshot list */

    u32    s_error_count;                                /* number of fs errors                */
    u32    s_first_error_time;                           /* first time an error happened       */
    u32    s_first_error_ino;                            /* inode involved in first error      */
/*1a0*/
    u64    s_first_error_block;                          /* block involved in first error      */
    u8     s_first_error_func[32] __nonstring;           /* function where error hit, no NUL?  */
/*1c8*/
    u32    s_first_error_line;                           /* line number where error happened   */
    u32    s_last_error_time;                            /* most recent time of an error       */
/*1d0*/
    u32    s_last_error_ino;                             /* inode involved in last error       */
    u32    s_last_error_line;                            /* line number where error happened   */
    u64    s_last_error_block;                           /* block involved of last error       */
/*1e0*/
    u8     s_last_error_func[32] __nonstring;            /* function where error hit, no NUL?  */

/*200*/
    u8     s_mount_opts[64] __nonstring;                 /* default mount options, no NUL?     */
/*240*/
    u32    s_usr_quota_inum;                             /* inode number of user quota file    */
    u32    s_grp_quota_inum;                             /* inode number of group quota file   */
    u32    s_overhead_clusters;                          /* overhead blocks/clusters in fs     */
/*24c*/
    u32    s_backup_bgs[2];                              /* If sparse_super2 enabled           */
/*254*/
    u8     s_encrypt_algos[4];                           /* Encryption algorithms in use       */
/*258*/
    u8     s_encrypt_pw_salt[16];                        /* Salt used for string2key algorithm */
/*268*/
    u32    s_lpf_ino;                                    /* Location of the lost+found inode   */
    u32    s_prj_quota_inum;                             /* inode for tracking project quota   */
/*270*/
    u32    s_checksum_seed;                              /* crc32c(orig_uuid) if csum_seed set */
/*274*/
    u8     s_wtime_hi;
    u8     s_mtime_hi;
    u8     s_mkfs_time_hi;
    u8     s_lastcheck_hi;
    u8     s_first_error_time_hi;
    u8     s_last_error_time_hi;
    u8     s_first_error_errcode;
    u8     s_last_error_errcode;
/*27c*/
    u16    s_encoding;                                   /* Filename charset encoding          */
    u16    s_encoding_flags;                             /* Filename charset encoding flags    */
    u32    s_reserved[95];                               /* Padding to the end of the block    */
/*3fc*/
    u32    s_checksum;                                   /* crc32c(superblock)                 */
} Super_Block;




typedef struct {
    u32         adid;
    Super_Block sb;
} Instance;

static array_t instances;

void fs_ext4(void) {
    instances = array_make(Instance*);
    fs_impl(FS_EXT4, impl);
}


static Super_Block *get_super_block(u32 adid) {
    Instance **it;

    array_traverse(instances, it) {
        if ((*it)->adid == adid) {
            return &((*it)->sb);
        }
    }

    return NULL;
}

static s64 identify(u32 adid) {
    Super_Block  sb;
    Instance    *inst;

    if (blk_read(adid, 1024, (void*)&sb, sizeof(sb)) == -1) {
        return 0;
    }

    if (sb.s_magic == 0xEF53) {
        inst = kmalloc(sizeof(*inst));
        inst->adid = adid;
        memcpy(&inst->sb, &sb, sizeof(inst->sb));
        array_push(instances, inst);
        return 1;
    }

    return 0;
}

static s64 mount(u32 adid, File *mount_point) {
    Super_Block *sb;

    if ((sb = get_super_block(adid)) == NULL) { return -1; }

    return -1;
}

static s64 create(File *dir, const char *name, u32 kind) {
    return -1;
}

static s64 read(File *file, u8 *dst, u64 offset, u64 n_bytes) {
    return -1;
}

static s64 write(File *file, u8 *src, u64 offset, u64 n_bytes) {
    return -1;
}

static s64 size(File *file) {
    return -1;
}
