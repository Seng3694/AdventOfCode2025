#include <stdio.h>
#include <stdint.h>
#include <string.h>

#include "../ext/toolbelt/src/assert.h"
#include "../common/fileutils.h"

// wc -l day11/input.txt
#define MAX_NODES 600 // 594 -> 600

// grep -Po '(?<=\: ).+' day11/input.txt | awk '{print NF}' | sort -nu | tail -n 1
#define MAX_CHILDREN 30 // 26 -> 30

// grep -Po '(?<=\: ).+' day11/input.txt | grep -Po '\w+' | sort | uniq -c | sort -nk1 | awk 'END{print $1}'
#define MAX_INPUT_NODES 28

// grep -Po '^[a-z]+(?=\:)' day11/input.txt | sort | uniq -c | awk 'END{print $1}'
// no entries with the same key

typedef union node_id {
  char string_id[4];
  uint32_t uint_id;
} node_id;

typedef struct node {
  struct node *children[MAX_CHILDREN];
  node_id id;
  uint64_t path_count;
  uint8_t children_count;
} node;

#define TLBT_T node
#define TLBT_DYNAMIC_MEMORY
#define TLBT_BASE2_CAPACITY
#define TLBT_STATIC
#include "../ext/toolbelt/src/deque.h"

inline static uint32_t node_id_hash_func(const node_id id) {
  const uint32_t hash = 36591911;
  return ((hash << 5) + hash) + id.uint_id;
}

inline static bool node_id_compare_func(const node_id left, const node_id right) {
  return left.uint_id == right.uint_id;
}

#define TLBT_KEY_T node_id
#define TLBT_KEY_T_NAME id
#define TLBT_VALUE_T node *
#define TLBT_VALUE_T_NAME node
#define TLBT_HASH_FUNC node_id_hash_func
#define TLBT_EQUALS_FUNC node_id_compare_func
#define TLBT_DYNAMIC_MEMORY
#define TLBT_BASE2_CAPACITY
#define TLBT_STATIC
#include "../ext/toolbelt/src/hashmap.h"

typedef struct rack {
  tlbt_map_id_node nodes;
} rack;

#define TLBT_IMPLEMENTATION
#include "../ext/toolbelt/src/arena.h"

inline static void parse_id(char *input, char **output, node_id *const id) {
  tlbt_assert_fmt(input[0] >= 'a' && input[0] <= 'z', "expected lower case char, actual '%c' (%d)", *input, *input);
  tlbt_assert_fmt(input[1] >= 'a' && input[1] <= 'z', "expected lower case char, actual '%c' (%d)", *input, *input);
  tlbt_assert_fmt(input[2] >= 'a' && input[2] <= 'z', "expected lower case char, actual '%c' (%d)", *input, *input);
  id->string_id[0] = input[0];
  id->string_id[1] = input[1];
  id->string_id[2] = input[2];
  tlbt_assert_fmt(id->string_id[3] == '\0', "expected id to have zero terminator, actual '%c' (%d)", id->string_id[3],
                  id->string_id[3]);
  *output = input + 3;
}

static void parse_input(char *input, rack *const r, tlbt_deque_node *const nodes) {
  for (;;) {
    switch (*input) {
    case '\0':
      return;
    case '\n':
      ++input;
      break;
    default: {
      node_id id = {0};
      parse_id(input, &input, &id);
      tlbt_assert_fmt(*input == ':', "expected ':', actual '%c' (%d)\n", *input, *input);
      ++input;
      node *n = NULL;
      if (!tlbt_map_id_node_get(&r->nodes, id, &n)) {
        tlbt_deque_node_push_back(nodes, (node){0});
        n = tlbt_deque_node_peek_back(nodes);
        n->id = id;
        n->children_count = 0;
        n->path_count = UINT64_MAX;
        tlbt_map_id_node_insert(&r->nodes, id, n);
      }

      do {
        tlbt_assert_fmt(*input == ' ', "expected space, actual '%c' (%d)\n", *input, *input);
        node_id child_id = {0};
        parse_id(input + 1, &input, &child_id);
        node *child = NULL;
        if (!tlbt_map_id_node_get(&r->nodes, child_id, &child)) {
          tlbt_deque_node_push_back(nodes, (node){0});
          child = tlbt_deque_node_peek_back(nodes);
          child->id = child_id;
          child->children_count = 0;
          child->path_count = UINT64_MAX;
          tlbt_map_id_node_insert(&r->nodes, child_id, child);
        }
        n->children[n->children_count++] = child;
      } while (*input == ' ');
      break;
    }
    }
  }
}

