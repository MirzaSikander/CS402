#include "db_fine.h"
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <assert.h>

/* Forward declaration */
node_t *search(char *, node_t *, node_t **);

node_t head = { "",
                "",
                0,
                0,
                PTHREAD_MUTEX_INITIALIZER,
                PTHREAD_COND_INITIALIZER,
                0,
                PTHREAD_MUTEX_INITIALIZER
};

void EnterAsReader(node_t* node){
    if(node != NULL){ 
        pthread_mutex_lock(&(node->w_mutex));
        pthread_mutex_unlock(&(node->w_mutex));

        pthread_mutex_lock(&(node->r_mutex));
        node->readers_count++;
        pthread_mutex_unlock(&(node->r_mutex));
    }
    return;
}

void LeaveAsReader(node_t* node){
    if(node != NULL){
        pthread_mutex_lock(&(node->r_mutex)); node->readers_count--; 
        if(node->readers_count == 0){
            pthread_cond_broadcast(&(node->waitOnR_cv));
        }
        pthread_mutex_unlock(&(node->r_mutex));
    }
    return;
}

void EnterAsWriter(node_t* node){
    if(node != NULL){
        for(;;){
            pthread_mutex_lock(&(node->w_mutex));
            pthread_mutex_lock(&(node->r_mutex));
            if(node->readers_count > 0){
                pthread_mutex_unlock(&(node->w_mutex));
                pthread_cond_wait(&(node->waitOnR_cv), &(node->r_mutex));
                pthread_mutex_unlock(&(node->r_mutex));
                continue;
            }
            pthread_mutex_unlock(&(node->r_mutex));
            break;
        }
    }
    return;
}

void LeaveAsWriter(node_t* node){
    if(node != NULL)
    pthread_mutex_unlock(&(node->w_mutex));
    return;
}
/*
 * Allocate a new node with the given key, value and children.
 */
node_t *node_create(char *arg_name, char *arg_value, node_t * arg_left,
	node_t * arg_right) {
    node_t *new_node;

    new_node = (node_t *) malloc(sizeof(node_t));
    if (!new_node) return NULL;

    if (!(new_node->name = (char *)malloc(strlen(arg_name) + 1))) {
	free(new_node);
	return NULL;
    }

    if (!(new_node->value = (char *)malloc(strlen(arg_value) + 1))) {
	free(new_node->name);
	free(new_node);
	return NULL;
    }

    strcpy(new_node->name, arg_name);
    strcpy(new_node->value, arg_value);
    new_node->lchild = arg_left;
    new_node->rchild = arg_right;
    //initializing the locking mechanism 
    pthread_mutex_init(&(new_node->r_mutex), NULL);
    pthread_mutex_init(&(new_node->w_mutex),NULL);
    pthread_cond_init(&(new_node->waitOnR_cv), NULL);
    new_node->readers_count = 0;
    return new_node;
}

/* Free the data structures in node and the node itself. */
void node_destroy(node_t * node) {
    /* Clearing name and value after they are freed is defensive programming in
     * case the node_destroy is called again. */
    if (node->name) {free(node->name); node->name = NULL; }
    if (node->value) { free(node->value); node->value = NULL; }
    pthread_mutex_destroy(&node->r_mutex);
    pthread_mutex_destroy(&node->w_mutex);
    pthread_cond_destroy(&node->waitOnR_cv);
    free(node);
}

/* Find the node with key name and return a result or error string in result.
 * Result must have space for len characters. */
void query(char *name, char *result, int len) {
    node_t *target;
    //need to lock head before sending it as a argument to search
    EnterAsReader(&head);
    //target came already locked if it is not null
    target = search(name, &head, NULL);

    if (!target) {
        //target is null so no need to unlock it
        strncpy(result, "not found", len - 1);
        return;
    } else {
        //target needs to be unlocked
        strncpy(result, target->value, len - 1);
        LeaveAsReader(target);
        return;
    }
}

/* Insert a node with name and value into the proper place in the DB rooted at
 * head. */
