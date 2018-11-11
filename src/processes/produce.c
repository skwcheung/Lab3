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

int check_sqrt(int value){
	double result;
	result = sqrt(value);
	if(result == value)
		return value;
	else	
		return 0;
}

/* Function to read messages from queue */
int recieve_from_queue(mqd_t mq,char* qname){
	mq  = mq_open(qname, O_RDONLY);
	if (mq == -1 ) {
		perror("mq_open()");
		exit(1);
	}

	int value;
	/* only block for a limited time if the queue is empty */
	if (mq_receive(mq, (char *) &value, sizeof(int), 0) == -1) {
		perror("\nmq_receive() failed");

		return -1;
	} else {
		printf("Recieved value %d",value);
		return value;
	}
}

/* Function to write message to pipe */
int push_to_queue(int value, mqd_t mq){
	if(mq_send(mq, (char *) &value, sizeof(value),0) == -1){
		perror("Error: Send message failed");
	}
	printf("Produced value %d to queue",value);
}

/* Function producers will call to determine values to push*/
int produce_values(int id,int num_producers,int size, mqd_t mq,char* qname){
	/* Open message queue at beginning of produce_values */
	mq  = mq_open(qname, O_WRONLY);
	int itr;
	if (mq == -1 ) {
		perror("mq_open() failed");
		exit(1);
	}

	for(itr=0;itr < size;itr++){
		if(itr%num_producers==id){
			push_to_queue(itr,mq);
		}
	}

	/* cleanup */
    if(mq_close(mq) == -1){
	exit(1);
	printf("Error: Could not close queue");
    }	
}

int consume_values(int id,mqd_t mq, char* qname){
	int value;
	value = recieve_from_queue(mq,qname);

	int root = check_sqrt(value);
	if(root){
		printf("&d &d &d",id,value,root);
	}

	if (mq_close(mq) == -1) {
		perror("mq_close() failed");
		exit(2);
	}

}

/* Loop through and fork num_p times to create num_p producer processes */
int create_producers(int num_of_producers,int num,mqd_t mq,char* qname){
	int id;
	for(int producer = 0;producer < num_of_producers;producer++){
		id = producer;
		pid_t pid;

		pid = fork();
		if(pid < 0){
			perror("fork failed");
		}
		else if(pid == 0){
			/* Child process */
			produce_values(id,num_of_producers,num,mq,qname);
		}
		else{
			/* Parent process */
		}
	}
}

int create_consumers(int num_of_consumers, mqd_t mq, char* qname){
	int id;
	for(int consumer = 0;consumer < num_of_consumers;consumer++){
		id = consumer;
		pid_t pid;

		pid = fork();
		if(pid < 0){
			perror("fork failed");
		}
		else if(pid == 0){
			/* Child process */
			consume_values(id,mq,qname);
		}
		else{
			/* Parent process */
		}
	}
}

int main(int argc, char *argv[]){
	int num;
	int maxmsg;
	int num_p;
	int num_c;
	int i;
	struct timeval tv;
	char *qname = "/mailbox1_skwcheun";
	mqd_t mq;
	mode_t mode = S_IRUSR | S_IWUSR;
    struct mq_attr attr;
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

	gettimeofday(&tv, NULL);
	g_time[0] = (tv.tv_sec) + tv.tv_usec/1000000.;

	/* create the message queue */
	printf("Attempting mq_open first time");
    mq = mq_open(qname, O_RDWR | O_CREAT , mode, &attr);
	if (mq == -1 ) {
		perror("mq_open() failed");
		exit(1);
	}

	create_producers(num_p,num,mq,qname);
	create_consumers(num_c,mq,qname);

    gettimeofday(&tv, NULL);
    g_time[1] = (tv.tv_sec) + tv.tv_usec/1000000.;

    printf("System execution time: %.6lf seconds\n", \
            g_time[1] - g_time[0]);
	exit(0);
}

