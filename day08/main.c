#include <ctype.h>
#include <stdio.h>
#include <stdint.h>
#include <math.h>
#include <string.h>

#include "../ext/toolbelt/src/assert.h"
#include "../common/fileutils.h"

// wc -l day08/input.txt -> 1000
#define MAX_JUNCTION_BOXES 1000

// grep -Po '\d+' day08/input.txt | sort -nu | tail -n 1 -> 32-99964
typedef struct point {
  int32_t x;
  int32_t y;
  int32_t z;
} point;

static void parse_input(char *input, point *const points, uint32_t *point_count) {
  uint32_t c = 0;
  for (;;) {
    switch (*input) {
    case '\0':
      *point_count = c;
      return;
    case '\n':
      ++input;
      break;
    default: {
      tlbt_assert_fmt(c < MAX_JUNCTION_BOXES, "too many junction boxes. max: %u", MAX_JUNCTION_BOXES);
      tlbt_assert_fmt(isdigit(*input), "expected digit, actual '%c' (%d)", *input, *input);
      point p = {0};
      p.x = strtoul(input, &input, 10);
      tlbt_assert_fmt(*input == ',', "expected ',', actual '%c' (%d)", *input, *input);
      p.y = strtoul(input + 1, &input, 10);
      tlbt_assert_fmt(*input == ',', "expected ',', actual '%c' (%d)", *input, *input);
      p.z = strtoul(input + 1, &input, 10);
      tlbt_assert_fmt(*input == '\0' || *input == '\n', "expected new line or zero terminator, actual '%c' (%d)",
                      *input, *input);
      points[c++] = p;
      break;
    }
    }
  }
}

static uint32_t point_distance(const point a, const point b) {
  return (uint32_t)sqrt(pow(a.x - b.x, 2) + pow(a.y - b.y, 2) + pow(a.z - b.z, 2));
}

static inline uint32_t point_hash(const point p) {
  uint32_t hash = 36591911;
  hash = ((hash << 5) + hash) + (uint32_t)p.x;
  hash = ((hash << 5) + hash) + (uint32_t)p.y;
  hash = ((hash << 5) + hash) + (uint32_t)p.z;
  return hash;
}

static inline bool point_equals(const point left, const point right) {
  return left.x == right.x && left.y == right.y && left.z == right.z;
}

#define TLBT_KEY_T point
#define TLBT_VALUE_T uint32_t
#define TLBT_VALUE_T_NAME circuit
#define TLBT_HASH_FUNC(p) point_hash(p)
#define TLBT_EQUALS_FUNC(l, r) point_equals(l, r)
#define TLBT_BASE2_CAPACITY
#define TLBT_DYNAMIC_MEMORY
#define TLBT_STATIC
#include "../ext/toolbelt/src/hashmap.h"

typedef struct connection {
  uint32_t a;
  uint32_t b;
  uint32_t dist;
} connection;

inline static int connection_compare(const connection left, const connection right) {
  return (int64_t)left.dist - (int64_t)right.dist;
}

#define TLBT_T connection
#define TLBT_DYNAMIC_MEMORY
#define TLBT_COMPARE(l, r) connection_compare(l, r)
#define TLBT_STATIC
#include "../ext/toolbelt/src/heap.h"

inline static int u32_compare(const uint32_t *const left, const uint32_t *const right) {
  return (int32_t)*right - (int32_t)*left;
}

