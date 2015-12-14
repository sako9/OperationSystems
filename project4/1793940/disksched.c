/*
 * =====================================================================================
 *
 *       Filename:  disksched.c
 *
 *    Description:  
 *
 *        Version:  1.0
 *        Created:  04/13/2015 12:13:07 PM
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  Samuel Nwosu (), 
 *   Organization:  
 *
 * =====================================================================================
 */
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <math.h>

#define SECTORSPERTRACK 200
#define TRACKSPERCYLINDER 8
#define CYLINDERS 500000
#define RPM 10000
#define PHYSICALSECTORSIZE 512 //(Bytes)
#define LOGICALBLOCKSIZE 4096 //(Bytes)
#define TRACKTOTRACKSEEK 2 //(milliseconds)
#define FULLSEEK 16 //(milliseconds)
#define TRANSFERRATE 1 //(Gb/s)

struct Request{
	double arrivalTime;
	double finishTime;
	double waitingTime;
	int LBN;
	int size;
	int cylinder;
	int PSN;
	int surface;
	double sectorOffset;
	int updatedSectorOffset;
	int seekDistance;
	int processed;
};
typedef struct Request Request;

struct Queue{
	struct Queue *next;
	struct Request *request;
};
typedef struct Queue Queue;

Queue *insertSort(Queue *list)
{
	if(!list || !list->next){
		return list;
	}
	Queue *sorted = NULL;
	while(list != NULL){
		Queue *current = list;
		list = list->next;
		if(sorted == NULL || current->request->cylinder < sorted->request->cylinder){
			current->next = sorted;
			sorted = current;
		}else{
			Queue *p = sorted;
			while(p){
				if(p->next ==NULL || current->request->cylinder < p->next->request->cylinder){
					current->next = p->next;
					p->next = current;
					break;
				}
				p = p->next;
			}
		}
	}
		
	return sorted;
}

void freeList(Queue *list)
{
	Queue *temp = NULL;
	while(list){
		temp = list->next;
		free(list->request);
		free(list);
		list = temp;
	}
}

double TransferTime(int requestSize){
	return (double)((requestSize*PHYSICALSECTORSIZE*8)/pow(2,30)*1000);//ms
}

double RotationalLatency(double updatedCurrentOffset,int updatedSectorOffset){
	if(updatedCurrentOffset > updatedSectorOffset){
		return (double)(SECTORSPERTRACK -updatedCurrentOffset + updatedSectorOffset)/((RPM * SECTORSPERTRACK)/(60.0 *1000.0));
	}else{
		return (double)(updatedSectorOffset - updatedCurrentOffset)/((RPM * SECTORSPERTRACK)/(60.0 *1000.0));
	}
}

int PSN(int LBN,int requestSize){
	return LBN*8 + requestSize;
}

int cylinder(int PSN){
	return (PSN / SECTORSPERTRACK) / 8;
}

int surface(int PSN){
	return (PSN/SECTORSPERTRACK)%8;
}

double sectorOffset(int PSN){
	return fmod(PSN, 200);
}

int seekDistance(int prev, int cylinder){
	if(cylinder < prev){
		return (CYLINDERS - 1 - prev) + (CYLINDERS - 1 - cylinder);
	}
	return cylinder - prev;
}

int seekDistanceLook(int prev, int cylinder){
	if(cylinder - prev < 0){
		return  prev - cylinder;
	}
	return cylinder -prev;
}

double updatedOffset(double seektime, int currentOffset){
	double a = currentOffset + (seektime * RPM * SECTORSPERTRACK)/(60 * 1000);
	double b = (double) SECTORSPERTRACK;
	return fmod(a,b);
}

double seekTime(int dis){
	if(dis == 0){
		return 0;
	}else{
		return ((double)(FULLSEEK - TRACKTOTRACKSEEK)/CYLINDERS) * (double)dis + TRACKTOTRACKSEEK;
	}
}

double finishTime(double arrivalTime, double serviceTime){
	return arrivalTime + serviceTime;
}

double serviceTime(double seekTime,double rotLat, double transferTime){
	return (seekTime + rotLat + transferTime)/1000;
}

