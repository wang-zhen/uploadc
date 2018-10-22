#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <linux/fs.h>
#include <assert.h>
#include <pthread.h>
#include <sys/ioctl.h>

#define BUFERSIZE 131072
pthread_mutex_t sp_mutex = PTHREAD_MUTEX_INITIALIZER;

ssize_t ssdpread(int fd, int bfd, void *buf, size_t count, off_t offset)
{
    int blocksize, index, readsize;
    ssize_t n, block;
    off_t flsize,boffset=0;
    size_t ret = 0;
    struct stat st; 

    if (ioctl(fd, FIGETBSZ, &blocksize)) {
        perror("FIBMAP ioctl failed");
        goto error;
    }   

    if (fstat(fd, &st)) {
        perror("fstat error");
        goto error;
    }   

    flsize = st.st_size;
	if(flsize & 15)
		flsize = (flsize & ~15) + 16;


    index = offset/blocksize;
	//boffset = offset & ~(blocksize-1);
	boffset = offset % blocksize;
	readsize = blocksize - boffset;

    if((offset + count) > flsize)
        count = flsize - offset;

    printf("blocksize:%d flsize: %ld count%ld\n",blocksize, flsize, count);
    while(count > 0){
        block = index;
        if (ioctl(fd, FIBMAP, &block)) {
            perror("FIBMAP ioctl failed");
            goto error;
        }   

        if(count < readsize)
            readsize = count;

        printf("wangzhe index:%d  block:%ld\n",index,block);
        n = pread(bfd,buf,readsize,(off_t)(block*blocksize + boffset));
        if(n < 0){
            perror("pread err");
            goto error;
        }

		if(boffset){
			boffset = 0;
			readsize = blocksize;
		}
        count -= n;
        ret += n;
        index++;
        buf += n;
    }

    return ret; 
error:
    return -1; 
}

int main(int argc, char **argv)
{
    int sfd, dfd, bfd, n, seek = 0;
    char buf[BUFERSIZE] = {0};

    assert(argv[1] != NULL);
    assert(argv[2] != NULL);
    assert(argv[3] != NULL);

    sfd = open(argv[1], O_RDONLY);
    if (sfd <= 0) {
        perror("error opening file");
        goto end;
    }

    dfd = open(argv[2], O_RDWR);
    if (dfd <= 0) {
        perror("error opening file");
        goto end;
    }

    bfd = open(argv[3], O_RDWR);
    if (bfd <= 0) {
        perror("error opening file");
        goto end;
    }

	while(1){
    	n=ssdpread(sfd, bfd, buf, BUFERSIZE, seek);
		if(n <= 0){
			perror("read error");
			break;
		}
		if(pwrite(dfd, buf, n, seek) < 0 ){
			perror("pwrite error");
			break;
		}

		seek += n;
		memset(buf, 0, BUFERSIZE);
	   // printf("buf:%s\n",buf);
	}
end:
	if(sfd)
    	close(sfd);
	if(dfd)
    	close(dfd);
	if(bfd)
    	close(bfd);
    return 0;
}
