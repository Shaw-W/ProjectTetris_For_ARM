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
#include <time.h>
#include <sys/types.h>
#include <sys/wait.h>

#include "ascii.h"

#define _CRT_SECURE_NO_WARNINGS 1
#define KEY 8888
#define SIZE 100
/**
 * 定义块颜色
 *line	 	#92d050
 *T 		#ff0000
 *block 	#ffff00
 *ll		#00b0f0
 *rl		#0000ff
 *lz		#f79646
 *rz		#ff00ff
**/
#define LineB 38538
#define TB 63488
#define CubeB 65504
#define LLB 1438
#define RLB 31
#define LZB 62632
#define RZB 63519

//定义LCD基础颜色
#define RED 63488
#define GREEN 2016
#define YELLOW 65504
#define BLUE 31
#define BLACK 0
#define WHITE 65535

//LCD屏幕参数
#define LCD_WIDTH 800
#define LCD_HEIGHT 480
#define LCD_PIXCLOCK 2

#define LCD_RIGHT_MARGIN 67
#define LCD_LEFT_MARGIN 40
#define LCD_HSYNC_LEN 31

#define LCD_UPPER_MARGIN 25
#define LCD_LOWER_MARGIN 5
#define LCD_VSYNC_LEN 1

#define LCD_XSIZE LCD_WIDTH
#define LCD_YSIZE LCD_HEIGHT
#define SCR_XSIZE LCD_WIDTH
#define SCR_YSIZE LCD_HEIGHT

#define LCD_BLANK 30
#define C_UP (LCD_XSIZE - LCD_BLANK * 2)
#define C_RIGHT (LCD_XSIZE - LCD_BLANK * 2)
#define V_BLACK ((LCD_YSIZE - LCD_BLANK * 4) / 6)

//定义俄罗斯方块的相关参数
#define ROCK_SQUARE_WIDTH 20 //俄罗斯方块的大小 20
#define LEFT_PREVIEW 184	 //左半区预览偏移量 184
#define LtoRIGHT 491		 //左到右偏移量

int game_mode = 0;
int game_mode_R = 0;

int cwhich = 0;
int ctype = 0;
int cwhich_pre = 0;
int ctype_pre = 0;
int ctype_rem = 0;
int x_offset = 0;
int y_offset = 0;
int score = 0;

int cwhich_R = 0;
int ctype_R = 0;
int cwhich_pre_R = 0;
int ctype_pre_R = 0;
int ctype_rem_R = 0;
int x_offset_R = 0;
int y_offset_R = 0;
int score_R = 0;

int FlagArr[800][480];
struct fb_var_screeninfo fb_var; //当前缓冲区的可变参数，用于LCD的基地址显示
struct fb_fix_screeninfo fb_fix; //固定参数，同上
char *fb_base_addr = NULL;		 //映射的LCD基地址
void DelFull(int x);
int buttons_fd;

//Block Array
static int LineBlock[2][8] = {{145, 76, 145, 56, 145, 36, 145, 16},
							  {125, 16, 145, 16, 165, 16, 185, 16}};

static int CubeBlock[1][8] = {145, 36, 165, 36, 145, 16, 165, 16};

static int TBlock[4][8] = {{125, 36, 145, 36, 165, 36, 145, 16},
						   {165, 56, 145, 36, 165, 36, 165, 16},
						   {145, 36, 125, 16, 145, 16, 165, 16},
						   {145, 56, 145, 36, 165, 36, 145, 16}};

static int LLBlock[4][8] = {{145, 56, 165, 56, 145, 36, 145, 16},
							{125, 36, 145, 36, 165, 36, 165, 16},
							{165, 56, 165, 36, 165, 16, 145, 16},
							{125, 36, 125, 16, 145, 16, 165, 16}};

static int RLBlock[4][8] = {{145, 56, 165, 56, 165, 36, 165, 16},
							{165, 36, 125, 16, 145, 16, 165, 16},
							{145, 56, 145, 36, 145, 16, 165, 16},
							{125, 36, 145, 36, 165, 36, 125, 16}};

static int LZBlock[2][8] = {{145, 56, 145, 36, 165, 36, 165, 16},
							{145, 36, 165, 36, 145, 16, 125, 16}};

static int RZBlock[2][8] = {{165, 56, 165, 36, 145, 36, 145, 16},
							{125, 36, 145, 36, 145, 16, 165, 16}};

//画点
void draw_point(int x, int y, int color)
{
	*((unsigned short *)(fb_base_addr + y * fb_var.xres * 2 + x * 2)) = color;
	FlagArr[x][y] = color;
}

//使用某种颜色清屏
void Lcd_ClearScr(int color)
{
	unsigned int x, y;

	for (y = 0; y < SCR_YSIZE; y++)
	{
		for (x = 0; x < SCR_XSIZE; x++)
		{
			draw_point(x, y, color);
		}
	}
}

//画线
void Glib_Line(int x1, int y1, int x2, int y2, int color)
{
	int dx, dy, e;
	dx = x2 - x1;
	dy = y2 - y1;
	if (dx >= 0)
	{
		if (dy >= 0) // dy>=0
		{
			if (dx >= dy) // 1/8 octant
			{
				e = dy - dx / 2;
				while (x1 <= x2)
				{
					draw_point(x1, y1, color);
					if (e > 0)
					{
						y1 += 1;
						e -= dx;
					}
					x1 += 1;
					e += dy;
				}
			}
			else // 2/8 octant
			{
				e = dx - dy / 2;
				while (y1 <= y2)
				{
					draw_point(x1, y1, color);
					if (e > 0)
					{
						x1 += 1;
						e -= dy;
					}
					y1 += 1;
					e += dx;
				}
			}
		}
		else // dy<0
		{
			dy = -dy; // dy=abs(dy)

			if (dx >= dy) // 8/8 octant
			{
				e = dy - dx / 2;
				while (x1 <= x2)
				{
					draw_point(x1, y1, color);
					if (e > 0)
					{
						y1 -= 1;
						e -= dx;
					}
					x1 += 1;
					e += dy;
				}
			}
			else // 7/8 octant
			{
				e = dx - dy / 2;
				while (y1 >= y2)
				{
					draw_point(x1, y1, color);
					if (e > 0)
					{
						x1 += 1;
						e -= dy;
					}
					y1 -= 1;
					e += dx;
				}
			}
		}
	}
	else //dx<0
	{
		dx = -dx;	 //dx=abs(dx)
		if (dy >= 0) // dy>=0
		{
			if (dx >= dy) // 4/8 octant
			{
				e = dy - dx / 2;
				while (x1 >= x2)
				{
					draw_point(x1, y1, color);
					if (e > 0)
					{
						y1 += 1;
						e -= dx;
					}
					x1 -= 1;
					e += dy;
				}
			}
			else // 3/8 octant
			{
				e = dx - dy / 2;
				while (y1 <= y2)
				{
					draw_point(x1, y1, color);
					if (e > 0)
					{
						x1 -= 1;
						e -= dy;
					}
					y1 += 1;
					e += dx;
				}
			}
		}
		else // dy<0
		{
			dy = -dy; // dy=abs(dy)

			if (dx >= dy) // 5/8 octant
			{
				e = dy - dx / 2;
				while (x1 >= x2)
				{
					draw_point(x1, y1, color);
					if (e > 0)
					{
						y1 -= 1;
						e -= dx;
					}
					x1 -= 1;
					e += dy;
				}
			}
			else // 6/8 octant
			{
				e = dx - dy / 2;
				while (y1 >= y2)
				{
					draw_point(x1, y1, color);
					if (e > 0)
					{
						x1 -= 1;
						e -= dy;
					}
					y1 -= 1;
					e += dx;
				}
			}
		}
	}
}

