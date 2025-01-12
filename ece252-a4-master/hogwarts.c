#define _XOPEN_SOURCE 500 

#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <semaphore.h>
#include <openssl/sha.h>

#define NUM_ELVES 5

/* Please don't change anything between this line and 
   the start of the global variables */
#define HASH_BUFFER_LENGTH 32

unsigned int random_seed = 252;
int task_id_counter = 0;

typedef struct {
    int id;
    unsigned char* input;
} task;

typedef struct node {
    task* task;
    struct node* next;
} node;

node* list_head;

/* Global Variables -- Add more if you need! */
int total_tasks;
int active_tasks;
pthread_mutex_t mutex;
sem_t empty_list;
sem_t full_list;

//extra global variables
pthread_t *thread_for_house_elf;
int posted = 0;

/* Function Prototypes for pthreads */
void* dobby( void* );
void* house_elf( void * );

/* Don't change the function prototypes below;
   they are your API for how the house elves do work */

/* Removes a task from the list of things to do
   To be run by: house elf threads
   Takes no arguments
   NOT thread safe!
   return: a pointer to a task for an elf to do
*/
task* take_task();

/* Perform the provided task
   To be run by: house elf threads
   argument: pointer to task to be done
   IS thread safe
   return: nothing
*/  
void do_work( task* todo );

/* Put tasks in the list for elves to do
   To be run by: Dobby
   argument: how many tasks to put in the list
   NOT thread safe
   return: nothing
*/
void post_tasks( int howmany );

/* Used to unlock a mutex, if necessary, if a thread
   is cancelled when blocked on a semaphore
*/
void house_elf_cleanup( void * );

/* Complete the implementation of main() */

int main( int argc, char** argv ) {
  if ( argc != 2 ) {
      printf( "Usage: %s total_tasks\n", argv[0] );
      return -1;
  }
  /* Init global variables here */
  list_head = NULL;
  total_tasks = atoi( argv[1] );
  printf("There are %d tasks to do today.\n", total_tasks);
  active_tasks = 0;
  /*shared value is 0 and initial value is 5 
    later will be used for function dobby when they will wait for the list
    will decrement the values in dobby
  */
  sem_init(&empty_list, 0, 0);
  sem_init(&full_list,0,0);
  pthread_mutex_init(&mutex, NULL);

  /* Launch threads here */
  thread_for_house_elf = malloc(NUM_ELVES* sizeof(pthread_t));
  for (int i = 0; i < NUM_ELVES; i++){
      //printf("creating threads\n");
      pthread_create(&thread_for_house_elf[i], NULL, house_elf, NULL);
  }

  /* Wait for Dobby to be done */
  pthread_t dobby_thread;
  pthread_create(&dobby_thread,NULL,dobby, NULL);
  pthread_join(dobby_thread, NULL);

  /* Cleanup Global Variables here */
  free(thread_for_house_elf);
  return 0;
}

/* Write the implementation of dobby() */
//doby post the tasks for the elves to do
void* dobby( void * arg ) {
    int tasks_remaining = total_tasks; //tasks that are remaining

    while(tasks_remaining != 0){
        sem_wait(&empty_list);
        pthread_mutex_lock(&mutex);
        posted = 0;
        //waiting for dobby to post
        if(tasks_remaining >= 10){
            post_tasks(10);
            active_tasks = 10;
            tasks_remaining -= 10;
        }

        else{
            post_tasks(tasks_remaining);
            active_tasks = tasks_remaining;
            tasks_remaining = 0;
        }

        //when dobby post the tasks, post semaphores so that the elves can do their work
        for(int i = 0; i < NUM_ELVES; i++){
            sem_post(&full_list);
        }
        pthread_mutex_unlock(&mutex);
    }

    if(tasks_remaining == 0){
        sem_wait(&empty_list);
        for(int i=0; i < NUM_ELVES; i++){
            pthread_cancel(thread_for_house_elf[i]);
            pthread_join(thread_for_house_elf[i], NULL);
        }
    }
}  

/* Complete the implementation of house_elf() */
//elves do the tasks that the dobbies posted
void* house_elf( void * ignore ) {
  /* The following function call registers the cleanup
     handler to make sure that pthread_mutex_t locks 
     are unlocked if the thread is cancelled. It must
     be bracketed with a call to pthread_cleanup_pop 
     so don't change this line or the other one */
  pthread_cleanup_push( house_elf_cleanup, NULL );
  task *taskforelf;
  int wokeup = 1;
  while( 1 ) {
    //printf("before sem_wait\n");
    pthread_mutex_lock(&mutex);
    //printf("after lock\n");
    
    if(active_tasks != 0){
        pthread_setcancelstate(PTHREAD_CANCEL_DISABLE,NULL);
        //printf("checking decrement\n");
        //printf("active_tasks: %d \n",active_tasks);
        active_tasks -=1;
        taskforelf = take_task();
        pthread_mutex_unlock(&mutex);
        do_work(taskforelf);
        pthread_setcancelstate(PTHREAD_CANCEL_ENABLE,NULL);
    }
    
    else{
        //printf("checking last\n");
        if(posted == 0){
            //printf("checking posted\n");
            posted = 1;
            wokeup = 1;
            sem_post(&empty_list);
        }
        pthread_mutex_unlock(&mutex);
        sem_wait(&full_list);
    }
    
    //printf("checking dowrk\n");
  }
  
  /* This cleans up the registration of the cleanup handler */
  pthread_cleanup_pop( 0 ) ;
}

/* Implement unlocking of any pthread_mutex_t mutex types
   that are locked in the house_elf thread, if any 
*/
void house_elf_cleanup( void* arg ) {
    pthread_mutex_unlock(&mutex);
}

/****** Do not change anything below this line -- Internal Functionality Only ********/ 

/* Generate random string code based off original by Ates Goral */
char* random_string(const int len, unsigned int * generator_seed) {
    static const char alphanum[] =
        "0123456789"
        "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
        "abcdefghijklmnopqrstuvwxyz";
    char* s = malloc( (len+1) * sizeof( char ));

    for (int i = 0; i < len; ++i) {
        s[i] = alphanum[rand_r(generator_seed) % (sizeof(alphanum) - 1)];
    }
    s[len] = 0;
    return s;
}

void post_tasks( int howmany ) {
    printf("Adding %d tasks to the list of active tasks.\n", howmany);
    for ( int i = 0; i < howmany; ++i ) {
        task* t = malloc( sizeof( task ));
        t->id = ++task_id_counter;
        t->input = random_string( HASH_BUFFER_LENGTH, &random_seed );
        node* n = malloc( sizeof( node ));
        n->task = t;
        n->next = list_head;
        list_head = n;
    }
}

void do_work( task* todo ) {
     unsigned char* output_buffer = calloc( HASH_BUFFER_LENGTH , sizeof ( unsigned char ) );
     SHA256( todo->input, HASH_BUFFER_LENGTH, output_buffer );
     printf("Task %d was completed!\n", todo->id);
     free( output_buffer );
     free( todo->input );
     free( todo );
}

task* take_task( ) {
    if (list_head == NULL) {
        /* Something has gone wrong */
        printf("Trying to take from an empty list!\n");
        exit( -1 );
    }
    node* head = list_head;
    task* t = head->task;
    list_head = list_head->next;
    free( head );
    return t;
}

