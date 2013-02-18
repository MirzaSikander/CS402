#include <assert.h>
/* FreeBSD */
#define _WITH_GETLINE
#include <stdio.h>
#include <sys/types.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include "window.h"
#include "db.h"
#include "words.h"
#include <pthread.h>
#include <sys/time.h>
#include <time.h>
#include <signal.h>

/* the encapsulation of a client thread, i.e., the thread that handles
 * commands from clients */
typedef struct Client {
	pthread_t thread;
	window_t *win;
} client_t;

typedef struct ClientNode{
    struct ClientNode *next;
    client_t *client;
}clientNode_t;

int started; 
clientNode_t* top;

pthread_mutex_t perm_mutex;
pthread_cond_t perm_given_cv;
int permission;

/* Interface with a client: get requests, carry them out and report results */
void *client_run(void *);
/* Interface to the db routines.  Pass a command, get a result */
int handle_command(char *, char *, int len);

/*
 * Create an interactive client - one with its own window.  This routine
 * creates the window (which starts the xterm and a process under it.  The
 * window is labelled with the ID passsed in.  On error, a NULL pointer is
 * returned and no process started.  The client data structure returned must be
 * destroyed using client_destroy()
 */
client_t *client_create(int ID) {
    client_t *new_Client = (client_t *) malloc(sizeof(client_t));
    char title[16];

    if (!new_Client) return NULL;

    sprintf(title, "Client %d", ID);

    /* Creates a window and set up a communication channel with it */
    if ((new_Client->win = window_create(title))) return new_Client;
    else {
	free(new_Client);
	return NULL;
    }
}

/*
 * Create a client that reads cmmands from a file and writes output to a file.
 * in and out are the filenames.  If out is NULL then /dev/stdout (the main
 * process's standard output) is used.  On error a NULL pointer is returned.
 * The returned client must be disposed of using client_destroy.
 */
client_t *client_create_no_window(char *in, char *out) {
    char *outf = (out) ? out : "/dev/stdout";
    client_t *new_Client = (client_t *) malloc(sizeof(client_t));
    if (!new_Client) return NULL;

    /* Creates a window and set up a communication channel with it */
    if( (new_Client->win = nowindow_create(in, outf))) return new_Client;
    else {
	free(new_Client);
	return NULL;
    }
}

/*
 * Destroy a client created with either client_create or
 * client_create_no_window.  The cient data structure, the underlying window
 * (if any) and process (if any) are all destroyed and freed, and any open
 * files are closed.  Do not access client after calling this function.
 */
void client_destroy(client_t *client) {
	/* Remove the window */
	window_destroy(client->win);
	free(client);
}

/* Code executed by the client */
void *client_run(void *arg)
{
    pthread_mutex_lock(&perm_mutex);
    if(!permission)
        pthread_cond_wait(&perm_given_cv, &perm_mutex);
    pthread_mutex_unlock(&perm_mutex);

	client_t *client = (client_t *) arg;

	/* main loop of the client: fetch commands from window, interpret
	 * and handle them, return results to window. */
	char *command = 0;
	size_t clen = 0;
	/* response must be empty for the first call to serve */
	char response[256] = { 0 };

	/* Serve until the other side closes the pipe */
	while (serve(client->win, response, &command, &clen) != -1) {
        pthread_mutex_lock(&perm_mutex);
        if(!permission)
            pthread_cond_wait(&perm_given_cv,&perm_mutex);
        pthread_mutex_unlock(&perm_mutex);
	    handle_command(command, response, sizeof(response));
	}
    started--;
    pthread_exit(NULL);
}

