#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

int main(){
	int fd;
	int ret;
	char send[20];
	//open the driver
	fd = open("/dev/char_driver", O_RDWR);
	if(fd < 0){
		printf("Error in open the file. \n");
		return 0;
	}
	ret = read(fd,send,sizeof(int));
	if(ret < 0){
		printf("Error in read the file.\n");
		return 0;
	}

	printf("Enter a number that go into kernel: ");
	scanf("%s",send);

	ret = write(fd,send,sizeof(int));
	if(ret < 0){
		printf("Error in the write file.\n");
		return 0;
	}
	ret = close(fd);		
	return 1;
}








