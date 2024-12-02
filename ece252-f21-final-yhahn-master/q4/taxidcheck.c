#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdbool.h>
#include <pthread.h>
#include <unistd.h>

#define TAX_ID_LENGTH 9

bool check_tax_id (char* tax_id);
char *get_next_tax_id(FILE *inputfile);
void write_output(char *tax_id, bool valid, FILE *outputfile);
pthread_t *tax_thread;
void *tax_verify(void* ignore);
FILE *input_file;
FILE *output_file;
int* invalid_tax_id_count;
int* valid_tax_id_count;
pthread_mutex_t mutex;


int main( int argc, char** argv ) {
    if (argc != 2) {
        printf("Usage: %s [filename]", argv[0]);
        return -1;
    }
    invalid_tax_id_count = malloc( sizeof(int) );
    valid_tax_id_count = malloc( sizeof(int) );
    *invalid_tax_id_count = 0;
    *valid_tax_id_count = 0;

    input_file = fopen(argv[1], "r");
    if (input_file == NULL) {
        printf("Unable to open input file!\n");
        return -1;
    }
    output_file = fopen("results.txt", "w");
    if (output_file == NULL) {
        printf("Unable to open output file!\n");
        return -1;
    }

    pthread_mutex_init(&mutex, NULL);

    /* Complete the code here to use 4 threads */
    tax_thread = malloc(4 * sizeof(pthread_t));
    for(int i = 0; i < 4; i++){
        pthread_create(&tax_thread[i], NULL, tax_verify, NULL);
    }
    
    for(int i = 0; i < 4; i++){
        pthread_join(tax_thread[i], NULL);
    }


    printf( "Number of valid tax IDs: %d\n", *valid_tax_id_count );
    printf( "Number of invalid tax IDs: %d\n", *invalid_tax_id_count );
    free( invalid_tax_id_count );
    free( valid_tax_id_count );
    free(tax_thread);
    fclose(input_file);
    fclose(output_file);
    pthread_mutex_destroy(&mutex);
    return 0;
}

void *tax_verify(void *ignore){
    while(1){
        pthread_mutex_lock(&mutex);
        char* tax_id = get_next_tax_id(input_file);
        pthread_mutex_unlock(&mutex);
        if(tax_id == NULL){
            break;
        }
        bool checking_id = check_tax_id(tax_id);
        if(checking_id){
            pthread_mutex_lock(&mutex);
            (*valid_tax_id_count)++;
            pthread_mutex_unlock(&mutex);
        }
        else{
            pthread_mutex_lock(&mutex);
            (*invalid_tax_id_count)++;
            pthread_mutex_unlock(&mutex);
        }
        pthread_mutex_lock(&mutex);
        write_output(tax_id, checking_id, output_file);
        pthread_mutex_unlock(&mutex);
    }
    //input output mutex invalid_tax_id_count valid_tax_id_count
    //while all input
    //get_next_tax_id() of the input file premade
    // check if this is null (end of file)
    //if not check the bool 
    //use bottom function to validate
    // get result of the boolean if valid or not check_tax_id
    // based on the boolean increment
    //write to the output file
    
    
}


/*
 ***************************************
 * Don't change anything below this line
 ***************************************
 */

char *get_next_tax_id(FILE *inputfile)  {
    char *tax_id = malloc( TAX_ID_LENGTH );
    memset(tax_id, 0,  TAX_ID_LENGTH );
    int read = fscanf(inputfile, "%s\n", tax_id);
    if (read == 0 || read == -1) { /* EOF */
        free(tax_id);
        return NULL;
    }
    return tax_id;
}

void write_output(char *tax_id, bool valid, FILE *outputfile) {
    fprintf(outputfile, "%s: %s\n", tax_id, (valid ? "Valid": "Invalid"));
    free(tax_id);
}

bool check_tax_id( char* tax_id ) {
    /* Simulate network delay */
    usleep(5000);
    /* This is not how tax ID checks work; remote server would say yes or no */
    return tax_id[8] != '0';
}