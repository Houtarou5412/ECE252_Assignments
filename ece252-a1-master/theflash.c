#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>
#include <sys/wait.h>

#define CITIES_LENGTH 7
#define NUM_CITIES (CITIES_LENGTH - 1)

static const char* cities[] = { "Central City", "Starling City", "Gotham City", "Metropolis", "Coast City", "National City" };


const int distances[CITIES_LENGTH - 1][ CITIES_LENGTH - 1] = {
    {0, 793, 802, 254, 616, 918},
    {793, 0, 197, 313, 802, 500},
    {802, 197, 0, 496, 227, 198},
    {254, 313, 496, 0, 121, 110},
    {616, 802, 227, 121, 0, 127},
    {918, 500, 198, 110, 127, 0}
};

int initial_vector[CITIES_LENGTH] = { 0, 1, 2, 3, 4, 5, 0 };

typedef struct {
    int cities[CITIES_LENGTH];
    int total_dist; 
} route;

route* possibilities;
int index_of_array = 0;

void print_route ( route* r ) {
    printf ("Route: ");
    for ( int i = 0; i < CITIES_LENGTH; i++ ) {
        if ( i == CITIES_LENGTH - 1 ) {
            printf( "%s\n", cities[r->cities[i]] );
        } else {
            printf( "%s - ", cities[r->cities[i]] );
        }
    }
}

void calculate_distance( route* r ) {
    if ( r->cities[0] != 0 ) {
        printf( "Route must start with %s (but was %s)!\n", cities[0], cities[r->cities[0]]);
        exit( -1 );
    } 
    if ( r->cities[6] != 0 ) {
        printf( "Route must end with %s (but was %s)!\n", cities[0], cities[r->cities[6]]);
        exit ( -2 );
    }
    int distance = 0;
    for ( int i = 1; i < CITIES_LENGTH; i++ ) {
        int to_add = distances[r->cities[i-1]][r->cities[i]];
        if ( to_add == 0 ) {
            printf( "Route cannot have a zero distance segment.\n");
            exit ( -3 );
        }
        distance += to_add;
    }
    r->total_dist = distance;
}

void swap(int* a, int* b) { //swaps the value
    int tmp = *a;
    *a = *b;
    *b = tmp;
}

void permute(route* r, int left, int right) {
    if (left == right) {
        //memcpy(array index, r, cities_length)
        possibilities[index_of_array]= *r;
        index_of_array++;
    }
    else{
        for (int i = left; i <= right; i++) {
        swap(&r->cities[left], &r->cities[i]);
        permute(r, left + 1, right);
        swap(&r->cities[left], &r->cities[i]);
        }
    }

    
}   

// void assign_best(route** best, route* candidate) { //find the shorter distance
//     if (*best == NULL) {
//         *best = candidate;
//         return;
//     }

//     int a = candidate->total_dist;
//     int b = (*best)->total_dist;

//     if (a < b) {
//         free(*best);
//         *best = candidate;
//     } else {
//         free(candidate);
//     }
// }

int factorials(int a){
    int i;
    int result = 1;
    for(i=1; i <= a; i++){
        result *= i;
    }
    return result;
}

// route* find_best_route( ) {
//     route* candidate = malloc( sizeof(route) );
//     memcpy (candidate->cities, initial_vector, CITIES_LENGTH * sizeof( int )); //copies 2nd to 1st and third parameter is size
//     candidate->total_dist = 0;

//     route* best = malloc( sizeof(route) );
//     memset( best, 0, sizeof(route) );
//     best->total_dist = 999999;

//     // permute( candidate, 1, 5, best );

//     free( candidate );
//     return best;
// }

int main( int argc, char** argv ) {
    
    route* candidate = malloc( sizeof(route) );
    memcpy (candidate->cities, initial_vector, CITIES_LENGTH * sizeof( int )); //copies 2nd to 1st and third parameter is size
    candidate->total_dist=0;
    
    pid_t pid = -1;
    int child_status;
    possibilities = malloc(factorials(CITIES_LENGTH-2) * sizeof(route));
    
    for (int i = 0; i < CITIES_LENGTH; i++)
    {
        candidate->cities[i] = initial_vector[i];
    }
    
    permute(candidate, 1, CITIES_LENGTH-2);
    FILE *f_dist;
    for(int i = 0; i < factorials(CITIES_LENGTH -2); i++){
        pid = fork();
        f_dist = fopen("total_distance.txt", "w+");
        if (pid < 0){
            printf("Error occurred: %d.\n", pid);
            return -1;
        }
        else if(pid == 0){
            //printf("Child Executing...\n");
            
            calculate_distance(&possibilities[i]);
            fprintf(f_dist,"%d\n", possibilities[i].total_dist);
            
            free(candidate);
            fclose(f_dist);
            free(possibilities);
            return 0;
        }
        else{
            //printf("Parent executing...\n");
            wait(&child_status);
            // printf("This is the parent process. Child returned %d.\n",child_status);
            char a[30];
            fscanf(f_dist,"%s\n",a);
            // printf("This is the 'a' value: %s\n",a);
            possibilities[i].total_dist = atoi(a);
            
            if(candidate->total_dist == 0) {
                candidate->total_dist = possibilities[i].total_dist;
            }
            else if(candidate->total_dist > possibilities[i].total_dist){
                candidate->total_dist = possibilities[i].total_dist;
                for(int j =0; j < CITIES_LENGTH; j++){
                    candidate->cities[j] = possibilities[i].cities[j];
                }
            }
            // printf("This is the best total distance: %d\n", candidate->total_dist);
        }
        fclose(f_dist);
    }
    
    // for (int i = 0; i < factorials(CITIES_LENGTH-2); i++)
    // {
    //     printf("%d %d %d %d %d %d %d\n", possibilities[i].cities[0],possibilities[i].cities[1],possibilities[i].cities[2],possibilities[i].cities[3],possibilities[i].cities[4],possibilities[i].cities[5],possibilities[i].cities[6]);
    // }
    
    print_route(candidate);
    printf( "Distance: %d\n", candidate->total_dist );  

    free(candidate);
    free(possibilities);
    return 0;
}

