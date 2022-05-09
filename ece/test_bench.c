#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>


#define LED_ON "E"
#define LED_offset "7F0"


int main(){
        int fd;
        u_int32_t ret;
        char send[20];
	int repeat = 0;	
	//open the driver
        fd = open("/dev/LED_blink", O_RDWR);
        if(fd < 0){
                printf("Error in open the file. \n");
                return 0;
        }
	while(repeat == 0){
		ret = read(fd,send,sizeof(int));
		if(ret < 0){
			printf("Error in read the file.\n");
			return 0;
		}

		printf("The blink rate value we getting back from the read() is %s. \n",send);

		printf("Enter a blink rate for the PCI: ");
		scanf("%s",send);
		/*
		//write LED_ON/LED_OFF to LED0
		char * buff;
		if(send[0] == 'E'){
			buff = "2046";
		}
		else{
			buff = "2047";
		}
		*/
		ret = write(fd,send,sizeof(int));
		if(ret < 0){
			printf("Error in the write file.\n");
			return 0;
		}
		printf("continue?(1/N)(0/Y): ");
		scanf("%d",&repeat);
	}
        ret = close(fd);
        return 1;
}	