//绘制实心矩形
static void Glib_FilledRectangle(int x1, int y1, int x2, int y2, int color) //填充矩形
{
	int i;
	for (i = y1; i <= y2; i++)
		Glib_Line(x1, i, x2, i, color);
}

void DrawTable()
{
	Lcd_ClearScr(WHITE);
	Glib_Line(0, 480, 800, 480, BLACK);
	Glib_Line(0, 479, 800, 479, BLACK);
	Glib_Line(0, 478, 800, 478, BLACK);
	Glib_Line(0, 477, 800, 477, BLACK);
	Glib_Line(0, 476, 800, 476, BLACK);
	Glib_Line(0, 15, 800, 15, BLACK);
	Glib_Line(0, 480, 0, 15, BLACK);
	Glib_Line(1, 480, 1, 15, BLACK);
	Glib_Line(2, 480, 2, 15, BLACK);
	Glib_Line(3, 480, 3, 15, BLACK);
	Glib_Line(4, 480, 4, 15, BLACK);
	Glib_Line(305, 475, 305, 15, BLACK);
	Glib_Line(400, 480, 400, 15, BLACK);
	Glib_Line(401, 480, 401, 15, BLACK);
	Glib_Line(796, 480, 796, 15, BLACK);
	Glib_Line(797, 480, 797, 15, BLACK);
	Glib_Line(798, 480, 798, 15, BLACK);
	Glib_Line(799, 480, 799, 15, BLACK);
	Glib_Line(800, 480, 800, 15, BLACK);
	Glib_Line(495, 480, 495, 15, BLACK);
}

void start_LCD()
{
	int display_fd;			 //lcd设备的文件句柄
	long int screensize = 0; //屏幕大小

	//打开LCD驱动
	display_fd = open("/dev/fb0", O_RDWR); //打开设备
	if (display_fd < 0)
	{ //打开设备失败
		perror("open device buttons failed！");
		exit(1);
	}
	printf("lcd is open\n");

	if (ioctl(display_fd, FBIOGET_FSCREENINFO, &fb_fix))
	{
		printf("Error reading fb fixed information.\n");
		exit(1);
	}
	printf("Get fixed screen information OK\n");
	/* Get variable screen information 	*/
	if (ioctl(display_fd, FBIOGET_VSCREENINFO, &fb_var))
	{
		printf("Error reading fb variable information.\n");
		exit(1);
	}
	//初始化屏幕
	screensize = fb_var.xres * fb_var.yres * fb_var.bits_per_pixel / 8;
	printf("fb_var.xres:%d\n", fb_var.xres);
	printf("fb_var.xres:%d\n", fb_var.yres);
	fb_base_addr = (char *)mmap(NULL, screensize, PROT_READ | PROT_WRITE, MAP_SHARED, display_fd, 0);
	printf("buffer is ok\n");
	if (-1 == (int)fb_base_addr)
	{
		printf("errno\n");
		exit(1);
	}
	memset(fb_base_addr, 0xff, screensize);
}

//左判断
void JudgeFull()
{
	int x, i, j;
	int LineISFull = 1;
	//printf("JudgeFull start\n");
	for (j = 456; j >= 16; j = j - 20)
	{
		for (i = 5; i <= 285; i = i + 20)
		{
			if (FlagArr[i][j] == WHITE)
			{
				LineISFull = 0;
				break;
			}
		}
		if (LineISFull == 1)
		{
			DelFull(j);
			j = j + 20;
		}
		LineISFull = 1;
	}
}

//右判断
void JudgeFull_R()
{
	int x, i, j;
	int LineISFull = 1;
	//printf("JudgeFull start\n");
	for (j = 456; j >= 16; j = j - 20)
	{
		for (i = 496; i <= 776; i = i + 20)
		{
			if (FlagArr[i][j] == WHITE)
			{
				LineISFull = 0;
				break;
			}
		}
		if (LineISFull == 1)
		{
			DelFull_R(j);
			j = j + 20;
		}
		LineISFull = 1;
	}
}

//左删除
void DelFull(int x)
{
	int i, j;
	//printf("DelFull start! del:%d\n", x);
	for (j = x; j >= 16; j = j - 20)
	{
		for (i = 5; i <= 285; i = i + 20)
		{
			Glib_FilledRectangle(i, j, i + 19, j + 19, FlagArr[i][j - 19]);
			//printf("%d", FlagArr[i][j - 19]);
			//printf("Del del:%d %d %d %d!\n", i, j, i + 19, j - 19);
		}
	}
	Glib_FilledRectangle(5, 16, 304, 35, WHITE);
	score = score + 50;
}

//右删除
void DelFull_R(int x)
{
	int i, j;
	//printf("DelFull start! del:%d\n", x);
	for (j = x; j >= 16; j = j - 20)
	{
		for (i = 496; i <= 776; i = i + 20)
		{
			Glib_FilledRectangle(i, j, i + 19, j + 19, FlagArr[i][j - 19]);
			//printf("%d", FlagArr[i][j - 19]);
			//printf("Del del:%d %d %d %d!\n", i, j, i + 19, j - 19);
		}
	}
	Glib_FilledRectangle(496, 16, 795, 35, WHITE);
	score = score + 50;
}

//左画方块
void DrawBlock(int cw, int cty)
{
	int i;
	//printf("Draw %d,%d\n", cw, cty);
	switch (cw)
	{
	case 0:
		for (i = 0; i < 8; i = i + 2)
		{
			Glib_FilledRectangle(LineBlock[cty][i] + (x_offset * ROCK_SQUARE_WIDTH), LineBlock[cty][i + 1] + (y_offset * ROCK_SQUARE_WIDTH), LineBlock[cty][i] + 19 + (x_offset * ROCK_SQUARE_WIDTH), LineBlock[cty][i + 1] + 19 + (y_offset * ROCK_SQUARE_WIDTH), LineB);
		}
		break;
	case 1:
		for (i = 0; i < 8; i = i + 2)
		{
			Glib_FilledRectangle(CubeBlock[cty][i] + (x_offset * ROCK_SQUARE_WIDTH), CubeBlock[cty][i + 1] + (y_offset * ROCK_SQUARE_WIDTH), CubeBlock[cty][i] + 19 + (x_offset * ROCK_SQUARE_WIDTH), CubeBlock[cty][i + 1] + 19 + (y_offset * ROCK_SQUARE_WIDTH), CubeB);
		}
		break;
	case 2:
		for (i = 0; i < 8; i = i + 2)
		{
			Glib_FilledRectangle(TBlock[cty][i] + (x_offset * ROCK_SQUARE_WIDTH), TBlock[cty][i + 1] + (y_offset * ROCK_SQUARE_WIDTH), TBlock[cty][i] + 19 + (x_offset * ROCK_SQUARE_WIDTH), TBlock[cty][i + 1] + 19 + (y_offset * ROCK_SQUARE_WIDTH), TB);
		}
		break;
	case 3:
		for (i = 0; i < 8; i = i + 2)
		{
			Glib_FilledRectangle(LLBlock[cty][i] + (x_offset * ROCK_SQUARE_WIDTH), LLBlock[cty][i + 1] + (y_offset * ROCK_SQUARE_WIDTH), LLBlock[cty][i] + 19 + (x_offset * ROCK_SQUARE_WIDTH), LLBlock[cty][i + 1] + 19 + (y_offset * ROCK_SQUARE_WIDTH), LLB);
		}
		break;
	case 4:
		for (i = 0; i < 8; i = i + 2)
		{
			Glib_FilledRectangle(RLBlock[cty][i] + (x_offset * ROCK_SQUARE_WIDTH), RLBlock[cty][i + 1] + (y_offset * ROCK_SQUARE_WIDTH), RLBlock[cty][i] + 19 + (x_offset * ROCK_SQUARE_WIDTH), RLBlock[cty][i + 1] + 19 + (y_offset * ROCK_SQUARE_WIDTH), RLB);
		}
		break;
	case 5:
		for (i = 0; i < 8; i = i + 2)
		{
			Glib_FilledRectangle(LZBlock[cty][i] + (x_offset * ROCK_SQUARE_WIDTH), LZBlock[cty][i + 1] + (y_offset * ROCK_SQUARE_WIDTH), LZBlock[cty][i] + 19 + (x_offset * ROCK_SQUARE_WIDTH), LZBlock[cty][i + 1] + 19 + (y_offset * ROCK_SQUARE_WIDTH), LZB);
		}
		break;
	case 6:
		for (i = 0; i < 8; i = i + 2)
		{
			Glib_FilledRectangle(RZBlock[cty][i] + (x_offset * ROCK_SQUARE_WIDTH), RZBlock[cty][i + 1] + (y_offset * ROCK_SQUARE_WIDTH), RZBlock[cty][i] + 19 + (x_offset * ROCK_SQUARE_WIDTH), RZBlock[cty][i + 1] + 19 + (y_offset * ROCK_SQUARE_WIDTH), RZB);
		}
		break;
	default:
		printf("DrawBlock error!\n");
		exit(1);
	}
}