int add(char *name, char *value) {
	node_t *parent;	    /* The new node will be the child of this node */
	node_t *target;	    /* The existing node with key name if any */
	node_t *newnode;    /* The new node to add */
    //need to lock head before sending it in
    EnterAsWriter(&head);
	if ((target = search(name, &head, &parent))) {
	    /* There is already a node with this key in the tree */
        //target and parent are already locked so need to unlock it before returning
        LeaveAsWriter(target);
        LeaveAsWriter(parent);
	    return 0;
	}
	/* No idea how this could happen, but... */
	if (!parent) return 0;

    //parent is locked as a reader
	/* make the new node and attach it to parent */
	newnode = node_create(name, value, 0, 0);

	if (strcmp(name, parent->name) < 0) parent->lchild = newnode;
	else parent->rchild = newnode;
    
    LeaveAsWriter(parent);
	return 1;
}

/*
 * When deleting a node with 2 children, we swap the contents leftmost child of
 * its right subtree with the node to be deleted.  This is used to swap those
 * content pointers without copying the data, which is unsafe if the
 * allocations are different sizes (copying "alamorgodo" into "ny" for
 * example).
 */
static inline void swap_pointers(char **a, char **b) {
    char *tmp = *b;
    *b = *a;
    *a = tmp;
}

/* Remove the node with key name from the tree if it is there.  See inline
 * comments for algorithmic details.  Return true if something was deleted. */
int xremove(char *name) {
	node_t *parent;	    /* Parent of the node to delete */
	node_t *dnode;	    /* Node to delete */
	node_t *next;	    /* used to find leftmost child of right subtree */
	node_t **pnext;	    /* A pointer in the tree that points to next so we
			       can change that nodes children (see below). */
    //Need to lock the head node before searching
    EnterAsWriter(&head);
	/* first, find the node to be removed */
	if (!(dnode = search(name, &head, &parent))) {
	    /* it's not there */
        //dnode and parent will be locked when the function returns
        LeaveAsWriter(dnode);
        LeaveAsWriter(parent);
	    return 0;
	}

	/* we found it.  Now check out the easy cases.  If the node has no
	 * right child, then we can merely replace its parent's pointer to
	 * it with the node's left child. */
	if (dnode->rchild == 0) {
	    if (strcmp(dnode->name, parent->name) < 0)
		parent->lchild = dnode->lchild;
	    else
		parent->rchild = dnode->lchild;

	    /* done with dnode */
        //no need to unlock the writers lock on dnode since nobody will be waiting on it. 
	    node_destroy(dnode);
	} else if (dnode->lchild == 0) {
	    /* ditto if the node had no left child */
	    if (strcmp(dnode->name, parent->name) < 0)
		parent->lchild = dnode->rchild;
	    else
		parent->rchild = dnode->rchild;

	    /* done with dnode */
	    node_destroy(dnode);
	} else {
	    /* So much for the easy cases ...
	     * We know that all nodes in a node's right subtree have
	     * lexicographically greater names than the node does, and all
	     * nodes in a node's left subtree have lexicographically smaller
	     * names than the node does. So, we find the lexicographically
	     * smallest node in the right subtree and replace the node to be
	     * deleted with that node. This new node thus is lexicographically
	     * smaller than all nodes in its right subtree, and greater than
	     * all nodes in its left subtree. Thus the modified tree is well
	     * formed. */

	    /* pnext is the address of the pointer which points to next (either
	     * parent's lchild or rchild) */

        EnterAsWriter(dnode->rchild);
	    pnext = &dnode->rchild;
	    next = *pnext;
        EnterAsWriter(next->lchild);
        LeaveAsWriter(dnode->rchild);
	    while (next->lchild != 0) {
		    /* work our way down the lchild chain, finding the smallest
		     * node in the subtree. */
		    pnext = &next->lchild;
            LeaveAsWriter(next);
		    next = *pnext;
            EnterAsWriter(next->lchild);
	    }
	    swap_pointers(&dnode->name, &next->name);
	    swap_pointers(&dnode->value, &next->value);
	    *pnext = next->rchild;
        LeaveAsWriter(next);

	    node_destroy(next);
    }
    LeaveAsWriter(dnode);
    LeaveAsWriter(parent);
    return 1;
}

