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
#include <unistd.h>
#include <random>
#include <time.h>
typedef struct
{
	char name[10];
	std::queue<float> prices;
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
		int init=0;
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

void log(char str[])
{
	char buff[100];
	struct timespec start;
	
    	if( clock_gettime( CLOCK_REALTIME, &start) == -1 )
    	{
      		perror( "clock gettime" );
      		exit( EXIT_FAILURE );
    	}
	time_t t = time(NULL);
  	struct tm tm = *localtime(&t);
  	fprintf(stderr,"[%02d-%02d-%02d %02d:%02d:%02d.%ld] %s", tm.tm_mon + 1,tm.tm_mday,tm.tm_year + 1900 ,tm.tm_hour, tm.tm_min, tm.tm_sec,start.tv_nsec/100000,str);
	std::ofstream outfile;
	outfile.open("log.txt", std::ios_base::app); // append instead of overwrite
    outfile << buff;
	outfile.close();
	return;
}
int main(int argc, char** argv)
{
	int mutex,can_produce,can_consume;
	key_t s_key;
	int k=1;
	/******************************get values, init buffer************************************/
	float mean=atof(argv[2]);
	float stddev=atof(argv[3]);
	int stime=atoi(argv[4]);
	int buffsize=atoi(argv[5]);
	char comname[10];
	char message[100];
	std::default_random_engine generator;
  	std::normal_distribution<float> distribution(mean,stddev);
	/******************************get semaphores************************************/
	union semun  
	{
		int val;
		struct semid_ds *buf;
		ushort array [1];
	}sem_attr;
	/******************************get mutex************************************/
	if ((s_key = ftok (".", k+7)) == -1)
	{
        perror ("ftok");
		exit (1);
    }
	if ((mutex = semget(s_key, 1, 0666)) == -1) 
    {
		perror("semget mutex");
		exit(1);
    }
	/******************************get can_produce************************************/
	if ((s_key = ftok (".", k+14)) == -1) {
        perror ("ftok");
		exit (1);
    }
	if ((can_produce = semget(s_key, 1, 0666)) == -1) 
	{
		perror("semget can_produce");
		exit(1);
	}
	/******************************get can_consume************************************/
	if ((s_key = ftok (".", k+21)) == -1) {
        perror ("ftok");
		exit (1);
    }
	if ((can_consume = semget(s_key, 1, 0666)) == -1) 
	{
		perror("semget can_consume");
		exit(1);
	}
	/******************************start code************************************/
	struct sembuf semaphore_state;
	semaphore_state.sem_num = 0;
	semaphore_state.sem_op = 0;
	semaphore_state.sem_flg = 0;
	while(1)
	{
			float ran_price=distribution(generator);
			snprintf(message, sizeof(message),"%s: generating new value %.02f \n", argv[1],ran_price);
			log(message);
			strcpy(comname,argv[1]);
			produced pr;
			strcpy(pr.name,comname);
			pr.price=ran_price;
			//first wait on can_produce semaphore
			semaphore_state.sem_op=-1;
			if (semop (can_produce, &semaphore_state, 1) == -1)
			{
				perror ("semop: can_produce");
				exit (1);
			}
			//wait on mutex
			snprintf(message, sizeof(message),"%s: trying to get mutex on shared buffer\n", argv[1]);
			log(message);
			semaphore_state.sem_op=-1;
			if (semop (mutex, &semaphore_state, 1) == -1)
			{
				perror ("semop: mutex");
				exit (1);
			}
			//produce()
			if ((s_key = ftok (".", k)) == -1)
			{
				perror ("ftok");
				exit (1);
			}
			int shmid=shmget(s_key,1000, IPC_CREAT | 0666);
			if (shmid < 0)
			{
				perror("couldn't create shared memory prod");
				exit(1);
			}
			void* data=shmat(shmid,(void*)0,0);
			buff* b= new(data) buff(buffsize);  
			if ( b->init != 1 )
			{
				
				printf("buffer was initiallized twice\n");
				exit(1);
			}
			snprintf(message, sizeof(message),"%s: placing %.02f on shared buffer \n",argv[1],ran_price);
			log(message);
			b->push(pr);
			shmdt(b);
			//critical section ended
			//signal the mutex
			semaphore_state.sem_op=1;
			if (semop (mutex, &semaphore_state, 1) == -1)
			{
				perror ("semop: inc mutex");
				exit (1);
			}
			//signal can_cosume
			semaphore_state.sem_op=1;
			if (semop (can_consume, &semaphore_state, 1) == -1)
			{
				perror ("semop: inc can_consume");
				exit (1);
			}
			snprintf(message, sizeof(message),"%s: sleeping for %d \n",argv[1],stime);
			log(message);	
			sleep((stime*1.0)/1000);
	}
	return 0;
}