static int connect(const point a, const point b, tlbt_map_point_circuit *const m, uint32_t *const next_circuit_id) {
  uint32_t a_circuit = 0;
  uint32_t b_circuit = 0;
  const bool a_is_in_circuit = tlbt_map_point_circuit_get(m, a, &a_circuit);
  const bool b_is_in_circuit = tlbt_map_point_circuit_get(m, b, &b_circuit);

  if (a_is_in_circuit && b_is_in_circuit && a_circuit != b_circuit) {
    // different circuits -> merge
    tlbt_map_iterator_point_circuit iter = {0};
    tlbt_map_iterator_point_circuit_init(&iter, m);
    point *p = NULL;
    uint32_t *circ = NULL;
    // merge b into a
    while (tlbt_map_iterator_point_circuit_iterate(&iter, &p, &circ)) {
      if (*circ == b_circuit)
        *circ = a_circuit;
    }
    return -1; // circuit removed
  } else if (a_is_in_circuit && !b_is_in_circuit) {
    // add b to circuit of a
    tlbt_map_point_circuit_insert(m, b, a_circuit);
    return 0; // no change in the amount of circuits
  } else if (!a_is_in_circuit && b_is_in_circuit) {
    // add a to circuit of b
    tlbt_map_point_circuit_insert(m, a, b_circuit);
    return 0; // no change in the amount of circuits
  } else if (!a_is_in_circuit && !b_is_in_circuit) {
    // both in no circuit, create new one
    tlbt_map_point_circuit_insert(m, a, *next_circuit_id);
    tlbt_map_point_circuit_insert(m, b, *next_circuit_id);
    (*next_circuit_id)++;
    return 1; // new circuit
  }

  return 0; // already part of the same circuit. no change
}

static void solve(const point *const points, const uint32_t point_count, const uint32_t connection_count,
                  uint32_t *const part1, uint64_t *const part2) {

  tlbt_map_point_circuit m = {0};
  tlbt_map_point_circuit_create(&m, 1024);

  uint32_t next_circuit_id = 0;

  tlbt_min_heap_connection connections = {0};
  tlbt_min_heap_connection_create(&connections, 500000);

  // TODO: this is the main bottleneck
  // should create an octree or kd-tree for faster spatial querying of close neighbors instead of bruteforcing. this
  // also requires way more memory
  for (uint32_t i = 0; i < point_count - 1; ++i) {
    const point a = points[i];
    for (uint32_t j = i + 1; j < point_count; ++j) {
      const point b = points[j];
      tlbt_min_heap_connection_push(&connections, (connection){.a = i, .b = j, .dist = point_distance(a, b)});
    }
  }

  int32_t circuit_count = 0;
  for (uint32_t i = 0; i < connection_count; ++i) {
    connection c = {0};
    tlbt_min_heap_connection_peek(&connections, &c);
    circuit_count += connect(points[c.a], points[c.b], &m, &next_circuit_id);
    tlbt_min_heap_connection_pop(&connections);
  }

  tlbt_assert_msg(next_circuit_id <= 512, "expected less than 512 circuit ids");
  uint32_t circuit_id_counts[512] = {0};

  tlbt_map_iterator_point_circuit iter = {0};
  tlbt_map_iterator_point_circuit_init(&iter, &m);
  point *p = NULL;
  uint32_t *circ = NULL;
  while (tlbt_map_iterator_point_circuit_iterate(&iter, &p, &circ)) {
    circuit_id_counts[*circ]++;
  }

  qsort(circuit_id_counts, 512, sizeof(uint32_t), (__compar_fn_t)u32_compare);

  uint32_t p1 = 1;
  for (uint32_t i = 0; i < 3; ++i) {
    p1 *= circuit_id_counts[i];
  }
  *part1 = p1;

  connection last_connection = {0};
  for (uint32_t i = connection_count; i < connections.count && (circuit_count != 1 || m.count != point_count); ++i) {
    tlbt_min_heap_connection_peek(&connections, &last_connection);
    circuit_count += connect(points[last_connection.a], points[last_connection.b], &m, &next_circuit_id);
    tlbt_min_heap_connection_pop(&connections);
  }

  *part2 = (uint64_t)points[last_connection.a].x * (uint64_t)points[last_connection.b].x;

  tlbt_min_heap_connection_destroy(&connections);
  tlbt_map_point_circuit_destroy(&m);
}

int main(int argc, char **argv) {
  if (argc != 2)
    return 1;

  char *input = NULL;
  size_t length = 0;
  if (!fileutils_read_all(argv[1], &input, &length))
    return 1;

  uint32_t point_count = 0;
  point points[MAX_JUNCTION_BOXES] = {0};

  parse_input(input, points, &point_count);
  free(input);

  uint32_t part1 = 0;
  uint64_t part2 = 0;
  solve(points, point_count, 1000, &part1, &part2);

  printf("%u\n", part1);
  printf("%lu\n", part2);
}

