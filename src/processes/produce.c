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

double g_time[2];
int MAX_SIZE = 10;
char  *QUEUE_NAME = "/queue";

/* Function to write message to pipe */
int push_to_queue(int value, mqd_t mq){
	if(mq_send(mq, (char *) &value, sizeof(value),0) == -1){
		perror("Error: Send message failed");
	}
	printf("Produced value %d to queue",value);
}

/* Function producers will call to determine values to push*/
int produce_values(int id,int num_producers,int input_array[],int size, mqd_t mq,char* qname){
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

int main(int argc, char *argv[])
{
	int num;
	int maxmsg;
	int num_p;
	int num_c;
	int i;
	struct timeval tv;
	mqd_t mq;
	mode_t mode = S_IRUSR | S_IWUSR;
    struct mq_attr attr;
    char buffer[MAX_SIZE + 1];

    /* initialize the queue attributes */
    attr.mq_flags = 0;
    attr.mq_maxmsg = 100;
    attr.mq_msgsize = sizeof(int);
    attr.mq_curmsgs = 0;

    /* create the message queue */
    mq = mq_open(QUEUE_NAME, O_CREAT | O_RDONLY, mode, &attr);
	if (mq == -1 ) {
		perror("mq_open() failed");
		exit(1);
	}


	if (argc != 5) {
		printf("Usage: %s <N> <B> <P> <C>\n", argv[0]);
		exit(1);
	}

	num = atoi(argv[1]);	/* number of items to produce */
	maxmsg = atoi(argv[2]); /* buffer size                */
	num_p = atoi(argv[3]);  /* number of producers        */
	num_c = atoi(argv[4]);  /* number of consumers        */


	gettimeofday(&tv, NULL);
	g_time[0] = (tv.tv_sec) + tv.tv_usec/1000000.;


    gettimeofday(&tv, NULL);
    g_time[1] = (tv.tv_sec) + tv.tv_usec/1000000.;

    printf("System execution time: %.6lf seconds\n", \
            g_time[1] - g_time[0]);
	exit(0);
}
