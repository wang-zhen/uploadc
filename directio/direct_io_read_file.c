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
    int fd;
    int ret;
    unsigned char *buf;
    ret = posix_memalign((void **)&buf, 512, BUF_SIZE);
    if (ret) {
        perror("posix_memalign failed");
        exit(1);
    }
 
    fd = open(argv[1], O_RDONLY | O_DIRECT | O_SYNC, 0755);
    if (fd < 0){
        perror("open failed");
        exit(1);
    }
 
    do {
        ret = read(fd, buf, BUF_SIZE);
        if (ret < 0) {
            perror("read error");
			break;
        }
		printf("%s",buf);
        memset(buf,0,BUF_SIZE);
    } while (ret > 0);
     
    free(buf);
    close(fd);

	return 0;
}
