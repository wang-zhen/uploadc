#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/file.h>
#include <sys/types.h>
#include <sys/stat.h> 
#include <string.h>
#define BUF_SIZE 1024
 
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
 
    fd = open(argv[1], O_RDONLY, 0755);
    if (fd < 0){
        perror(argv[1]);
        free(buf);
        exit(1);
    }
 
    do {
        ret = read(fd, buf, BUF_SIZE);
        if (ret < 0) {
            perror(argv[1]);
        }
		printf("%s\n",buf);
    } while (ret > 0);
     
    free(buf);
    close(fd);

	return 0;
}
