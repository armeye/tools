#include <u.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>
#include <errno.h>
#include <fcntl.h>

#include <libc.h>
#include <fcall.h>
#include <thread.h>
#include <9pclient.h>

#include <fuse.h>

int errstr2errno(void);

CFsys *fs;

static uvlong
qid2inode(Qid q)
{
	return q.path | ((uvlong)q.type<<56);
}

static void
dir2attr(Dir *d, struct stat *attr)
{
	attr->st_ino = qid2inode(d->qid);
	attr->st_size = d->length;
	attr->st_blocks = (d->length+8191)/8192;
	attr->st_atim.tv_sec = d->atime; 
	attr->st_atim.tv_nsec = 0;
	attr->st_mtim.tv_sec =d->mtime; 
	attr->st_mtim.tv_nsec = 0 ;
	attr->st_ctim = attr->st_mtim;	/* not right */
	attr->st_mode = d->mode&0777;
	if(d->mode&DMDIR)
		attr->st_mode |= S_IFDIR;
	else if(d->mode&DMSYMLINK)
		attr->st_mode |= S_IFLNK;
	else
		attr->st_mode |= S_IFREG;
	attr->st_nlink = 1;	/* works for directories! - see FUSE FAQ */
	attr->st_uid = getuid();
	attr->st_gid = getgid();
	attr->st_rdev = 0;
}

static int p9fsopen(const char *path, struct fuse_file_info *fi)
{
	CFid *id;
	int ret = 0;
	int openmode;

	openmode = fi->flags&3;

	puts("p9fsopen");

	if(fi->flags & O_TRUNC)
		openmode |= OTRUNC;

	id = fsopen(fs, (char *)path, openmode);

	if(id == nil){
		ret = errstr2errno();
		goto OUT;
	}

	if(openmode != OREAD && fsqid(id).type&QTDIR){
		fsclose(id);
		ret = -EISDIR;
		goto OUT;
	}
	
	fi->fh = (uintptr_t) id;
OUT:
	return ret;
}

static int p9fsrelease(const char *path, struct fuse_file_info *fi)
{
	CFid *id;

	id = (CFid *) (uintptr_t) fi->fh;
	fsclose(id);
	return 0;
}

static int p9fswrite(const char *path, char *buf, size_t sz, off_t off, struct fuse_file_info *fi)
{
	CFid *id;
	int ret;

	id = (CFid *) (uintptr_t) fi->fh;
	if(id == nil){
		ret = -ESTALE;
		goto OUT;
	}

	ret = fspwrite(id, buf, sz, off);
OUT:
	return ret;
}

static int p9fsread(const char *path, char *buf, size_t sz, off_t off, struct fuse_file_info *fi)
{
	CFid *id;
	int ret;
	
	id = (CFid *) (uintptr_t) fi->fh;
	if(id == nil){
		ret = -ESTALE;
		goto OUT;
	}
	ret = fspread(id, buf, sz, off);
OUT:
	return ret;
}

static int p9fsmkdir(const char *path, mode_t mode)
{
	fscreate(fs, (char *) path, OREAD, DMDIR | mode);
	return 0;
}

static int p9fsopendir(const char *path, struct fuse_file_info *fi)
{
	CFid *id;
	int ret = 0;

	id = fsopen(fs, (char *) path, fi->flags&3);

	if(id == nil){
		ret = errstr2errno();
		goto OUT;
	}

	if(!(fsqid(id).type&QTDIR)){
		ret = -ENOTDIR;
		fsclose(id);
		goto OUT;
	}

	fi->fh = (uintptr_t) id;
OUT:
	return ret;
}

static int p9fsreaddir(const char *path, 
		void *buf,
		fuse_fill_dir_t filler,
		off_t offset,
		struct fuse_file_info *fi)
{
	CFid *id;
	Dir **d = nil;
	int ret = 0;
	int i;
	long n;
	struct stat stat;

	id = (CFid *)(uintptr_t) fi->fh;

	if(id == nil){
		ret = -ESTALE;
		goto OUT;
	}

	n = fsdirreadall(id, d);

	if(n < 0){
		ret = errstr2errno();
		goto OUT;
	}

	for(i = 0; i < n; i++){
		dir2attr(d[i], &stat);
		filler(buf, d[i]->name, &stat, 0);
	}
	free(d);
OUT:
	return ret;
}

static int p9fscreate(const char *path, mode_t mode, struct fuse_file_info *fi)
{
	CFid *id;

	id = fscreate(fs, (char *) path, OREAD, mode&3);
	fi->fh = (uintptr_t) id;
	return 0;
}

static int p9fsaccess(const char *path, int amode)
{
	int ret = 0;

	ret = fsaccess(fs, (char *) path, amode);

	if(ret == 0)
		goto OUT;

	if(amode == AEXIST){
		ret = -ENOENT;
	} else {
		ret = -EACCES;
	}
OUT:
	return ret;
}

