#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <pthread.h>
#include <unistd.h>
#include <math.h>
#include <string.h>
#include <signal.h>
#include "draw.h"


pthread_t graphics_thread;
void m0_draw();
void m3_draw();

void * graphics_threadf()
{
	// initialize the SDL screen
	char res = screen_init("GBA");

	// return if failed to initialize, ending thread
	if (!res)
		return NULL;

	// clear the display
 	screen_clear();

	// the main display loop, constantly polls for pixels in memory and updates them
	// depending on the mode, will take an appropriate action
	while(1)
	{	
		// grab the video mode
		char video_mode = *(char*)(0x4000000);

		// perform the appropriate drawing method based on the video mode
		switch (video_mode)
		{
			case 0x00:
				m0_draw();
				break;
			case 0x03:
				m3_draw();
				break;
			default:
				break;
		}
	}

// 	screen_end();

	return NULL;
}

void m0_draw()
{

	typedef unsigned char  uint8;
	typedef unsigned short uint16;
	typedef unsigned int   uint32;
	typedef uint16 rgb15;
	typedef struct obj_attrs {
		uint16 attr0;
		uint16 attr1;
		uint16 attr2;
		uint16 pad;
	} __attribute__((packed, aligned(4))) obj_attrs;
	typedef uint32    tile_4bpp[8];
	typedef tile_4bpp tile_block[512];
	
}       

void m3_draw()
{
        // go through all pixels of the display
        for (int y = 0; y < 160; y++)
                for (int x = 0; x < 240; x++)
                {
                        // grab the appropriate address of this pixel from GBA memory
                        uint16_t * base_addr = (uint16_t *)((0x6000000 + x*2 + (240 * y)*2) );

                        // draw this pixel using the two colors at the address
                        draw_pixel(x, y, *base_addr);
                }

        // commit the buffers to display the drawn image
        screen_flip();
}

void vmap(int start, int end)
{
	// map virtual memory between specified addresses
	mmap((void*)start, end-start, PROT_WRITE | PROT_READ, MAP_ANONYMOUS | MAP_SHARED | MAP_FIXED, -1, 0);
}

void intHandler(int dummy)
{
	// quit the program
	exit(0);
}

int init()
{
	// info is from:
	// https://www.reinterpretcast.com/writing-a-game-boy-advance-game

	// 16 KB System ROM (executable, but not readable)
	vmap(0x00000000, 0x00003FFF);

	// 256 KB EWRAM (general purpose RAM external to the CPU)
	vmap(0x02000000, 0x02030000);

	// 32 KB IWRAM (general purpose RAM internal to the CPU)
	vmap(0x03000000, 0x03007FFF);

	// I/O Registers	
	vmap(0x04000000, 0x040003FF);

	// 1 KB Colour Palette RAM
	vmap(0x05000000, 0x050003FF);

	// 96 KB VRAM (Video RAM)
	vmap(0x06000000, 0x06017FFF);

	// 1 KB OAM RAM (Object Attribute Memory)
	vmap(0x07000000, 0x070003FF);

	// Game Pak ROM (0 to 32 MB)
	vmap(0x08000000, 0x09E84800);

	// Game Pak RAM
	vmap(0x0E000000, 0x0F000000);

	// create the SDL thread to display the image
	// TODO: races the asm code
	pthread_create(&graphics_thread, NULL, graphics_threadf, NULL);

	// attach the interrupt handler to the SIGINT (Ctrl-C)
	signal(SIGINT, intHandler);
}

int deinit()
{
	// hang forever so that the SDL window stays up
	while(1);
}

// this method will perform a fake software interrupt depending on
// the value that is passed into it
int fake_swi(int arg1, int arg2, int arg3, int swi)
{

	// Sqrt
	if (swi == 0x80000)
		return sqrt(arg1);

	// ArcTan2	
	if (swi == 0xA0000)
		return atan2(arg1, arg2);

	return sqrt(arg1);
}
