#include <stdbool.h>
#include <stdio.h>
#include <stdint.h>

#include "../ext/toolbelt/src/assert.h"
#include "../common/fileutils.h"

// awk '{print length($0)}' day04/input.txt | sort -u
#define GRID_MAX_COLUMNS 137
// wc -l day04/input.txt
#define GRID_MAX_ROWS 137
#define GRID_PADDING 1

typedef struct grid {
  // tried bitset and it was slower. still less than 20KB which fits nicely in my L1 cache
  // inflate grid to avoid bounds checks
  bool data[(GRID_MAX_COLUMNS + GRID_PADDING * 2) * (GRID_MAX_ROWS + GRID_PADDING * 2)];
  uint32_t width;
  uint32_t height;
} grid;

#define GRID_PADDED_INDEX(x, y, w) (((y) + GRID_PADDING) * ((w) + (GRID_PADDING * 2)) + ((x) + GRID_PADDING))

static inline void grid_clear(grid *const g, const uint32_t x, const uint32_t y) {
  g->data[GRID_PADDED_INDEX(x, y, g->width)] = false;
}

void parse_input(char *input, grid *const g) {
  // find width first
  char *start = input;
  while (*input != '\n')
    ++input;
  g->width = input - start;
  input = start;

  uint32_t row = 0;
  const uint32_t row_offset = g->width + (GRID_PADDING * 2);
  uint32_t current = row_offset + GRID_PADDING;

  for (;;) {
    switch (*input) {
    case '\0':
      g->height = row;
      return;
    case '\n':
      ++input;
      ++row;
      current += GRID_PADDING * 2;
      break;
    case '.':
      ++current;
      ++input;
      break;
    case '@':
      g->data[current++] = true;
      ++input;
      break;
    default:
      tlbt_assert_fmt(false, "found invalid character '%c' (%d)", *input, *input);
      break;
    }
  }
  tlbt_assert_unreachable();
}

typedef struct point {
  uint8_t x;
  uint8_t y;
} point;

#define TLBT_T point
#define TLBT_BASE2_CAPACITY
#define TLBT_STATIC
#include "../ext/toolbelt/src/deque.h"

void get_movable_paper_rolls(const grid *const g, tlbt_deque_point *const rolls) {
  // no bounds checks required because of padding
  const uint32_t row_offset = g->width + (GRID_PADDING * 2);
  for (register uint32_t y = 0; y < g->height; ++y) {
    const uint32_t base = (y + GRID_PADDING) * row_offset;
    for (uint32_t x = 0; x < g->width; ++x) {
      const uint32_t i = base + x + GRID_PADDING;
      if (g->data[i]) {
        const uint32_t c = g->data[i - row_offset - 1] + g->data[i - row_offset] + g->data[i - row_offset + 1] +
                           g->data[i - 1] + g->data[i + 1] + g->data[i + row_offset - 1] + g->data[i + row_offset] +
                           g->data[i + row_offset + 1];
        if (c < 4) {
          tlbt_deque_point_push_back(rolls, (point){x, y});
        }
      }
    }
  }
}

void solve(grid *const g, tlbt_deque_point *const rolls, uint32_t *const part1, uint32_t *const part2) {
  uint32_t p2 = 0;
  get_movable_paper_rolls(g, rolls);
  *part1 = rolls->count;

  tlbt_deque_iterator_point iter = {0};
  tlbt_deque_iterator_point_init(&iter, rolls);
  point p = {0};

  do {
    p2 += rolls->count;
    tlbt_deque_iterator_point_reset(&iter);
    while (tlbt_deque_iterator_point_iterate(&iter, &p)) {
      grid_clear(g, p.x, p.y);
    }
    tlbt_deque_point_clear(rolls);
    get_movable_paper_rolls(g, rolls);
  } while (rolls->count != 0);

  *part2 = p2;
}

int main(int argc, char **argv) {
  if (argc != 2)
    return 1;

  char *input = NULL;
  size_t length = 0;
  if (!fileutils_read_all(argv[1], &input, &length))
    return 1;

  grid g = {0};
  parse_input(input, &g);
  free(input);

  // works for my input. 1024 is too small. tested with dynamic memory before
  point buffer[2048] = {0};
  tlbt_deque_point rolls = {0};
  tlbt_deque_point_init(&rolls, 2048, buffer);

  uint32_t part1, part2;
  solve(&g, &rolls, &part1, &part2);

  printf("%u\n", part1);
  printf("%u\n", part2);
}