//右画方块
void DrawBlock_R(int cw, int cty)
{
	int i;
	//printf("Draw %d,%d\n", cw, cty);
	switch (cw)
	{
	case 0:
		for (i = 0; i < 8; i = i + 2)
		{
			Glib_FilledRectangle(LineBlock[cty][i] + LtoRIGHT + (x_offset_R * ROCK_SQUARE_WIDTH), LineBlock[cty][i + 1] + (y_offset_R * ROCK_SQUARE_WIDTH), LineBlock[cty][i] + 19 + LtoRIGHT + (x_offset_R * ROCK_SQUARE_WIDTH), LineBlock[cty][i + 1] + 19 + (y_offset_R * ROCK_SQUARE_WIDTH), LineB);
		}
		break;
	case 1:
		for (i = 0; i < 8; i = i + 2)
		{
			Glib_FilledRectangle(CubeBlock[cty][i] + LtoRIGHT + (x_offset_R * ROCK_SQUARE_WIDTH), CubeBlock[cty][i + 1] + (y_offset_R * ROCK_SQUARE_WIDTH), CubeBlock[cty][i] + LtoRIGHT + 19 + (x_offset_R * ROCK_SQUARE_WIDTH), CubeBlock[cty][i + 1] + 19 + (y_offset_R * ROCK_SQUARE_WIDTH), CubeB);
		}
		break;
	case 2:
		for (i = 0; i < 8; i = i + 2)
		{
			Glib_FilledRectangle(TBlock[cty][i] + LtoRIGHT + (x_offset_R * ROCK_SQUARE_WIDTH), TBlock[cty][i + 1] + (y_offset_R * ROCK_SQUARE_WIDTH), TBlock[cty][i] + LtoRIGHT + 19 + (x_offset_R * ROCK_SQUARE_WIDTH), TBlock[cty][i + 1] + 19 + (y_offset_R * ROCK_SQUARE_WIDTH), TB);
		}
		break;
	case 3:
		for (i = 0; i < 8; i = i + 2)
		{
			Glib_FilledRectangle(LLBlock[cty][i] + LtoRIGHT + (x_offset_R * ROCK_SQUARE_WIDTH), LLBlock[cty][i + 1] + (y_offset_R * ROCK_SQUARE_WIDTH), LLBlock[cty][i] + LtoRIGHT + 19 + (x_offset_R * ROCK_SQUARE_WIDTH), LLBlock[cty][i + 1] + 19 + (y_offset_R * ROCK_SQUARE_WIDTH), LLB);
		}
		break;
	case 4:
		for (i = 0; i < 8; i = i + 2)
		{
			Glib_FilledRectangle(RLBlock[cty][i] + LtoRIGHT + (x_offset_R * ROCK_SQUARE_WIDTH), RLBlock[cty][i + 1] + (y_offset_R * ROCK_SQUARE_WIDTH), RLBlock[cty][i] + LtoRIGHT + 19 + (x_offset_R * ROCK_SQUARE_WIDTH), RLBlock[cty][i + 1] + 19 + (y_offset_R * ROCK_SQUARE_WIDTH), RLB);
		}
		break;
	case 5:
		for (i = 0; i < 8; i = i + 2)
		{
			Glib_FilledRectangle(LZBlock[cty][i] + LtoRIGHT + (x_offset_R * ROCK_SQUARE_WIDTH), LZBlock[cty][i + 1] + (y_offset_R * ROCK_SQUARE_WIDTH), LZBlock[cty][i] + LtoRIGHT + 19 + (x_offset_R * ROCK_SQUARE_WIDTH), LZBlock[cty][i + 1] + 19 + (y_offset_R * ROCK_SQUARE_WIDTH), LZB);
		}
		break;
	case 6:
		for (i = 0; i < 8; i = i + 2)
		{
			Glib_FilledRectangle(RZBlock[cty][i] + LtoRIGHT + (x_offset_R * ROCK_SQUARE_WIDTH), RZBlock[cty][i + 1] + (y_offset_R * ROCK_SQUARE_WIDTH), RZBlock[cty][i] + LtoRIGHT + 19 + (x_offset_R * ROCK_SQUARE_WIDTH), RZBlock[cty][i + 1] + 19 + (y_offset_R * ROCK_SQUARE_WIDTH), RZB);
		}
		break;
	default:
		printf("DrawBlock error!\n");
		exit(1);
	}
}

