#include "kconsole.h"
#include "sbi.h"
#include "kprint.h"
#include "array.h"
#include "kmalloc.h"
#include "utils.h"
#include "symbols.h"
#include "hart.h"
#include "pci.h"
#include "rng.h"
#include "blk.h"
#include "gpu.h"
#include "process.h"
#include "sched.h"
#include "vfs.h"
#include "elf.h"

static void _do_tree(File *f, u32 lvl, s32 last) {
    array_t   back;
    u8        b;
    u32       i;
    u8       *bit;
    File     *p;
    File    **it;

    back = array_make(u8);

    p = f;
    for (i = 1; i < lvl; i += 1) {
        p = p->parent;

        b = p && p->parent && p != *(File**)array_last(p->parent->dir_entries);
        array_push(back, b);
    }

    array_rtraverse(back, bit) {
        if (*bit) {
            kprint("│  ");
        } else {
            kprint("   ");
        }
    }

    if (lvl) {
        if (last) {
            kprint("└─ %s%s%_", f->kind == FILE_DIRECTORY ? PR_FG_BLUE : PR_FG_YELLOW, f->name);
        } else {
            kprint("├─ %s%s%_", f->kind == FILE_DIRECTORY ? PR_FG_BLUE : PR_FG_YELLOW, f->name);
        }
    } else {
        kprint(PR_FG_BLUE "%s" PR_RESET, f->name);
    }

    if (f->kind == FILE_DIRECTORY) {
        kprint("/\n");
        array_traverse(f->dir_entries, it) {
            if (strcmp((*it)->name, ".")  != 0
            &&  strcmp((*it)->name, "..") != 0) {
                _do_tree(*it, lvl + 1, it == array_last(f->dir_entries));
            }
        }
    } else {
        kprint("\n");
    }

    array_free(back);
}

static void do_tree(const char *path) {
    File *f;

    f = get_file(path);

    if (f == NULL) {
        kprint("file or directory not found\n");
        return;
    } else if (f->kind != FILE_DIRECTORY) {
        kprint("file is not a directory\n");
        return;
    }

    _do_tree(f, 0, 1);
}

static char cwd[256];

static File *F(const char *path, s32 create) {
    File *f;
    char  buff[256];
    char  dirname[64];

    if (path[0] == '/') {
        f = get_file(path);
    } else {
        strcpy(buff, cwd);
        strcat(buff, "/");
        strcat(buff, path);
        f = get_file(buff);
    }

    if (f == NULL) {
        if (create) {
            path_dirname(path, dirname);
            if (dirname[0] == '/') {
                strcpy(buff, dirname);
                kprint("dirname: %s\n", dirname);
            } else {
                strcpy(buff, cwd);
                strcat(buff, "/");
                strcat(buff, dirname);
            }

            f = get_file(buff);

            if (f == NULL) {
                kprint("file or directory not found: %s\n", buff);
            } else {
                file_create(f, path + strlen(dirname) + 1, FILE_REGULAR);
                f = get_file(path);
            }
        } else {
            kprint("file or directory not found: %s\n", path);
        }
    }

    return f;
}

