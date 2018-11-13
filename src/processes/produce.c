// Use this to see if a number has an integer square root
#define EPS 1.E-7

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <mqueue.h>
#include <sys/stat.h>
#include <time.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/wait.h>
#include <math.h>
#include <errno.h>

double g_time[2];
int MAX_SIZE = 10;

/* Function to check if parameter has integer square root */
int check_sqrt(int value)
{
	double result = sqrt(value);
	if ((int)result == result)
		return result;
	else
		return 0;
}

/* Function to read messages from queue */
int recieve_from_queue(mqd_t mq)
{
	int value;
	/* only block for a limited time if the queue is empty */
	if (mq_receive(mq, (char *)&value, sizeof(int), 0) == -1)
	{
		perror("\nmq_receive() failed");
		return -1;
	}
	else
	{
		return value;
	}
}

/* Function producers will call to determine values to push */
int produce_values(int id, int num_producers, int size, char *qname, struct mq_attr attr)
{
	/* Open message queue at beginning of produce_values */
	mqd_t mq = mq_open(qname, O_RDWR | O_CREAT, S_IRUSR | S_IWUSR, &attr);
	if (mq == -1)
	{
		printf("Producer with id: %d failed mq_open()\n", id);
	}

	int itr = id;
	/* Each producer will produce and send values into queue until done their own set of values determined by the producer id */
	while (itr < size)
	{
		if (mq_send(mq, (char *)&itr, sizeof(int), 0) == -1)
		{
			perror("Error: Send message failed");
		}
		/* This helps satisfy the producer condition of %id */
		itr += num_producers;
	}

	/* cleanup */
	if (mq_close(mq) == -1)
	{
		exit(1);
		printf("Error: Could not close queue");
	}
}

int consume_values(int id, int num, char *qname, char *qname2, struct mq_attr attr, struct mq_attr attr2)
{
	int value;

	/* Open message queue */
	mqd_t mq = mq_open(qname, O_RDONLY | O_CREAT, S_IRUSR | S_IWUSR, &attr);
	if (mq == -1)
	{
		printf("Consumer with id: %d failed to open message queue\n", id);
	}

	/* Consume numbers until producers are done */
	while (1)
	{
		value = recieve_from_queue(mq);
		if(value == -1){
			exit(0);
		}
		int root = check_sqrt(value);
		if (root)
		{
			printf("%d %d %d \n", id, value, root);
		}
	}

	/* cleanup */
	if (mq_close(mq) == -1)
	{
		perror("Error: Could not close queue");
		exit(1);
	}
}

/* Loop through and fork num_p times to create num_p producer processes */
int create_producers(int id, int num_of_producers, int num, char *qname, struct mq_attr attr)
{
	pid_t pid;
	pid = fork();

	if (pid < 0){
		perror("fork failed");
	}
	/* These are child processes that will run until completion and exit 0 to let parent know they are done */
	else if (pid == 0)
	{
		produce_values(id, num_of_producers, num, qname, attr);
		exit(0);
	}
	return pid;
}

int create_consumers(int id,int num_of_consumers, int num, char *qname, char *qname2, struct mq_attr attr, struct mq_attr attr2)
{
	pid_t pid;
	pid = fork();
	if (pid < 0){
		perror("fork failed");
	}
	/* These are child processes that will run until completion and exit 0 to let parent know they are done */
	else if (pid == 0){
		consume_values(id, num, qname, qname2, attr, attr2);
		exit(0);
	}
	return pid;
}

int main(int argc, char *argv[])
{
	int num;
	int maxmsg;
	int num_p;
	int num_c;
	int nums_produced = 0;
	int id;
	struct timeval tv;
	// TODO: make unique names
	char *qname = "/message_skwcheun";
	char *qname2 = "/produced_skwcheun";
	struct mq_attr attr;
	struct mq_attr attr2;
	char buffer[MAX_SIZE + 1];

	if (argc != 5)
	{
		printf("Usage: %s <N> <B> <P> <C>\n", argv[0]);
		exit(1);
	}

	num = atoi(argv[1]);	/* number of items to produce */
	maxmsg = atoi(argv[2]); /* buffer size                */
	num_p = atoi(argv[3]);  /* number of producers        */
	num_c = atoi(argv[4]);  /* number of consumers        */

	int values[num];

	gettimeofday(&tv, NULL);
	g_time[0] = (tv.tv_sec) + tv.tv_usec / 1000000.;

	/* initialize the queue attributes */
	attr.mq_flags = 0;
	attr.mq_maxmsg = maxmsg;
	attr.mq_msgsize = sizeof(int);

	mqd_t mq = mq_open(qname, O_RDWR | O_CREAT, S_IRUSR | S_IWUSR, &attr);

	int producers_pid[num_p]; 
	int consumers_pid[num_c];

	for(id = 0;id < num_p; id++){
		producers_pid[id] = create_producers(id,num_p, num, qname, attr);
	}

	for(id = 0; id < num_c; id++){
		consumers_pid[id] = create_consumers(id,num_c, num, qname, qname2, attr, attr2);
	}	
	
	
	/* WAIT FOR ALL PRODUCERS TO COMPLETE */
	for(id = 0;id < num_p; id++){
		wait(producers_pid[id]);
		// printf("Producer %d finished \n",id);
	}

	int kill_sig = -1;
	for(id = 0; id < num_c; ++id){
		if (mq_send(mq, (char *)&kill_sig, sizeof(int), 0) == -1){
			perror("Error with sending kill:");
		}
	}

	for(id = 0; id < num_c; id++){
		wait(consumers_pid[id]);
		// printf("Consumer %d finished \n",id);
	}
	// NOW WE KNOW THERE ARE NO MORE ITEMS TO BE PRODUCERS 

	// TODO close pq and mq before unlinking them
	if (mq_close(mq) == -1)
	{
		perror("Error: Could not close queue");
		exit(1);
	}

	if (mq_unlink(qname))
	{
		printf("mq_unlink() for qname failed\n");
	}

	gettimeofday(&tv, NULL);
	g_time[1] = (tv.tv_sec) + tv.tv_usec / 1000000.;

	printf("System execution time: %.6lf seconds\n",
		   g_time[1] - g_time[0]);

	exit(0);
}
