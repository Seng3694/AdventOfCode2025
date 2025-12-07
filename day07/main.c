#include <stdio.h>
#include <stdint.h>
#include <string.h>

#include "../ext/toolbelt/src/assert.h"
#include "../common/fileutils.h"

typedef struct point {
  int32_t x;
  int32_t y;
} point;

static inline uint32_t point_hash(const point p) {
  uint32_t hash = 36591911;
  hash = ((hash << 5) + hash) + p.x;
  hash = ((hash << 5) + hash) + p.y;
  return hash;
}

static inline bool point_equals(const point left, const point right) {
  return left.x == right.x && left.y == right.y;
}

#define TLBT_KEY_T point
#define TLBT_HASH_FUNC(p) point_hash(p)
#define TLBT_EQUALS_FUNC(l, r) point_equals(l, r)
#define TLBT_BASE2_CAPACITY
#define TLBT_DYNAMIC_MEMORY
#define TLBT_STATIC
#include "../ext/toolbelt/src/hashmap.h"

typedef struct node {
  struct node *left;
  struct node *right;
  uint64_t value;
  point coords;
} node;

#define TLBT_KEY_T point
#define TLBT_VALUE_T node *
#define TLBT_VALUE_T_NAME node
#define TLBT_HASH_FUNC(n) point_hash(n)
#define TLBT_EQUALS_FUNC(l, r) point_equals(l, r)
#define TLBT_BASE2_CAPACITY
#define TLBT_DYNAMIC_MEMORY
#define TLBT_STATIC
#include "../ext/toolbelt/src/hashmap.h"

#define TLBT_IMPLEMENTATION
#include "../ext/toolbelt/src/arena.h"

static void parse_input(char *input, point *const start, uint32_t *const height, tlbt_map_point_node *const nodes,
                        tlbt_arena *const allocator) {
  uint32_t line = 0;
  uint32_t column = 0;

  for (;;) {
    switch (*input) {
    case '\0':
      *height = line + 1;
      return;
    case '\n':
      ++input;
      if (*input != '\n' && *input != '\0') {
        ++line;
        column = 0;
      }
      break;
    case 'S':
      start->x = column;
      start->y = line;
      ++column;
      ++input;
      break;
    case '.':
      ++column;
      ++input;
      break;
    case '^': {
      node *n = tlbt_arena_malloc(sizeof(node), allocator);
      n->left = NULL;
      n->right = NULL;
      n->value = 0;
      n->coords.x = column;
      n->coords.y = line;
      tlbt_map_point_node_insert(nodes, n->coords, n);
      ++column;
      ++input;
      break;
    }
    default:
      tlbt_assert_unreachable();
      break;
    }
  }
}

static uint32_t count_part1(const node *const n, tlbt_set_point *const visited) {
  if (tlbt_set_point_contains(visited, n->coords))
    return 0;
  tlbt_set_point_insert(visited, n->coords);

  return (n->left ? count_part1(n->left, visited) : 0) + (n->right ? count_part1(n->right, visited) : 0) + 1;
}

static uint64_t count_part2(node *const n) {
  if (n->value == 0)
    n->value = (n->left ? count_part2(n->left) : 1) + (n->right ? count_part2(n->right) : 1);
  return n->value;
}

static void solve(const point start, const uint32_t height, tlbt_map_point_node *const nodes, uint32_t *const part1,
                  uint64_t *const part2) {
  // build tree
  tlbt_map_iterator_point_node iter = {0};
  tlbt_map_iterator_point_node_init(&iter, nodes);
  point *p = NULL;
  node **n = NULL;
  while (tlbt_map_iterator_point_node_iterate(&iter, &p, &n)) {
    point left = {p->x - 1, p->y + 2};
    point right = {p->x + 1, p->y + 2};

    while (left.y < height && !tlbt_map_point_node_contains(nodes, left)) {
      left.y += 2;
    }
    if (left.y >= height) {
      (*n)->left = NULL;
    } else {
      tlbt_map_point_node_get(nodes, left, &(*n)->left);
    }

    while (right.y < height && !tlbt_map_point_node_contains(nodes, right)) {
      right.y += 2;
    }
    if (right.y >= height) {
      (*n)->right = NULL;
    } else {
      tlbt_map_point_node_get(nodes, right, &(*n)->right);
    }
  }

  node *root = NULL;
  tlbt_map_point_node_get(nodes, (point){start.x, start.y + 2}, &root);

  tlbt_set_point visited = {0};
  tlbt_set_point_create(&visited, 4096);
  *part1 = count_part1(root, &visited);
  *part2 = count_part2(root);

  tlbt_set_point_destroy(&visited);
}

int main(int argc, char **argv) {
  if (argc != 2)
    return 1;

  char *input = NULL;
  size_t length = 0;
  if (!fileutils_read_all(argv[1], &input, &length))
    return 1;

  tlbt_arena a = {0};
  tlbt_arena_create(60000, &a);

  point start = {0};
  uint32_t height = 0;
  tlbt_map_point_node nodes = {0};
  tlbt_map_point_node_create(&nodes, 4096);
  parse_input(input, &start, &height, &nodes, &a);
  free(input);

  uint32_t part1 = 0;
  uint64_t part2 = 0;
  solve(start, height, &nodes, &part1, &part2);

  printf("%u\n", part1);
  printf("%lu\n", part2);

  tlbt_map_point_node_destroy(&nodes);
  tlbt_arena_destroy(&a);
}

