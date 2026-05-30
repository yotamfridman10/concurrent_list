#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
#include <stdbool.h>
#include "concurrent_list.h"

void down_active_threads(list* list);

struct node {
  int value;
  struct node* next;
  pthread_mutex_t lock;
};

struct list {
  struct node* head;
  pthread_mutex_t head_lock;
  bool is_destroyed; 
  int active_threads;
};

void print_node(node* node)
{
  // DO NOT DELETE
  if(node)
  {
    printf("%d ", node->value);
  }
}

list* create_list()
{
  struct list* my_list = (struct list*)malloc(sizeof(struct list));
  if (!my_list) {
    return NULL;
  }
  my_list->head = NULL;
  my_list->is_destroyed = false;
  my_list->active_threads = 0;
  pthread_mutex_init(&my_list->head_lock, NULL);
  return my_list;
}

void delete_list(list* list)
{
  if (!list) return;

  pthread_mutex_lock(&list->head_lock);
  list->is_destroyed = true; 
  pthread_mutex_unlock(&list->head_lock);

  while (true) {
    pthread_mutex_lock(&list->head_lock);
    if (list->active_threads == 0) {
      pthread_mutex_unlock(&list->head_lock);
      break;
    }
    pthread_mutex_unlock(&list->head_lock);
    sched_yield(); 
  }

  struct node* current = list->head;
  while (current) {
    struct node* next_node = current->next;
    pthread_mutex_destroy(&current->lock);
    free(current);
    current = next_node;
  }
  list->head = NULL;

  pthread_mutex_destroy(&list->head_lock);
  free(list);
}

void insert_value(list* list, int value)
{
  if(!list){
    return;
  }
  
  pthread_mutex_lock(&list->head_lock);
  if (list->is_destroyed){
    pthread_mutex_unlock(&list->head_lock); 
    return; 
  }
  list->active_threads++; 
  pthread_mutex_unlock(&list->head_lock);

  struct node* new_node = (struct node*)malloc(sizeof(struct node));
  if (!new_node) {
    down_active_threads(list);
    return;
  }

  new_node->value = value;
  new_node->next = NULL;
  pthread_mutex_init(&new_node->lock, NULL);

  pthread_mutex_lock(&list->head_lock);
  if (!list->head){
    list->head = new_node;
    pthread_mutex_unlock(&list->head_lock);

    down_active_threads(list);
    return;
  }

  if (list->head->value >= value){
    new_node->next = list->head;
    list->head = new_node;
    pthread_mutex_unlock(&list->head_lock);

    down_active_threads(list);
    return;
  }

  struct node* p = list->head;
  pthread_mutex_lock(&p->lock);
  pthread_mutex_unlock(&list->head_lock);

  struct node* q = NULL;
  while(p->next){
    q = p->next;
    pthread_mutex_lock(&q->lock);
    if (q->value >= value) {
      new_node->next = q;
      p->next = new_node;
      pthread_mutex_unlock(&p->lock);
      pthread_mutex_unlock(&q->lock);

      down_active_threads(list);
      return;
    }
    pthread_mutex_unlock(&p->lock);
    p = q;
  }
  
  p->next = new_node;
  pthread_mutex_unlock(&p->lock);

  down_active_threads(list);
}

void remove_value(list* list, int value)
{
  if(!list){
    return;
  }

  pthread_mutex_lock(&list->head_lock);
  if (list->is_destroyed || !list->head){
    pthread_mutex_unlock(&list->head_lock);
    return;
  }
  list->active_threads++;
  
  struct node* p = list->head;
  pthread_mutex_lock(&p->lock);

  if (p && p->value == value){
    list->head = p->next;
    pthread_mutex_unlock(&list->head_lock);
    pthread_mutex_unlock(&p->lock);
    pthread_mutex_destroy(&p->lock);
    free(p);
    down_active_threads(list);
    return;
  }

  pthread_mutex_unlock(&list->head_lock);

  struct node* q = NULL;
  while(p && p->next && p->next->value < value){
    pthread_mutex_lock(&p->next->lock);
    q = p;
    p = p->next;
    pthread_mutex_unlock(&q->lock);
  }
  
  if (p && p->next && p->next->value == value){
    pthread_mutex_lock(&p->next->lock);
    struct node* to_delete = p->next; 
    if (to_delete->next){
      pthread_mutex_lock(&to_delete->next->lock);
      p->next = to_delete->next;
      pthread_mutex_unlock(&to_delete->next->lock);
    }
    else{
      p->next = NULL;
    }
    pthread_mutex_unlock(&to_delete->lock);
    pthread_mutex_destroy(&to_delete->lock);
    free(to_delete);
    pthread_mutex_unlock(&p->lock);
    down_active_threads(list);
    return;
  }

  pthread_mutex_unlock(&p->lock);
  down_active_threads(list);
}

void print_list(list* list)
{
  if(!list){
    printf("\n"); // DO NOT DELETE
    return;
  }

  pthread_mutex_lock(&list->head_lock);
  if (list->is_destroyed){
    pthread_mutex_unlock(&list->head_lock); 
    return; 
  }
  list->active_threads++; 
  
  struct node* p = list->head;
  if (!p){
    printf("\n"); // DO NOT DELETE
    pthread_mutex_unlock(&list->head_lock);
    down_active_threads(list);
    return;
  }
  pthread_mutex_lock(&p->lock);
  pthread_mutex_unlock(&list->head_lock);

  while(p){
    printf("%d ", p->value);
    if (p->next)
      pthread_mutex_lock(&p->next->lock);
    pthread_mutex_unlock(&p->lock);
    p = p->next;
  }

  down_active_threads(list);
  printf("\n"); // DO NOT DELETE
}

void count_list(list* list, int (*predicate)(int))
{
  int count = 0; // DO NOT DELETE

  if(!list){
    printf("%d items were counted\n", count); // DO NOT DELETE
    return;
  }

  pthread_mutex_lock(&list->head_lock);
  if (list->is_destroyed){
    pthread_mutex_unlock(&list->head_lock); 
    return; 
  }
  list->active_threads++; 
  
  struct node* p = list->head;
  if (p){
    pthread_mutex_lock(&p->lock);
  }
  pthread_mutex_unlock(&list->head_lock);

  while(p){
    count += predicate(p->value);
    if (p->next)
      pthread_mutex_lock(&p->next->lock);
    pthread_mutex_unlock(&p->lock);
    p = p->next;
  }

  printf("%d items were counted\n", count); // DO NOT DELETE

  down_active_threads(list);
}


void down_active_threads(list* list){
  pthread_mutex_lock(&list->head_lock);
  list->active_threads--;
  pthread_mutex_unlock(&list->head_lock);
}