int handle_command(char *command, char *response, int len) {
    if (command[0] == EOF) {
	strncpy(response, "all done", len - 1);
	return 0;
    }
    interpret_command(command, response, len);
    return 1;
}
void stackInsert(client_t *arg){
    clientNode_t * newClient = (clientNode_t *) malloc(sizeof(clientNode_t)); 
    newClient->client = arg;
    if(top == NULL){
        newClient->next = NULL;
        top = newClient;
    }else{
        newClient->next = top;
        top = newClient;
    }
    
    return;
}
void deleteStack(){
   clientNode_t * current = top;
   while(current!=NULL){
        current = current->next;       
        free(top);
        top = current;
   }
   return;
}
void client_init(char type, char* inputfile, char* outputfile){
 
    client_t *c = NULL;	    /* A client to serve */
    pthread_attr_t attr;
    pthread_attr_init(&attr);
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);

    int rc;
    if(type == 'e'){
        //no need to lock started because it is not being used for any conditions
        c = client_create(started++);
    }else if(type == 'E'){
        started++;
        c = client_create_no_window(inputfile, outputfile);
    }
    if(c == NULL) {
                fprintf(stderr,"ERROR: client could not be created");
                exit(-1);
    }
    rc = pthread_create(&(c->thread),&attr, client_run,(void *)c);
    if (rc) {
        fprintf(stderr,"ERROR; return code from pthread_create() is %d\n", rc);
        exit(-1);
    }

    stackInsert(c);
    pthread_attr_destroy(&attr);
    return;
}

void stop_all_clients(){
    pthread_mutex_lock(&perm_mutex);
    permission = 0;
    pthread_mutex_unlock(&perm_mutex);
    return; 
} 

void allow_all_clients(){
    pthread_mutex_unlock(&perm_mutex);
    permission = 1;
    pthread_cond_broadcast(&perm_given_cv);
    pthread_mutex_unlock(&perm_mutex);
    return;
}
    

void server_wait_for_clients(){
    clientNode_t * current = top;
    void * status;
    while(current!=NULL){
        int rc;
        rc= pthread_join(current->client->thread, &status);
        if (rc) {
             fprintf(stderr,"ERROR: return code from pthread_join() is %d\n", rc);
             exit(-1);
        }
        client_destroy(current->client);
        current = current->next;
    }

    deleteStack();
    fprintf(stdout,"Main: All threads have finished executing\n");
    return;
}

int main(int argc, char *argv[]) {
    started = 0;	    /* Number of clients started */
    pthread_mutex_init(&perm_mutex, NULL);
    pthread_cond_init (&perm_given_cv, NULL);
    permission = 0;// denied by default

    if (argc != 1) {
	fprintf(stderr, "Usage: server\n");
	exit(1);
    }

    char c;
    for(;;){ 
        printf(">");
        scanf("%c",&c);
        if(c == 'e'){
            client_init(c,NULL,NULL);
            fprintf(stdout, "Created a new interactive client \n");
        }else if(c == 'E'){
            char input[4096];
            char output[4096];
            scanf("%s",input); 
            scanf("%s",output);
            client_init(c,input,output);
            fprintf(stdout, "Created a new automated client \n");
        }else if(c == 's'){
            if(permission){
                stop_all_clients();
                fprintf(stdout, "Stopped all clients\n");
            }else{
                fprintf(stdout, "Clients were already stopped. This command had no effect\n");
            }
        }else if(c == 'g'){
            if(!permission){
                allow_all_clients();
                fprintf(stdout, "Clients have now started\n");
            }else
                fprintf(stdout, "Clients are already executing\n");
        }else if(c == 'w'){
             if(!permission){
                allow_all_clients();
                fprintf(stdout, "Clients have now started\n");
            }else
                fprintf(stdout, "Clients are already executing\n");
            fprintf(stdout, "Wrapping up. No more commands will be taken now\n");
            break;
        }else{
            printf("WRONG INPUT. PLEASE CHOOSE FROM e E s g w");
        }
        getchar();
    }
    server_wait_for_clients(); 
    fprintf(stderr, "Terminating.\n");
    /* Clean up the window data */
    window_cleanup();
    pthread_mutex_destroy(&perm_mutex);
    pthread_cond_destroy(&perm_given_cv);
    pthread_exit(NULL);
}