/* Search the tree, starting at parent, for a node containing name (the "target
 * node").  Return a pointer to the node, if found, otherwise return 0.  If
 * parentpp is not 0, then it points to a location at which the address of the
 * parent of the target node is stored.  If the target node is not found, the
 * location pointed to by parentpp is set to what would be the the address of
 * the parent of the target node, if it were there.
 *
 * Assumptions:
 * parent node is reader locked, is not null and does not contain name */
node_t *search(char *name, node_t * parent, node_t ** parentpp) {
    //assumption that parent is readerlocked so that it cannot be modified while its being used
    node_t *next;
    node_t *result;
    
    if (strcmp(name, parent->name) < 0) next = parent->lchild;
    else next = parent->rchild;
    
    //lock the child which is going to be used later
    if (parentpp != 0)
        EnterAsWriter(next);
    else
        EnterAsReader(next);

    if (next == NULL) {
        result = NULL;
    } else {
        if (strcmp(name, next->name) == 0) {
            /* Note that this falls through to the if (parentpp .. ) statement
             * below. */
            result = next;
        } else {
            /* "We have to go deeper!" This recurses and returns from here
             * after the recursion has returned result and set parentpp 
             * next is locked going in*/
            
            //let go of the parent since there is no interaction with it any longer
            if (parentpp != 0)
                LeaveAsWriter(parent);
            else
                LeaveAsReader(parent);

            result = search(name, next, parentpp);
            return result;
        }
    }

    /* record a parent if we are looking for one */
    //parent and result are already locked
    //Need to unlock the parent if it is not required
    if (parentpp != 0) *parentpp = parent;
    else{
        LeaveAsReader(parent);
    }

    return (result);
}

/*
 * Parse the command in command, execute it on the DB rooted at head and return
 * a string describing the results.  Response must be a writable string that
 * can hold len characters.  The response is stored in response.
 */
void interpret_command(char *command, char *response, int len)
{
    char value[256];
    char ibuf[256];
    char name[256];

    if (strlen(command) <= 1) {
	strncpy(response, "ill-formed command", len - 1);
	return;
    }

    switch (command[0]) {
    case 'q':
	/* Query */
	sscanf(&command[1], "%255s", name);
	if (strlen(name) == 0) {
	    strncpy(response, "ill-formed command", len - 1);
	    return;
	}

	query(name, response, len);
	if (strlen(response) == 0) {
	    strncpy(response, "not found", len - 1);
	}

	return;

    case 'a':
	/* Add to the database */
	sscanf(&command[1], "%255s %255s", name, value);
	if ((strlen(name) == 0) || (strlen(value) == 0)) {
	    strncpy(response, "ill-formed command", len - 1);
	    return;
	}

	if (add(name, value)) {
	    strncpy(response, "added", len - 1);
	} else {
	    strncpy(response, "already in database", len - 1);
	}

	return;

    case 'd':
	/* Delete from the database */
	sscanf(&command[1], "%255s", name);
	if (strlen(name) == 0) {
	    strncpy(response, "ill-formed command", len - 1);
	    return;
	}

	if (xremove(name)) {
	    strncpy(response, "removed", len - 1);
	} else {
	    strncpy(response, "not in database", len - 1);
	}

	    return;

    case 'f':
	/* process the commands in a file (silently) */
	sscanf(&command[1], "%255s", name);
	if (name[0] == '\0') {
	    strncpy(response, "ill-formed command", len - 1);
	    return;
	}

	{
	    FILE *finput = fopen(name, "r");
	    if (!finput) {
		strncpy(response, "bad file name", len - 1);
		return;
	    }
	    while (fgets(ibuf, sizeof(ibuf), finput) != 0) {
		interpret_command(ibuf, response, len);
	    }
	    fclose(finput);
	}
	strncpy(response, "file processed", len - 1);
	return;

    default:
	strncpy(response, "ill-formed command", len - 1);
	return;
    }
}

void destroyDBMutex(){
}
