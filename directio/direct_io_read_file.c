#define _GNU_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/file.h>
#include <sys/types.h>
#include <sys/stat.h> 
#include <string.h>
#define BUF_SIZE 4096
 
int main(int argc, char *argv[])
{
    int fd,fd1;
    int ret,rn,wn;
    unsigned char *buf;
    ret = posix_memalign((void **)&buf, 4096, BUF_SIZE);
    if (ret) {
        perror("posix_memalign failed");
        exit(1);
    }
 
    fd = open(argv[1], O_RDONLY | O_DIRECT | O_SYNC, 0755);
    //fd = open(argv[1], O_RDONLY );
    if (fd < 0){
        perror("open failed");
        exit(1);
    }
    fd1 = open(argv[2], O_RDWR | O_CREAT, 0755);
    if (fd1 < 0){
        perror(argv[2]);
        exit(1);
    }
 
    do {
        rn = read(fd, buf, BUF_SIZE);
        if (rn < 0) {
            perror("read error");
            break;
        }
        wn = write(fd1, buf, rn);
	if(wn != rn)
		perror("write error!\n");

	printf("wangzhen:%d\n",rn);
        memset(buf,0,BUF_SIZE);
    } while (rn > 0);
     
    free(buf);
    close(fd);

	return 0;
}
