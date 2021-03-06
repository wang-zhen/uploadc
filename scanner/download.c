#define  _GNU_SOURCE

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
#include <dirent.h>
#include <attr/xattr.h>
#include <unistd.h>

#define BUFERSIZE 4096
#define GF_TRUESIZE_KEY  "trusted.glusterfs.truesize"
#define GF_SSDOP_KEY  "trusted.glusterfs.ssd.security"

int do_copy(const char *src, const char *dest)
{
	int sfd             = -1;
	int dfd             = -1;
	int ret             = -1;
	off_t offset        = 0;
	ssize_t rn          = 0;
	ssize_t wn          = 0;
	struct stat st      = {0,};
	unsigned char *buf  = NULL;

	if(!src || !dest)
		goto out;

    ret = posix_memalign((void **)&buf, 512, BUFERSIZE);
    if (ret) {
        perror("posix_memalign failed");
        exit(1);
    } 

	sfd = open(src, O_RDONLY);
	if(sfd < 0){
		perror(src);
		goto out;
	}
	if(stat(src, &st)){
		perror(src);
		goto out;
	}

	dfd = open(dest, O_RDWR | O_DIRECT | O_SYNC);
	if(dfd < 0){
		perror(dest);
		goto out;
	}
	
	
	ret = 0;
	while(1){
		rn = pread(sfd, buf, BUFERSIZE, offset);
		if(rn == -1){
			perror("read error");
			break;
		}
		if(rn == 0){
			printf("end of file\n");
			break;
		}
		wn = pwrite(dfd, buf, rn, offset);
		if(wn == -1){
			perror("write error");
			break;
		}
		if(wn != rn){
			perror("write error");
			break;
		}

		offset += rn;
		ret += rn;
		memset(buf, 0, BUFERSIZE);
	}

	syncfs(dfd);
out:
	if(buf)
		free(buf);
	if(sfd > 0)
		close(sfd);
	if(dfd > 0)
		close(dfd);
	return ret;
}

int do_direct_copy(const char *src, const char *dest)
{

	int sfd             = -1;
	int dfd             = -1;
	ssize_t rn          = 0;
	ssize_t wn          = 0;
	int ret             = -1;
	unsigned char *buf  = NULL;

	if(!src || !dest)
		goto out;

    ret = posix_memalign((void **)&buf, 512, BUFERSIZE);
    if (ret) {
        perror("posix_memalign failed");
        exit(1);
    } 

	sfd = open(src, O_RDONLY | O_DIRECT | O_SYNC);
	if(sfd < 0){
		perror(src);
		goto out;
	}

	syncfs(sfd);
	dfd = open(dest, O_RDWR | O_CREAT , 0644);
	if(dfd < 0){
		perror(dest);
		goto out;
	}

	syncfs(dfd);
    do {
        rn = read(sfd, buf, BUFERSIZE);
        if (rn < 0) {
            perror("read error");
            break;
        }   
        wn = write(dfd, buf, rn);
		if(wn != rn){
			perror("write error");
			break;
		}
		printf("wangzhen:%ld\n",rn);
        memset(buf,0,BUFERSIZE);
    } while (rn > 0); 
    
out:
    if(buf)
        free(buf);
    if(sfd)
        close(sfd);
    if(dfd)
        close(dfd);

    return 0;
}

int do_getxattr(const char *path, const char *name, void *value)
{
	size_t si;
	si = getxattr(path, name, NULL, 0);
	return getxattr(path, name, value, si);
}

int do_createhole(const char *path,off_t size,mode_t mode)
{
	int fd,n=0;
	off_t offset = 0;
	size_t count = 4096;
	char buf[4096] = {0,};

	if(!path)
		return -1;

	if((fd = open(path, O_CREAT|O_RDWR|O_SYNC,  0644)) < 0){
		perror("creat error");
		return -1;
	}

	if(size < 4096);
		count = size;

	while(size > 0){
		n = pwrite(fd,buf,count,offset);
		offset += n;
		size -= n;
		if(size < count)
			count = size;
	};

	syncfs(fd);
	close(fd);

	return 0;
}