static const node_id you_id = {.string_id = "you"};
static const node_id svr_id = {.string_id = "svr"};
static const node_id fft_id = {.string_id = "fft"};
static const node_id dac_id = {.string_id = "dac"};
static const node_id out_id = {.string_id = "out"};

static uint64_t count_paths(node *const n, const node_id dest) {
  if (n->path_count != UINT64_MAX)
    return n->path_count;

  if (n->id.uint_id == dest.uint_id)
    return 1;

  if (n->id.uint_id == out_id.uint_id)
    return 0;

  n->path_count = 0;
  for (uint8_t i = 0; i < n->children_count; ++i) {
    n->path_count += count_paths(n->children[i], dest);
  }

  return n->path_count;
}

static uint64_t solve_part1(rack *const r) {
  uint64_t solution = 0;

  node *start = NULL;
  tlbt_map_id_node_get(&r->nodes, you_id, &start);
  solution = count_paths(start, out_id);

  return solution;
}

static void clear_cached_values(tlbt_deque_node *const nodes) {
  for (size_t i = 0; i < nodes->count; ++i)
    nodes->data[i].path_count = UINT64_MAX;
}

static uint64_t solve_part2(rack *const r, tlbt_deque_node *const nodes) {
  uint64_t solution = 1;

  node *svr = NULL;
  node *dac = NULL;
  node *fft = NULL;
  tlbt_map_id_node_get(&r->nodes, svr_id, &svr);
  tlbt_map_id_node_get(&r->nodes, dac_id, &dac);
  tlbt_map_id_node_get(&r->nodes, fft_id, &fft);
  tlbt_assert(svr && dac && fft);

  clear_cached_values(nodes);
  const uint32_t paths_from_dac_to_fft = count_paths(dac, fft_id);

  clear_cached_values(nodes);
  const uint32_t paths_from_fft_to_dac = count_paths(fft, dac_id);

  node *first = NULL;
  node *second = NULL;

  if (paths_from_dac_to_fft > 0) {
    tlbt_assert(paths_from_fft_to_dac == 0);
    first = dac;
    second = fft;
  } else if (paths_from_fft_to_dac > 0) {
    tlbt_assert(paths_from_dac_to_fft == 0);
    first = fft;
    second = dac;
  }

  // calculate the amount of paths from the start "svr" to the first node (either fft or dac)
  // then calculate the amount of paths from the first node to the second node (if the first was fft, then second is
  // dac) then calculate the amount of paths from the second node to the end node "out". multiplying them together
  // should be the solution
  clear_cached_values(nodes);
  solution *= count_paths(svr, first->id);

  clear_cached_values(nodes);
  solution *= count_paths(first, second->id);

  clear_cached_values(nodes);
  solution *= count_paths(second, out_id);

  return solution;
}

int main(int argc, char **argv) {
  if (argc != 2)
    return 1;

  char *input = NULL;
  size_t length = 0;
  if (!fileutils_read_all(argv[1], &input, &length))
    return 1;

  tlbt_deque_node nodes = {0};
  tlbt_deque_node_create(&nodes, 1024);

  rack r = {0};
  tlbt_map_id_node_create(&r.nodes, 1024);

  parse_input(input, &r, &nodes);
  free(input);

  uint64_t part1 = solve_part1(&r);
  uint64_t part2 = solve_part2(&r, &nodes);

  printf("%lu\n", part1);
  printf("%lu\n", part2);

  tlbt_deque_node_destroy(&nodes);
  tlbt_map_id_node_destroy(&r.nodes);
}

