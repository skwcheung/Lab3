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
#include <pthread.h>
#include <semaphore.h>
#include <stdbool.h>

double g_time[2];

int total_to_produce;
int total_number_of_producers;
int total_number_of_consumers;
int total_number_produced = 0;
int total_number_consumed = 0;
int number_of_active_consumers;

int first = -1;
int last = -1;

int buffer_size;
int* buffer;
int number_of_items_in_buffer = 0;

sem_t empty_count;
sem_t fill_count;

pthread_mutex_t buffer_mutex;
pthread_mutex_t consume_mutex;
pthread_mutex_t produce_mutex;

int produce_id = 0;
int consume_id = 0;


void add_to_queue(int value) {
	number_of_items_in_buffer++;
	if (first == -1) {
		buffer[0] = value;
		first = 0;
		last = 0;
	}
	else {
		if (last == buffer_size - 1) {
			last = 0;
			buffer[last] = value;
		}
		else {
			last++;
			buffer[last] = value;
		}
	}
}

int remove_from_queue() {
	number_of_items_in_buffer--;
	int item;
	item = buffer[first];
	buffer[first] = -1;
	if (first == buffer_size - 1) {

		first = 0;
	}
	else {
		first++;
	}
	return item;
}

void produce_function(void * p_id) {
	int i;
	pthread_mutex_lock(&produce_mutex);
		int id = produce_id;
		produce_id ++;
	pthread_mutex_unlock(&produce_mutex);

	for (i = 0; i < total_to_produce; ++i) {
		if (i % total_number_of_producers == id) {
			sem_wait(&empty_count);
				pthread_mutex_lock(&buffer_mutex);
					total_number_produced++;
					add_to_queue(i);
				pthread_mutex_unlock(&buffer_mutex);
			sem_post(&fill_count);
		}
	}
	pthread_exit(0);
}

void consumer_function(void * c_id) {

	pthread_mutex_lock(&consume_mutex);
		int id = consume_id;
		consume_id += 1;
	pthread_mutex_unlock(&consume_mutex);

	int item;
	int square_root;
	int square;
	while (1){
		sem_wait(&fill_count);
			pthread_mutex_lock(&buffer_mutex);
				item = remove_from_queue();
				total_number_consumed++;
			pthread_mutex_unlock(&buffer_mutex);
		sem_post(&empty_count);
		
		square_root = (int)sqrt(item);
		square = square_root * square_root;
		if (item  ==  square) {
			printf("%d %d %d \n", id, item, square_root);
		}
		pthread_mutex_lock(&consume_mutex);
			int left_to_consume = total_to_produce - total_number_consumed;
			if (left_to_consume + 1 <= number_of_active_consumers) {
				pthread_mutex_unlock(&consume_mutex);
				number_of_active_consumers--;
				break;
			}
		pthread_mutex_unlock(&consume_mutex);
	}


	pthread_exit(0);
}


int main(int argc, char *argv[])
{
	int num;
	int maxmsg;
	int num_p;
	int num_c;
	int i;
	struct timeval tv;

	if (argc != 5) {
		printf("Usage: %s <N> <B> <P> <C>\n", argv[0]);
		exit(1);
	}

	num = atoi(argv[1]);	/* number of items to produce */
	maxmsg = atoi(argv[2]); /* buffer size                */
	num_p = atoi(argv[3]);  /* number of producers        */
	num_c = atoi(argv[4]);  /* number of consumers        */

	total_to_produce = num;
	total_number_of_producers = num_p;
	total_number_of_consumers = num_c;
	number_of_active_consumers = num_c;


	gettimeofday(&tv, NULL);
	g_time[0] = (tv.tv_sec) + tv.tv_usec/1000000.;

	buffer_size = maxmsg;
	buffer = malloc(buffer_size * sizeof(int));

	pthread_t p_id[num_p];
	pthread_t c_id[num_c];

	sem_init(&empty_count, 0, buffer_size);
	sem_init( &fill_count, 0 ,0);

	
	for (i = 0; i < num_p; ++i) {
		pthread_create(&p_id[i], NULL, produce_function, NULL);
	}

	for (i = 0; i < num_c; ++i) {

		pthread_create(&c_id[i], NULL, consumer_function, NULL);
	}

	int* retval;
	for (i = 0; i < num_c; ++i) {
		
		pthread_join(c_id[i], &retval);
	}

    gettimeofday(&tv, NULL);
    g_time[1] = (tv.tv_sec) + tv.tv_usec/1000000.;

    printf("System execution time: %.6lf seconds\n", \
            g_time[1] - g_time[0]);
	exit(0);
}