/*% cc -o # %
 * com [-n] [file ...]
 * looks for the sequence /*% in each file, and sends the rest of the
 * line off to the shell, after replacing any instances of a `%' character
 * with the filename, and any instances of `#' with the filename with its
 * suffix removed.  Used to allow information about how to compile a program
 * to be stored with the program.  The -n flag causes com not to
 * act, but to print out the action it would have taken.
 */
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
int nflag=0;
int maxstat=0;
void make(char []);
int main(int argc, char *argv[]){
	int i;
	FILE *fd;
	char str[8192], *nl;

	while(argc>1 && argv[1][0]=='-'){
		for(i=1;argv[1][i];i++) switch(argv[1][i]){
		case 'n': nflag++; break;
		default:
		Usage:
			fprintf(stderr, "Usage: %s [-n] [file ...]\n", argv[0]);
			exit(1);
		}
		argv++;
		--argc;
	}
	if(argc==1){
		if((fd=fopen(".comfile", "r"))==NULL) goto Usage;
		while(fgets(str, sizeof(str), fd)!=NULL){
			nl=strchr(str, '\n');
			if(nl) *nl='\0';
			make(str);
		}
	}
	else{
		if(!nflag && (fd=fopen(".comfile", "w"))!=NULL){
			for(i=1;i!=argc;i++) fprintf(fd, "%s\n", argv[i]);
			fclose(fd);
		}
		for(i=1;i!=argc;i++) make(argv[i]);
	}
	exit(maxstat);
}
void make(char file[]){
	char command[8192];
	char *s, *t, *suffix;
	int c;
	FILE *f;
	int stat;

	if((f=fopen(file, "r"))==NULL){
		fprintf(stderr, "com: ");
		perror(file);
		if(maxstat<1) maxstat=1;
		return;
	}
	/*
	 * Look for /*%
	 */
	for(;;){
		c=getc(f);
		if(c==EOF){
			fprintf(stderr, "%s: no command\n", file);
			fclose(f);
			if(maxstat<1) maxstat=1;
			return;
		}
		if(c=='/'){
			if((c=getc(f))=='*' && (c=getc(f))=='%') break;
			ungetc(c, f);	/* might be another slash! */
		}
	}
	s=command;
	do;while((c=getc(f))==' ' || c=='\t');
	suffix=strrchr(file, '.');
	if(suffix==NULL) suffix=file+strlen(file);
	while(c>=0 && c!='\n'){
		switch(c){
		default:
			*s++=c;
			break;
		case '%':
			c=getc(f);
			if(c=='%') *s++='%';
			else{
				ungetc(c, f);
				t=file;
				while(*t) *s++ = *t++;
			}
			break;
		case '#':
			c=getc(f);
			if(c=='#') *s++='#';
			else{
				ungetc(c, f);
				t=file;
				while(t!=suffix) *s++ = *t++;
			}
		}
		c=getc(f);
	}
	*s='\0';
	if(nflag){
		printf("%s\n", command);
		return;
	}
	fprintf(stderr, "%s\n", command);
	switch(fork()){
	case -1:
		fprintf(stderr, "com: can't fork\n");
		exit(1);
	case 0:
		execl("/bin/sh", "sh", "-c", command, 0);
		fprintf(stderr, "com: No shell!\n");
		exit(1);
	default:
		wait(&stat);
		if(stat>maxstat) maxstat=stat;
		fclose(f);
	}
}
