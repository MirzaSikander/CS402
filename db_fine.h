#include <pthread.h>

typedef struct Node {
	char *name;
	char *value;
	struct Node *lchild;
	struct Node *rchild;
    pthread_mutex_t r_mutex;
    pthread_cond_t waitOnR_cv;
    int readers_count;
    pthread_mutex_t w_mutex;
} node_t;

extern node_t head;

void interpret_command(char *, char *, int);

void destroyDBMutex();