void C_LOOK(Queue* work,int limit,char *out){
	Queue* iter = NULL;
	Queue* siter = NULL;
	FILE *fp;
	fp = fopen(out,"w");
	if(fp == NULL){
		fprintf(stderr,"Failed to open file %s",out);
		exit(1);
	}
	iter = work;
	int prevCylinder = 0;
	double sectorOffset =0.0;
	double wait = 0.0;
	int seekDis = iter->request->cylinder;
	double seektime = seekTime(seekDis);
	double updatedCurrentOffset = updatedOffset(seektime, prevCylinder);
	double rotation = RotationalLatency(updatedCurrentOffset, iter->request->updatedSectorOffset);
	double transfer = TransferTime(iter->request->size);
	double servicetime = serviceTime(seektime, rotation,transfer);
	double finish = finishTime(iter->request->arrivalTime, servicetime);
	iter->request->seekDistance =seekDis;
	iter->request->finishTime = finish;
	iter->request->waitingTime = 0;
	iter->request->processed = 1;
	fprintf(fp,"%lf %lf %lf %d %d %d %lf %d\n",iter->request->arrivalTime, iter->request->finishTime, iter->request->waitingTime, iter->request->PSN, iter->request->cylinder, iter->request->surface, iter->request->sectorOffset, iter->request->seekDistance);
	prevCylinder = iter->request->cylinder;
	sectorOffset = iter->request->sectorOffset;
	iter = insertSort(iter);
	siter = iter;
	int reset = 0;
	
	while(iter){
		if(iter->request->processed == 0 && iter->request->arrivalTime < finish && (iter->request->cylinder >= prevCylinder ||reset == 1)){
			seekDis = seekDistanceLook(prevCylinder,iter->request->cylinder);
			seektime = seekTime(seekDis);
			updatedCurrentOffset = updatedOffset(seektime, sectorOffset);
			rotation = RotationalLatency(updatedCurrentOffset, iter->request->updatedSectorOffset);
			transfer = TransferTime(iter->request->size);
			servicetime = serviceTime(seektime, rotation,transfer);
			wait = finish - iter->request->arrivalTime;
			iter->request->waitingTime = wait;
			finish = finishTime(iter->request->arrivalTime, servicetime) + wait;
			iter->request->seekDistance =seekDis;
			iter->request->finishTime = finish;
			iter->request->processed = 1;
			prevCylinder = iter->request->cylinder;
			sectorOffset = iter->request->sectorOffset;
			fprintf(fp,"%lf %lf %lf %d %d %d %lf %d\n",iter->request->arrivalTime, iter->request->finishTime, iter->request->waitingTime, iter->request->PSN, iter->request->cylinder, iter->request->surface, iter->request->sectorOffset, iter->request->seekDistance);
	
			if(!iter->next){
				reset = 1;
			}
			iter = siter;
			continue;
		}
		iter = iter->next;
	}
	fclose(fp);
}