//左清方块
void BlockClr(int cw, int cty)
{
	int i;
	switch (cw)
	{
	case 0:
		for (i = 0; i < 8; i = i + 2)
		{
			Glib_FilledRectangle(LineBlock[cty][i] + (x_offset * ROCK_SQUARE_WIDTH), LineBlock[cty][i + 1] + (y_offset * ROCK_SQUARE_WIDTH), LineBlock[cty][i] + 19 + (x_offset * ROCK_SQUARE_WIDTH), LineBlock[cty][i + 1] + 19 + (y_offset * ROCK_SQUARE_WIDTH), WHITE);
		}
		break;
	case 1:
		for (i = 0; i < 8; i = i + 2)
		{
			Glib_FilledRectangle(CubeBlock[cty][i] + (x_offset * ROCK_SQUARE_WIDTH), CubeBlock[cty][i + 1] + (y_offset * ROCK_SQUARE_WIDTH), CubeBlock[cty][i] + 19 + (x_offset * ROCK_SQUARE_WIDTH), CubeBlock[cty][i + 1] + 19 + (y_offset * ROCK_SQUARE_WIDTH), WHITE);
		}
		break;
	case 2:
		for (i = 0; i < 8; i = i + 2)
		{
			Glib_FilledRectangle(TBlock[cty][i] + (x_offset * ROCK_SQUARE_WIDTH), TBlock[cty][i + 1] + (y_offset * ROCK_SQUARE_WIDTH), TBlock[cty][i] + 19 + (x_offset * ROCK_SQUARE_WIDTH), TBlock[cty][i + 1] + 19 + (y_offset * ROCK_SQUARE_WIDTH), WHITE);
		}
		break;
	case 3:
		for (i = 0; i < 8; i = i + 2)
		{
			Glib_FilledRectangle(LLBlock[cty][i] + (x_offset * ROCK_SQUARE_WIDTH), LLBlock[cty][i + 1] + (y_offset * ROCK_SQUARE_WIDTH), LLBlock[cty][i] + 19 + (x_offset * ROCK_SQUARE_WIDTH), LLBlock[cty][i + 1] + 19 + (y_offset * ROCK_SQUARE_WIDTH), WHITE);
		}
		break;
	case 4:
		for (i = 0; i < 8; i = i + 2)
		{
			Glib_FilledRectangle(RLBlock[cty][i] + (x_offset * ROCK_SQUARE_WIDTH), RLBlock[cty][i + 1] + (y_offset * ROCK_SQUARE_WIDTH), RLBlock[cty][i] + 19 + (x_offset * ROCK_SQUARE_WIDTH), RLBlock[cty][i + 1] + 19 + (y_offset * ROCK_SQUARE_WIDTH), WHITE);
		}
		break;
	case 5:
		for (i = 0; i < 8; i = i + 2)
		{
			Glib_FilledRectangle(LZBlock[cty][i] + (x_offset * ROCK_SQUARE_WIDTH), LZBlock[cty][i + 1] + (y_offset * ROCK_SQUARE_WIDTH), LZBlock[cty][i] + 19 + (x_offset * ROCK_SQUARE_WIDTH), LZBlock[cty][i + 1] + 19 + (y_offset * ROCK_SQUARE_WIDTH), WHITE);
		}
		break;
	case 6:
		for (i = 0; i < 8; i = i + 2)
		{
			Glib_FilledRectangle(RZBlock[cty][i] + (x_offset * ROCK_SQUARE_WIDTH), RZBlock[cty][i + 1] + (y_offset * ROCK_SQUARE_WIDTH), RZBlock[cty][i] + 19 + (x_offset * ROCK_SQUARE_WIDTH), RZBlock[cty][i + 1] + 19 + (y_offset * ROCK_SQUARE_WIDTH), WHITE);
		}
		break;
	default:
		printf("BloClr error!\n");
		exit(1);
	}
}

//右清方块
void BlockClr_R(int cw, int cty)
{
	int i;
	switch (cw)
	{
	case 0:
		for (i = 0; i < 8; i = i + 2)
		{
			Glib_FilledRectangle(LineBlock[cty][i] + LtoRIGHT + (x_offset_R * ROCK_SQUARE_WIDTH), LineBlock[cty][i + 1] + (y_offset_R * ROCK_SQUARE_WIDTH), LineBlock[cty][i] + LtoRIGHT + 19 + (x_offset_R * ROCK_SQUARE_WIDTH), LineBlock[cty][i + 1] + 19 + (y_offset_R * ROCK_SQUARE_WIDTH), WHITE);
		}
		break;
	case 1:
		for (i = 0; i < 8; i = i + 2)
		{
			Glib_FilledRectangle(CubeBlock[cty][i] + LtoRIGHT + (x_offset_R * ROCK_SQUARE_WIDTH), CubeBlock[cty][i + 1] + (y_offset_R * ROCK_SQUARE_WIDTH), CubeBlock[cty][i] + LtoRIGHT + 19 + (x_offset_R * ROCK_SQUARE_WIDTH), CubeBlock[cty][i + 1] + 19 + (y_offset_R * ROCK_SQUARE_WIDTH), WHITE);
		}
		break;
	case 2:
		for (i = 0; i < 8; i = i + 2)
		{
			Glib_FilledRectangle(TBlock[cty][i] + LtoRIGHT + (x_offset_R * ROCK_SQUARE_WIDTH), TBlock[cty][i + 1] + (y_offset_R * ROCK_SQUARE_WIDTH), TBlock[cty][i] + LtoRIGHT + 19 + (x_offset_R * ROCK_SQUARE_WIDTH), TBlock[cty][i + 1] + 19 + (y_offset_R * ROCK_SQUARE_WIDTH), WHITE);
		}
		break;
	case 3:
		for (i = 0; i < 8; i = i + 2)
		{
			Glib_FilledRectangle(LLBlock[cty][i] + LtoRIGHT + (x_offset_R * ROCK_SQUARE_WIDTH), LLBlock[cty][i + 1] + (y_offset_R * ROCK_SQUARE_WIDTH), LLBlock[cty][i] + LtoRIGHT + 19 + (x_offset_R * ROCK_SQUARE_WIDTH), LLBlock[cty][i + 1] + 19 + (y_offset_R * ROCK_SQUARE_WIDTH), WHITE);
		}
		break;
	case 4:
		for (i = 0; i < 8; i = i + 2)
		{
			Glib_FilledRectangle(RLBlock[cty][i] + LtoRIGHT + (x_offset_R * ROCK_SQUARE_WIDTH), RLBlock[cty][i + 1] + (y_offset_R * ROCK_SQUARE_WIDTH), RLBlock[cty][i] + LtoRIGHT + 19 + (x_offset_R * ROCK_SQUARE_WIDTH), RLBlock[cty][i + 1] + 19 + (y_offset_R * ROCK_SQUARE_WIDTH), WHITE);
		}
		break;
	case 5:
		for (i = 0; i < 8; i = i + 2)
		{
			Glib_FilledRectangle(LZBlock[cty][i] + LtoRIGHT + (x_offset_R * ROCK_SQUARE_WIDTH), LZBlock[cty][i + 1] + (y_offset_R * ROCK_SQUARE_WIDTH), LZBlock[cty][i] + LtoRIGHT + 19 + (x_offset_R * ROCK_SQUARE_WIDTH), LZBlock[cty][i + 1] + 19 + (y_offset_R * ROCK_SQUARE_WIDTH), WHITE);
		}
		break;
	case 6:
		for (i = 0; i < 8; i = i + 2)
		{
			Glib_FilledRectangle(RZBlock[cty][i] + LtoRIGHT + (x_offset_R * ROCK_SQUARE_WIDTH), RZBlock[cty][i + 1] + (y_offset_R * ROCK_SQUARE_WIDTH), RZBlock[cty][i] + LtoRIGHT + 19 + (x_offset_R * ROCK_SQUARE_WIDTH), RZBlock[cty][i + 1] + 19 + (y_offset_R * ROCK_SQUARE_WIDTH), WHITE);
		}
		break;
	default:
		printf("BloClr error!\n");
		exit(1);
	}
}

//左边随机
void CreateRand()
{
	cwhich_pre = rand() % 7;
	//cwhich_pre = 0;

	//left
	switch (cwhich_pre)
	{
	case 0:
	case 5:
	case 6:
		ctype_pre = rand() % 2;
		break;
	case 1:
		ctype_pre = 0;
		break;
	case 2:
	case 3:
	case 4:
		ctype_pre = rand() % 4;
		break;
	default:
		printf("Srand error!,%d,%d\n", cwhich_pre, ctype_pre);
		exit(1);
	}
	ctype_rem = ctype;
}

//右边随机
void CreateRand_R()
{
	cwhich_pre_R = (rand() % 8) % 7;

	//right
	switch (cwhich_pre_R)
	{
	case 0:
	case 5:
	case 6:
		ctype_pre_R = (rand() % 3) % 2;
		break;
	case 1:
		ctype_pre_R = 0;
		break;
	case 2:
	case 3:
	case 4:
		ctype_pre_R = (rand() % 5) % 4;
		break;
	default:
		printf("SrandR error!,%d,%d\n", cwhich_pre_R, ctype_pre_R);
		exit(1);
	}
	ctype_rem_R = ctype_R;
}