int do_truncate(const char *path, off_t length)
{
	int fd,ret;

	fd = open(path, O_RDWR | O_SYNC | O_DIRECT);
	if(fd < 0){
		perror("open error");
		return -1;
	}

	syncfs(fd);
	ret = ftruncate(fd, length);
	close(fd);

	return ret;
}

int main(int argc, char **argv)
{
	char *srcpath              = NULL;
	char *destpath             = NULL;
	DIR  *srcdirp              = NULL; 
	struct dirent *dp          = NULL;
	struct stat st             = {0};
	off_t flsize               = 0;
	off_t trsize               = 0;
	long int i                 = 0;

    
	if(argc < 3){
		printf("%s <srcpath> <destpath>\n",argv[0]);
		exit(-1);
	}

	srcpath = argv[1];
	destpath = argv[2];
	if( *(srcpath + strlen(srcpath) - 1) == '/')
		*(srcpath + strlen(srcpath) - 1) = '\0';
	if( *(destpath + strlen(destpath) - 1) == '/')
		*(destpath + strlen(destpath) - 1) = '\0';

	if((srcdirp=opendir(destpath))==NULL){
		perror(destpath);
		exit(-1);
	}
	closedir(srcdirp);
	if((srcdirp=opendir(srcpath))==NULL){
		perror(srcpath);
		exit(-1);
	}
	
	while((dp=readdir(srcdirp))!=NULL){
		char srcfile[512]; 
		char destfile[512]; 
	    char tmppath[512];
		char value[512]; 
		if (dp->d_name[0] == '.' &&
					((dp->d_name[1] == 0) ||
					((dp->d_name[1] == '.') && (dp->d_name[2] == 0))))
				continue;

		if (fstatat(dirfd(srcdirp), dp->d_name, &st, 0))
				continue;

		if (!S_ISREG(st.st_mode))
			continue;
	
		flsize = st.st_size;

		memset(value,0,sizeof(value));
		memset(srcfile,0,sizeof(srcfile));
		memset(tmppath,0,sizeof(tmppath));
		memset(destfile,0,sizeof(destfile));
		strcpy(srcfile,srcpath);
		strcpy(destfile,destpath);
		strcat(srcfile,"/");
		strcat(destfile,"/");
		strcat(srcfile,dp->d_name);
		strcat(destfile,dp->d_name);
		strcpy(tmppath,destfile);
		strcat(tmppath,".back");
		
	    printf("create hole dest:%s size:%ld\n", tmppath, flsize);

		if(do_createhole(tmppath, flsize, st.st_mode)){
			printf("do_createhole error\n");
		}

	    printf("setxattr\n");
        if(setxattr(tmppath,GF_SSDOP_KEY,"1",1,0)){
            printf("setxattr error!\n");
        }   

	    printf("do_copy\n");
		if(do_copy(srcfile,tmppath) <= 0){
			printf("do copy error!\n");
		}
        if(removexattr(tmppath,GF_SSDOP_KEY)){
            printf("removexattr error!\n");
        }   
	    printf("do_getxattr\n");
		trsize = do_getxattr(srcfile,GF_TRUESIZE_KEY,value);
		if(trsize == -1){
			printf("do_getxattr error!\n");
		}
		trsize = atol(value);
	    printf("%ld.  src:%s dest:%s size:%ld trsize:%ld\n",++i,srcfile, tmppath, flsize,trsize);
	
		//if(do_createhole(destfile, flsize, st.st_mode)){
		//	printf("do_createhole error\n");
		//}
		if(do_direct_copy(tmppath,destfile) <= 0){
			printf("do direct_copy error!\n");
		}
/*  
		strcat(destfile,".sleep1");
		if(do_direct_copy(tmppath,destfile) <= 0){
			printf("do direct_copy error!\n");
		}
		if(do_truncate(destfile, trsize)){
			perror("truncate error");
			trsize = 0;
			continue;
		}
*/
		trsize = 0;
	}

	if(srcdirp)
    	closedir(srcdirp);
    return 0;
}
