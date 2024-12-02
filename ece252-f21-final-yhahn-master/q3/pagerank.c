#include <stdio.h>
#include <stdlib.h>
#include <sys/shm.h>
#include <sys/time.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <unistd.h>
#include <pthread.h>

#include "q3helper.h"

void pagerank(link_t l[], int nLinks, double rank[], int nPages, double delta,
		int iterations, int nProcesses) {
	for (int i = 0; i < nPages; i++){
		rank[i] = 0;
	}

	pthread_mutex_t * mutex = NULL;
    pthread_mutexattr_t attrmutex;
    pthread_mutexattr_init(&attrmutex);
    pthread_mutexattr_setpshared(&attrmutex, PTHREAD_PROCESS_SHARED);
    int mutex_id = shmget(IPC_PRIVATE, sizeof(pthread_mutex_t), IPC_CREAT | IPC_EXCL | S_IRUSR | S_IWUSR);
    mutex = shmat(mutex_id, NULL, 0);
    pthread_mutex_init(mutex, &attrmutex);

	// compute number of outgoing links for each page
	int outDegree[nPages];
	for(int i=0; i<nPages; i++) outDegree[i] = 0;
	for(int j=0; j<nLinks; j++) outDegree[l[j].src]++;

	double r[nPages]; // ranks
	double s[nPages]; // new ranks

	// initial guess
	for(int i=0; i<nPages; i++) r[i] = 1.0/nPages;

	// FORK PROCESSES HERE
	pid_t pid = -1;
	int child_status;
	int task_each_process_do = nPages/nProcesses;
	int remaining = task_each_process_do + (nPages % nProcesses);
	int current_process_location = 0;
	int bar_id = shmget(IPC_PRIVATE, sizeof(barrier_t), IPC_CREAT | IPC_EXCL | S_IRUSR | S_IWUSR);
	barrier_t *bar = shmat(bar_id,NULL,0);
	bInit(bar, nProcesses); // Initializing a barrier

	for (int i = 0; i < nProcesses-1; i ++){
		current_process_location = i;
		pid = fork();
		if(pid == 0){
			break;
		}
	}

	// power method
	if(pid < 0){
		return;
	}
	else if(pid == 0){
		for(int k=0; k<iterations; k++) {
			// calculate new values, placing them in s[]
			for(int i= (current_process_location)*task_each_process_do; i< (current_process_location + 1)*(task_each_process_do); i++) {
				s[i] = (1.0-delta)/nPages;
				for(int j=0; j<nLinks; j++){
					if(l[j].dst == i){
						pthread_mutex_lock(mutex);
						s[i] += delta * r[l[j].src] / outDegree[l[j].src];
						pthread_mutex_unlock(mutex);
					}
				}
			}
			
			bSync(bar);
			// copy s[] to r[] for next iteration
			for(int i= (current_process_location)*task_each_process_do; i< (current_process_location + 1)*(task_each_process_do); i++){
				pthread_mutex_lock(mutex);
				r[i] = s[i];
				pthread_mutex_unlock(mutex);
			}

			bSync(bar);
		}
	}
	else{
		for(int k=0; k<iterations; k++) {
			// calculate new values, placing them in s[]
			for(int i= (nPages - remaining); i < nPages; i++) {
				s[i] = (1.0-delta)/nPages;
				for(int j=0; j<nLinks; j++){
					if(l[j].dst == i){
						pthread_mutex_lock(mutex);
						s[i] += delta * r[l[j].src] / outDegree[l[j].src];
						pthread_mutex_unlock(mutex);
					}
				}
			}
			
			bSync(bar);
			// copy s[] to r[] for next iteration
			for(int i= nPages - remaining; i < nPages; i++){
				pthread_mutex_lock(mutex);
				r[i] = s[i];
				pthread_mutex_unlock(mutex);
			}

			bSync(bar);
		}
	}
	

	// JOIN PROCESSES HERE
	waitpid(pid, NULL, 0);
	for(int i=0; i<nPages; i++){
		rank[i] = r[i];
	}
	shmdt(bar);
	shmdt(mutex);
	shmctl(bar_id, IPC_RMID, NULL);
	shmctl(mutex_id, IPC_RMID, NULL);
	shmdt(r);
}

int main(int argc, char *argv[]) {
	if(argc != 3) {
		printf("usage: %s <n_process> <link_file>\n", argv[0]);
		return 0;
	}

	// read in links file
	link_t *links;
	char (*names)[NAME_MAX];
	size_t n_links, n_pages;
	if(!readLinks(argv[2], &links, &n_links, &names, &n_pages)) {
		printf("failed to read links file %s\n", argv[2]);
		return 0;
	}

	// start timer
	struct timeval tv;
	gettimeofday(&tv, NULL);
	double startTime = (tv.tv_sec) + tv.tv_usec/1000000.;

	// run pagerank
	double *rank = malloc(n_pages * sizeof(double));
	double delta = 0.80; // convergence rate
	double iterations = 1000;
	int processes = atoi(argv[1]);
	if(processes < 1 || processes > 4) {
		printf("processes %d not in [1,4]\n", processes);
		return 0;
	}
	pagerank(links, n_links, rank, n_pages, delta, iterations, processes);

	// stop timer
	gettimeofday(&tv, NULL);
	double endTime = (tv.tv_sec) + tv.tv_usec/1000000.;
	printf("%s execution time: %.2f seconds\n", argv[0], endTime - startTime);

	// write ranks.txt
	FILE *fout = fopen("ranks.txt", "w");
	for(int i=0; i<n_pages; i++)
		fprintf(fout, "%.8f %s\n", rank[i], names[i]);
	fclose(fout);

	// clean up
	free(links);
	free(names);
	free(rank);
}

