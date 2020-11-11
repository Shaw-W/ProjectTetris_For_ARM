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
#define NEW_DATA 1
#define OLD_DATA 0
char getch()
{
	char c;
	system("stty raw");
	c = getchar();
	system("stty cooked");
	return c;
}

int main()
{
	int id; //共享内存文件句柄
	char c;
	int *sharedMem = NULL; //共享内存指针
	id = shmget(KEY, SIZE, IPC_CREAT | 0666);
	sharedMem = (int *)shmat(id, NULL, 0);
	memset(sharedMem, 0x00, SIZE);
	printf("id2:%d\n", id);
	while (1)
	{
		c = getch();
		printf("big in!\n");
		printf("42:%d\n  44:%d\n 46:%d\n", sharedMem[42], sharedMem[44], sharedMem[46]);
		switch (c)
		{
		case 'q':
			sharedMem[44] = 0;
			sharedMem[42] = 1;
			printf("Q\n");
			printf("42:%d\n  44:%d\n", sharedMem[42], sharedMem[44]);
			break;
		case 'w':
			sharedMem[44] = 1;
			sharedMem[42] = 1;
			printf("W\n");
			printf("42:%d\n  44:%d\n", sharedMem[42], sharedMem[44]);
			break;
		case 'a':
			sharedMem[44] = 2;
			sharedMem[42] = 1;
			printf("A\n");
			printf("42:%d\n  44:%d\n", sharedMem[42], sharedMem[44]);
			break;
		case 's':
			sharedMem[44] = 3;
			sharedMem[42] = 1;
			printf("S\n");
			printf("42:%d\n  44:%d\n", sharedMem[42], sharedMem[44]);
			break;
		case 'd':
			sharedMem[44] = 4;
			sharedMem[42] = 1;
			printf("D\n");
			printf("42:%d\n  44:%d\n", sharedMem[42], sharedMem[44]);
			break;
		default:
			break;
		}
		if (sharedMem[46] == -1)
		{
			printf("K\n");
			break;
		}
		printf("%d\n%d\n", sharedMem[42], sharedMem[44]);
	}
	printf("end!\n");
	//共享内存解除链接
	shmdt(sharedMem);
	return 0;
}
