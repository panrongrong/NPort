#include <vxWorks.h>
#include <spinLockLib.h>
#include <intLib.h>
#include <stdio.h>
#include <iosLib.h>
#include "sysGpio.h"

int fd0=-1;
int fd1=-1;
int fd2=-1;
int fd3=-1;
/* gpio driver test : create */
int gcrt(UINT8 channel){
	GpioDrv();
	GpioDevCreate(channel);
	return OK;
}
/* gpio driver test : dir */
int gdir(int channel,UINT32 dir){
	int ret;
	switch(channel){
	case 0:
		ret=ioctl(fd0, GPIO_DIR, dir);
		break;
	case 1:
		ret=ioctl(fd1, GPIO_DIR, dir);
		break;
	case 2:
		ret=ioctl(fd2, GPIO_DIR, dir);
		break;
	case 3:
		ret=ioctl(fd3, GPIO_DIR, dir);
		break;
	default:
		printf("channel err\n");
		return ERROR;
	}
	if(ERROR==ret){
		printf("file descriptor does not exist\n");
		return ERROR;
	}
	printf("dir op ok\n");
	return OK;
}
/* gpio driver test open */
int gop(UINT8 channel){
	switch(channel){
	case 0:
		fd0=open("/Gpio/0",O_RDWR,0);
		if(fd0<0){
			printf("gpio %d open err\n",channel);
			return ERROR;
		}
		break;
	case 1:
		fd1=open("/Gpio/1",O_RDWR,0);
		if(fd1<0){
			printf("gpio %d open err\n",channel);
			return ERROR;
		}		
		break;
	case 2:
		fd2=open("/Gpio/2",O_RDWR,0);
		if(fd2<0){
			printf("gpio %d open err\n",channel);
			return ERROR;
		}		
		break;
	case 3:
		fd3=open("/Gpio/3",O_RDWR,0);
		if(fd3<0){
			printf("gpio %d open err\n",channel);
			return ERROR;
		}		
		break;
	default:
		printf("channel err\n");
		return ERROR;
	}
	return OK;
}
/* gpio driver test : remove */
int grd(int channel){
	UINT32 val;
	ssize_t ret;
	switch(channel){
	case 0:
		ret=read(fd0, (char*)&val, 4);		
		break;
	case 1:
		ret=read(fd1, (char*)&val, 4);		
		break;
	case 2:
		ret=read(fd2, (char*)&val, 4);		
		break;
	case 3:
		ret=read(fd3, (char*)&val, 4);		
		break;
	default:
		printf("channel err\n");
		return ERROR;
	}
	if(ret<0){
		printf("read err %d\n",ret);
		return ERROR;
	}
	printf("channel %d val:0x%x",channel,val);
	return OK;
}
/* gpio driver test write channel0 */
int gwr(int channel,UINT32 val){
	ssize_t ret;
	switch(channel){
	case 0:
		ret=write(fd0, (char*)&val, 4);		
		break;
	case 1:
		ret=write(fd1, (char*)&val, 4);		
		break;
	case 2:
		ret=write(fd2, (char*)&val, 4);		
		break;
	case 3:
		ret=write(fd3, (char*)&val, 4);		
		break;
	default:
		printf("channel err\n");
		return ERROR;
	}
	if(ret<0){
		printf("write err %d\n",ret);
		return ERROR;
	}
	printf("write ok\n");
	return OK;
}