static void do_cmd(array_t words) {
    char                         *cmd;
    u32                           i;
    s64                           status;
    const char                   *status_string;
    u32                           bus;
    u32                           device;
    volatile PCI_Ecam            *ecam;
    s32                           n_bytes;
    u8                           *bytes;
    u8                            c;
    Scheduler                    *sched;
    tree_it(u64, process_ptr_t)   it;
    Process                      *proc;
    File                         *f;
    File                        **fit;
    s64                           file_len;
    s32                           hart;

    cmd = array_len(words) == 0 ? "" : *(char**)array_item(words, 0);

    if (strcmp(cmd, "harts") == 0) {
        kprint("HART    STATUS\n");
        kprint("--------------\n");
        for (i = 0; i < MAX_HARTS; i += 1) {
            status = sbicall(SBI_HART_STATUS, i);
            status_string = "unknown";
            switch (status) {
                case HART_INVALID:  status_string = PR_BG_RED    "invalid"  PR_RESET; break;
                case HART_STOPPING: status_string = PR_FG_YELLOW "stopping" PR_RESET; break;
                case HART_STOPPED:  status_string = PR_FG_RED    "stopped"  PR_RESET; break;
                case HART_STARTING: status_string = PR_FG_CYAN   "starting" PR_RESET; break;
                case HART_STARTED:  status_string = PR_FG_GREEN  "started"  PR_RESET; break;
            }
            kprint("%m%-4u%_   %8s\n", i, status_string);
        }
    } else if (strcmp(cmd, "map") == 0) {
        kprint("SECTION        START         END\n");
        kprint("--------------------------------\n");
        kprint("%m%-8s%_  %y0x%08X  0x%08X%_\n", "memory", sym_start(memory), sym_end(memory));
        kprint("%m%-8s%_  %y0x%08X  0x%08X%_\n", "text",   sym_start(text),   sym_end(text));
        kprint("%m%-8s%_  %y0x%08X  0x%08X%_\n", "rodata", sym_start(rodata), sym_end(rodata));
        kprint("%m%-8s%_  %y0x%08X  0x%08X%_\n", "data",   sym_start(data),   sym_end(data));
        kprint("%m%-8s%_  %y0x%08X  0x%08X%_\n", "bss",    sym_start(bss),    sym_end(bss));
        kprint("%m%-8s%_  %y0x%08X  0x%08X%_\n", "heap",   sym_start(heap),   sym_end(heap));
    } else if (strcmp(cmd, "lspci") == 0) {
        for (bus = 0; bus < 256; bus += 1) {
            for (device = 0; device < 32; device += 1) {
                ecam = pci_device_ecam_addr(bus, device, 0, 0);
                if (ecam->vendor_id == 0xffff) { continue; }
                kprint("Device at bus %u, device %u (MMIO @ 0x%08X), class: 0x%04x\n",
                        bus, device, ecam, ecam->class_code);
                kprint("   Device ID    : 0x%04x, Vendor ID    : 0x%04x\n",
                        ecam->device_id, ecam->vendor_id);
            }
        }
    } else if (strcmp(cmd, "rand") == 0) {
        if (array_len(words) < 2) {
            kprint("missing argument\n");
        } else {
            n_bytes = stoi(*(char**)array_item(words, 1));
            bytes   = kmalloc(n_bytes);
            rng_fill(bytes, n_bytes);
            hexdump(bytes, n_bytes);
            kfree(bytes);
        }
    } else if (strcmp(cmd, "fault") == 0) {
        *((volatile u32*)NULL) = 123;
    } else if (strcmp(cmd, "display-reset") == 0) {
        gpu_reset_display();
    } else if (strcmp(cmd, "procs") == 0) {
        do {
            kprint(PR_CLS PR_CURSOR_HOME);
            kprint("PID  KIND    STATE     HART      VRUNTIME\n");
            kprint("-----------------------------------------\n");

            for (i = 1; i < MAX_HARTS; i += 1) {
                sched = scheds + i;
                spin_lock(&sched->lock);

                const char *states[] = {"INVALID", "SLEEPING", "WAITING", "RUNNABLE", "RUNNING"};
                const char *kinds[]  = {"KERNEL", "USER", "IDLE"};

                proc = sched->current;
                kprint("%3u  ", proc->pid);
                kprint("%-6s  ", kinds[proc->kind]);
                kprint("%-8s  ", states[proc->state]);
                kprint("%4u  ", proc->on_hart);
                kprint("%12U", proc->vruntime);
                kprint("\n");
                tree_traverse(sched->procs, it) {
                    proc = tree_it_val(it);

                    kprint("%3u  ", proc->pid);
                    kprint("%-6s  ", kinds[proc->kind]);
                    kprint("%-8s  ", states[proc->state]);
                    kprint("%4u  ", proc->on_hart);
                    kprint("%12U", proc->vruntime);
                    kprint("\n");
                }

                spin_unlock(&sched->lock);
            }

            kprint("\npress 'q' to quit\n");

            for (i = 0; i < 100; i += 1) {
                if ((c = sbicall(SBI_GETC)) == 'q') { goto out; }
                WAIT_FOR_INTERRUPT();
            }
        } while ((c = sbicall(SBI_GETC)) != 'q');
out:;
    } else if (strcmp(cmd, "cd") == 0) {
        if (array_len(words) < 2) {
            strcpy(cwd, "/");
        } else {
            if ((f = F(*(char**)array_item(words, 1), 0)) != NULL) {
                if (f->kind != FILE_DIRECTORY) {
                    kprint("file is not a directory\n");
                } else {
                    get_file_path(f, cwd);
                }
            }
        }
    } else if (strcmp(cmd, "ls") == 0) {
        if ((f = F(array_len(words) < 2 ? cwd : *(char**)array_item(words, 1), 0)) != NULL) {
            if (f->kind != FILE_DIRECTORY) {
                kprint("argument is not a directory\n");
            } else {
                array_traverse(f->dir_entries, fit) {
                    kprint("%s%s (%UB)\n", (*fit)->name, (*fit)->kind == FILE_DIRECTORY ? "/" : "", file_size(*fit));
                }
            }
        }
    } else if (strcmp(cmd, "tree") == 0) {
        if ((f = F(array_len(words) < 2 ? cwd : *(char**)array_item(words, 1), 0)) != NULL) {
            bytes = kmalloc(256);
            get_file_path(f, (void*)bytes);
            do_tree((const char*)bytes);
            kfree(bytes);
        }
    } else if (strcmp(cmd, "cat") == 0) {
        if (array_len(words) < 2) {
            kprint("missing argument\n");
        } else {
            if ((f = F(*(char**)array_item(words, 1), 0)) != NULL) {
                if (f->kind != FILE_REGULAR) {
                    kprint("file is not a regular file\n");
                } else {
                    file_len = file_size(f);

                    if (file_len == -1) { kprint("an error occurred\n"); }
                    else {
                        bytes = kmalloc(file_len);
                        file_read(f, bytes, 0, file_len);
                        for (i = 0; i < file_len; i += 1) {
                            sbicall(SBI_PUTC, bytes[i]);
                        }
                        kfree(bytes);
                    }
                }
            }
        }
    } else if (strcmp(cmd, "hexcat") == 0) {
        if (array_len(words) < 2) {
            kprint("missing argument\n");
        } else {
            if ((f = F(*(char**)array_item(words, 1), 0)) != NULL) {
                if (f->kind != FILE_REGULAR) {
                    kprint("file is not a regular file\n");
                } else {
                    file_len = file_size(f);

                    if (file_len == -1) { kprint("an error occurred\n"); }
                    else {
                        bytes = kmalloc(file_len);
                        file_read(f, bytes, 0, file_len);
                        hexdump(bytes, file_len);
                        kfree(bytes);
                    }
                }
            }
        }
    } else if (strcmp(cmd, "append") == 0) {
        if (array_len(words) < 2) {
            kprint("missing file and content arguments\n");
        } else if (array_len(words) < 3) {
            kprint("missing content argument\n");
        } else {
            if ((f = F(*(char**)array_item(words, 1), 1)) != NULL) {
                if (f->kind != FILE_REGULAR) {
                    kprint("file is not a regular file\n");
                } else {
                }
            }
        }
    } else if (strcmp(cmd, "run") == 0) {
        if (array_len(words) < 1) {
            kprint("missing file argument\n");
        } else if ((f = F(*(char**)array_item(words, 1), 0)) != NULL) {
            if (f->kind != FILE_REGULAR) {
                kprint("file is not a regular file\n");
            } else {
                file_len = file_size(f);

                if (file_len == -1) { kprint("an error occurred\n"); }
                else {
                    bytes = kmalloc(file_len);
                    file_read(f, bytes, 0, file_len);
                    proc = new_process(PROC_USER);
                    status = elf_process_image(proc, bytes);
                    if (status == 0) {
                        hart = get_idle_hart();
                        sched_add_on_hart(proc, hart == -1 ? 1 : hart);
                    } else {
                        kprint("could not execute file\n");
                    }
                    kfree(bytes);
                }
            }
        }
    } else if (strcmp(cmd, "help") == 0) {
        kprint("%bhelp%_                %mShow this help.%_\n");
        kprint("%bharts%_               %mPrint the status of each HART.%_\n");
        kprint("%bmap%_                 %mShow a memory map.%_\n");
        kprint("%blspci%_               %mList PCI devices.%_\n");
        kprint("%brand%_ %gN%_              %mGenerate %gN%m random bytes and hex dump them.%_\n");
        kprint("%bfault%_               %mCause a page fault.%_\n");
        kprint("%bdisplay-reset%_       %mReset the display if the GPU is active.%_\n");
        kprint("%bprocs%_               %mShow scheduling information in an updating table.%_\n");
        kprint("%bcd%_ %g[PATH]%_           %mChange the current working directory to '/' or %gPATH%m if provided.%_\n");
        kprint("%bls%_ %g[PATH]%_           %mShow the contents of the current directory or %gPATH%m if provided.%_\n");
        kprint("%btree%_ %g[PATH]%_         %mShow a tree view of the current directory or %gPATH%m if provided.%_\n");
        kprint("%bcat%_ %gPATH%_            %mPrint the contents of the file at %gPATH%m.%_\n");
        kprint("%bhexcat%_ %gPATH%_         %mHexdump the contents of the file at %gPATH%m.%_\n");
        kprint("%bappend%_ %gPATH%_ %gSTRING%_  %mAppend %gSTRING%m to the file at %gPATH%m, creating it if it does not exist.%_\n");
        kprint("%brun%_ %gPATH%_            %mRun the ELF file at %gPATH%m.%_\n");
    } else {
        kprint("%runknown command '%s'%_\n", cmd);
    }
}

