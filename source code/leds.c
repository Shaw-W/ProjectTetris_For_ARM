#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <fcntl.h>
#include <sys/time.h>
#include <sys/stat.h>
#include <sys/select.h>
#include <sys/time.h>
#include <errno.h>
#include <pthread.h>
#include <linux/fb.h>
#include <sys/mman.h>
#include <string.h>
#include <sys/shm.h>
#include <termios.h>

#define KEY 8888
#define SIZE 100

void delay(long time)
{
	long tmp = 5000;
	while (time--)
	{
		while (tmp != 0)
		{
			tmp--;
		}
		tmp = 5000;
	}
}

int main(void)
{
	int i = 0, j = 0;
	int fd;
	int on = 0;
	int id;				   //共享内存文件句柄
	int *sharedMem = NULL; //共享内存指针
	id = shmget(KEY, SIZE, IPC_CREAT | 0666);
	sharedMem = (int *)shmat(id, NULL, 0);
	memset(sharedMem, 0x00, SIZE);

	fd = open("/dev/leds", 0);
	if (fd < 0)
	{
		perror("open device leds failed!");
		exit(1);
	}

	ioctl(fd, 0, 0);
	ioctl(fd, 0, 1);
	ioctl(fd, 0, 2);
	ioctl(fd, 0, 3);
	delay(2000);

	while (1)
	{
		sharedMem[46] = 1;
		if (sharedMem[46] == 1)
		{
			ioctl(fd, 1, 0);
			ioctl(fd, 0, 1);
			ioctl(fd, 0, 2);
			ioctl(fd, 1, 3);
			delay(2000);

			ioctl(fd, 0, 0);
			ioctl(fd, 1, 1);
			ioctl(fd, 1, 2);
			ioctl(fd, 0, 3);
			delay(2000);

			ioctl(fd, 0, 0);
			ioctl(fd, 0, 1);
			ioctl(fd, 0, 2);
			ioctl(fd, 0, 3);
			delay(2000);

			ioctl(fd, 0, 0);
			ioctl(fd, 1, 1);
			ioctl(fd, 1, 2);
			ioctl(fd, 0, 3);
			delay(2000);

			ioctl(fd, 1, 0);
			ioctl(fd, 0, 1);
			ioctl(fd, 0, 2);
			ioctl(fd, 1, 3);
			delay(2000);

			ioctl(fd, 0, 0);
			ioctl(fd, 0, 1);
			ioctl(fd, 0, 2);
			ioctl(fd, 0, 3);
			delay(2000);
		}
		else if (sharedMem[46] == 0)
		{
			ioctl(fd, 1, 0);
			ioctl(fd, 0, 1);
			ioctl(fd, 0, 2);
			ioctl(fd, 0, 3);
			delay(2000);

			ioctl(fd, 0, 0);
			ioctl(fd, 1, 1);
			ioctl(fd, 0, 2);
			ioctl(fd, 0, 3);
			delay(2000);

			ioctl(fd, 0, 0);
			ioctl(fd, 0, 1);
			ioctl(fd, 1, 2);
			ioctl(fd, 0, 3);
			delay(2000);

			ioctl(fd, 0, 0);
			ioctl(fd, 0, 1);
			ioctl(fd, 0, 2);
			ioctl(fd, 1, 3);
			delay(2000);

			ioctl(fd, 0, 0);
			ioctl(fd, 0, 1);
			ioctl(fd, 0, 2);
			ioctl(fd, 1, 3);
			delay(2000);

			ioctl(fd, 0, 0);
			ioctl(fd, 0, 1);
			ioctl(fd, 1, 2);
			ioctl(fd, 0, 3);
			delay(2000);

			ioctl(fd, 0, 0);
			ioctl(fd, 1, 1);
			ioctl(fd, 0, 2);
			ioctl(fd, 0, 3);
			delay(2000);

			ioctl(fd, 1, 0);
			ioctl(fd, 0, 1);
			ioctl(fd, 0, 2);
			ioctl(fd, 0, 3);
			delay(2000);
		}
		else
		{
			exit(1);
			break;
		}
	}
	close(fd);
	return 0;
}
