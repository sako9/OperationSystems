/*
 * =====================================================================================
 *
 *       Filename:  uoflinsort.c
 *
 *    Description:  First 420 project
 *
 *        Version:  1.0
 *        Created:  01/22/2015 08:49:16 AM
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

struct Student {
	int StudentId;
	char *FirstName;
	char *LastName;
	char *Department;
	float GPA;
};				/* ----------  end of struct Student  ---------- */

typedef struct Student Student;


struct Node {
	struct Node *next;
	//struct Node *prev;
	struct Student *stu;
};				/* ----------  end of struct Node  ---------- */

typedef struct Node Node;

/* 
 * ===  FUNCTION  ======================================================================
 *         Name:  readFile
 *  Description:  reads the students form a file
 * =====================================================================================
 */

Node *readFile(char *fname)
{
	FILE *fp;
	Node *head= NULL;
	Node *n = NULL;
	int id;
	char *Fname;
	char *Lname;
	char *depart;
	float gpa;
	
	fp = fopen(fname,"r");
	if(fp == NULL){
		fprintf(stderr,"Failed to open file %s",fname);
		//add some sort of exit function that frees all the momory we've used so far
		exit(1);
	}
	  while(fscanf(fp,"%d %ms %ms %ms %f",&id,&Fname,&Lname,&depart,&gpa)==5){
		n = (Node *)malloc(sizeof(Node));
		n->stu = malloc(sizeof(Student));
		n->stu->StudentId = id;
		n->stu->FirstName = strdup(Fname);
		n->stu->LastName = strdup(Lname);
		n->stu->Department = strdup(depart);
		n->stu->GPA = gpa;
		n->next = head;
		head = n;
		free(Fname);
		free(Lname);
		free(depart);
	}
	fclose(fp);
	return head;
}
/* -----  end of function readFile  ----- */


void writeFile(Node *list, char *outName){

	FILE *fp = NULL;
	fp = fopen(outName, "w");
	if(fp == NULL){
		fprintf(stderr,"Failed to open output  file");
		exit(1);
	}
	while(list){
		fprintf(fp,"%d %s %s %s %f\n",list->stu->StudentId,list->stu->FirstName,list->stu->LastName,list->stu->Department, list->stu->GPA);
		list = list->next;
	}
	fclose(fp);
}


/* 
 * ===  FUNCTION  ======================================================================
 *         Name:  swap
 *  Description:  switch two nodes
 * =====================================================================================
 */
void swap (Node *a, Node *b )
{
	Student *tempn = NULL;
	if(a && b){
		 tempn = a->stu;
		 a->stu = b->stu;
		 b->stu = tempn;
	}
}		/* -----  end of function swap  ----- */

/* 
 * ===  FUNCTION  ======================================================================
 *         Name:  insertSort(Node list)
 *  Description:implementation of the insertion sort algorithm   
 * =====================================================================================
 */
Node *insertSort(Node *list)
{
	if(!list || !list->next){
		return list;
	}
	Node *sorted = NULL;
	while(list != NULL){
		Node *current = list;
		list = list->next;
		if(sorted == NULL || current->stu->StudentId < sorted->stu->StudentId){
			current->next = sorted;
			sorted = current;
		}else{
			Node *p = sorted;
			while(p){
				if(p->next ==NULL || current->stu->StudentId < p->next->stu->StudentId){
					current->next = p->next;
					p->next = current;
					break;
				}
				p = p->next;
			}
		}
	}
		
	return sorted;
}		/* -----  end of function insertSort(Node list)  ----- */

void freeList(Node *list)
{
	Node *temp = NULL;
	while(list){
		temp = list->next;
		free(list->stu->FirstName);
		free(list->stu->LastName);
		free(list->stu->Department);
		free(list->stu);
		free(list);
		list = temp;
	}
}

void printList(Node *list)
{
	while(list){
		printf("%d %s %s %s %f\n",list->stu->StudentId,list->stu->FirstName,list->stu->LastName,list->stu->Department,list->stu->GPA);
		list = list->next;
	}
}

int main(int argc, char *argv[]){
	Node *studentList = NULL;
	Node *sorted = NULL;
	if(argc != 3){
		fprintf(stderr,"uasage ./uoflinsort in.txt out.txt");
		exit(1);
	}
	studentList =readFile(argv[1]);
	printList(studentList);
	studentList = insertSort(studentList);
	printf("\n");
	printList(studentList);
	writeFile(studentList, argv[2]);
	freeList(studentList);

}