//左创方块
void CreateBlock()
{
	int i;
	x_offset = 0;
	y_offset = 0;
	if (FlagArr[146][16] != WHITE)
	{
		printf("Player A Defeated!\n");
		game_mode = 3;
	}
	cwhich = cwhich_pre;
	ctype = ctype_pre;
	CreateRand();
	DrawBlock(cwhich, ctype);
	//printf("Create %d %d \n", cwhich, ctype);
}

//右创方块
void CreateBlock_R()
{
	int i;
	x_offset_R = 0;
	y_offset_R = 0;
	if (FlagArr[636][16] != WHITE)
	{
		printf("Player B Defeated!\n");
		game_mode_R = 3;
	}
	cwhich_R = cwhich_pre_R;
	ctype_R = ctype_pre_R;
	CreateRand_R();
	DrawBlock_R(cwhich_R, ctype_R);
	//printf("Create %d %d \n", cwhich, ctype);
}

//左预览
void PreviewBlock()
{
	int i;
	int x, y, z;
	Glib_FilledRectangle(306, 16, 399, 475, WHITE);
	draw_small_letter('P', 320, 150, RED);
	draw_small_letter('L', 350, 150, RED);
	draw_small_letter('S', 320, 310, BLUE);
	draw_small_letter('C', 350, 310, BLUE);

	z = 0;
	y = (score / 10) % 10;
	x = score / 100;

	//printf("%d %d %d\n", x, y, z);
	draw_number(x, 315, 370, GREEN);
	draw_number(y, 340, 370, GREEN);
	draw_number(z, 365, 370, GREEN);
	switch (cwhich_pre)
	{
	case 0:
		for (i = 0; i < 8; i = i + 2)
		{
			Glib_FilledRectangle(LineBlock[ctype_pre][i] + LEFT_PREVIEW, LineBlock[ctype_pre][i + 1] + LEFT_PREVIEW, LineBlock[ctype_pre][i] + 19 + LEFT_PREVIEW, LineBlock[ctype_pre][i + 1] + 19 + LEFT_PREVIEW, LineB);
		}
		break;
	case 1:
		for (i = 0; i < 8; i = i + 2)
		{
			Glib_FilledRectangle(CubeBlock[ctype_pre][i] + LEFT_PREVIEW, CubeBlock[ctype_pre][i + 1] + LEFT_PREVIEW, CubeBlock[ctype_pre][i] + 19 + LEFT_PREVIEW, CubeBlock[ctype_pre][i + 1] + 19 + LEFT_PREVIEW, CubeB);
		}
		break;
	case 2:
		for (i = 0; i < 8; i = i + 2)
		{
			Glib_FilledRectangle(TBlock[ctype_pre][i] + LEFT_PREVIEW, TBlock[ctype_pre][i + 1] + LEFT_PREVIEW, TBlock[ctype_pre][i] + 19 + LEFT_PREVIEW, TBlock[ctype_pre][i + 1] + 19 + LEFT_PREVIEW, TB);
		}
		break;
	case 3:
		for (i = 0; i < 8; i = i + 2)
		{
			Glib_FilledRectangle(LLBlock[ctype_pre][i] + LEFT_PREVIEW, LLBlock[ctype_pre][i + 1] + LEFT_PREVIEW, LLBlock[ctype_pre][i] + 19 + LEFT_PREVIEW, LLBlock[ctype_pre][i + 1] + 19 + LEFT_PREVIEW, LLB);
		}
		break;
	case 4:
		for (i = 0; i < 8; i = i + 2)
		{
			Glib_FilledRectangle(RLBlock[ctype_pre][i] + LEFT_PREVIEW, RLBlock[ctype_pre][i + 1] + LEFT_PREVIEW, RLBlock[ctype_pre][i] + 19 + LEFT_PREVIEW, RLBlock[ctype_pre][i + 1] + 19 + LEFT_PREVIEW, RLB);
		}
		break;
	case 5:
		for (i = 0; i < 8; i = i + 2)
		{
			Glib_FilledRectangle(LZBlock[ctype_pre][i] + LEFT_PREVIEW, LZBlock[ctype_pre][i + 1] + LEFT_PREVIEW, LZBlock[ctype_pre][i] + 19 + LEFT_PREVIEW, LZBlock[ctype_pre][i + 1] + 19 + LEFT_PREVIEW, LZB);
		}
		break;
	case 6:
		for (i = 0; i < 8; i = i + 2)
		{
			Glib_FilledRectangle(RZBlock[ctype_pre][i] + LEFT_PREVIEW, RZBlock[ctype_pre][i + 1] + LEFT_PREVIEW, RZBlock[ctype_pre][i] + 19 + LEFT_PREVIEW, RZBlock[ctype_pre][i + 1] + 19 + LEFT_PREVIEW, RZB);
		}
		break;
	default:
		printf("Preview error!\n");
		exit(1);
	}
}

//右预览
void PreviewBlock_R()
{
	int i;
	int x, y, z;
	Glib_FilledRectangle(402, 16, 494, 475, WHITE);
	draw_small_letter('P', 416, 150, RED);
	draw_small_letter('L', 446, 150, RED);
	draw_small_letter('S', 416, 310, BLUE);
	draw_small_letter('C', 446, 310, BLUE);

	z = 0;
	y = (score_R / 10) % 10;
	x = score_R / 100;

	//printf("%d %d %d\n", x, y, z);
	draw_number(x, 411, 370, GREEN);
	draw_number(y, 436, 370, GREEN);
	draw_number(z, 461, 370, GREEN);
	switch (cwhich_pre_R)
	{
	case 0:
		for (i = 0; i < 8; i = i + 2)
		{
			Glib_FilledRectangle(LineBlock[ctype_pre_R][i] + LEFT_PREVIEW + 96, LineBlock[ctype_pre_R][i + 1] + LEFT_PREVIEW, LineBlock[ctype_pre_R][i] + 19 + LEFT_PREVIEW + 96, LineBlock[ctype_pre_R][i + 1] + 19 + LEFT_PREVIEW, LineB);
		}
		break;
	case 1:
		for (i = 0; i < 8; i = i + 2)
		{
			Glib_FilledRectangle(CubeBlock[ctype_pre_R][i] + LEFT_PREVIEW + 96, CubeBlock[ctype_pre_R][i + 1] + LEFT_PREVIEW, CubeBlock[ctype_pre_R][i] + 19 + LEFT_PREVIEW + 96, CubeBlock[ctype_pre_R][i + 1] + 19 + LEFT_PREVIEW, CubeB);
		}
		break;
	case 2:
		for (i = 0; i < 8; i = i + 2)
		{
			Glib_FilledRectangle(TBlock[ctype_pre_R][i] + LEFT_PREVIEW + 96, TBlock[ctype_pre_R][i + 1] + LEFT_PREVIEW, TBlock[ctype_pre_R][i] + 19 + LEFT_PREVIEW + 96, TBlock[ctype_pre_R][i + 1] + 19 + LEFT_PREVIEW, TB);
		}
		break;
	case 3:
		for (i = 0; i < 8; i = i + 2)
		{
			Glib_FilledRectangle(LLBlock[ctype_pre_R][i] + LEFT_PREVIEW + 96, LLBlock[ctype_pre_R][i + 1] + LEFT_PREVIEW, LLBlock[ctype_pre_R][i] + 19 + LEFT_PREVIEW + 96, LLBlock[ctype_pre_R][i + 1] + 19 + LEFT_PREVIEW, LLB);
		}
		break;
	case 4:
		for (i = 0; i < 8; i = i + 2)
		{
			Glib_FilledRectangle(RLBlock[ctype_pre_R][i] + LEFT_PREVIEW + 96, RLBlock[ctype_pre_R][i + 1] + LEFT_PREVIEW, RLBlock[ctype_pre_R][i] + 19 + LEFT_PREVIEW + 96, RLBlock[ctype_pre_R][i + 1] + 19 + LEFT_PREVIEW, RLB);
		}
		break;
	case 5:
		for (i = 0; i < 8; i = i + 2)
		{
			Glib_FilledRectangle(LZBlock[ctype_pre_R][i] + LEFT_PREVIEW + 96, LZBlock[ctype_pre_R][i + 1] + LEFT_PREVIEW, LZBlock[ctype_pre_R][i] + 19 + LEFT_PREVIEW + 96, LZBlock[ctype_pre_R][i + 1] + 19 + LEFT_PREVIEW, LZB);
		}
		break;
	case 6:
		for (i = 0; i < 8; i = i + 2)
		{
			Glib_FilledRectangle(RZBlock[ctype_pre_R][i] + LEFT_PREVIEW + 96, RZBlock[ctype_pre_R][i + 1] + LEFT_PREVIEW, RZBlock[ctype_pre_R][i] + 19 + LEFT_PREVIEW + 96, RZBlock[ctype_pre_R][i + 1] + 19 + LEFT_PREVIEW, RZB);
		}
		break;
	default:
		printf("Preview_R error!\n");
		exit(1);
	}
}

