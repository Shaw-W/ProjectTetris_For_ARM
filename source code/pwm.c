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
#define PWM_IOCTL_SET_FREQ 1
#define PWM_IOCTL_STOP 0
#define ESC_KEY 0x1b

#define NOTE_B0 31
#define NOTE_C1 33
#define NOTE_CS1 35
#define NOTE_D1 37
#define NOTE_DS1 39
#define NOTE_E1 41
#define NOTE_F1 44
#define NOTE_FS1 46
#define NOTE_G1 49
#define NOTE_GS1 52
#define NOTE_A1 55
#define NOTE_AS1 58
#define NOTE_B1 62
#define NOTE_C2 65
#define NOTE_CS2 69
#define NOTE_D2 73
#define NOTE_DS2 78
#define NOTE_E2 82
#define NOTE_F2 87
#define NOTE_FS2 93
#define NOTE_G2 98
#define NOTE_GS2 104
#define NOTE_A2 110
#define NOTE_AS2 117
#define NOTE_B2 123
#define NOTE_C3 131
#define NOTE_CS3 139
#define NOTE_D3 147
#define NOTE_DS3 156
#define NOTE_E3 165
#define NOTE_F3 175
#define NOTE_FS3 185
#define NOTE_G3 196
#define NOTE_GS3 208
#define NOTE_A3 220
#define NOTE_AS3 233
#define NOTE_B3 247
#define NOTE_C4 262
#define NOTE_CS4 277
#define NOTE_D4 294
#define NOTE_DS4 311
#define NOTE_E4 330
#define NOTE_F4 349
#define NOTE_FS4 370
#define NOTE_G4 392
#define NOTE_GS4 415
#define NOTE_A4 440
#define NOTE_AS4 466
#define NOTE_B4 494
#define NOTE_C5 523
#define NOTE_CS5 554
#define NOTE_D5 587
#define NOTE_DS5 622
#define NOTE_E5 659
#define NOTE_F5 698
#define NOTE_FS5 740
#define NOTE_G5 784
#define NOTE_GS5 831
#define NOTE_A5 880
#define NOTE_AS5 932
#define NOTE_B5 988
#define NOTE_C6 1047
#define NOTE_CS6 1109
#define NOTE_D6 1175
#define NOTE_DS6 1245
#define NOTE_E6 1319
#define NOTE_F6 1397
#define NOTE_FS6 1480
#define NOTE_G6 1568
#define NOTE_GS6 1661
#define NOTE_A6 1760
#define NOTE_AS6 1865
#define NOTE_B6 1976
#define NOTE_C7 2093
#define NOTE_CS7 2217
#define NOTE_D7 2349
#define NOTE_DS7 2489
#define NOTE_E7 2637
#define NOTE_F7 2794
#define NOTE_FS7 2960
#define NOTE_G7 3136
#define NOTE_GS7 3322
#define NOTE_A7 3520
#define NOTE_AS7 3729
#define NOTE_B7 3951
#define NOTE_C8 4186
#define NOTE_CS8 4435
#define NOTE_D8 4699
#define NOTE_DS8 4978
#define REST 0

int tempo = 144;

int pinlv[] = {

	NOTE_E5, 4, NOTE_B4, 8, NOTE_C5, 8, NOTE_D5, 4, NOTE_C5, 8, NOTE_B4, 8,
	NOTE_A4, 4, NOTE_A4, 8, NOTE_C5, 8, NOTE_E5, 4, NOTE_D5, 8, NOTE_C5, 8,
	NOTE_B4, -4, NOTE_C5, 8, NOTE_D5, 4, NOTE_E5, 4, NOTE_C5, 4, NOTE_A4, 4, NOTE_A4, 8, NOTE_A4, 8, NOTE_B4, 8, NOTE_C5, 8,

	NOTE_D5, -4, NOTE_F5, 8, NOTE_A5, 4, NOTE_G5, 8, NOTE_F5, 8,
	NOTE_E5, -4, NOTE_C5, 8, NOTE_E5, 4, NOTE_D5, 8, NOTE_C5, 8,
	NOTE_B4, 4, NOTE_B4, 8, NOTE_C5, 8, NOTE_D5, 4, NOTE_E5, 4,
	NOTE_C5, 4, NOTE_A4, 4, NOTE_A4, 4, REST, 4,

	NOTE_E5, 4, NOTE_B4, 8, NOTE_C5, 8, NOTE_D5, 4, NOTE_C5, 8, NOTE_B4, 8,
	NOTE_A4, 4, NOTE_A4, 8, NOTE_C5, 8, NOTE_E5, 4, NOTE_D5, 8, NOTE_C5, 8,
	NOTE_B4, -4, NOTE_C5, 8, NOTE_D5, 4, NOTE_E5, 4,
	NOTE_C5, 4, NOTE_A4, 4, NOTE_A4, 8, NOTE_A4, 4, NOTE_B4, 8, NOTE_C5, 8,

	NOTE_D5, -4, NOTE_F5, 8, NOTE_A5, 4, NOTE_G5, 8, NOTE_F5, 8,
	NOTE_E5, -4, NOTE_C5, 8, NOTE_E5, 4, NOTE_D5, 8, NOTE_C5, 8,
	NOTE_B4, 4, NOTE_B4, 8, NOTE_C5, 8, NOTE_D5, 4, NOTE_E5, 4,
	NOTE_C5, 4, NOTE_A4, 4, NOTE_A4, 4, REST, 4,

	NOTE_E5, 2, NOTE_C5, 2,
	NOTE_D5, 2, NOTE_B4, 2,
	NOTE_C5, 2, NOTE_A4, 2,
	NOTE_GS4, 2, NOTE_B4, 4, REST, 8,
	NOTE_E5, 2, NOTE_C5, 2,
	NOTE_D5, 2, NOTE_B4, 2,
	NOTE_C5, 4, NOTE_E5, 4, NOTE_A5, 2,
	NOTE_GS5, 2

};

