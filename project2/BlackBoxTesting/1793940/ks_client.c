/*
 * =====================================================================================
 *
 *       Filename:  ks_client.c
 *
 *    Description:  
 *
 *        Version:  1.0
 *        Created:  02/17/2015 10:08:55 PM
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
#include <sys/msg.h>
#include <sys/ipc.h>
#include <sys/types.h>
#include <string.h>
#include <unistd.h>

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
typedef struct message_c CMessage;
int main(int argc, char *argv[]){
	key_t key;
	key_t ckey;
	int msgflg = 0666;
	int msgid;
	int cmsgid;
	Message mess;
	CMessage cmess;


	if(argc != 3){
		fprintf(stderr,"Usage ks_server 'key' 'dir'");
		exit(1);
	}

	if((key = ftok("ks_server.c",1)) == -1){
		fprintf(stderr,"ftok failed");
		exit(1);
	}
	if((mess.data.cid =ckey = ftok("ks_client.c",getpid())) == -1){
		fprintf(stderr,"client ftok failed");
	}
	if((msgid = msgget(key,msgflg)) == -1){
		fprintf(stderr,"msget failed");
		exit(1);
	}
	mess.type = 1;
	strcpy(mess.data.key, argv[1]);
	strcpy(mess.data.dir, argv[2]);

	if(msgsnd(msgid,&mess,sizeof(mess)-sizeof(long),0) == -1){
		fprintf(stderr,"failed to send message to message queue");
		exit(1);
	}
	if(strcmp("exit",argv[1])!=0){
		while(1){
		if((cmsgid = msgget(ckey,IPC_CREAT | 0666)) == -1){
			fprintf(stderr,"client msgget failed");
			exit(1);
		}
			if(msgrcv(cmsgid,&cmess,sizeof(cmess)-sizeof(long),0,0)==-1){
				fprintf(stderr,"failed to get message");
				exit(1);
			}
			if(strcmp("done",cmess.msg)==0)
				break;
			printf("%s",cmess.msg);
		}
		if(msgctl(cmsgid,IPC_RMID,NULL)==-1){
			fprintf(stderr,"failed to clear message queue");
			exit(1);
		}
	}
	return 0;
}