static int p9fsreadlink(const char *path, char *lnk, size_t lsz)
{
	CFid *id;
	Dir *d;
	int ret = 0;

	id = fsopen(fs, (char *) path, OREAD);

	if(id == nil){
		ret = errstr2errno();
		goto OUT;
	}

	d = fsdirfstat(id);

	if(!(d->mode&DMSYMLINK)){
		ret = -EINVAL;
		goto OUT;
	}

	lnk = d->ext;

	if(strlen(lnk) + 1 > lsz){
		ret = -ENAMETOOLONG;
		goto OUT;
	}
OUT:
	return ret;
}

static int 
p9fsstatfs(const char *path, struct statvfs *sfs)
{
	memset(sfs, 0, sizeof(struct statvfs));
	return 0;
}

static int p9fsflush(const char *path, struct fuse_file_info *fi)
{
	return 0;
}

static int p9fsgetattr(const char * path, struct stat *sfs)
{
	CFid *id;
	Dir *d;
	int ret = 0;

	id = fsopen(fs, (char *) path, OREAD);

	if(id == nil){
		ret = errstr2errno();
		goto OUT;
	}

	d = fsdirfstat(id);

	if(d == nil){
		ret = errstr2errno();
		goto OUT;
	}

	dir2attr(d, sfs);

	free(d);
	fsclose(id);
OUT:
	return ret;
}

static int p9fsfgetattr(const char *path, struct stat *sfs, struct fuse_file_info *fi)
{
	CFid *id;
	Dir *d;
	int ret = 0;

	id = (CFid *)(uintptr_t) fi->fh;

	if(id == nil){
		ret = -ESTALE;
		goto OUT;
	}

	d = fsdirfstat(id);

	if(d == nil){
		ret = errstr2errno();
		goto OUT;
	}

	dir2attr(d, sfs);
	free(d);
OUT:
	return ret;
}


static int
_9pfsremove(const char *name, int isdir)
{
	CFid *id;
	int ret = 0;
	
	if((id = fsopen(fs, (char *) name, ORDWR)) == nil){
		ret  = -ESTALE;
		goto OUT;
	}
	if(strchr(name, '/')){
		ret = -ENOENT;
		fsclose(id);
		goto OUT;
	}
	if(isdir && !(fsqid(id).type&QTDIR)){
		ret = -ENOTDIR;
		fsclose(id);
		goto OUT;
	}
	if(!isdir && (fsqid(id).type&QTDIR)){
		ret = -EISDIR;
		fsclose(id);
		goto OUT;
	}
	if(fsfremove(id) < 0){
		ret = errstr2errno();
		fsclose(id);
		goto OUT;
	}
OUT:
	return ret;
}


static int p9fsunlink(const char *path)
{
	return _9pfsremove((char *) path, 0);
}

static int p9fsrename(const char *from, const char *to)
{
	int ret = 0;
	CFid *id;
	Dir d;

	if(strchr(from, '/') || strchr(to, '/')){
		ret = -ENOENT;
		goto OUT;
	}

	id = fsopen(fs, (char *) from, ORDWR);

	if(id == nil){
		ret = errstr2errno();
		goto OUT;
	}

	nulldir(&d);
	d.name = (char *) to;
	
	if(fsdirfwstat(id, &d) < 0){
		ret = errstr2errno();
		fsclose(id);
		goto OUT;
	} 
OUT:
	return ret;
}

static int p9fsrmdir(const char *path)
{
	return _9pfsremove((char *) path, 1);
}

struct fuse_operations p9fsops = {
	.read = p9fsread,
	.readlink = p9fsreadlink,
	.write = p9fswrite,
	.open = p9fsopen,
	.create = p9fscreate,
	.access = p9fsaccess,
	.mkdir = p9fsmkdir,
	.opendir = p9fsopendir,
	.readdir = p9fsreaddir,
	.statfs = p9fsstatfs,
	.flush = p9fsflush,
	.getattr = p9fsgetattr,
	.fgetattr = p9fsfgetattr,
	.unlink = p9fsunlink,
	.rmdir = p9fsrmdir,
	.release = p9fsrelease,
	.releasedir = p9fsrelease,
	.rename = p9fsrename
};

void
watchfd(void *v)
{
	int fd = (int)(uintptr)v;

	/* wait for exception (file closed) */
	fd_set set;
	FD_ZERO(&set);
	FD_SET(fd, &set);
	if(select(fd+1, NULL, NULL, &set, NULL) >= 0)
		threadexitsall(nil);
	return;
}

void threadmain(int argc, char *argv[])
{
	//int fd;

  	//fd = dial(netmkaddr("/tmp/ns.root.:0/plumb", "tcp", "564"), nil, nil, nil);
	//if(fd < 0)
		//sysfatal("dial %s: %r", argv[2]);
	//proccreate(watchfd, (void*)(uintptr)fd, 8192);

	setsid();

	fs = nsmount("plumb", "");
	if(fs == nil)
		sysfatal("fsmount: %r");
	fuse_main(argc, argv, &p9fsops, NULL);
	puts("oops");
	threadexits(0);
}
