#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <stdio.h>

typedef char ALIGN[16];

union header {
  struct {
    size_t size;
    unsigned is_free;
    union header *next;
  } s;
  ALIGN stub;
};

typedef union header header_t;

header_t *head, *tail;

pthread_mutex_t malloc_lock;

header_t *get_free_block(size_t size) {
  header_t *curr = head;
  while(curr) {
    if (curr->s.is_free && curr->s.size >= size)
      return curr; curr = curr->s.next; }
  return NULL;
}

void *malloc(size_t size) {
  size_t total_size;
  void *block;
  header_t *header;
  if (!size)
    return NULL;

  pthread_mutex_lock(&malloc_lock);
  
  header = get_free_block(size);
  if (header) {
    header->s.is_free = 0;
    pthread_mutex_unlock(&malloc_lock);
    return (void*)(header + 1);
  }
  total_size = sizeof(header_t) + size;

  block = sbrk(total_size);
  if (block == (void*) -1) {
    pthread_mutex_unlock(&malloc_lock);
    return NULL;
  }

  header = block;
  header->s.size = size;
  header->s.is_free = 0;
  header->s.next = NULL;
  if (!head)
    head = header;
  if (tail)
    tail->s.next = header;
  tail = header;
  pthread_mutex_unlock(&malloc_lock);

  return (void*)(header + 1);
}

void print_mem() {
  header_t *curr = head;
  printf("head = %p, tail = %p \n", (void*)head, (void*)tail);
  while(curr) {
    printf("addr = %p, size = %zu, is_free=%u, next=%p\n",
        (void*)curr, curr->s.size, curr->s.is_free, (void*)curr->s.next);
    curr = curr->s.next;
  }
}
