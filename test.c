#define _GNU_SOURCE
#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <pthread.h>
#include <unistd.h>
#include <errno.h>
#include <ctype.h>
#include <sys/time.h>
#include <sys/resource.h>
#include "concurrent_list.h"

#define CMD_BUFFER_SIZE 100
#define MAX_THREAD_COUNT 252

char* delimiters = " \n\r\t";
char* string_delimiter = "\"";
char command[CMD_BUFFER_SIZE];
char parsed_command[CMD_BUFFER_SIZE];
pthread_t threads[MAX_THREAD_COUNT];
int thread_count = 0;
list* mylist = NULL;

void* delete_list_task(void* arg)
{
	(void)arg;
	delete_list(mylist);
	mylist = NULL;
	return 0;
}

void* print_list_task(void* arg)
{
	(void)arg;
	print_list(mylist);
	return 0;
}

void* insert_value_task(void* arg)
{
	int value = (int)(intptr_t)arg;
	insert_value(mylist, value);
	return 0;
}

void* remove_value_task(void* arg)
{
	int value = (int)(intptr_t)arg;
	remove_value(mylist, value);
	return 0;
}

void* count_greater_task(void* arg)
{
	int threshold = (int)(intptr_t)arg;
	int pred(int value)
	{
		return value > threshold;
	}
	count_list(mylist, pred);
	return 0;
}


static void start_thread(void* (*task)(void*), void* arg)
{
	if (thread_count >= MAX_THREAD_COUNT)
	{
		printf("too many threads (max %d) - run 'join' first\n", MAX_THREAD_COUNT);
		return;
	}

	int rc = pthread_create(&threads[thread_count], NULL, task, arg);
	if (rc != 0)
	{
		printf("pthread_create failed: %s\n", strerror(rc));
		return;
	}
	thread_count++;
}

void parse_command(char* command, char* command_out, int* arg)
{
    int token_num = 0;
    char* token = strtok(command, delimiters);
    while (token != NULL)
    {
    	if(token_num == 0)
    	{
			strcpy(command_out, token);   		
    	}
    	else
    	{
			*arg = atoi(token);		
    	}

        token = strtok (NULL, delimiters);
        token_num++;
    }
}

int execute_command(char* command, int value)
{
	if(strcmp(command, "create_list") == 0)
	{
		mylist = create_list();
	}
	else if(strcmp(command, "delete_list") == 0)
	{
		start_thread(delete_list_task, NULL);
	}
	else if(strcmp(command, "print_list") == 0)
	{
		start_thread(print_list_task, NULL);
	}		
	else if(strcmp(command, "insert_value") == 0)
	{	
		start_thread(insert_value_task, (void*)(intptr_t)value);
	}
	else if(strcmp(command, "remove_value") == 0)
	{
		start_thread(remove_value_task, (void*)(intptr_t)value);
	}		
	else if(strcmp(command, "count_greater") == 0)
	{
		start_thread(count_greater_task, (void*)(intptr_t)value);
	}
	else if(strcmp(command, "join") == 0)
	{
		for(int i = 0; i < thread_count; i++)
		{
			pthread_join(threads[i], NULL);
		}
		thread_count = 0;
	}
	else
	{
		printf("unknown command\n");
	}				

	return 0;
}

int main(int argc, const char** argv)
{
    (void)argc;
    (void)argv;

    while (1)
    {
        memset(command, 0, CMD_BUFFER_SIZE);
        memset(parsed_command, 0, CMD_BUFFER_SIZE);

        /* Stop at end-of-input (e.g. piped script or Ctrl+D) instead of
         * spinning forever on a NULL return from fgets. */
        if (fgets(command, CMD_BUFFER_SIZE, stdin) == NULL)
        {
            break;
        }
        if(strncmp(command, "exit", 4) == 0)
        {
            break;
        }

        int value = 0;
        parse_command(command, parsed_command, &value);

      
        if (parsed_command[0] == '\0')
        {
            continue;
        }

        execute_command(parsed_command, value);
    }

    for(int i = 0; i < thread_count; i++)
    {
        pthread_join(threads[i], NULL);
    }
    thread_count = 0;

    return 0;
}