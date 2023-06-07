/* This is the only file you will be editing.
 * - Copyright of Starter Code: Prof. Kevin Andrea, George Mason University.  All Rights Reserved
 * - Copyright of Student Code: You!  
 * - Restrictions on Student Code: Do not post your code on any public site (eg. Github).
 * -- Feel free to post your code on a PRIVATE Github and give interviewers access to it.
 * - Date: Aug 2022
 */

/* Fill in your Name and GNumber in the following two comment fields
 * Name:Amanjot Dhillon
 * GNumber:G01032809
 */

// System Includes
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/time.h>
#include <pthread.h>
#include <sched.h>
#include <stdbool.h>
// Local Includes
#include "avan_sched.h"
#include "vm_support.h"
#include "vm_process.h"

/* Feel free to create any helper functions you like! */

/* Sets the ready state to on, return true if successful*/
int set_ready_flag(int flags, bool val) {
	if (val) {
		flags = (flags | (1<<0));
		flags = (flags & (~(1 << 1)));
		flags = (flags & (~(1 << 2)));
	} else {
		int mask = ~(1<<0);
		flags = flags&mask;
	}
	return flags;
}
/* Sets the suspended state to on, return true if successful*/
int set_suspend_flag(int flags, bool val) {
	if (val) {
		flags = (flags | (1<<1));
		flags = (flags & (~(1 << 0)));
		flags = (flags & (~(1 << 2)));
	} else {
		int mask = ~(1<<1);
		flags = flags&mask;
	}
	return flags;
}
/* Sets the terminated  state to on, return true if successful*/
int set_terminate_flag(int flags, bool val) {
	if (val) {
		flags = (flags | (1<<2));
		flags = (flags & (~(1 << 0)));
		flags = (flags & (~(1 << 1)));
	} else {
		int mask = ~(1<<2);
		flags = flags&mask;
	}
	return flags;
}
/* Sets the critical state to on, return true if successful*/
int set_critical_flag(int flags, bool val) {
	if (val) {
		flags = (flags | (1<<3));
	} else {
		int mask = ~(1<<3);
		flags = flags&mask;
	}
	return flags;
}
/*checks the status of critical flag, returns true if not equal to 0*/
bool critical_status(int flags) {
	int status = (flags>>3)&1;
	if(status != 0) {
		return true;
	}
	else 
		return false;
}
/*sets the exit code and returns flags*/
int set_exit_code(int flags, int code) {
	int mask = 0xFFFFFFF0;
	flags= (code&mask)|flags;

	return flags;
}
/* find the process based upon given pid returns the process if found*/
process_node_t* queue_find(queue_header_t *queue, pid_t pid) {
	process_node_t *temp;
	temp=queue->head;
	if(temp==NULL){
		return NULL;
	}
	while(temp->pid!=pid){
		temp=temp->next;
		if(temp==NULL){
			return NULL;
		}
	}
	return temp;
}
/*finds the highest prioirity process in a given queue and returns it prioirity value*/
int highest_priorty(queue_header_t *queue) {
	process_node_t *process;
	int lp=0;
	process=queue->head;
	if(queue->head!=NULL) {
		lp=process->priority;
	}
	while(process!=NULL) {
		if(process->next!=NULL &&(process->next->priority < lp)){
			lp=process->next->priority;
		}
		process=process->next;
	}
	return lp;
}  
/*increments skips after process has been skipped returns 0 if succesful*/
int reset_skips (queue_header_t *queue) {
	process_node_t *process;
	process=queue->head;
	if(queue==NULL){
		return -1;
	}

	while(process!=NULL){
		process->skips++;
		process=process->next;
	}

	return 0;
}
/*inserts function in ascending order in queue, returns 0 if sucessful*/
int ascending_order(queue_header_t *queue, process_node_t *process, pid_t pid) {
	process_node_t *curr;
	process_node_t *prev; 
	curr=queue->head;

	if(curr==NULL){
		queue->head = process;
		queue->count++;
		return 0;
	}

	if(curr->pid > pid){
		queue->head = process;
		process->next = curr;
		queue->count++;
		return 0;
	}

	prev = curr;
	curr = curr->next;

	while(curr!=NULL && curr->pid < process->pid){
		prev=curr;
		curr=prev->next;
	}
	prev->next=process;
	process->next=curr;
	queue->count++;
	return 0;

}