//左碰撞
int JudgeHit(int tmp)
{
	int ColorNext[4];
	int i, j, k, x, y;
	int n_x = 0;
	int n_y = 0;
	switch (tmp)
	{
	case 0:
		break;
	case 1: //down
		n_y = ROCK_SQUARE_WIDTH;
		break;
	case 2: //left
		n_x--;
		break;
	case 3: //right
		n_x = ROCK_SQUARE_WIDTH;
		break;
	default:
		printf("JudgeHit error!\n");
		exit(1);
		break;
	}
	k = 0;
	BlockClr(cwhich, ctype_rem);
	switch (cwhich)
	{
	case 0:
		for (i = 0; i < 8; i = i + 2)
		{
			j = i + 1;
			x = LineBlock[ctype][i] + (x_offset * ROCK_SQUARE_WIDTH) + n_x;
			y = LineBlock[ctype][j] + (y_offset * ROCK_SQUARE_WIDTH) + n_y;
			ColorNext[k] = FlagArr[x][y];
			k++;
		}
		break;
	case 1:
		for (i = 0; i < 8; i = i + 2)
		{
			j = i + 1;
			x = CubeBlock[ctype][i] + (x_offset * ROCK_SQUARE_WIDTH) + n_x;
			y = CubeBlock[ctype][j] + (y_offset * ROCK_SQUARE_WIDTH) + n_y;
			ColorNext[k] = FlagArr[x][y];
			k++;
		}
		break;
	case 2:
		for (i = 0; i < 8; i = i + 2)
		{
			j = i + 1;
			x = TBlock[ctype][i] + (x_offset * ROCK_SQUARE_WIDTH) + n_x;
			y = TBlock[ctype][j] + (y_offset * ROCK_SQUARE_WIDTH) + n_y;
			ColorNext[k] = FlagArr[x][y];
			k++;
		}
		break;
	case 3:
		for (i = 0; i < 8; i = i + 2)
		{
			j = i + 1;
			x = LLBlock[ctype][i] + (x_offset * ROCK_SQUARE_WIDTH) + n_x;
			y = LLBlock[ctype][j] + (y_offset * ROCK_SQUARE_WIDTH) + n_y;
			ColorNext[k] = FlagArr[x][y];
			k++;
		}
		break;
	case 4:
		for (i = 0; i < 8; i = i + 2)
		{
			j = i + 1;
			x = RLBlock[ctype][i] + (x_offset * ROCK_SQUARE_WIDTH) + n_x;
			y = RLBlock[ctype][j] + (y_offset * ROCK_SQUARE_WIDTH) + n_y;
			ColorNext[k] = FlagArr[x][y];
			k++;
		}
		break;
	case 5:
		for (i = 0; i < 8; i = i + 2)
		{
			j = i + 1;
			x = LZBlock[ctype][i] + (x_offset * ROCK_SQUARE_WIDTH) + n_x;
			y = LZBlock[ctype][j] + (y_offset * ROCK_SQUARE_WIDTH) + n_y;
			ColorNext[k] = FlagArr[x][y];
			k++;
		}
		break;
	case 6:
		for (i = 0; i < 8; i = i + 2)
		{
			j = i + 1;
			x = RZBlock[ctype][i] + (x_offset * ROCK_SQUARE_WIDTH) + n_x;
			y = RZBlock[ctype][j] + (y_offset * ROCK_SQUARE_WIDTH) + n_y;
			ColorNext[k] = FlagArr[x][y];
			k++;
		}
		break;
	default:
		break;
	}
	for (i = 0; i < 4; i++)
	{
		if (ColorNext[i] != WHITE)
		{
			return 1;
		}
	}
	return 0;
}

//右碰撞
int JudgeHit_R(int tmp)
{
	int ColorNext[4];
	int i, j, k, x, y;
	int n_x = 0;
	int n_y = 0;
	switch (tmp)
	{
	case 0:
		break;
	case 1: //down
		n_y = ROCK_SQUARE_WIDTH;
		break;
	case 2: //left
		n_x--;
		break;
	case 3: //right
		n_x = ROCK_SQUARE_WIDTH;
		break;
	default:
		printf("JudgeHit_R error!\n");
		exit(1);
		break;
	}
	k = 0;
	BlockClr_R(cwhich_R, ctype_rem_R);
	switch (cwhich_R)
	{
	case 0:
		for (i = 0; i < 8; i = i + 2)
		{
			j = i + 1;
			x = LineBlock[ctype_R][i] + (x_offset_R * ROCK_SQUARE_WIDTH) + LtoRIGHT + n_x;
			y = LineBlock[ctype_R][j] + (y_offset_R * ROCK_SQUARE_WIDTH) + n_y;
			ColorNext[k] = FlagArr[x][y];
			k++;
		}
		break;
	case 1:
		for (i = 0; i < 8; i = i + 2)
		{
			j = i + 1;
			x = CubeBlock[ctype_R][i] + (x_offset_R * ROCK_SQUARE_WIDTH) + LtoRIGHT + n_x;
			y = CubeBlock[ctype_R][j] + (y_offset_R * ROCK_SQUARE_WIDTH) + n_y;
			ColorNext[k] = FlagArr[x][y];
			k++;
		}
		break;
	case 2:
		for (i = 0; i < 8; i = i + 2)
		{
			j = i + 1;
			x = TBlock[ctype_R][i] + (x_offset_R * ROCK_SQUARE_WIDTH) + LtoRIGHT + n_x;
			y = TBlock[ctype_R][j] + (y_offset_R * ROCK_SQUARE_WIDTH) + n_y;
			ColorNext[k] = FlagArr[x][y];
			k++;
		}
		break;
	case 3:
		for (i = 0; i < 8; i = i + 2)
		{
			j = i + 1;
			x = LLBlock[ctype_R][i] + (x_offset_R * ROCK_SQUARE_WIDTH) + LtoRIGHT + n_x;
			y = LLBlock[ctype_R][j] + (y_offset_R * ROCK_SQUARE_WIDTH) + n_y;
			ColorNext[k] = FlagArr[x][y];
			k++;
		}
		break;
	case 4:
		for (i = 0; i < 8; i = i + 2)
		{
			j = i + 1;
			x = RLBlock[ctype_R][i] + (x_offset_R * ROCK_SQUARE_WIDTH) + LtoRIGHT + n_x;
			y = RLBlock[ctype_R][j] + (y_offset_R * ROCK_SQUARE_WIDTH) + n_y;
			ColorNext[k] = FlagArr[x][y];
			k++;
		}
		break;
	case 5:
		for (i = 0; i < 8; i = i + 2)
		{
			j = i + 1;
			x = LZBlock[ctype_R][i] + (x_offset_R * ROCK_SQUARE_WIDTH) + LtoRIGHT + n_x;
			y = LZBlock[ctype_R][j] + (y_offset_R * ROCK_SQUARE_WIDTH) + n_y;
			ColorNext[k] = FlagArr[x][y];
			k++;
		}
		break;
	case 6:
		for (i = 0; i < 8; i = i + 2)
		{
			j = i + 1;
			x = RZBlock[ctype_R][i] + (x_offset_R * ROCK_SQUARE_WIDTH) + LtoRIGHT + n_x;
			y = RZBlock[ctype_R][j] + (y_offset_R * ROCK_SQUARE_WIDTH) + n_y;
			ColorNext[k] = FlagArr[x][y];
			k++;
		}
		break;
	default:
		printf("JudgeHit_R Error!\n");
		break;
	}
	for (i = 0; i < 4; i++)
	{
		if (ColorNext[i] != WHITE)
		{
			return 1;
		}
	}
	return 0;
}

