//
//  fpgalink.c
//  Octree
//
//  Created by Audun Wilhelmsen on 09.06.12.
//  Copyright (c) 2012 NTNU. All rights reserved.
//
//


#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <libfpgalink.h>

#define CMD_WRITE 0x01
#define CMD_READ 0x02

#define FAIL(x) exit(x)

#define CHECK(x) \
if ( status != FL_SUCCESS ) {       \
fprintf(stderr, "%s\n", error); \
flFreeError(error);             \
FAIL(x);									\
}
static bool opened = false;
static struct FLContext *handle = NULL;
static FLStatus status;
static const char *error = NULL;
static bool flag;
static uint8 byte = 0;

static char *vp = "1443:0007";
static char *ivp = "1443:0007";

static bool isNeroCapable, isCommCapable;

unsigned int swap_endian(unsigned int value)
{
    char *ptr = (char*)&value;
    char out[4];
    out[0] = ptr[3]; out[1] = ptr[2]; out[2] = ptr[1]; out[3] = ptr[0];
    return *((unsigned int*)out);
}

void fl_init()
{
	if (opened) return;
	flInitialise();
	
	printf("Attempting to open connection to FPGALink device %s...\n", vp);
	status = flOpen(vp, &handle, NULL);
	if (status ) {
		
		printf("Loading firmware...\n");
		status = flLoadStandardFirmware(ivp, vp, &error);
        
		int count = 60;
		printf("Awaiting renumeration");
		flSleep(1000);
		do {
			printf(".");
			fflush(stdout);
			flSleep(100);
			status = flIsDeviceAvailable(vp, &flag, &error);
			CHECK(9);
			count--;
		} while ( !flag && count );
		printf("\n");
		if ( !flag ) {
			fprintf(stderr, "FPGALink device did not renumerate properly as %s\n", vp);
			FAIL(10);
		}
		flSleep(2000);
		printf("Attempting to open connection to FPGLink device %s again...\n", vp);
		status = flOpen(vp, &handle, &error);
		CHECK(11);
		
	}
	
	printf("Connection open\n");
	
	isNeroCapable = flIsNeroCapable(handle);
	isCommCapable = flIsCommCapable(handle);
    
    opened = true;
    
}


void fl_upload(const char *file_path, unsigned int address, bool verify, bool swap)
{
	if (isCommCapable) {
		// Stall CPU
        if (!handle) return;
		byte = 0x01;
		status = flWriteRegister(handle, 1000, 0x00, 1, &byte, &error); 
        CHECK(19);	
        
        
        
		// Open file
		FILE *f;
		f = fopen(file_path, "rb");
		if (!f) {
			printf("File not found: %s\n", file_path);
		}
		
		// Get file size
		fseek(f, 0L, SEEK_END);
        unsigned int size = ftell(f);
        unsigned int size_in_words = (size + (4-1)) / 4;
        fseek(f, 0L, SEEK_SET);
		
		// Send command
        unsigned int cmd[3] = {
            swap_endian(CMD_WRITE),
            swap_endian(size_in_words),
            swap_endian(address)
        };
        status = flWriteRegister(handle, 1000, 0x01, 12, (unsigned char*)cmd, &error); CHECK(19);
        
        // Allocate buffer
        unsigned int *buffer = malloc(size_in_words*4);
		
		
		// Upload file
		size_t n_read;
		
        n_read = fread(buffer, 1, size, f);
        
        if (swap) {
            unsigned int i;
            for (i=0;i<size_in_words;i++)
                buffer[i] = swap_endian(buffer[i]);
        }
        
        status = flWriteRegister(handle, 2000, 0x01, size_in_words*4, (unsigned char*)buffer, &error); CHECK(30);
        
        // Verify data
        if (verify) {
            printf("Verifying...\n");
            // Send command
            unsigned int cmd[3] = {
                swap_endian(CMD_READ),
                swap_endian(size_in_words),
                swap_endian(address)
            };
            status = flWriteRegister(handle, 1000, 0x01, 12, (unsigned char*)cmd, &error); CHECK(19);
            
            
            unsigned int *verify_buffer = malloc(size_in_words*4);
            unsigned int *verify_ptr = verify_buffer;
            unsigned int i = 0;
            unsigned int block_size = 256;
            
            while((i+block_size)<size_in_words) {            
                status = flReadRegister(handle, 2000, 0x01, block_size*4, (unsigned char*)verify_ptr, &error); CHECK(30); 
                i += block_size; 
                verify_ptr += block_size;      
                // sleep(1);
            }
            for(;i<size_in_words;i++) {
                // printf("%d %d\n", i, size_in_words);
                status = flReadRegister(handle, 2000, 0x01, 4, (unsigned char*)verify_ptr, &error); CHECK(30);   
                verify_ptr++;         
            }
            printf("\n");
            
            // status = flReadRegister(handle, 2000, 0x01, size_in_words*4, (unsigned char*)&verify_buffer, &error); CHECK(30);			
            for (i=0;i<size_in_words;i++) {
                if (buffer[i] != verify_buffer[i]) {
                    printf("Error at address %d - %.8x vs %.8x\n", address+i*4, buffer[i], verify_buffer[i]);
                    {
                        unsigned int cmd[4] = {
                            swap_endian(CMD_WRITE),
                            swap_endian(1),
                            swap_endian(address+i*4),
                            buffer[i]
                        };
                        status = flWriteRegister(handle, 1000, 0x01, 16, (unsigned char*)cmd, &error); CHECK(19);  
                    }
                    {             
                        unsigned int cmd[3] = {
                            swap_endian(CMD_READ),
                            swap_endian(1),
                            swap_endian(address+i*4)
                        };
                        unsigned int data;
                        status = flWriteRegister(handle, 1000, 0x01, 12, (unsigned char*)cmd, &error); CHECK(19);  
                        status = flReadRegister(handle, 2000, 0x01, 4, (unsigned char*)&data, &error); CHECK(30);   
                        // printf("%.8x\n", data);
                        if (data == buffer[i])
                            printf("Double-check OK\n");
                    }
                }
            }
            printf("\n");
        }
        
        
		
		fclose(f);
        free(buffer);
		printf("Upladed %d bytes\n", (int)n_read*4);
		
		// Reset & resume CPU. TODO: Optional reset
        byte = 0x03;
        status = flWriteRegister(handle, 1000, 0x00, 1, &byte, &error); CHECK(19);
        byte = 0x02;
        status = flWriteRegister(handle, 1000, 0x00, 1, &byte, &error); CHECK(19);
		byte = 0x00;
		status = flWriteRegister(handle, 1000, 0x00, 1, &byte, &error); CHECK(19);
		
		// TODO: Check data
	}	else {
		fprintf(stderr, "Device at %s does not support comm", vp);
		FAIL(20);
	}
}