/*removes a process in queue returns 0 on success*/
int queue_remove(queue_header_t *queue, pid_t pid){
	process_node_t *curr;
	curr=queue->head;
	if(curr==NULL){
		return -1;
	}
	if(curr->next == NULL) {
		if (curr->pid == pid) {
			queue->head= NULL;
			queue->count--;
			curr->next = NULL;
			return 0;
		}
		return -1;
	}
	if (curr->pid == pid) {
		queue->head= curr->next;
		queue->count--;
		curr->next = NULL;
		return 0;
	}
	while(curr->next->pid!=pid){
		curr=curr->next;
		if(curr->next==NULL){
			return -1;
		}
	}
	process_node_t *node_to_delete;
	node_to_delete= curr->next;
	curr->next = node_to_delete->next;
	node_to_delete->next = NULL;
	queue->count--;

	return 0;

}
/*frees all processes for a given queue, returns 0 on sucess*/
process_node_t* free_processes(queue_header_t *queue){
	process_node_t *process;
	process_node_t *nextProcess;
	process=queue->head;
	while(process!=NULL){
		nextProcess=process->next;
		free(process);
		process=nextProcess;
	}

	return 0;
}

/* Initialize the avan_header_t Struct
 * Follow the specification for this function.
 * Returns a pointer to the new avan_header_t or NULL on any error.
 */
avan_header_t* avan_create() {
	avan_header_t* new_avan_header;
	new_avan_header = (avan_header_t *) malloc(sizeof(avan_header_t));
	if(new_avan_header == NULL) {
		fflush(stdout);
		return NULL;
	}
	new_avan_header->ready_queue=malloc(sizeof(queue_header_t));
	if(new_avan_header->ready_queue == NULL) {
		fflush(stdout);
		return NULL;
	}
	new_avan_header->ready_queue->count=0;
	new_avan_header->ready_queue->head=NULL;
	new_avan_header->suspended_queue=malloc(sizeof(queue_header_t));
	if(new_avan_header->suspended_queue == NULL) {
		fflush(stdout);
		return NULL;
	}
	new_avan_header->suspended_queue->count=0;
	new_avan_header->suspended_queue->head=NULL;
	new_avan_header->terminated_queue=malloc(sizeof(queue_header_t));
	if(new_avan_header->terminated_queue == NULL) {
		fflush(stdout);
		return NULL;
	}
	new_avan_header->terminated_queue->count=0;
	new_avan_header->terminated_queue->head=NULL;
	return new_avan_header;
}

/* Adds a process into the appropriate singly linked list.
 * Follow the specification for this function.
 * Returns a 0 on success or a -1 on any error.
 */
int avan_insert(avan_header_t *header, process_node_t *process) {
	pid_t pid;
	pid=process->pid;
	if(header==NULL || process==NULL){
		return -1;
	}

	process->flags=set_ready_flag(process->flags, true);
	ascending_order(header->ready_queue, process, pid);
	return 0; // Replace Me with Your Code!
}

/* Move the process with matching pid from Ready to Stopped.
 * Follow the specification for this function.
 * Returns a 0 on success or a -1 on any error.
 */
int avan_suspend(avan_header_t *header, pid_t pid) {
	process_node_t *process;
	process=queue_find(header->ready_queue, pid);
	if(header==NULL|| process==NULL){
		return -1;
	}
	process->flags=set_suspend_flag(process->flags, true);
	ascending_order(header->suspended_queue, process, pid);
	return 0; // Replace Me withi Your Code!
}
/* Move the process with matching pid from Suspended to Ready queue.
 * Follow the specification for this function.
 * Returns a 0 on success or a -1 on any error (such as process not found).
 */
int avan_resume(avan_header_t *header, pid_t pid) {

	process_node_t *process; 
	process= queue_find(header->suspended_queue, pid);
	if (process == NULL || process==NULL) {
		return -1;
	}
	queue_remove(header->suspended_queue, pid);
	process->flags=set_ready_flag(process->flags, true);
	ascending_order(header->ready_queue, process,pid);
	return 0; 
}
/* Insert the process in the Terminated Queue and add the Exit Code to it.
 * Follow the specification for this function.
 * Returns a 0 on success or a -1 on any error.
 */
int avan_quit(avan_header_t *header, process_node_t *node, int exit_code) {
	pid_t pid;
	pid=node->pid;
	node->flags=set_terminate_flag(node->flags, true);
	node->flags=set_exit_code(node->flags,exit_code);
	ascending_order(header->terminated_queue, node, pid);
	return 0;
}

/* Move the process with matching pid from Ready to Terminated and add the Exit Code to it.
 * Follow the specification for this function.
 * Returns its exit code (from flags) on success or a -1 on any error.
 */
