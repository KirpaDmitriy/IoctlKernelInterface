#ifndef CHARDEV_H 
#define CHARDEV_H 
 
#include <linux/ioctl.h> 
 
/* Старший номер устройства. Мы больше не можем полагаться на  
 * динамическую регистрацию, поскольку функции ioctl должны его знать. 
 */ 
#define MAJOR_NUM 100 
#define MAJOR_NUM1 500 
 
/* Установка сообщения драйвера устройства. */ 
#define IOCTL_SET_MSG _IOW(MAJOR_NUM, 0, char *)
/* _IOW означает, что мы создаем номер команды ioctl для передачи 
 * информации от пользовательского процесса модулю ядра. 
 * 
 * Первый аргумент, MAJOR_NUM, это используемый старший номер устройства 
 * 
 * Второй аргумент – это номер команды (их может быть несколько с 
 * разными смыслами). 
 * 
 * Третий аргумент – это тип, который мы хотим передать от процесса ядру 
 */ 
 
/* Получение сообщения драйвера устройства. */ 
#define IOCTL_GET_MSG _IOR(MAJOR_NUM, 1, char *)
/* Эта IOCTL используется для вывода с целью получить сообщение  
 * драйвера устройства. Однако нам все равно нужен буфер для размещения 
 * этого сообщения в качестве ввода при его передаче процессом. 
 */ 
 
/* Получение n-ного байта сообщения. */ 
#define IOCTL_GET_NTH_BYTE _IOWR(MAJOR_NUM, 2, int)
/* Эта IOCTL используется как для ввода, так и для вывода. Она получает
 * от пользователя число, n, и возвращает message[n]. 
 */ 
 
/* Имя файла устройства. */ 
#define DEVICE_FILE_NAME "char_dev" 
#define DEVICE_PATH "/dev/char_dev" 

#define DEVICE_FILE1_NAME "char_dev1" 
#define DEVICE1_PATH "/dev/char_dev1" 
 
#endif