void start_kconsole(void) {
    u8       c;
    char     cmd_buff[512];
    char    *cmd_p;
    array_t  words;

    cwd[0] = '/';
    cwd[1] = 0;

    words = array_make(char*);
    cmd_p = "tree";
    array_push(words, cmd_p);
    do_cmd(words);
    array_free(words);

    cmd_buff[0] = 0;
    cmd_p       = cmd_buff;

    kprint("\r\e[0K%cOS %b%s%c >%_ %s", cwd, cmd_buff);

    for (;;) {
        WAIT_FOR_INTERRUPT();

        while ((c = sbicall(SBI_GETC)) != 255) {

            if (c == '\r') {
                kprint("\n");

                *cmd_p = 0;
                words = sh_split(cmd_buff);
                do_cmd(words);
                free_string_array(words);
                *(cmd_p = cmd_buff) = 0;
            } else {
                switch (c) {
                    case 127:
                    case '\b':
                        if (cmd_p > cmd_buff) { cmd_p -= 1; }
                        break;
                    default:
                        *cmd_p  = c;
                        cmd_p  += 1;
                        break;
                }

                *cmd_p = 0;
            }

            *cmd_p = 0;
            kprint("\r\e[0K%cOS %b%s%c >%_ %s", cwd, cmd_buff);
        }
    }
}