int avan_terminate(avan_header_t *header, pid_t pid, int exit_code) {
	process_node_t *process;
	process=queue_find(header->ready_queue, pid);
	if(process!=NULL){
		queue_remove(header->ready_queue, pid);
		process->flags=set_terminate_flag(process->flags, true);
		process->flags=set_exit_code(process->flags, exit_code);
		ascending_order(header->terminated_queue, process, pid);
		return exit_code;
		process=queue_find(header->suspended_queue, pid);	
	}else if(process!=NULL){
		queue_remove(header->suspended_queue, pid);
		process->flags=set_terminate_flag(process->flags, true);
		process->flags=set_exit_code(process->flags, exit_code);
		ascending_order(header->terminated_queue, process, pid);
		return exit_code;
	}else
		return -1;
}

/* Create a new process_node_t with the given information.
 * - Malloc and copy the command string, don't just assign it!
 * Follow the specification for this function.
 * Returns the process_node_t on success or a NULL on any error.
 */
process_node_t *avan_new_process(char *command, pid_t pid, int priority, int critical) {
	process_node_t *process;
	process=malloc(sizeof(process_node_t));
	if(process == NULL) {
		fflush(stdout);
		return NULL;
	}
	strncpy(process->cmd,command,MAX_CMD);
	process->next=NULL;
	process->flags=set_ready_flag(process->flags, true);
	process->priority=priority;
	process->skips=0;
	if(critical==0){
		process->flags=set_critical_flag(process->flags,false);
	}
	if(critical!=0){
		process->flags=set_critical_flag(process->flags,true);
	}
	process->flags=set_exit_code(process->flags, 0);
	process->pid=pid;
	return process; // Replace Me with Your Code!
}

/* Schedule the next process to run from Ready Queue.
 * Follow the specification for this function.
 * Returns the process selected or NULL if none available or on any errors.
 */
process_node_t *avan_select(avan_header_t *header) {
	queue_header_t *ready_queue;
	process_node_t *curr;
	process_node_t *best;
	ready_queue=header->ready_queue;
	int lp=0;

	curr=header->ready_queue->head;
	if(curr!=NULL && critical_status(curr->flags)) {
		best=curr;
		header->ready_queue->head = curr->next;
		best->next=NULL;
		best->skips=0;
		reset_skips(ready_queue);
		header->ready_queue->count--;
		return best;
	} else{
		while(curr!=NULL) {
			if(curr->next!=NULL && (critical_status(curr->next->flags))) {
				best = curr->next;
				curr->next=curr->next->next;
				best->next=NULL;
				best->skips=0;
				reset_skips(ready_queue);
				header->ready_queue->count--;
				return best;
			}
			curr=curr->next;
		}
	}

	curr = header->ready_queue->head;
	if(curr!=NULL && curr->skips >=MAX_SKIPS) {
		best=curr;
		header->ready_queue->head = curr->next;
		best->next=NULL;
		best->skips=0;
		reset_skips(ready_queue);
		header->ready_queue->count--;
		return best;
	} else {
		while(curr!=NULL) {
			if(curr->next!=NULL && (curr->next->skips >=MAX_SKIPS)) {
				best = curr->next;
				curr->next=curr->next->next;
				best->next=NULL;
				best->skips=0;
				reset_skips(ready_queue);
				header->ready_queue->count--;
				return best;
			}
			curr=curr->next;
		}
	}

	lp=highest_priorty(ready_queue);
	curr = header->ready_queue->head;
	if(curr!=NULL && curr->priority == lp) {
		best=curr;
		header->ready_queue->head = curr->next;
		best->next=NULL;
		best->skips=0;
		reset_skips(ready_queue);
		header->ready_queue->count--;
		return best;	
	} else {
		while(curr!=NULL) {
			if(curr->next!=NULL && (curr->next->priority ==lp)) {
				best = curr->next;
				curr->next=curr->next->next;
				best->next=NULL;
				best->skips=0;
				reset_skips(ready_queue);
				header->ready_queue->count--;
				return best;

			}	
			curr=curr->next;
		}
	}

	best=best;	
	return NULL;
}

/* Returns the number of items in a given queue_header_t
 * Follow the specification for this function.
 * Returns the number of processes in the list or -1 on any errors.
 */
int avan_get_size(queue_header_t *ll) {
	return ll->count;
}

/* Frees all allocated memory in the avan_header_tr */
void avan_cleanup(avan_header_t *header) {
	queue_header_t *ready_queue;
	ready_queue=header->ready_queue;
	queue_header_t *suspended_queue;
	suspended_queue=header->suspended_queue;
	queue_header_t *terminated_queue;
	terminated_queue=header->terminated_queue;

	header->ready_queue->head=free_processes(ready_queue);
	header->suspended_queue->head=free_processes(suspended_queue);
	header->terminated_queue->head=free_processes(terminated_queue);
	free(ready_queue);
	free(suspended_queue);
	free(terminated_queue);
	free(header);
	return; // Replace Me with Your Code!
}