char getch()
{
	char c;
	system("stty raw");
	c = getchar();
	system("stty cooked");
	return c;
}

void draw_number(int number, int location_x, int location_y, int color)
{
	int x = 16, y = 34;
	switch (number)
	{
	case 0:
		Draw_ASCII(location_x, location_y, x, y, color, zero);
		break;
	case 1:
		Draw_ASCII(location_x, location_y, x, y, color, one);
		break;
	case 2:
		Draw_ASCII(location_x, location_y, x, y, color, two);
		break;
	case 3:
		Draw_ASCII(location_x, location_y, x, y, color, three);
		break;
	case 4:
		Draw_ASCII(location_x, location_y, x, y, color, four);
		break;
	case 5:
		Draw_ASCII(location_x, location_y, x, y, color, five);
		break;
	case 6:
		Draw_ASCII(location_x, location_y, x, y, color, six);
		break;
	case 7:
		Draw_ASCII(location_x, location_y, x, y, color, seven);
		break;
	case 8:
		Draw_ASCII(location_x, location_y, x, y, color, eight);
		break;
	case 9:
		Draw_ASCII(location_x, location_y, x, y, color, nine);
		break;
	}
}

void draw_small_letter(char letter, int location_x, int location_y, int color)
{
	int x1 = 16, y1 = 34;
	int x2 = 32, y2 = 34;
	int x3 = 23, y3 = 34;
	switch (letter)
	{
	case 'W':
		Draw_ASCII(location_x, location_y, x2, y2, color, W_s);
		break;
	case 'C':
		Draw_ASCII(location_x, location_y, x1, y1, color, C_s);
		break;
	case 'F':
		Draw_ASCII(location_x, location_y, x1, y1, color, F_s);
		break;
	case 'I':
		Draw_ASCII(location_x, location_y, x1, y1, color, I_s);
		break;
	case 'J':
		Draw_ASCII(location_x, location_y, x1, y1, color, J_s);
		break;
	case 'L':
		Draw_ASCII(location_x, location_y, x1, y1, color, L_s);
		break;
	case 'P':
		Draw_ASCII(location_x, location_y, x1, y1, color, P_s);
		break;
	case 'S':
		Draw_ASCII(location_x, location_y, x1, y1, color, S_s);
		break;
	case 'Z':
		Draw_ASCII(location_x, location_y, x1, y1, color, Z_s);
		break;
	case 'A':
		Draw_ASCII(location_x, location_y, x3, y3, color, A_s);
		break;
	case 'B':
		Draw_ASCII(location_x, location_y, x3, y3, color, B_s);
		break;
	case 'D':
		Draw_ASCII(location_x, location_y, x3, y3, color, D_s);
		break;
	case 'E':
		Draw_ASCII(location_x, location_y, x3, y3, color, E_s);
		break;
	case 'G':
		Draw_ASCII(location_x, location_y, x3, y3, color, G_s);
		break;
	case 'H':
		Draw_ASCII(location_x, location_y, x3, y3, color, H_s);
		break;
	case 'K':
		Draw_ASCII(location_x, location_y, x3, y3, color, K_s);
		break;
	case 'M':
		Draw_ASCII(location_x, location_y, x3, y3, color, M_s);
		break;
	case 'N':
		Draw_ASCII(location_x, location_y, x3, y3, color, N_s);
		break;
	case 'O':
		Draw_ASCII(location_x, location_y, x3, y3, color, O_s);
		break;
	case 'Q':
		Draw_ASCII(location_x, location_y, x3, y3, color, Q_s);
		break;
	case 'R':
		Draw_ASCII(location_x, location_y, x3, y3, color, R_s);
		break;
	case 'T':
		Draw_ASCII(location_x, location_y, x3, y3, color, T_s);
		break;
	case 'U':
		Draw_ASCII(location_x, location_y, x3, y3, color, U_s);
		break;
	case 'V':
		Draw_ASCII(location_x, location_y, x3, y3, color, V_s);
		break;
	case 'X':
		Draw_ASCII(location_x, location_y, x3, y3, color, X_s);
		break;
	case 'Y':
		Draw_ASCII(location_x, location_y, x3, y3, color, Y_s);
		break;
	}
}

//绘制大小为8×16的ASCII码
void Draw_ASCII(unsigned int location_x, unsigned int location_y, unsigned int x, unsigned int y, unsigned int color, const unsigned char ch[])
{
	unsigned short int j, k;
	unsigned char mask, buffer;
	int a = x * y / 8;
	for (k = 0; k < a; k++)
	{
		mask = 0x01;
		buffer = ch[k];
		for (j = 0; j < 8; j++)
		{
			if (mask & buffer)
			{
				draw_point(location_x + j + (k % (x / 8)) * 8, location_y + k / (x / 8), color);
			}
			mask = mask << 1;
		}
	}
}

