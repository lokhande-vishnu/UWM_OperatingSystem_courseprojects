#include "cs537.h"
#include "request.h"
#include <pthread.h>

int nbuffers;
int nthreads;
int *buffer;
int count = 0;
int filled = 0;
int used = 0;
pthread_t *thread_pool;
pthread_cond_t empty, fill;
pthread_mutex_t mutex;

void getargs(int *port, int argc, char *argv[]) {
    if (argc != 4) {
      fprintf(stderr, "Usage: %s <port> <threads> <buffers>\n", argv[0]);
	exit(1);
    }
    *port = atoi(argv[1]);
    nthreads = atoi(argv[2]);
    if(nthreads <= 0)
      server_error("Number of threads have to be positive\n");
    nbuffers = atoi(argv[3]);
    if(nbuffers <= 0)
      server_error("Number of buffers have to be positive\n");
    
}

void *worker_threads() {
  // Consumer code
  while(1) {
    if(pthread_mutex_lock(&mutex) != 0)
      server_error("consumer lock failed\n");
    while (count == 0) {
      if(pthread_cond_wait(&fill,&mutex) != 0)
        server_error("consumer cond wait failed\n");
    }
    int curr_fd = buffer[used];
    used = (used + 1) % nbuffers;
    count--;
    
    if(pthread_cond_signal(&empty) != 0)
      server_error("consumer cond signal failed\n");
    if(pthread_mutex_unlock(&mutex) != 0)
      server_error("consumer unlock failed\n");
    requestHandle(curr_fd);
    Close(curr_fd);
  }
  return NULL;
}

int main(int argc, char *argv[])
{
    int listenfd, connfd, port, clientlen;
    struct sockaddr_in clientaddr;

    // Initialisation
    pthread_mutex_init(&mutex, NULL);
    pthread_cond_init(&empty, NULL);
    pthread_cond_init(&fill, NULL);

    // Reading the arguments
    getargs(&port, argc, argv);

    // Creating a buffer for the threads
    buffer = (int*)malloc(nbuffers*sizeof(int*));

    thread_pool = (pthread_t*)malloc(nthreads*sizeof(pthread_t*));
    int i;
    for(i = 0; i < nthreads; i++) {
      if(pthread_create(&thread_pool[i], NULL, &worker_threads, NULL) != 0)
        server_error("thread creation failed\n");
    }
    
    listenfd = Open_listenfd(port);
    while (1) {
	clientlen = sizeof(clientaddr);
	connfd = Accept(listenfd, (SA*)&clientaddr, (socklen_t*) &clientlen);

        // Producer Code
        if (pthread_mutex_lock(&mutex) != 0)
          server_error("producer lock failed\n");
        while(count == nbuffers) {
          if(pthread_cond_wait(&empty,&mutex) != 0)
            server_error("producer cond wait failed\n");
        }
        buffer[filled] = connfd;
        filled = (filled + 1) % nbuffers;
        count ++;
        if(pthread_cond_signal(&fill) != 0)
          server_error("producer cond signal failed\n");
        if(pthread_mutex_unlock(&mutex) != 0)
          server_error("producer unlock failed\n");
    }
}


    


 
