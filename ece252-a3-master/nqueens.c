// from the Cilk manual: http://supertech.csail.mit.edu/cilk/manual-5.4.6.pdf
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>

struct info {
    int n;
    int starting_value;
};

int safe(char * config, int i, int j)
{
    int r, s;

    for (r = 0; r < i; r++)
    {
        s = config[r];
        if (j == s || i-r==j-s || i-r==s-j)
            return 0;
    }
    return 1;
}

int * count;

void nqueens(char *config, int n, int i, int starting_value)
{
    char *new_config;
    int j;

    if (i==n)
    {
        count[starting_value]++;
    }
    
    if(i == 0){
         /* allocate a temporary array and copy the config into it */
            new_config = malloc((i+1)*sizeof(char));
            memcpy(new_config, config, i*sizeof(char));
            if (safe(new_config, i, starting_value))
            {
                new_config[i] = starting_value;
                nqueens(new_config, n, i+1, starting_value);
            }
            free(new_config);
    }
    
    else{
        /* try each possible position for queen <i> */
        for (j=0; j<n; j++)
        {
            /* allocate a temporary array and copy the config into it */
            new_config = malloc((i+1)*sizeof(char));
            memcpy(new_config, config, i*sizeof(char));
            if (safe(new_config, i, j))
            {
                new_config[i] = j;
                nqueens(new_config, n, i+1, starting_value);
            }
            free(new_config);
        }
    }
    return;
}

void * call_nqueen(void * a){
    struct info *in;
    in = (struct info*)a;
    char *config;
    config = malloc(in->n * sizeof(char));
    memset(config,0,sizeof(char)*in->n);
    nqueens(config, in->n, 0, in->starting_value);
    free(config);
    free(in);
}

int main(int argc, char *argv[])
{
    int n;

    if (argc < 2)
    {
        printf("%s: number of queens required\n", argv[0]);
        return 1;
    }

    n = atoi(argv[1]);
    count = malloc(sizeof(int) * n);
    memset(count,0,sizeof(int)*n);
    
    pthread_t *p_tids = malloc(sizeof(pthread_t)*n);

    printf("running queens %d\n", n);
    for(int i = 0; i < n; i++){
        struct info * temp = malloc(sizeof(struct info));
        temp->starting_value = i;
        temp->n = n;
        
        pthread_create(p_tids + i, NULL, call_nqueen, temp);
    }
    
    for (int j = 0; j < n; j++)
    {
        pthread_join(p_tids[j], NULL);
    }
    
    int total_count = 0;
    for(int k = 0; k < n; k++){
        total_count += count[k];
    }

    printf("# solutions: %d\n", total_count);
    free(count);
    free(p_tids);
    return 0;
}


