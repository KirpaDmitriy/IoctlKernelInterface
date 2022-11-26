#include <linux/cdev.h>
#include <linux/delay.h>
#include <linux/device.h>
#include <linux/fs.h>
#include <linux/init.h>
#include <linux/irq.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/poll.h>
#include <linux/fs.h>
#include <linux/blkdev.h>
#include <linux/types.h>

#include "md.h"
#define SUCCESS 0
#define DEVICE_NAME "char_dev"
#define DEVICE1_NAME "char_dev1"
#define BUF_LEN 250

static char message[BUF_LEN];
static char message1[BUF_LEN];

static struct class *cls_dm_io;
static struct class *cls_tmt;

static ssize_t device_read_dm_io(struct file *file,
                           char __user *buffer,
                           size_t length,
                           loff_t *offset) {
    int bytes_read = 0;
    static int last_was_error = 0;
    pr_info("Searching for %s", message);
    struct block_device *bd = blkdev_get_by_path(message, FMODE_READ, NULL);
    if(IS_ERR(bd)) {
	copy_to_user(buffer, "Nothing found\n", 15);
        if(last_was_error == 0) {
            last_was_error = 1;
            return 15;
	}
        return 0;
    }
    last_was_error = 0;
    sector_t str_sec = bd->bd_start_sect;
    sector_t n_sec = (sector_t) (bd->bd_inode->i_size >> SECTOR_SHIFT);

    char *mt = "Block device pointer is %p, start sector is %d, number of sectors is %d";
    char kostyl[BUF_LEN];
    char *message_ptr = kostyl;

    size_t string_size = snprintf(NULL, 0, mt, bd, str_sec, n_sec);
    snprintf(message_ptr, string_size, mt, bd, str_sec, n_sec);

    if (!*(message_ptr + *offset)) {
        *offset = 0;
        return 0;
    }

    message_ptr += *offset;

    while (length && *message_ptr) {
        put_user(*(message_ptr++), buffer++);
        length--;
        bytes_read++;
    }
    put_user('\n', buffer++);
    pr_info("Read %d bytes, %ld left\n", bytes_read, length);
    *offset += bytes_read;
    return bytes_read + 1;
}

static ssize_t device_write_dm_io(struct file *file, const char __user *buffer,
                            size_t length, loff_t *offset) {
    int i;

    pr_info("device_write_dm_io(%p,%p,%ld)", file, buffer, length);

    for (i = 0; i < length && i < BUF_LEN; i++) {
        get_user(message[i], buffer + i);
    }
    char endstr = '\0';
    get_user(message[i], &endstr);

    return i;
}


////////////////////////////////////////////////////////////////////


static ssize_t device_read_tgt(struct file *file,
                           char __user *buffer,
                           size_t length,
                           loff_t *offset) {
    static int read_status = 0;
    if(read_status != 0) {
        read_status = 0;
        return 0;
    }
    long seeked_pid = 1;
    kstrtol(message1, 10, &seeked_pid);
    struct task_struct *task;
    int found_flag = 0;
    for_each_process(task) {
        if(task->pid == seeked_pid) {
            found_flag = 1;
            break;
        }
    }
    if(found_flag == 1) {
        struct thread_group_cputimer tgt = task->signal->cputimer;
        atomic64_t t = tgt.cputime_atomic.utime;
        const char fields_values_str[BUF_LEN];
        const char format_answer[] = "Utime: %u\n";
        size_t string_size = snprintf(NULL, 0, format_answer, t) + 1;
        snprintf(fields_values_str, string_size, format_answer, t);
        copy_to_user(buffer, fields_values_str, string_size);
        read_status = string_size;
    }
    else {
        copy_to_user(buffer, "Nothing found\n", 15);
        read_status = 15;
    }
    return read_status;
}

static ssize_t device_write_tgt(struct file *file, const char __user *buffer,
                            size_t length, loff_t *offset) {
    int i;

    pr_info("device_write_dm_io(%p,%p,%ld)", file, buffer, length);

    for (i = 0; i < length && i < BUF_LEN; i++) {
        get_user(message1[i], buffer + i);
    }
    char endstr = '\0';
    get_user(message1[i], &endstr);

    return i;
}

static struct file_operations fops_dm_io = {
    .read = device_read_dm_io,
    .write = device_write_dm_io,
};

static struct file_operations fops_tgt = {
    .read = device_read_tgt,
    .write = device_write_tgt,
};

static int __init chardev2_init(void) {
    int ret_val = register_chrdev(MAJOR_NUM, DEVICE_NAME, &fops_dm_io); 
    int ret_val1 = register_chrdev(MAJOR_NUM, DEVICE1_NAME, &fops_tgt);

    if ((ret_val < 0)  || (ret_val1 < 0)) {
        pr_alert("Sorry, registering the character device failed");
        return 1;
    }

    cls_dm_io = class_create(THIS_MODULE, DEVICE_FILE_NAME);
    device_create(cls_dm_io, NULL, MKDEV(MAJOR_NUM, 0), NULL, DEVICE_FILE_NAME);
    pr_info("Device created on /dev/%s\n", DEVICE_FILE_NAME);

    cls_tmt = class_create(THIS_MODULE, DEVICE_FILE1_NAME);
    device_create(cls_tmt, NULL, MKDEV(MAJOR_NUM, 0), NULL, DEVICE_FILE1_NAME);
    pr_info("Device created on /dev/%s\n", DEVICE_FILE1_NAME);

    return 0;
}

static void __exit chardev2_exit(void) {
    device_destroy(cls_dm_io, MKDEV(MAJOR_NUM, 0));
    class_destroy(cls_dm_io);
    device_destroy(cls_tmt, MKDEV(MAJOR_NUM, 0));
    class_destroy(cls_tmt);

    unregister_chrdev(MAJOR_NUM, DEVICE_NAME);
    unregister_chrdev(MAJOR_NUM, DEVICE1_NAME);
}

module_init(chardev2_init);
module_exit(chardev2_exit);

MODULE_LICENSE("GPL");
