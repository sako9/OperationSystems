/*
 afj =====================================================================================
 *
 *       Filename:  spksp.c
 *
 *    Description:  
 *
 *        Version:  1.0
 *        Created:  03/19/2015 03:09:54 PM
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  Samuel Nwosu (), 
 *   Organization:  
 *
 * =====================================================================================
 */
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <dirent.h>
#include <sys/stat.h>
#include <pthread.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <limits.h>
#include <semaphore.h>
#include <fcntl.h>


int max;
int running = 1;
int use = 0;
int fill = 0;
sem_t empty;
sem_t full;
sem_t mutex;
int pi =0;
struct flock fLock = {F_WRLCK, SEEK_SET,0,0,0};

struct Items{
	struct Items *next;
	char *out; 
};
typedef struct Items Items;

struct List{
	struct Items *first;
	int count;
};
typedef struct List List;

List *List_create(){
	return calloc(1, sizeof(List));
}

void List_free(List *list){
	Items *temp = NULL;
	while(list->first){
		temp = list->first->next;
		free(list->first->out);
		free(list->first);
		list->first = temp;
	}
	free(list);
}

int List_getSize(List *list){
	return list->count;
}


void List_add(List *list, char *output){
	Items *item = calloc(1,sizeof(Items));
	Items *temp = NULL;
	item->out = strdup(output);

	if(list->first == NULL){
		list->first = item;
	}else{
		temp = list->first;
		while(temp){
			if(temp->next){
				temp = temp->next;
			}else{
				temp->next =item;
				break;
			} 
		}
	}
	list->count++;
}


void List_pop(List *list,FILE *fp){
	Items *item = NULL;
	if(list->first){
		item = list->first;
		list->first = list->first->next;
	//	fprintf(stderr,"%s",item->out);
		fprintf(fp,"%s",item->out);
		free(item->out);
		free(item);
	}
	list->count--;
}


struct Arguments{
	char path[1024];
	char FileName[1024];
	char key[32];
	List *list;
	int num;
};
typedef struct Arguments Arguments;


void *printOut(void *args){
	FILE *fp = NULL;
	fLock.l_pid = getpid();
	Arguments *arg =args; 
	if((fp = fopen("output.out","ab+")) == NULL){
		fprintf(stderr,"Could not open file output.out");
		pthread_exit(0);
	}
	while(1){
	sem_wait(&full);
	sem_wait(&mutex);
	fcntl(fileno(fp),F_SETLKW,&fLock);
	List_pop(arg->list,fp);
	sem_post(&mutex);
	sem_post(&empty);
	fLock.l_type = F_UNLCK;
	fcntl(fileno(fp),F_SETLK, &fLock);
	}
	fclose(fp);
	pthread_exit(0);
}


void *searchFile(void *arguments){
	FILE *fp = NULL;
	char *line = NULL;
	char *linecopy = NULL;
	size_t line_size = 1024;
	char *stok, *savePoint;
	char output[2048];
	Arguments *args = arguments;
	int lineNum = 0;

	if((fp = fopen(args->path,"r")) == NULL){
		fprintf(stderr,"Could not open file %s\n",args->FileName);
		free(args);
		free(line);
		pthread_exit(0);
		
	}

	line = (char *) malloc(line_size*sizeof(char));
	while(-1 != getline(&line,&line_size,fp)){
		lineNum++;
		linecopy = strdup(line);
		stok = strtok_r(line," .,!@#$%^&*-_+`~\n\t\r\"()[]{};:",&savePoint);
		while(stok != NULL){
			if(strcmp(args->key,stok)==0){
				sprintf(output,"%s:%d:%s",args->FileName,lineNum,linecopy);
				sem_wait(&empty);
				sem_wait(&mutex);
				List_add(args->list,output);
				sem_post(&mutex);
				sem_post(&full);
				break;
			}else{
				stok = strtok_r(NULL," .,!@#$%^&*-_+`~\n\t\r\"()[]{};:",&savePoint);
			}
		}
		free(linecopy);
	}
	fclose(fp);
	free(line);
	free(args);
	pthread_exit(0);
}


void searchDir(char *dirName,char *key,List *list){
	DIR *d;
	Arguments *args;
	struct dirent *dir;
	pthread_t threads[100];
	int numThreads= 0;
	pthread_attr_t attr;
	pthread_attr_init(&attr);

	if((d = opendir(dirName)) == NULL){
		fprintf(stderr,"Error opening directory: %s\n",dirName);
		return;
	}
	while((dir = readdir(d)) != NULL){
			char path[1024];
			int len = snprintf(path, sizeof(path)-1,"%s/%s",dirName,dir->d_name);
			path[len] = 0;

		if(dir->d_type == DT_DIR){
			if(strcmp(".",dir->d_name) == 0 || strcmp("..",dir->d_name) == 0)
				continue;
		}else{
			args = (Arguments *) malloc(sizeof(Arguments));
			strcpy(args->path,path);
			strcpy(args->FileName,dir->d_name);
			strcpy(args->key,key);
			args->list = list;
			pthread_attr_init(&attr);
			pthread_create(&threads[numThreads],&attr,searchFile,args);
			numThreads++;
		}
	}		
			args = (Arguments *)malloc(sizeof(Arguments));
			args->list = list;
			args->num = numThreads;
			pthread_t pid;
			pthread_create(&pid,NULL,printOut,args);
			int j;
			for(j=0;j<numThreads;j++){
				pthread_join(threads[j], NULL);
			}
			while(List_getSize(list) >0){

			}
			//pthread_join(pid,NULL);
	closedir(d);
}


void readCmdFile(char* fname,int buffersize){
	FILE *fp;
	char *command = NULL;
	char *key = NULL;
	int pid;
	fp = fopen(fname,"r");
	if(fp == NULL){
		fprintf(stderr,"Failed to open file %s\n",fname);
		fclose(fp);
	}
	while(fscanf(fp,"%ms %ms",&command,&key) == 2){
		pid = fork();
		if(pid<0){
			fprintf(stderr,"Error creating child process");
			exit(1);
		}else if(pid == 0){
			List *list = List_create();
			sem_init(&empty, 0 , buffersize);
			sem_init(&full, 0,0);
			sem_init(&mutex,0 ,1);
			searchDir(command,key,list);	
			return;
		}else{
			wait(NULL);
			free(command);
			free(key);
		}
	}
	fclose(fp);
}

int main(int argc, char* argv[]){
	char fname[2056];
	int buff = atoi(argv[2]);
	strcpy(fname, argv[1]);

	readCmdFile(fname,buff);
	return 0;
}