int notes = sizeof(pinlv) / sizeof(pinlv[0]) / 2; //得出音符数量为99
int wholenote = (60000 * 4) / 144;
int divider = 0, noteDuration = 0;
int buzzer = 3;

void DelayMS(unsigned int z)
{
	unsigned int x, y, k;
	x = z;
	while (x > 0)
	{
		y = 120;
		while (y > 0)
		{
			k = 100;
			while (k > 0)
			{
				k--;
			}
			y--;
		}
		x--;
	}
}

static int getch(void)
{
	struct termios oldt, newt;
	int ch;

	if (!isatty(STDIN_FILENO))
	{
		fprintf(stderr, "this problem should be run at a terminal\n");
		exit(1);
	}
	// save terminal setting
	if (tcgetattr(STDIN_FILENO, &oldt) < 0)
	{
		perror("save the terminal setting");
		exit(1);
	}

	// set terminal as need
	newt = oldt;
	newt.c_lflag &= ~(ICANON | ECHO);
	if (tcsetattr(STDIN_FILENO, TCSANOW, &newt) < 0)
	{
		perror("set terminal");
		exit(1);
	}

	ch = getchar();

	// restore termial setting
	if (tcsetattr(STDIN_FILENO, TCSANOW, &oldt) < 0)
	{
		perror("restore the termial setting");
		exit(1);
	}
	return ch;
}

static int fd = -1;
static void close_buzzer(void);
static void open_buzzer(void)
{
	fd = open("/dev/pwm", 0);
	if (fd < 0)
	{
		perror("open pwm_buzzer device");
		exit(1);
	}

	// any function exit call will stop the buzzer
	atexit(close_buzzer);
}

static void close_buzzer(void)
{
	if (fd >= 0)
	{
		ioctl(fd, PWM_IOCTL_STOP);
		close(fd);
		fd = -1;
	}
}

static void set_buzzer_freq(int freq)
{
	// this IOCTL command is the key to set frequency
	int ret = ioctl(fd, PWM_IOCTL_SET_FREQ, freq);
	if (ret < 0)
	{
	}
}
static void stop_buzzer(void)
{
	int ret = ioctl(fd, PWM_IOCTL_STOP);
	if (ret < 0)
	{
		perror("stop the buzzer");
		exit(1);
	}
}

int main(int argc, char **argv)
{
	int id;				   //共享内存文件句柄
	int *sharedMem = NULL; //共享内存指针
	id = shmget(KEY, SIZE, IPC_CREAT | 0666);
	sharedMem = (int *)shmat(id, NULL, 0);
	memset(sharedMem, 0x00, SIZE);
	//int freq = 1000 ;
	int i = 0;
	open_buzzer();

	while (1)
	{
		if (sharedMem[46] == 3)
			break;
		set_buzzer_freq(pinlv[i]);
		divider = pinlv[i + 1];

		if (divider > 0)
		{
			noteDuration = (wholenote) / divider;
		}
		else if (divider < 0)
		{
			//负数，是加点的音符
			noteDuration = (wholenote) / abs(divider);
			noteDuration *= 1.5; //点状音符的持续时间为 二分音符 + 四分音符
		}
		DelayMS(noteDuration);
		i = i + 2;
		if (i == 190)
		{
			i = 0;
			sharedMem[46] = 0;
		}
	}
	shmdt(sharedMem);
}