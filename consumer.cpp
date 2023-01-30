#include <stdio.h>
#include <stdlib.h>
#include <iostream>
#include <sys/shm.h>
#include <string.h>
#include <queue>
#include <cstdlib>
#include <algorithm>
#include <bits/stdc++.h>
#include <math.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/sem.h>


typedef struct
{
	char name[10];
	float prices[4];
	float prev_avg;
	int count;
	float prev_price;
	int priceflg;
	int avgflg;
	float cur_price;
	float cur_avg;
}comm;

typedef struct
{
	char name[10];
	float price;
}produced;

class buff
{
	public:
		produced buffer[50];
		int in;
		int out;
		int count;
		int size;
		int init;
	public:
		buff(int size);
		void push(produced pr);
		produced pop();
		//produced top();
};

buff::buff(int sizee)
{
	size=sizee;
	if(init !=1)
	{
		out=0;
		in=0;
		count=0;
		init=1;

	}
}
void buff::push(produced pr)
{
	buffer[in]=pr;
	count++;
	in=(in+1)%size;
}
produced buff::pop()
{
	if(count!=0)
	{
		produced pr=buffer[out];
		out=(out+1)%(size);
		count--;
		return pr;
	}
	exit(0);
}





comm* comm_init(char name[])
{
	comm* c=(comm *)malloc(sizeof(comm));
	strcpy(c->name,name);
	c->count=-1;
	c->prev_avg=0.0;
	c->prev_price=0.0;
	c->cur_price=0.0;
	c->cur_avg=0.0;
	c->priceflg=c->avgflg=0;
	return c;
}

void consume(comm* c,float cur_price)
{
	if (c->prev_price > cur_price)
	{
		c->priceflg=-1;
	}
	else if(c->prev_price < cur_price)
	{
		c->priceflg=1;
	}
	else
	{
		c->priceflg=0;
	}
	//get current average
	int i=c->count;
	float totalsum=0.0;
	float cur;
	for(int i=0; i<=c->count && i<4;i++)
	{
		totalsum+=c->prices[i];
	}
	totalsum+=cur_price;
	float cur_avg;
	if (c->count <= 3)
	{
		cur_avg=totalsum/(c->count+2);
	}
	else
	{
		cur_avg=totalsum/5;
	}
	if (c->prev_avg > cur_avg)
	{
		c->avgflg=-1;
	}
	else if(c->prev_avg < cur_avg)
	{
		c->avgflg=1;
	}
	else
	{
		c->avgflg=0;
	}
	c->prev_avg=cur_avg;
	//c->cur_avg=cur_avg;
	c->prev_price=cur_price;
	//c->cur_price=cur_price;
	c->count++;
	c->prices[(c->count)%4]=cur_price;
}

void print_output(comm* comms[])
{
	char b[20];
	printf("\e[1;1H\e[2J");
	printf("+-------------------------------------+\n");
	printf("| Currency      |  Price  |  AvgPrice |\n");
	printf("+-------------------------------------+\n");
	for( int i=0;i<11;i++)
	{
		printf("| ");
		printf("%-14s|",comms[i]->name);
		if(comms[i]->priceflg==1)
		{
			snprintf(b, sizeof(b),"%.02f↑", comms[i]->prev_price);
			printf("\033[1;32m%11s\033[0m|",b);
		}
		if(comms[i]->priceflg==0)
		{
			snprintf(b, sizeof(b),"%.02f", comms[i]->prev_price);
			printf("\033[1;34m%9s\033[0m|",b);
		}
		if(comms[i]->priceflg==-1)
		{
			snprintf(b,  sizeof(b),"%.02f↓", comms[i]->prev_price);
			printf("\033[1;31m%11s\033[0m|",b);
		}
		
		if(comms[i]->avgflg==1)
		{
			snprintf(b,  sizeof(b),"%.02f↑", comms[i]->prev_avg);
			printf("\033[1;32m%13s\033[0m|",b);
		}
		if(comms[i]->avgflg==0)
		{
			snprintf(b,  sizeof(b),"%.02f", comms[i]->prev_avg);
			printf("\033[1;34m%11s\033[0m|",b);
		}
		if(comms[i]->avgflg==-1)
		{
			snprintf(b,  sizeof(b),"%.02f↓", comms[i]->prev_avg);
			printf("\033[1;31m%13s\033[0m|",b);
		}
		printf("\n");	
	}
	printf("+-------------------------------------+\n");
}


