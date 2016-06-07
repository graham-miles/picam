#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <string.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/ioctl.h>
#include "wishbone_wrapper.h"
#include "config.h"

char text_buffer [512] ;

#define MAX_WIDTH 640
#define MAX_HEIGHT 480
#define NB_CHAN 2
#define NB_GRAB 1
#define SINGLE_ACCESS_SIZE 2048

unsigned char grab_buffer[MAX_WIDTH*MAX_HEIGHT*3*2] ;
unsigned char rgb_buffer[MAX_WIDTH*MAX_HEIGHT*3] ;
unsigned int fifo_id = 0 ;
unsigned int image_width = MAX_WIDTH ;
unsigned int image_height = MAX_HEIGHT ;

char rgb_file_name [128] ;
char yuv_file_name [128] ;
char jpeg_file_name [128] ;

int min(int a, int b){
	if(a > b ){
		return b ;	
	}
	return a ;
}

int grab_frame(void){
	FILE* jpeg_fd;
	int i = 0;
    unsigned int nb = 0 ;
    float y, u, v ;
    float r, g, b ;
    int remaining ; 
    char * fPointer ;
    int outlen = 0;
    int vsync = 0 ;
    unsigned short cmd_buffer[8] ;
    unsigned short vsync1, vsync2 ;
    unsigned char * start_buffer, * end_ptr;
    int inc=0;
    sprintf(jpeg_file_name, "./grabbed_frame%04d.jpg", inc);
		jpeg_fd  = fopen(jpeg_file_name, "wb");
		if(jpeg_fd == NULL){
			printf("Error opening output file \n");
			exit(EXIT_FAILURE);
		}
    cmd_buffer[0] = 0 ;
        cmd_buffer[1] = 0 ;
        cmd_buffer[2] = 0 ;
        wishbone_write((unsigned char *) cmd_buffer, 6, FIFO_ADDR+FIFO_CMD_OFFSET);
	nb = 0 ;
        while(nb < (((image_width)*(image_height)*NB_CHAN)+4)*NB_GRAB){
                wishbone_read((unsigned char *) cmd_buffer, 6, FIFO_ADDR+FIFO_CMD_OFFSET);
                while(cmd_buffer[2] < SINGLE_ACCESS_SIZE/2){
                         wishbone_read((unsigned char *) cmd_buffer, 6, FIFO_ADDR+FIFO_CMD_OFFSET);
                }
                wishbone_read(&grab_buffer[nb], SINGLE_ACCESS_SIZE, FIFO_ADDR);
                nb += SINGLE_ACCESS_SIZE ;
        }

	i = 0 ;
	vsync = 0 ;
	start_buffer = grab_buffer ;
	end_ptr = &start_buffer[((image_width*image_height*NB_CHAN)+4)*NB_GRAB];
	vsync1 = *((unsigned short *) start_buffer) ;
	vsync2 = *((unsigned short *) &start_buffer[(image_width*image_height*NB_CHAN)+2]) ;
	while(vsync1 != 0x55AA && vsync2 != 0x55AA && start_buffer < end_ptr){
			start_buffer+=2 ;
			vsync1 = *((unsigned short *) start_buffer) ;
			vsync2 = *((unsigned short *) &start_buffer[(image_width*image_height*NB_CHAN)+2]) ;
	}
	if(vsync1 == 0x55AA && vsync2 == 0x55AA){
			vsync = 1 ;
			fPointer = start_buffer ;
	} else {
		printf("VSYNC not working\n");
		exit(0);
	}
	if(vsync){
		for(i = 0 ; i < image_width*image_height ; i ++){
			y = (float) fPointer[(i*2)] ;
			if(i%2 == 1){
				u = (float) fPointer[(i*2)+1];
				v = (float) fPointer[(i*2)+3];
			}else{
				u = (float) fPointer[(i*2)-1];
        	        	v = (float) fPointer[(i*2)+1];
			}
			r =  y + (1.4075 * (v - 128.0));
			g =  y - (0.3455 * (u - 128.0)) - (0.7169 * (v - 128.0));
			b =  y + (1.7790 * (u - 128.0)) ;
			rgb_buffer[(i*3)] = (unsigned char) abs(min(r, 255)) ;
			rgb_buffer[(i*3)+1] = (unsigned char) abs(min(g, 255)) ;
			rgb_buffer[(i*3)+2] = (unsigned char) abs(min(b, 255)) ;
		} 
		write_jpegfile(rgb_buffer, image_width, image_height, 3, jpeg_fd, 100);
		fclose(jpeg_fd);
	}
	return 0 ;
}

int main(int argc, char ** argv){

	if (grab_frame() < 0) 
	{
		printf("\nFAIL");
		return -1; 
	}
	printf("\nSUCCESS");
	return 0 ;
	
}
