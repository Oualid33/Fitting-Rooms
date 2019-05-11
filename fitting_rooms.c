/*
 * This programs resolves the sychnronization
 * problem of fitting rooms:
 * Men and Women are sharing the same rooms.
 * rooms are labled as follows: empty, occuped by men, and occuped by women.
 * Men cannot access rooms when they are occuped by women, and vice versa.
 *
 * By Oualid Demigha, PhD
 * On May 8th, 2019
 * At EMP
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <semaphore.h>
#include <pthread.h>
#include <fcntl.h>
#include <sys/stat.h>

#define MAX_PERSONS 1000

typedef enum genre_t {M, W, N} genre_t;
typedef struct room_t {
	int id; // num. of room
	genre_t genre;  // g=M if room is occupied by men 
			// g=W if room is ocupied by women
			// g=N if room is empty
} room_t;

room_t rooms[2*MAX_PERSONS]; // rooms

typedef enum state_t {EMPTY, OCCUPED_MEN, OCCUPED_WOMEN} state_t;
int nb = 0; // number of persons (men & women) in rooms
state_t state = EMPTY; // state of the rooms, initially empty

sem_t men, women; // synch. semaphores for men and women
pthread_mutex_t mutex; // mutex to access nb and state safely

void men_enter(int i) {
	pthread_mutex_lock(&mutex);
	if (state == EMPTY || state == OCCUPED_MEN) { // access granted
		state = OCCUPED_MEN; // when empty
		rooms[nb].id = i;
		rooms[nb].genre = M;
		nb++; // inc index
		pthread_mutex_unlock(&mutex);
	} else {
		pthread_mutex_unlock(&mutex);
		sem_wait(&men); // waiting for the cabines to be emptied by women
		pthread_mutex_lock(&mutex);
		state = OCCUPED_MEN; // update state
		rooms[nb].id = i;
		rooms[nb].genre = M;
		nb++;
		pthread_mutex_unlock(&mutex);
	}
}

void women_enter(int i) {
	pthread_mutex_lock(&mutex);
	if (state == EMPTY || state == OCCUPED_WOMEN) {
		state = OCCUPED_WOMEN; // when empty
		rooms[nb].id = i;
		rooms[nb].genre = W;
		nb++; // inc index
		pthread_mutex_unlock(&mutex);
	} else {
		pthread_mutex_unlock(&mutex);
		sem_wait(&women); // waiting for the cabines to be emptied by men
		pthread_mutex_lock(&mutex);
		state = OCCUPED_WOMEN;
		rooms[nb].id = i;
		rooms[nb].genre = W;
		nb++;
		pthread_mutex_unlock(&mutex);
	}
}

void men_exit() {
	pthread_mutex_lock(&mutex);
	rooms[nb].id = -1;
	rooms[nb].genre = N;
	nb--;
	if (nb == 0) {
		state = EMPTY;
		sem_post(&women); // notify women
	}
	pthread_mutex_unlock(&mutex);
	sem_post(&men); // notify men
}

void women_exit() {
	pthread_mutex_lock(&mutex);
	rooms[nb].id = -1;
	rooms[nb].genre = N;
	nb--;
	if (nb == 0) {
		state = EMPTY;
		sem_post(&men); // notify men
	}
	pthread_mutex_unlock(&mutex);
	sem_post(&women); // notify women

}

// function for printing cabine state
void print_rooms() {
	int i;
	pthread_mutex_lock(&mutex);
	printf("Occupied rooms = %d\n", nb);
	for (i = 0; i <= nb; i++){
		if (rooms[i].id >= 0) {
			if (rooms[i].genre == M) printf("M");
			if (rooms[i].genre == W) printf("W");
		}
	}
	printf("\n");
	pthread_mutex_unlock(&mutex);
}

// Threads of men
void* mens(void* i) {
	int id = *((int*)i);
	men_enter(id);
	printf("MEN %d entered\n", id);
	print_rooms(); // should be a sequence of M's: MMMMM...M
	sleep(1.0);
	men_exit();
	printf("MEN %d exited\n", id);

	return NULL;
}

// Threads of women
void* womens(void* i) {
	int id = *((int*)i);
	women_enter(id);
	printf("WOMEN %d entered\n", id);
	print_rooms(); // should be a sequence of W's: WWWWW...W
	sleep(1.0);
	women_exit();
	printf("WOMEN %d exited\n", id);

	return NULL;
}


int main(int argc, char *argv[]) {
	int i, num_persons;
	pthread_t menT[MAX_PERSONS], womenT[MAX_PERSONS];

	sem_init(&men, 0, 0);
	sem_init(&women, 0, 0);
	pthread_mutex_init(&mutex, NULL);

	if (argc < 2) {
		printf("Usage: cabine #persons\n");
		return 1;
	}

	num_persons = atoi(argv[1]);

	for (i = 0; i < num_persons; i++) {
		rooms[i].id = -1;
		rooms[i].genre = N;
	}

	for (i = 0; i < num_persons; i++) {
		pthread_create(&menT[i], NULL, mens, &i);
		pthread_create(&womenT[i], NULL, womens, &i);
	}

	for (i = 0; i < num_persons; i++) {
		pthread_join(menT[i], NULL);
		pthread_join(womenT[i], NULL);
	}

	return 0;
}
