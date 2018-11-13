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
int check_sqrt(int value){
	double result = sqrt(value);
	if((int)result == result)
		return result;
	else	
		return 0;
}

/* Function to read messages from queue */
int recieve_from_queue(mqd_t mq){
	// mqd_t mq  = mq_open(qname, O_RDONLY);
	// if (mq == -1 ) {
	// 	perror("mq_open()");
	// 	exit(1);
	// }

	int value;
	/* only block for a limited time if the queue is empty */
	if (mq_receive(mq, (char *) &value, sizeof(int), 0) == -1) {
		perror("\nmq_receive() failed");

		return -1;
	} else {
		return value;
	}
}

/* Function producers will call to determine values to push */
int produce_values(int id,int num_producers,int size,char* qname,struct mq_attr attr){
	/* Open message queue at beginning of produce_values */
	mqd_t mq = mq_open(qname, O_RDWR | O_CREAT, S_IRUSR | S_IWUSR, &attr);
	if(mq == -1){
		printf("Producer with id: %d failed mq_open()\n", id);
	}

	int itr = id;
	
	/* Each producer will produce and send values into queue until done their own set of values determined by the producer id */
	while(itr < size){
		if(mq_send(mq, (char *) &itr, sizeof(int),0) == -1){
			perror("Error: Send message failed");
		}
		itr+=num_producers;
	}

	/* cleanup */
    if(mq_close(mq) == -1){
	exit(1);
	printf("Error: Could not close queue");
    }	
}

int consume_values(int id, int num, char* qname,char* qname2,struct mq_attr attr,struct mq_attr attr2){
	int value;
	int values_consumed;

	/* Open produced queue to check if producers are done */
	mqd_t pq = mq_open(qname2, O_RDWR | O_CREAT, S_IRUSR | S_IWUSR, &attr2);

	/* Open message queue */
	mqd_t mq = mq_open(qname, O_RDONLY | O_CREAT, S_IRUSR | S_IWUSR, &attr);
			if(mq == -1){
				printf("Consumer with id: %d failed to open message queue\n",id);
			}


	int counter = 0;

	/* Consume numbers until producers are done */
	while(1){
		/* Check how many values have been consumed so far */
		if(mq_receive(pq, (char *) &values_consumed, sizeof(values_consumed), 0) == -1){
		printf("Consumer with id: %d failed to check produced queue\n", id);
		perror("Error:");
		}
		
		/* If values consumed is the same as num (values produced) then consumers know to stop */
		if(values_consumed == num){
			if(mq_send(pq, (char *) &values_consumed, sizeof(values_consumed), 0) == -1){
				printf("Consumer with id: %d failed to send update to produced queue\n", id);
				perror("Error:");
			}
			break;
		}
		value = recieve_from_queue(mq);
		int root = check_sqrt(value);
		if(root){
			printf("%d %d %d \n",id,value,root);
		}
		counter++;

		/* Update values_consumed */
		values_consumed++;
		// printf("Values consumed is %d \n", values_consumed);
		if(mq_send(pq, (char *) &values_consumed, sizeof(values_consumed), 0) == -1){
			printf("Consumer with id: %d failed to update values consumed to produced queue\n", id);
			perror("Error:");
		}
	}

	/* cleanup */
	if (mq_close(mq) == -1) {
		perror("Error: Could not close queue");
		exit(1);
	}
	if (mq_close(pq) == -1) {
		perror("Error: Could not close queue");
		exit(1);
	}
}

/* Loop through and fork num_p times to create num_p producer processes */
int create_producers(int num_of_producers,int num,char* qname,struct mq_attr attr){
	int id;
	for(int producer = 0;producer < num_of_producers;producer++){
		id = producer;
		pid_t pid;

		pid = fork();
		if(pid < 0){
			perror("fork failed");
		}
		/* These are child processes that will run until completion and exit 0 to let parent know they are done */
		else if(pid == 0){
			produce_values(id,num_of_producers,num,qname,attr);
			exit(0);
		}
	}
}

int create_consumers(int num_of_consumers, int num, char* qname,char* qname2, struct mq_attr attr,struct mq_attr attr2){
	int id;
	for(int consumer = 0;consumer < num_of_consumers;consumer++){
		id = consumer;
		pid_t pid;

		pid = fork();
		if(pid < 0){
			perror("fork failed");
		}
		/* These are child processes that will run until completion and exit 0 to let parent know they are done */
		else if(pid == 0){
			consume_values(id,num,qname,qname2,attr,attr2);
			exit(0);
		}
	}
}

int main(int argc, char *argv[]){
	int num;
	int maxmsg;
	int num_p;
	int num_c;
	int i;
	int nums_produced = 0;
	struct timeval tv;
	char *qname = "/message_queue";
	char *qname2 = "/produced_queue";
    struct mq_attr attr;
	struct mq_attr attr2;
    char buffer[MAX_SIZE + 1];


	if (argc != 5) {
		printf("Usage: %s <N> <B> <P> <C>\n", argv[0]);
		exit(1);
	}

	num = atoi(argv[1]);	/* number of items to produce */
	maxmsg = atoi(argv[2]); /* buffer size                */
	num_p = atoi(argv[3]);  /* number of producers        */
	num_c = atoi(argv[4]);  /* number of consumers        */

	int values[num];

	/* initialize the queue attributes */
    attr.mq_flags = 0;
    attr.mq_maxmsg = maxmsg;
    attr.mq_msgsize = sizeof(int);

	attr2.mq_maxmsg = 1;
	attr2.mq_msgsize = sizeof(int);
	attr2.mq_flags = 0;

	if(mq_unlink(qname)){
		printf("mq_unlink() for qname failed\n");
	}
	if(mq_unlink(qname2)){
		printf("mq_unlink() for qname2 failed\n");
	}
	
	// Start second buffer with 0 to represent produced values to let consumers know when to finish 
	mqd_t pq = mq_open(qname2, O_RDWR | O_CREAT, S_IRUSR | S_IWUSR, &attr2);
	if(pq == -1){
		printf("Produced_queue failed to open\n");
	}
	if(mq_send(pq, (char *) &nums_produced, sizeof(nums_produced), 0) == -1){
		printf("pq could not instantiate with 0\n");
	}
	if(mq_close(pq) == -1){
		printf("Failed to close produced queue \n");
	}

	gettimeofday(&tv, NULL);
	g_time[0] = (tv.tv_sec) + tv.tv_usec/1000000.;

	printf("Creating stuff \n");
	create_producers(num_p,num,qname,attr);
	create_consumers(num_c,num,qname,qname2,attr,attr2);

	while(wait(NULL)>0);

	if(mq_unlink(qname)){
		printf("mq_unlink() for qname failed\n");
	}
	if(mq_unlink(qname2)){
		printf("mq_unlink() for qname2 failed\n");
	}


    gettimeofday(&tv, NULL);
    g_time[1] = (tv.tv_sec) + tv.tv_usec/1000000.;

    printf("\n System execution time: %.6lf seconds\n", \
            g_time[1] - g_time[0]);
	exit(0);
}

