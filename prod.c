
#include <pthread.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/time.h>
#include <math.h>
#include <sys/sysinfo.h>

#define QUEUESIZE 10000

#define TASK_PER_PROD 60000

#define NUM_PRODUCERS 2 //maybe i will change this
#define NUM_CONSUMERS 16

void *producer (void *args);
void *consumer (void *args);

int total_tasks = NUM_PRODUCERS * TASK_PER_PROD;
int consumed_tasks = 0;
pthread_mutex_t count_mutex = PTHREAD_MUTEX_INITIALIZER;

typedef struct{
    void *(*work)(void *);
    void *arg;
    struct timeval enqueue_time; // to record when the item was added
} workFunction;


typedef struct {
  workFunction buf[QUEUESIZE];
  long head, tail;
  int full, empty;
  pthread_mutex_t *mut;
  pthread_cond_t *notFull, *notEmpty;
} queue;



queue *queueInit (void);
void queueDelete (queue *q);
void queueAdd (queue *q, workFunction in);
void queueDel (queue *q, workFunction *out);


int main() {
    queue *fifo = queueInit();
    pthread_t producers[NUM_PRODUCERS], consumers[NUM_CONSUMERS];

    //prestart consumers
    for (int i = 0; i < NUM_CONSUMERS; i++)
    pthread_create(&consumers[i], NULL, consumer, fifo);

    for (int i = 0; i < NUM_PRODUCERS; i++)
        pthread_create(&producers[i], NULL, producer, fifo);

    // Join all threads
    for (int i = 0; i < NUM_PRODUCERS; i++)
        pthread_join(producers[i], NULL);

    for (int i = 0; i < NUM_CONSUMERS; i++)
        pthread_join(consumers[i], NULL);

    queueDelete(fifo);
    return 0;
}

//example function to be used in the queue
void *compute_sine(void *arg) {
    double *angles = (double *)arg;
    for (int i = 0; i < 10; i++) {
        double radians = angles[i] * M_PI / 180.0;
        double result = sin(radians);
        printf("sin(%f°) = %f\n", angles[i], result);
    }
    free(arg); 
    return NULL;
}

void *producer(void *q) {
    queue *fifo = (queue *)q;

    for (int i = 0; i < TASK_PER_PROD; i++) {
        workFunction wf;
        wf.work = compute_sine;

        // allocate and initialize argument 
        double *angles = malloc(10 * sizeof(double));
        for (int j = 0; j < 10; j++) {
            angles[j] = (double)(i * 10 + j); // some unique angles
        }
        wf.arg = angles;

        gettimeofday(&wf.enqueue_time, NULL); // timestamp the enqueue moment

        pthread_mutex_lock(fifo->mut);
        while (fifo->full) {
            pthread_cond_wait(fifo->notFull, fifo->mut);
        }
        queueAdd(fifo, wf);
        pthread_mutex_unlock(fifo->mut);
        pthread_cond_signal(fifo->notEmpty);
    
        // Print the result if needed
        printf("produser: task completed.\n");
    }

    return NULL;
}

void *consumer(void *q) {
    queue *fifo = (queue *)q;
    workFunction d;
    struct timeval dequeue_time, latency_time;
    long latency_usec;


    FILE *fp = fopen("latency_data.csv", "a");
    if (!fp) {
        perror("Failed to open log file");
        exit(1);
    }
    

    // Buffer to reduce I/O ops
    char buffer[1024];
    size_t buf_len = 0;
    while (1) {
      pthread_mutex_lock(fifo->mut);
      while (fifo->empty) {
          pthread_cond_wait(fifo->notEmpty, fifo->mut);
      }
  
      gettimeofday(&dequeue_time, NULL);
      queueDel(fifo, &d);
      pthread_mutex_unlock(fifo->mut);
      pthread_cond_signal(fifo->notFull);
  
      timersub(&dequeue_time, &d.enqueue_time, &latency_time);
      latency_usec = latency_time.tv_sec * 1000000 + latency_time.tv_usec;
  
      fprintf(fp, "%ld,%d\n", latency_usec, NUM_CONSUMERS);
      fflush(fp); // ensure it gets written
  
      d.work(d.arg);
  
      // increment global counter
      pthread_mutex_lock(&count_mutex);
      consumed_tasks++;
      if (consumed_tasks >= total_tasks) {
          pthread_mutex_unlock(&count_mutex);
          break;  // exit after all tasks are consumed
      }
      pthread_mutex_unlock(&count_mutex);
  }
  fclose(fp);
  
    return NULL;
}

queue *queueInit (void)
{
  queue *q;

  q = (queue *)malloc (sizeof (queue));
  if (q == NULL) return (NULL);

  q->empty = 1;
  q->full = 0;
  q->head = 0;
  q->tail = 0;
  q->mut = (pthread_mutex_t *) malloc (sizeof (pthread_mutex_t));
  pthread_mutex_init (q->mut, NULL);
  q->notFull = (pthread_cond_t *) malloc (sizeof (pthread_cond_t));
  pthread_cond_init (q->notFull, NULL);
  q->notEmpty = (pthread_cond_t *) malloc (sizeof (pthread_cond_t));
  pthread_cond_init (q->notEmpty, NULL);
	
  return (q);
}

void queueDelete (queue *q)
{
  pthread_mutex_destroy (q->mut);
  free (q->mut);	
  pthread_cond_destroy (q->notFull);
  free (q->notFull);
  pthread_cond_destroy (q->notEmpty);
  free (q->notEmpty);
  free (q);
}

void queueAdd (queue *q, workFunction in)
{
  q->buf[q->tail] = in;
  q->tail++;

  if (q->tail == QUEUESIZE)
    q->tail = 0;

  if (q->tail == q->head)
    q->full = 1;

  q->empty = 0;

  return;
}

void queueDel (queue *q, workFunction *out)
{
  *out = q->buf[q->head];

  q->head++;

  if (q->head == QUEUESIZE)
    q->head = 0;

  if (q->head == q->tail)
    q->empty = 1;

  q->full = 0;

  return;
}