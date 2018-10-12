#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <linux/fs.h>
#include <assert.h>
#include <pthread.h>
#include <fcntl.h>
#include <sys/ioctl.h>

#define BUFERSIZE 4096
pthread_mutex_t sp_mutex = PTHREAD_MUTEX_INITIALIZER;

ssize_t ssdpread(int fd, int sfd, void *buf, size_t count, off_t offset)
{
    int blocksize, index, readsize;
    char *cachebuf;
    ssize_t n, block;
    off_t flsize;
    size_t ret = 0;
    struct stat st; 

    if (ioctl(fd, FIGETBSZ, &blocksize)) {
        perror("FIBMAP ioctl failed");
        goto error;
    }   

    cachebuf = malloc(count);
    if(!cachebuf){
        perror("malloc error");
        goto error;

	}

    if (fstat(fd, &st)) {
        perror("fstat error");
        goto error;
    }   

    flsize = st.st_size;

    index = offset/blocksize;
    readsize = blocksize; 

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
        n = pread(sfd,buf,readsize,(off_t)(block*blocksize + offset));
        if(n < 0){
            perror("pread err");
            goto error;
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

ssize_t ssdpwrite(int fd, int sfd, const void *buf, size_t count, off_t offset)
{
    int blocksize, index, writesize;
    ssize_t n, block;
    off_t flsize;
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

    index = offset/blocksize;
    writesize = blocksize; 

    if((offset + count) > flsize)
        count = flsize - offset;

    while(count > 0){
        block = index;
        if (ioctl(fd, FIBMAP, &block)) {
            perror("FIBMAP ioctl failed");
            goto error;
        }   

        if(count < writesize)
            writesize = count;

        n = pwrite(sfd,buf,writesize,(off_t)(block*blocksize + offset));
        if(n < 0){
            perror("pread err");
            goto error;
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
    int sfd, dfd, bfd, n, offset = 0;
    char *buf = NULL;

    assert(argv[1] != NULL);
    assert(argv[2] != NULL);
    assert(argv[3] != NULL);

	buf =  malloc(BUFERSIZE*sizeof(char));
	if(!buf){
		perror("malloc error!");
		goto end;
	}

    //sfd = open(argv[1], O_RDWR | O_DIRECT);
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
    	n=ssdpread(sfd, bfd, buf, BUFERSIZE, offset);
    	//n=ssdpread(sfd, buf, BUFERSIZE, offset);
		if(n <= 0){
			printf("n=%d\n",n);
        	perror("END");
			break;
		}

    	//printf("size:%d offset:%d\n",n, offset);
    	//n=ssdpwrite(dfd, bfd, buf, n, offset);
    	n=pwrite(dfd, buf, n, offset);
		sleep(3);
    	//n=spwrite(dfd, buf, n, offset);

		offset += n;
		memset(buf, '\0', BUFERSIZE);
	}

end:
	if(buf)
    	free(buf);
	if(sfd)
    	close(sfd);
	if(dfd)
    	close(dfd);
	if(bfd)
    	close(bfd);
    return 0;
}