int main(void)
{
	srand((unsigned)time(NULL));
	start_LCD();
	pid_t fpid; //fork back pid
	int count = 0;
	int buttons_fd;
	char buttons[2] = {'0', '0'};
	char realchar;
	int p = 0;
	int id;									  //共享内存句柄
	char *sharedMem = NULL;					  //共享内存指针
	id = shmget(KEY, SIZE, IPC_CREAT | 0666); //创建共享内存
	sharedMem = (char *)shmat(id, NULL, 0);
	memset(sharedMem, 0x00, SIZE);
	fpid = fork(); //create fork
	sharedMem[42] = 0;
	sharedMem[43] = 0;
	sharedMem[44] = 0;
	sharedMem[45] = 0;
	sharedMem[46] = 1;
	DrawTable();
	//printf("id1:%d", id);
	if (fpid < 0)
	{
		printf("fork error!");
	}
	else if (fpid == 0)
	{
		int id;									  //共享内存句柄
		char *sharedMem = NULL;					  //共享内存指针
		id = shmget(KEY, SIZE, IPC_CREAT | 0666); //创建共享内存
		sharedMem = (char *)shmat(id, NULL, 0);
		memset(sharedMem, 0x00, SIZE);

		printf("left %d, idc:%d\n", getpid(), id);
		buttons_fd = open("/dev/dial_key", 0);
		if (buttons_fd < 0)
		{
			perror("open device buttons failed！");
			exit(1);
		}

		CreateRand();
		game_mode = 1;
		while (1)
		{
			CreateBlock();
			PreviewBlock();

			while (1)
			{
				usleep(1000000);
				do
				{
					//printf("46-1:%d\n", sharedMem[46]);
					if (read(buttons_fd, buttons, sizeof buttons) != sizeof buttons)
					{
						perror("read buttons:");
						exit(1);
					}
					if (game_mode == 3)
					{
						sharedMem[46] = 3;
					}

					sharedMem[42] = 0;
					sharedMem[44] = 0;
					sharedMem[43] = 0;
					sharedMem[45] = 0;
					//printf("%d %d", sharedMem[43], sharedMem[45]);

					switch (buttons[0])
					{
					case 51:
						switch (buttons[1])
						{
						case 51:
							realchar = '1';
							sharedMem[45] = 0;
							sharedMem[43] = 1;
							break;
						case 50:
							realchar = '2';
							sharedMem[45] = 1;
							sharedMem[43] = 1;
							break;
						case 49:
							realchar = '3';
							break;
						case 48:
							realchar = 'A';
							sharedMem[44] = 1;
							sharedMem[42] = 1;
							break;
						}
						break;
					case 50:
						switch (buttons[1])
						{
						case 51:
							realchar = '4';
							sharedMem[45] = 2;
							sharedMem[43] = 1;
							break;
						case 50:
							realchar = '5';
							break;
						case 49:
							realchar = '6';
							sharedMem[45] = 3;
							sharedMem[43] = 1;
							break;
						case 48:
							realchar = 'B';
							sharedMem[44] = 2;
							sharedMem[42] = 1;
							break;
						}
						break;
					case 49:
						switch (buttons[1])
						{
						case 51:
							realchar = '7';
							break;
						case 50:
							realchar = '8';
							sharedMem[45] = 4;
							sharedMem[43] = 1;
							break;
						case 49:
							realchar = '9';
							break;
						case 48:
							realchar = 'C';
							sharedMem[44] = 3;
							sharedMem[42] = 1;
							break;
						}
						break;
					case 48:
						switch (buttons[1])
						{
						case 51:
							realchar = '*';
							break;
						case 50:
							realchar = '0';
							break;
						case 49:
							realchar = '#';
							sharedMem[44] = 0;
							sharedMem[42] = 1;
							break;
						case 48:
							realchar = 'D';
							sharedMem[44] = 4;
							sharedMem[42] = 1;
							break;
						}
						break;
					}

					if (sharedMem[43] == 1)
					{
						switch (sharedMem[45])
						{
						case 0:
							if (sharedMem[46] == 1)
							{
								sharedMem[46] = 0;
								printf("Player A pause the game!\n");
							}
							else
							{
								sharedMem[46] = 1;
								printf("Player A restart the game!\n");
							}
							break;
						case 1:
							switch (cwhich)
							{
							case 1:
								break;
							case 0:
							case 5:
							case 6:
								if (ctype == 1)
									ctype = 0;
								else
									ctype++;
								if (JudgeHit(0))
								{
									ctype = ctype_rem;
								}
								ctype_rem = ctype;
								break;
							case 2:
							case 3:
							case 4:
								if (ctype == 3)
									ctype = 0;
								else
									ctype++;
								if (JudgeHit(0))
								{
									ctype = ctype_rem;
								}
								ctype_rem = ctype;
								break;
							default:
								printf("switch error!%d,%d,%d\n", cwhich, ctype, ctype_rem);
								exit(1);
							}
							break;
						case 2:
							if (!JudgeHit(2))
							{
								x_offset--;
							}
							break;
						case 3:
							if (!JudgeHit(3))
							{
								x_offset++;
							}
							break;
						case 4:
							if (!JudgeHit(1))
							{
								y_offset++;
							}
							break;
						default:
							printf("small keyboard error!\n");
							break;
						}
					}
					//printf("%d %d", sharedMem[43], sharedMem[45]);
				} while (!sharedMem[46]);

				if (JudgeHit(1))
				{
					DrawBlock(cwhich, ctype_rem);
					JudgeFull();
					break;
				}
				y_offset++;
				DrawBlock(cwhich, ctype_rem);
				//printf("in");
			}
			//printf("out");
			if (sharedMem[46] == 3)
			{
				printf("game end!");
				exit(1);
			}

			if (score == 50)
			{
				printf("A Win!\n");
				PreviewBlock();
				sharedMem[46] = 3;
				exit(1);
			}
		}
		sharedMem[46] = 3;
	}
	else
	{
		//printf("right %d\n, idf:%d", getpid(), id);
		CreateRand_R();
		game_mode_R = 1;
		while (1)
		{
			CreateBlock_R();
			PreviewBlock_R();
			while (1)
			{
				usleep(1000000);
				do
				{
					//printf("46-2:%d\n", sharedMem[46]);
					if (game_mode_R == 3)
					{
						sharedMem[46] = 3;
					}

					//printf("46:%d\n", sharedMem[46]);
					//printf("start right loop\n");
					if (sharedMem[42] == 1)
					{
						switch (sharedMem[44])
						{
						case 0:
							if (sharedMem[46] == 1)
							{
								sharedMem[46] = 0;
								printf("Player B pause the game!\n");
							}
							else
							{
								sharedMem[46] = 1;
								printf("Player B restart the game!\n");
							}
							break;
						case 1:
							switch (cwhich_R)
							{
							case 1:
								break;
							case 0:
							case 5:
							case 6:
								if (ctype_R == 1)
									ctype_R = 0;
								else
									ctype_R++;
								if (JudgeHit_R(0))
								{
									ctype_R = ctype_rem_R;
								}
								ctype_rem_R = ctype_R;
								break;
							case 2:
							case 3:
							case 4:
								if (ctype_R == 3)
									ctype_R = 0;
								else
									ctype_R++;
								if (JudgeHit_R(0))
								{
									ctype_R = ctype_rem_R;
								}
								ctype_rem_R = ctype_R;
								break;
							default:
								printf("switch error!%d,%d,%d\n", cwhich_R, ctype_R, ctype_rem_R);
								exit(1);
							}
							break;
						case 2:
							if (!JudgeHit_R(2))
							{
								x_offset_R--;
							}
							break;
						case 3:
							if (!JudgeHit_R(3))
							{
								x_offset_R++;
							}
							break;
						case 4:
							if (!JudgeHit_R(1))
							{
								y_offset_R++;
							}
							break;
						default:
							printf("big keyboard error!\n");
							break;
						}
						//printf("judge go!\n");
					}
					else
					{
						//printf("42:%d\n  44:%d\n", sharedMem[42], sharedMem[44]);
					}

					//printf("end of loop");

				} while (!sharedMem[46]);

				if (JudgeHit_R(1))
				{
					DrawBlock_R(cwhich_R, ctype_rem_R);
					JudgeFull_R();
					break;
				}
				y_offset_R++;
				DrawBlock_R(cwhich_R, ctype_rem_R);
			}

			if (sharedMem[46] == 3)
			{
				printf("game end!");
				exit(1);
			}

			if (score_R == 50)
			{
				printf("B Win!\n");
				PreviewBlock_R();
				sharedMem[46] = 3;
				exit(1);
			}
		}
		sharedMem[46] = 3;
	}
	return 0;
}