void SCAN(Queue* work, int limit,char* out){
	Queue* iter = NULL;
	Queue* siter = NULL;
	FILE *fp;
	fp = fopen(out,"w");
	if(fp == NULL){
		fprintf(stderr,"Failed to open file %s",out);
		exit(1);
	}
	iter = work;
	int prevCylinder = 0;
	int sectorOffset =0 ;
	double wait = 0.0;
	int seekDis = seekDistance(prevCylinder,iter->request->cylinder);
	double seektime = seekTime(seekDis);
	double updatedCurrentOffset = updatedOffset(seektime, prevCylinder);
	double rotation = RotationalLatency(updatedCurrentOffset, iter->request->updatedSectorOffset);
	double transfer = TransferTime(iter->request->size);
	double servicetime = serviceTime(seektime, rotation,transfer);
	double finish = finishTime(iter->request->arrivalTime, servicetime);
	iter->request->seekDistance =seekDis;
	iter->request->finishTime = finish;
	iter->request->waitingTime = 0;
	iter->request->processed = 1;
	prevCylinder = iter->request->cylinder;
	sectorOffset = iter->request->sectorOffset;
	fprintf(fp,"%lf %lf %lf %d %d %d %lf %d\n",iter->request->arrivalTime, iter->request->finishTime, iter->request->waitingTime, iter->request->PSN, iter->request->cylinder, iter->request->surface, iter->request->sectorOffset, iter->request->seekDistance);
	iter = insertSort(iter);
	siter = iter;
	int reset = 0;
	while(iter){
		if(iter->request->processed == 0 && iter->request->arrivalTime < finish && (iter->request->cylinder >= prevCylinder ||reset == 1)){
			seekDis = seekDistance(prevCylinder,iter->request->cylinder);
			seektime = seekTime(seekDis);
			updatedCurrentOffset = updatedOffset(seektime, sectorOffset);
			rotation = RotationalLatency(updatedCurrentOffset, iter->request->updatedSectorOffset);
			transfer = TransferTime(iter->request->size);
			servicetime = serviceTime(seektime, rotation,transfer);
			wait = finish - iter->request->arrivalTime;
			iter->request->waitingTime = wait;
			finish = finishTime(iter->request->arrivalTime, servicetime) + wait;
			iter->request->seekDistance =seekDis;
			iter->request->finishTime = finish;
			iter->request->processed = 1;
			prevCylinder = iter->request->cylinder;
			sectorOffset = iter->request->sectorOffset;
			fprintf(fp,"%lf %lf %lf %d %d %d %lf %d\n",iter->request->arrivalTime, iter->request->finishTime, iter->request->waitingTime, iter->request->PSN, iter->request->cylinder, iter->request->surface, iter->request->sectorOffset, iter->request->seekDistance);
			if(!iter->next){
				reset = 1;
			}
			iter = siter;
			continue;
		}
		iter = iter->next;
	}
}


void readFile(char* fname,char* ofname, char* alg, int limit){
	FILE *fp;
	Queue *rear = NULL;
	Queue *front = NULL;
	Queue *temp = NULL;
	double arrivalTime;
	int LBN;
	int size;
	int count = 0;


	fp = fopen(fname,"r");
	if(fp == NULL){
		fprintf(stderr,"Failed to open file %s",fname);
		exit(1);
	}
	while(fscanf(fp,"%lf %d %d",&arrivalTime,&LBN,&size) == 3 && count != limit){
		count++;
		if(!rear){
		rear = (Queue *)malloc(sizeof(Queue));
		rear->request = (Request *)malloc(sizeof(Request));
		rear->request->arrivalTime = arrivalTime;
		rear->request->LBN = LBN;
		rear->request->size = size;
		rear->request->PSN = PSN(LBN,size);
		rear->request->cylinder = cylinder(rear->request->PSN);
		rear->request->surface = surface(rear->request->PSN);
		rear->request->sectorOffset =sectorOffset(rear->request->PSN);
		rear->request->updatedSectorOffset = rear->request->sectorOffset - size;
		rear->request->processed = 0;
		rear->next = NULL;
		front = rear;
		}else{
			temp = (Queue *)malloc(sizeof(Queue));
			temp->request = (Request *)malloc(sizeof(Request));
			temp->request->arrivalTime = arrivalTime;
			temp->request->LBN = LBN;
			temp->request->size = size;
			temp->request->PSN = PSN(LBN,size);
			temp->request->cylinder = cylinder(temp->request->PSN);
			temp->request->surface = surface(temp->request->PSN);
			temp->request->sectorOffset = sectorOffset(temp->request->PSN);
			temp->request->updatedSectorOffset = temp->request->sectorOffset - size;
			temp->request->processed = 0;
			rear->next = temp;
			temp->next = NULL;
			rear = temp;
		}
	}

	if(strcmp(alg,"SCAN")==0){
		SCAN(front,limit,ofname);
	}else if(strcmp(alg,"CLOOK")==0){
		C_LOOK(front,limit,ofname);
	}else{
		fprintf(stderr,"Invalid algorithm");
	}
	freeList(front);
	fclose(fp);
}

		/* -----  end of function insertSort(Node list)  ----- */

int main(int argc, char* argv[]){
	int limit = -1;
	if(argc != 4 && argc != 5){
		fprintf(stderr,"usage ./disksched in.txt out.txt algorithm (optional)limit");
	}
	if(argc == 5){
		limit = atoi(argv[4]);
	}
	readFile(argv[1],argv[2],argv[3],limit);
	return 0;
}
