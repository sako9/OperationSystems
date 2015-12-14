/*
 * =====================================================================================
 *
 *       Filename:  ks_server.c
 *
 *    Description:  
 *
 *        Version:  1.0
 *        Created:  02/17/2015 10:19:45 PM
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  Samuel Nwosu (), 
 *   Organization:  
 *
 * =====================================================================================
 */
#include <stdlib.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <sys/types.h>
#include <dirent.h>
#include <string.h>
#include <stdio.h>
#include <sys/stat.h>
#include <pthread.h>


void *searchFile(char *KeyWord, char *fileName){

	FILE *fp;
	char *line = NULL;
	char *linecopy = NULL;
	size_t buffer_size= 1024;
	char *ktok,*stok,*savepoint1,*savepoint2;
	char key[32];
	strcpy(key,KeyWord);
	if((fp = fopen(fileName,"r")) == NULL){
		fprintf(stderr,"Error opening file %s",fileName);
		return;
	}

	line =(char *) malloc(buffer_size*sizeof(char));
	ktok= strtok_r(key," /t/n",&savepoint1);
	while(-1 != getline(&line,&buffer_size,fp)){
		linecopy = strdup(line);
		stok = strtok_r(line," ",&savepoint2);
		while(stok != NULL){
			if(strcmp(ktok,stok)==0){
				printf("\n%s:num:%s:%s",fileName,ktok,linecopy);
				break;
			}
			else{
				stok = strtok_r(NULL," ",&savepoint2);
			}
		}
		free(linecopy);
	}
	fclose(fp);
	free(line);
}
void searchDir(char *dirName){
	DIR *d;
	int i =0;
	struct dirent *dir;
	struct stat statbuf;

	if((d = opendir(dirName)) == NULL ){
		fprintf(stderr,"error opening directory: %s\n",dirName);
		return;
	}
	chdir(dirName);
		while((dir = readdir(d)) != NULL){
			lstat(dir->d_name,&statbuf);
			if(S_ISDIR(statbuf.st_mode)){
				if(strcmp(".",dir->d_name)==0||strcmp("..",dir->d_name) ==0)
					continue;
				//printf("the directory is %s\n",dir->d_name);
				searchDir(dir->d_name);
			}else{
				//printf("%s\n",dir->d_name);
				searchFile("id",dir->d_name);
			}
		}
		chdir("..");
		closedir(d);
}



int main(int argc, char *argv[]){
	//key_t key;
	//int msgflg = IPC_CREAT | 0666;
	//int msgid;

	//key = ftok("ks_server.c",1);
	//msgid = msget(key,msgflg);
	searchDir("/home");

}
