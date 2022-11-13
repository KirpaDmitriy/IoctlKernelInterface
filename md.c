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
#define BUF_LEN 250

enum {
    CDEV_NOT_USED = 0,
    CDEV_EXCLUSIVE_OPEN = 1,
};

/* Открыто ли сейчас устройство? Служит для предотвращения
 * конкурентного доступа к одному устройству.
 */
static atomic_t already_open = ATOMIC_INIT(CDEV_NOT_USED);

static char message[BUF_LEN];

static struct class *cls;

static int device_open(struct inode *inode, struct file *file) {
    pr_info("device_open(%p)\n", file);

    try_module_get(THIS_MODULE);
    return SUCCESS;
}

static int device_release(struct inode *inode, struct file *file) {
    pr_info("device_release(%p,%p)\n", inode, file);

    module_put(THIS_MODULE);
    return SUCCESS;
}

static ssize_t device_read(struct file *file,
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

static ssize_t device_write(struct file *file, const char __user *buffer,
                            size_t length, loff_t *offset) {
    int i;

    pr_info("device_write(%p,%p,%ld)", file, buffer, length);

    for (i = 0; i < length && i < BUF_LEN; i++) {
        get_user(message[i], buffer + i);
    }
    char endstr = '\0';
    get_user(message[i], &endstr);

    return i;
}

static long device_ioctl(struct file *file,
             unsigned int ioctl_num,
             unsigned long ioctl_param) {
    int i;
    long ret = SUCCESS;

    /* Мы не хотим взаимодействовать с двумя процессами одновременно */ 
    if (atomic_cmpxchg(&already_open, CDEV_NOT_USED, CDEV_EXCLUSIVE_OPEN)) 
        return -EBUSY;
 
    /* Переключение согласно вызванной ioctl. */ 
    switch (ioctl_num) {
    case IOCTL_SET_MSG: {
        /* Получение указателя на сообщение (в пользовательском  
         * пространстве) и установка его как сообщения устройства. 
         * Получение параметра, передаваемого ioctl процессом. 
         */ 
        char __user *tmp = (char __user *)ioctl_param; 
        char ch; 
 
        /* Определение длины сообщения. */ 
        get_user(ch, tmp); 
        for (i = 0; ch && i < BUF_LEN; i++, tmp++) 
            get_user(ch, tmp); 
 
        device_write(file, (char __user *)ioctl_param, i, NULL); 
        break; 
    } 
    case IOCTL_GET_MSG: { 
        loff_t offset = 0; 
 
        /* Передача текущего сообщения вызывающему процессу. Получаемый 
         * параметр является указателем, который мы заполняем. 
         */ 
        i = device_read(file, (char __user *)ioctl_param, 99, &offset); 
 
        /* Помещаем в конец буфера нуль, чтобы он правильно завершился. 
         */ 
        put_user('\0', (char __user *)ioctl_param + i); 
        break; 
    } 
    case IOCTL_GET_NTH_BYTE: 
        /* Эта ioctl является и вводом (ioctl_param), и выводом 
         * (возвращаемым значением этой функции). 
         */ 
        ret = (long)message[ioctl_param]; 
        break; 
    }

    atomic_set(&already_open, CDEV_NOT_USED);

    return ret;
}

static struct file_operations fops = {
    .read = device_read,
    .write = device_write,
    .unlocked_ioctl = device_ioctl,
    .open = device_open,
    .release = device_release,
};

static int __init chardev2_init(void) {
    /* Регистрация символьного устройства (попытка). */
    int ret_val = register_chrdev(MAJOR_NUM, DEVICE_NAME, &fops); 

    if (ret_val < 0) {
        pr_alert("%s failed with %d\n",
                 "Sorry, registering the character device ", ret_val);
        return ret_val;
    }

    cls = class_create(THIS_MODULE, DEVICE_FILE_NAME);
    device_create(cls, NULL, MKDEV(MAJOR_NUM, 0), NULL, DEVICE_FILE_NAME);

    pr_info("Device created on /dev/%s\n", DEVICE_FILE_NAME);

    return 0;
}

static void __exit chardev2_exit(void) {
    device_destroy(cls, MKDEV(MAJOR_NUM, 0));
    class_destroy(cls);

    unregister_chrdev(MAJOR_NUM, DEVICE_NAME);
}

module_init(chardev2_init);
module_exit(chardev2_exit);

MODULE_LICENSE("GPL");
