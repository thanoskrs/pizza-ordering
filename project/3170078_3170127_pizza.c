#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <pthread.h>
#include <time.h>
#include "3170078_3170127_pizza.h"

// initialize all mutexes and conds
pthread_mutex_t lock_tel;
pthread_cond_t cond_tel;

pthread_mutex_t lock_cook;
pthread_cond_t cond_cook;

pthread_mutex_t lock_oven;
pthread_cond_t cond_oven;

pthread_mutex_t lock_pack;
pthread_cond_t cond_pack;

pthread_mutex_t lock_deliverer;
pthread_cond_t cond_deliverer;

// initialize the employees
int N_tel = 3;
int N_cook = 2;
int N_oven = 10;
int N_packer = 1;
int N_deliverer = 7;

/* initialize the variables that will be used
   for the output of program */
int income = 0;
int succeed_orders = 0;
int failed_orders = 0;

// initialize time
double totalTimeForWaiting = 0.0;
double totalTimeForServing = 0.0;
double totalTimeForCold = 0.0;

double maxTimeForWaiting = 0.0;
double maxTimeForServing = 0.0;
double maxTimeForCold = 0.0;

int seed;

// create the thread-method for every customer 
void *order (void* threadId) {

	int *tid = (int*) (threadId); 
    	int rc, i;
    	
    	
    	struct timespec startOrdering, stopOrdering, stopServing, startCold, stopPack;
    	
    	clock_gettime(CLOCK_REALTIME, &startOrdering); // time of waiting in call
    	
    	/* the customer is waiting for an available telephonist 
    	   lock the mutex <lock_tel> */
    	rc = pthread_mutex_lock(&lock_tel);
    	while(N_tel == 0) {
    		rc = pthread_cond_wait(&cond_tel, &lock_tel);
    	}
    	
    	/* decrease the number of telephonists by 1, 
    	   because someone now is busy */
    	N_tel--;
    	rc = pthread_mutex_unlock(&lock_tel); // unlock mutex
    	
    	clock_gettime(CLOCK_REALTIME, &stopOrdering);
    	
    	// time for waiting
    	totalTimeForWaiting += stopOrdering.tv_sec - startOrdering.tv_sec;
    	if (stopOrdering.tv_sec - startOrdering.tv_sec > maxTimeForWaiting) {
    		maxTimeForWaiting = stopOrdering.tv_sec - startOrdering.tv_sec;
    	}
    	
    	// take the order and charge the customer
    	int pizzas = rand_r(&seed)%N_ORDERHIGH + N_ORDERLOW;
    	int payment_time = rand_r(&seed)%T_PAYMENTHIGH + T_PAYMENTLOW;
    	
    	sleep(payment_time);
    	
    	int fail = rand_r(&seed)%100 + 1;
    	
    	// rc = pthread_mutex_lock(&lock_tel); // mutex lock
    	
    	int failed = 1;
    	
    	if (fail <= P_FAIL) {
    		failed_orders++;
    		failed = 0;
    	} else {
    		
    		income += C_PIZZA * pizzas;
		succeed_orders++;   
    	}
    	rc = pthread_mutex_lock(&lock_tel); // mutex lock
    	
    	if (failed == 0) {
    		printf("FAIL: Order with ID <%d> failed.\n", *tid);
    		
    	} else {
    		printf("SUCCESS: Order with ID <%d> successfully registered.\n", *tid);  
    	}
    	
    	N_tel++; // the telephonist is now available for another call
    	
    	rc = pthread_cond_signal(&cond_tel);
    	rc = pthread_mutex_unlock(&lock_tel);
    	
    	// if failed, we exit pthread from here, because we need first to unlock mutex of tel.
    	if (failed == 0) {
    		pthread_exit(NULL);
    	}
    	
    	// wait for an available cook
    	rc = pthread_mutex_lock(&lock_cook);
    	while(N_cook == 0) {
    		rc = pthread_cond_wait(&cond_cook, &lock_cook);
    	}
    	
    	N_cook--; // cook is now busy
    	rc = pthread_mutex_unlock(&lock_cook);
    	
    	sleep(T_PREP * pizzas);
    	
    	/* cook is waiting for avbailable ovens. 
    	   cook is still busy */
    	rc = pthread_mutex_lock(&lock_oven);
    	while (N_oven < pizzas) {
    		rc = pthread_cond_wait(&cond_oven, &lock_oven);
    	}
    	
    	
    	/* decrease the number of N_oven by the number of
    	   ovens are used for the current order.*/
    	for (i = 1; i <= pizzas; i++) {
    		N_oven--;
    	}
  	rc = pthread_mutex_unlock(&lock_oven);
  	
  	/* now the cook is free.
  	   waiting until pizzas are baked. */
  	rc = pthread_mutex_lock(&lock_cook);

  	N_cook++;
  	
    	rc = pthread_cond_signal(&cond_cook);
    	rc = pthread_mutex_unlock(&lock_cook);
    	
  	
   	// baking pizzas. waiting for T_BAKE minutes total, because all pizzas are baking the same time
   	sleep(T_BAKE);
   	
   	clock_gettime(CLOCK_REALTIME, &startCold); // get the time that pizzas are baked
   	
   	rc = pthread_mutex_lock(&lock_pack);
   	
   	// wait until the pack-employee is available
   	while (N_packer == 0) {
   		rc = pthread_cond_wait(&cond_pack, &lock_pack);
   	}
   	
   	N_packer--;
   	rc = pthread_mutex_unlock(&lock_pack);
   	
   	sleep(T_PACK * pizzas);
   	clock_gettime(CLOCK_REALTIME, &stopPack);
   	
   	rc = pthread_mutex_lock(&lock_oven);
   	
   	printf("Order with ID <%d> prepeared in <%ld> minutes.\n", *tid, stopPack.tv_sec - startOrdering.tv_sec);
   	
   	for (i = 1; i <= pizzas; i++) {
    		N_oven++;
    	}
    	
    	rc = pthread_cond_signal(&cond_oven);
    	rc = pthread_mutex_unlock(&lock_oven);
    	
   	rc = pthread_mutex_lock(&lock_pack);
   	N_packer++;
    	
    	rc = pthread_cond_signal(&cond_pack);
    	rc = pthread_mutex_unlock(&lock_pack);
    	
    	// deliver the pizzas to the customer.
    	
    	rc = pthread_mutex_lock(&lock_deliverer);
    	
    	while (N_deliverer == 0) {
    		rc = pthread_cond_wait(&cond_deliverer, &lock_deliverer);
    	}
    	
    	N_deliverer--;
    	rc = pthread_mutex_unlock(&lock_deliverer);
    	
    	int deliver_time = rand_r(&seed)%T_DELHIGH + T_DELLOW;
    	
    	sleep(deliver_time);
    	
    	clock_gettime(CLOCK_REALTIME, &stopServing);
    	
    	// time for serving
    	totalTimeForServing += stopServing.tv_sec - startOrdering.tv_sec;
    	if (stopServing.tv_sec - startOrdering.tv_sec > maxTimeForServing) {
    		maxTimeForServing = stopServing.tv_sec - startOrdering.tv_sec;
    	}
    	
    	totalTimeForCold += stopServing.tv_sec - startCold.tv_sec;
    	if (stopServing.tv_sec - startCold.tv_sec > maxTimeForCold) {
    		maxTimeForCold = stopServing.tv_sec - startCold.tv_sec;
    	}
    	
    	printf("Order with ID <%d> delivered in <%ld> minutes.\n", *tid, stopServing.tv_sec - startOrdering.tv_sec);
    	
    	sleep(deliver_time);
    	
    	rc = pthread_mutex_lock(&lock_deliverer);
   	//printf("A deliverer is free.\n");
   	N_deliverer++;
    	
    	rc = pthread_cond_signal(&cond_deliverer);
    	rc = pthread_mutex_unlock(&lock_deliverer);
    	
    	pthread_exit(NULL);
}

