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
        //open the driver
        fd = open("/dev/LED_blink", O_RDWR);
	for(int i = 0;i < 4;i++){
		if(fd < 0){
			printf("Error in open the file. \n");
			return 0;
		}
		printf("Enter a the LED number for the PCI: ");
		scanf("%s",send);
		char * buff;
		if(send[0] == 'E'){
			buff = "2046";
		}
		else{
			buff = "2047";
		}
		ret = write(fd,buff,sizeof(int));
		if(ret < 0){
			printf("Error in the write file.\n");
			return 0;
		}
		ret = read(fd,send,sizeof(int));
		if(ret < 0){
			printf("Error in read the file.\n");
			return 0;
		}	
		printf("The value we getting back from the read() is %s. \n",send);
	}
        ret = close(fd);
        return 1;
}	





