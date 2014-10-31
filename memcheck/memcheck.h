/*
 * A basic leak-checking utility for single-source-file projects.  Simply
 * #include this header in your project and call memcheck_report at the end
 * of your main function to get a report of any leaks.  This complies with
 * C89/90, C99 and C11, and can be used with any OS or hosted implementation.
 */

#ifndef MYMALLOC_H__
#define MYMALLOC_H__

#include <stdlib.h>
#include <string.h>

typedef struct memcheck_list memcheck_list_t;
struct memcheck_list {
  char *file;
  char *func;
  int line;
  void *address;
  size_t size;
  memcheck_list_t *next;
};

static memcheck_list_t *memcheck_list = NULL;

static char *memcheck_strdup(const char *s)
{
  char *rv = NULL;

  if (s) {
    rv = malloc(strlen(s) + 1);
    strcpy(rv, s);
  }

  return rv;
}

static void memcheck_list_add(const char *file,
                              const char *func,
                              const int line,
                              size_t size)
{
  memcheck_list_t *new_node = malloc(sizeof(*new_node));

  new_node->file = memcheck_strdup(file);
  new_node->func = memcheck_strdup(func);
  new_node->line = line;
  new_node->size = size;
  new_node->address = malloc(size);
  if (!new_node->address) {
    char buf[BUFSIZ];
    snprintf(buf, sizeof(buf), "%s:%s:%d malloc %zd bytes", file, func, line, size);
    perror(buf);
  }
  new_node->next = memcheck_list;
  memcheck_list = new_node;
}

static void memcheck_list_update(memcheck_list_t *node,
                                 const char *file,
                                 const char *func,
                                 const int line,
                                 void *address,
                                 size_t size)
{
  if (node) {
    free(node->file);
    free(node->func);
    node->file = memcheck_strdup(file);
    node->func = memcheck_strdup(func);
    node->line = line;
    node->address = address;
  }
}

static void memcheck_list_remove(const char *file,
                                 const char *func,
                                 const int line,
                                 const void *address)
{
  memcheck_list_t *temp;
  memcheck_list_t *prev;

  if (!memcheck_list) {
    fprintf(stderr, "%s:%s:%d no memory allocated, nothing to free\n", file, func, line);
    return;
  }

  temp = memcheck_list;
  prev = NULL;

  while (temp != NULL && address != temp->address) {
    prev = temp;
    temp = temp->next;
  }

  if (!temp) {
    // not found
    fprintf(stderr, "%s:%s:%d attempt to free address %p\n", file, func, line, address);
  }
  else {
    if (temp == memcheck_list) {
      // removing head
      memcheck_list = temp->next;
    } else {
      // removing from middle/end
      prev->next = temp->next;
    }
    free(temp->file);
    free(temp->func);
    free(temp->address);
    free(temp);
  }
}

void *memcheck_malloc(const char *file, const char *func, const int line, size_t size)
{
  memcheck_list_add(file, func, line, size);
  return memcheck_list->address;
}

void *memcheck_calloc(const char *file, const char *func, const int line, size_t nmemb, size_t size)
{
  size_t total_size = nmemb * size;

  memcheck_list_add(file, func, line, total_size);
  if (memcheck_list->address != NULL) {
    memset(memcheck_list->address, '\0', total_size);
  }

  return memcheck_list->address;
}

void *memcheck_realloc(const char *file, const char *func, const int line, void *old_address, size_t size)
{
  void *temp;
  memcheck_list_t *p;

  if (old_address == NULL) {
    memcheck_list_add(file, func, line, size);
    return memcheck_list->address;
  }

  for (p = memcheck_list; p != NULL; p = p->next) {
    if (p->address == old_address) {
      break;
    }
  }

  if (p == NULL) {
    fprintf(stderr,
            "%s:%s:%d attempt to realloc address %p that was never alloced\n",
            file, func, line, old_address);
    return NULL;
  }

  temp = realloc(old_address, size);
  memcheck_list_update(p, file, func, line, temp, size);

  return p->address;
}

void memcheck_free(const char *file, const char *func, const int line, const void *address)
{
  memcheck_list_remove(file, func, line, address);
}

void memcheck_report(void)
{
  memcheck_list_t *p;

  for (p = memcheck_list; p != NULL; p = p->next) {
    printf("Failed to free %zd bytes allocated at %s:%s:%d (%p)\n",
           p->size, p->file, p->func, p->line, p->address);
  }
}


#define malloc(x)       memcheck_malloc(__FILE__, __func__, __LINE__, (x))
#define calloc(x, y)    memcheck_calloc(__FILE__, __func__, __LINE__, (x), (y))
#define realloc(x, y)   memcheck_realloc(__FILE__, __func__, __LINE__, (x), (y))
#define free(x)         memcheck_free(__FILE__, __func__, __LINE__, (x))


#endif  // MYMALLOC_H__
