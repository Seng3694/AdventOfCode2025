#include <stdbool.h>
#include <stdio.h>
#include <stdint.h>

#include "../ext/toolbelt/src/assert.h"
#include "../common/fileutils.h"

// awk '{print length($0)}' day04/input.txt | sort -u
#define GRID_MAX_COLUMNS 137
// wc -l day04/input.txt
#define GRID_MAX_ROWS 137

typedef struct grid {
  // start with bool. if nothing else is required, change to bitset later
  bool data[GRID_MAX_COLUMNS * GRID_MAX_ROWS];
  uint32_t width;
  uint32_t height;
} grid;

void parse_input(char *input, grid *const g) {
  uint32_t row = 0;
  uint32_t column = 0;
  g->width = 0;
  g->height = 0;
  for (;;) {
    switch (*input) {
    case '\0':
      g->height = row;
      return;
    case '\n':
      ++input;
      // last empty new line
      if (column >= g->width) {
        ++row;
        g->width = column;
      }
      column = 0;
      break;
    case '.':
      tlbt_assert_msg(row < GRID_MAX_ROWS, "too many rows");
      tlbt_assert_msg(column < GRID_MAX_COLUMNS, "too many columns");
      g->data[row * g->width + column] = false; // technically not necessary
      ++input;
      ++column;
      break;
    case '@':
      tlbt_assert_msg(row < GRID_MAX_ROWS, "too many rows");
      tlbt_assert_msg(column < GRID_MAX_COLUMNS, "too many columns");
      g->data[row * g->width + column] = true;
      ++input;
      ++column;
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
#define HAS_ROLL(x, y) (g->data[(y) * g->width + (x)])

  // corners always have less than 4 spaces to worry about. so if they are a roll, then they are accessible
  if (HAS_ROLL(0, 0)) // top left
    tlbt_deque_point_push_back(rolls, (point){0, 0});

  if (HAS_ROLL(g->width - 1, 0)) // top right
    tlbt_deque_point_push_back(rolls, (point){g->width - 1, 0});

  if (HAS_ROLL(0, g->height - 1)) // bottom left
    tlbt_deque_point_push_back(rolls, (point){0, g->height - 1});

  if (HAS_ROLL(g->width - 1, g->height - 1)) // bottom right
    tlbt_deque_point_push_back(rolls, (point){g->width - 1, g->height - 1});

  // top and bottom row
  for (uint32_t x = 1; x < g->width - 1; ++x) {
    // top row
    if (HAS_ROLL(x, 0)) {
      if ((HAS_ROLL(x - 1, 0) + HAS_ROLL(x + 1, 0) + HAS_ROLL(x - 1, 1) + +HAS_ROLL(x, 1) + +HAS_ROLL(x + 1, 1)) < 4) {
        tlbt_deque_point_push_back(rolls, (point){x, 0});
      }
    }

    // bottom row
    if (HAS_ROLL(x, g->height - 1)) {
      if ((HAS_ROLL(x - 1, g->height - 1) + HAS_ROLL(x + 1, g->height - 1) + HAS_ROLL(x - 1, g->height - 2) +
           HAS_ROLL(x, g->height - 2) + +HAS_ROLL(x + 1, g->height - 2)) < 4) {
        tlbt_deque_point_push_back(rolls, (point){x, g->height - 1});
      }
    }
  }

  // left and right column
  for (uint32_t y = 1; y < g->height - 1; ++y) {
    // left column
    if (HAS_ROLL(0, y)) {
      if ((HAS_ROLL(0, y - 1) + HAS_ROLL(0, y + 1) + HAS_ROLL(1, y - 1) + HAS_ROLL(1, y) + HAS_ROLL(1, y + 1)) < 4) {
        tlbt_deque_point_push_back(rolls, (point){0, y});
      }
    }

    // right column
    if (HAS_ROLL(g->width - 1, y)) {
      if ((HAS_ROLL(g->width - 1, y - 1) + HAS_ROLL(g->width - 1, y + 1) + HAS_ROLL(g->width - 2, y - 1) +
           HAS_ROLL(g->width - 2, y) + HAS_ROLL(g->width - 2, y + 1)) < 4) {
        tlbt_deque_point_push_back(rolls, (point){g->width - 1, y});
      }
    }
  }

  // now everything inside the frame. no bounds checking required
  for (uint32_t y = 1; y < g->height - 1; ++y) {
    for (uint32_t x = 1; x < g->width - 1; ++x) {
      if (HAS_ROLL(x, y)) {
        if ((HAS_ROLL(x - 1, y - 1) + HAS_ROLL(x, y - 1) + HAS_ROLL(x + 1, y - 1) + HAS_ROLL(x - 1, y) +
             HAS_ROLL(x + 1, y) + HAS_ROLL(x - 1, y + 1) + HAS_ROLL(x, y + 1) + HAS_ROLL(x + 1, y + 1)) < 4) {
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
      g->data[p.y * g->width + p.x] = false;
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