int main(int argc, char *argv[]) {

	// check for correct arguements
	if (argc != 3) {
		printf("ERROR: You should pass 2 arquements. Number of customers and seed. Try again ...\n");
		exit(-1);
	}
	
	// initialize num of clients, seed
	int customers = atoi(argv[1]);
	seed = atoi(argv[2]);
	int rc;
	
	// check that clients' number given is positive
	if (customers <= 0) {
		printf("ERROR. Number of clients should be positive");
		exit(-1);
	}
	
	// check that seed is positive
	if (seed = 0) {
		printf("ERROR. Seed should be positive");
		exit(-1);
	}
	// initialize mutex and cond.
	int mutexTel = pthread_mutex_init(&lock_tel, NULL);
	int mutexCook = pthread_mutex_init(&lock_cook, NULL);
	int mutexOven = pthread_mutex_init(&lock_oven, NULL);
	int mutexPack = pthread_mutex_init(&lock_pack, NULL);
	int mutexDeliverer = pthread_mutex_init(&lock_deliverer, NULL);
	
	if (mutexTel != 0 || mutexCook != 0 || mutexOven !=0 || mutexPack != 0 || mutexDeliverer != 0) {
		printf("Error on initializing mutexes");
		exit(-1);
	}
	
	int condTel = pthread_cond_init(&cond_tel, NULL);
	int condCook = pthread_cond_init(&cond_cook, NULL);
	int condOven = pthread_cond_init(&cond_oven, NULL);
	int condPack = pthread_cond_init(&cond_pack, NULL);
	int condDeliverer = pthread_cond_init(&cond_deliverer, NULL);
	
	if (condTel != 0 || condCook != 0 || condOven !=0 || condPack != 0 || condDeliverer != 0) {
		printf("Error on initializing conds");
		exit(-1);
	}
	
	pthread_t *customersThreads;
	customersThreads = malloc(customers * sizeof(pthread_t));
	
	if (customersThreads == NULL) {
		printf("Not enough memory!\n");
		exit(-1);
	}
	
	int id;
	int idArray[customers];
	
	for(id = 0; id < customers; id++) {
    	
		idArray[id] = id +1;
		// create thread
    		rc = pthread_create(&customersThreads[id], NULL, order, &idArray[id]);

		// check that thread was created correct
    		if (rc != 0) {
    			printf("ERROR; return code from pthread_create() is %d\n", rc);
       		exit(-1);
       	}
       	
       	int wait = rand_r(&seed)%T_ORDERHIGH + T_ORDERLOW;
       	sleep(wait);	
    	}

	void *status;
	for (id = 0; id < customers; id++) {
		rc = pthread_join(customersThreads[id], &status);
		
		if (rc != 0) {
			printf("ERROR: return code from pthread_join() is %d\n", rc);
			exit(-1);
		}	
	}
	
	pthread_mutex_destroy(&lock_tel);
	pthread_cond_destroy(&cond_tel);
	
	pthread_mutex_destroy(&lock_cook);
	pthread_cond_destroy(&cond_cook);
	
	pthread_mutex_destroy(&lock_oven);
	pthread_cond_destroy(&cond_oven);
	
	pthread_mutex_destroy(&lock_pack);
	pthread_cond_destroy(&cond_pack);
	
	pthread_mutex_destroy(&lock_deliverer);
	pthread_cond_destroy(&cond_deliverer);
	
	free(customersThreads);
	free(status);
	
	printf("Total income: %d\n", income);
	printf("Succeed orders: %d\n", succeed_orders);
	printf("Failed orders: %d\n", failed_orders);
	

	printf("Average time of waiting: %21lf minutes\n", totalTimeForWaiting / customers);
	printf("Maximum time of waiting: %21lf minutes\n", maxTimeForWaiting);
	
	printf("Average time of serving: %21lf minutes\n", totalTimeForServing / customers);
	printf("Maximum time of serving: %21lf minutes\n", maxTimeForServing);
	
	printf("Average time of getting pizzas cold: %21lf minutes\n", totalTimeForCold / customers);
	printf("Maximum time of getting pizzas cold: %21lf minutes\n", maxTimeForCold);
	
	return 1;
}
