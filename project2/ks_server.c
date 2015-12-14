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

struct Arguments{
	char *FileName;
	char *key;
	key_t id;
};

struct message_s{
	long type;
	struct data{
	char dir[128];
	char key[32];
	key_t cid;
	}data;
};

struct message_c{
	long type;
	char msg[2048];
};
typedef struct message_s Message;
typedef struct Arguments Arguments;
typedef struct message_c CMessage;
void sendMessage(char *msg,key_t key){
	int msgflg = 0666;
	int msgid;
	CMessage mess;

	if((msgid = msgget(key,msgflg)) == -1){
		fprintf(stderr,"msggent failed");
		exit(1);
	}
	mess.type = 1;
	strcpy(mess.msg,msg);

	if(msgsnd(msgid,&mess,sizeof(mess)-sizeof(long),0) == -1){
		fprintf(stderr,"failed to send message to client");
		exit(1);
	}
}

void *searchFile(void *arguments){

	FILE *fp;
	char *line = NULL;
	char *linecopy = NULL;
	size_t buffer_size= 1024;
	char *ktok,*stok,*savepoint1,*savepoint2;
	char key[32];
	char buff[2048];
	Arguments *args = arguments;
	int linenum =0;

	strcpy(key,args->key);
	if((fp = fopen(args->FileName,"r")) == NULL){
		//fprintf(stderr,"Error opening file %s\n",args->FileName);
		close(fp);
		free(line);
		free(args->FileName);
		free(args);
		pthread_exit(0);

	}

	line =(char *) malloc(buffer_size*sizeof(char));
	ktok= strtok_r(key," /t/n",&savepoint1);
	while(-1 != getline(&line,&buffer_size,fp)){
		linenum++;
		linecopy = strdup(line);
		stok = strtok_r(line," ",&savepoint2);
		while(stok != NULL){
			if(strcmp(ktok,stok)==0){
			//	printf("\n%s:%d:%s:%s",args->FileName,linenum,ktok,linecopy);
				sprintf(buff,"%s:%d:%s:%s",args->FileName,linenum,ktok,linecopy);
				sendMessage(buff,args->id);
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
	free(args->FileName);
	free(args);
	pthread_exit(0);
}
void searchDir(char *dirName,char *key,key_t cid){
	DIR *d;
	int i =0;
	Arguments *args;
	struct dirent *dir;
	struct stat statbuf;
	pthread_t threads[1024];

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
				searchDir(dir->d_name,key,cid);
			}else{
				args = (Arguments *) malloc(sizeof(Arguments));
				args->FileName= strdup(dir->d_name);
				args->key = key;
				args->id = cid;
				printf("I got called");
				pthread_create(&threads[i],NULL,searchFile,args);
				i++;
			}
		}
		int j =0;
		for(j; j<=i;j++){
			pthread_join(threads[j], NULL);
		}
		chdir("..");
		closedir(d);
}



int main(int argc, char *argv[]){
	key_t key;
	int msgflg = IPC_CREAT | 0666;
	int msgid;
	int pid;
	Message mess;

	while(1){

		if((key = ftok("ks_server.c",1))== -1){
			fprintf(stderr,"ftok failed");
			exit(1);
		}
		if((msgid = msgget(key,msgflg))==-1){
			fprintf(stderr,"msget failed");
			exit(1);
		}

		if(msgrcv(msgid,&mess,sizeof(mess)-sizeof(long),0,0) == -1){
			fprintf(stderr,"failed to get message");
			exit(1);
		}
		if(strcmp("exit",mess.data.key)==0)
			break;
		pid = fork();
		if(pid<0){
			fprintf(stderr,"Error creating child process");
			exit(1);
		}else if(pid == 0){
			 searchDir(mess.data.dir,mess.data.key,mess.data.cid);
			 
			 return 0;
		}else{
			 wait(NULL);
			 sendMessage("done",mess.data.cid);
		}
	}	
	if(msgctl(msgid,IPC_RMID,NULL) == -1){
			fprintf(stderr,"failed to remove is");
			exit(1);
		}
	return 0;

}
