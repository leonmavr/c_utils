#ifndef ARENA_HPP_
#define ARENA_HPP_

#include <stdio.h>
#include <stdlib.h> // malloc, free
#include <stdint.h> // SIZE_MAX, uintptr_t
#include <stddef.h> // max_align_t, _Alignof, size_t

#define CHECK_ALLOC(ptr, arena_ptr) \
  do { if (!(ptr)) { arena_destroy(arena_ptr); return 1; } } while (0)

typedef struct {
  const char* name;
  unsigned id;
} Data;

typedef struct Arena {
  char* base;
  size_t capacity;
  size_t offset;
  struct Arena* next;
} Arena;

static inline Arena* arena_create(size_t size) { 
  Arena* ret = (Arena*) malloc(sizeof(Arena));
  if (!ret) {
    return NULL;
  }
  ret->base = (char*) malloc(size);
  if (!ret->base) {
    free(ret);
    return NULL;
  }
  ret->capacity = size;
  ret->offset = 0;
  ret->next = NULL;
  return ret;
}

static inline void arena_reset(Arena* arena) {
  while (arena) {
    arena->offset = 0;
    arena = arena->next;
  }
}

static inline void arena_destroy(Arena* arena) {
    while (arena) {
        Arena* next = arena->next;
        free(arena->base);
        free(arena);
        arena = next;
    }
}

static inline void* arena_alloc(Arena* arena, size_t size, size_t alignment) {
    if (!arena || alignment == 0 || (alignment & (alignment - 1)) != 0) {
        return NULL;
    }

    // helper pointer to add new nodes
    Arena* current_arena = arena;
    while (current_arena) {
        // as an absolute address
        uintptr_t current_ptr = (uintptr_t)(current_arena->base + current_arena->offset);
        // round up to the next multiple of alignment
        uintptr_t aligned = (current_ptr + alignment - 1) & ~(alignment - 1);
        // relative to base
        size_t new_offset = (aligned - (uintptr_t)current_arena->base) + size;
        if (new_offset <= current_arena->capacity) {
          current_arena->offset = new_offset;
          return (void*)aligned;
        }

        // we have exceeded the capacity -
        //calculate next capacity before creating next node
        if (!current_arena->next) {
            size_t required = size + alignment - 1;
            // keep doubling the capacity till we exceed the required size
            size_t next_capacity = current_arena->capacity * 2;
            while (next_capacity < required) {
                // SIZE_MAX is the largest allocation we can make
                if (next_capacity > SIZE_MAX / 2) {
                    next_capacity = required;
                    break;
                }
                next_capacity *= 2;
            }
            current_arena->next = arena_create(next_capacity);
            if (!current_arena->next)
              return NULL;
        }
        current_arena = current_arena->next;
    }
    return NULL;
}

#endif // ARENA_HPP_