int main(int argc, char** argv)
{
	int mutex,can_produce,can_consume;
	int buffsize=atoi(argv[1]);
	key_t s_key;
	int k=1;
	/******************************init commoditites************************************/
	comm* GOLD=comm_init("GOLD");
	comm* SILVER=comm_init("SILVER");
	comm* CRUDEOIL=comm_init("CRUDEOIL");
	comm* NATURALGAS=comm_init("NATURALGAS");
	comm* ALUMINIUM=comm_init("ALUMINIUM");
	comm* COPPER=comm_init("COPPER");
	comm* NICKEL=comm_init("NICKEL");
	comm* LEAD=comm_init("LEAD");
	comm* ZINC=comm_init("ZINC");
	comm* MENTHAOIL=comm_init("MENTHAOIL");
	comm* COTTON=comm_init("COTTON");
	comm* comarr[]={ALUMINIUM,COPPER,COTTON,CRUDEOIL,GOLD,LEAD,MENTHAOIL,NATURALGAS,NICKEL,SILVER,ZINC};
	/******************************get shared memory************************************/
	/******************************create semaphores************************************/
	union semun  
	{
		int val;
		struct semid_ds *buf;
		ushort array [1];
	}sem_attr;
	/******************************create mutex************************************/
	if ((s_key = ftok (".", k+7)) == -1)
	{
        perror ("ftok"); 
		exit (1);
    }
	if ((mutex = semget(s_key, 1, IPC_CREAT | IPC_EXCL | 0666)) == -1) 
	{
		perror("semget mutex at con");
		exit(1);
	}
	else
	{
		sem_attr.val = 1;        
		if (semctl (mutex, 0, SETVAL, sem_attr) == -1) 
		{
			perror ("semctl SETVAL");
			exit (1);
		}
	}
	/******************************create can_produce************************************/
	if ((s_key = ftok (".", k+14)) == -1) {
        perror ("ftok");
		exit (1);
    }
	if ((can_produce = semget(s_key, 1, IPC_CREAT | IPC_EXCL | 0666)) == -1) 
	{
		perror("semget can_produce");
		exit(1);
	}
	else
	{
		sem_attr.val = buffsize;   
		if (semctl (can_produce, 0, SETVAL, sem_attr) == -1) 
		{
			perror ("semctl SETVAL");
			exit (1);
		}
	}
	/******************************create can_consume************************************/
	if ((s_key = ftok (".", k+21)) == -1) {
        perror ("ftok");
		exit (1);
    }
	if ((can_consume = semget(s_key, 1, IPC_CREAT | IPC_EXCL | 0666)) == -1) 
	{
		perror("semget can_consume");
		exit(1);
	}
	else
	{
		sem_attr.val = 0;  
		if (semctl (can_consume, 0, SETVAL, sem_attr) == -1) 
		{
			perror ("semctl SETVAL");
			exit (1);
		}
	}
	struct sembuf semaphore_state[1];
	semaphore_state [0].sem_num = 0;
	semaphore_state [0].sem_op = 0;
	semaphore_state [0].sem_flg = 0;
	while(1)
	{
		//first wait on can_consume
		semaphore_state[0].sem_op=-1;
		if (semop (can_consume, semaphore_state, 1) == -1)
		{
			perror ("semop: wait can_consume con");
			exit (1);
		}
		//wait on mutex
		semaphore_state[0].sem_op=-1;
		if (semop (mutex, semaphore_state, 1) == -1)
		{
			perror ("semop: wait mutex con");
			exit (1);
		}
		//consume()
		if ((s_key = ftok (".", k)) == -1)
		{
			perror ("ftok");
			exit (1);
		}
		int shmid=shmget(s_key,1000,0666);
		if (shmid < 0)
		{
			perror ("shared memory cannot be created con");
			exit (1);
		}
		void* data=shmat(shmid,(void*)0,0);
		buff* b= new(data) buff(buffsize);  
		if(b->init !=1)
		{
			printf("Buffer was not initialized\n");
			exit(1);
		}
		produced pr;
		pr.price=b->buffer[b->out].price;
		strcpy(pr.name,b->buffer[b->out].name);
		b->out=(b->out+1)%(b->size);
		b->count--;
		for(int i=0; i<11 ; i++)
		{
			if(!strcmp(comarr[i]->name,pr.name))
			{
				comm *c = comarr[i];
				float ppp = pr.price;
				consume(c,ppp);
				break;
			}
		}
		print_output(comarr);
		shmdt(b);
		//signal the mutex
		semaphore_state[0].sem_op=1;
		if (semop (mutex, semaphore_state, 1) == -1)
		{
			perror ("semop: inc mutex con");
			exit (1);
		}
		//signal can_produce
		semaphore_state[0].sem_op=1;
		if (semop (can_produce, semaphore_state, 1) == -1)
		{
			perror ("semop: inc can_produce con");
			exit (1);
		}
	}	
	return 0;
}
