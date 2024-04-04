#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <curses.h>
#include<unistd.h>


void* printThread(void* arg) {
    // Wait for a short while
    sleep(0.5); // Sleep for 0.5 seconds
    
    // Acquire curses lock and print
    pthread_mutex_lock((pthread_mutex_t*)arg);
    printw("Message from another thread!\n");
    refresh();
    pthread_mutex_unlock((pthread_mutex_t*)arg);

    return NULL;
}

int main() {
    // Initialize curses
    initscr();
    pthread_t thread;
    pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
    
    // Create a separate thread
    pthread_create(&thread, NULL, printThread, (void*)&mutex);
    
    // Acquire curses lock and print
    pthread_mutex_lock(&mutex);
    printw("Waiting for input...\n");
    refresh();
    pthread_mutex_unlock(&mutex);
    
    // Wait for user input
    getch();
    
    // Clean up
    endwin();
    pthread_join(thread, NULL);

    return 0;
}
