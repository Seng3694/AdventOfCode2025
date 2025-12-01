#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include "../common/fileutils.h"

#define TLBT_T int16_t
#define TLBT_T_NAME rot
#define TLBT_DYNAMIC_MEMORY
#define TLBT_BASE2_CAPACITY
#define TLBT_STATIC
#include "../ext/toolbelt/src/deque.h"

static void parse_input(char *input, tlbt_deque_rot *const rots) {
  for (;;) {
    switch (*input) {
    case 'L': {
      const int16_t value = strtoul(input + 1, &input, 10);
      tlbt_deque_rot_push_back(rots, -1 * value);
      break;
    }
    case 'R': {
      const int16_t value = strtoul(input + 1, &input, 10);
      tlbt_deque_rot_push_back(rots, value);
      break;
    }
    case '\n':
      ++input;
      break;
    case '\0':
      return;
    }
  }
}

static void solve(tlbt_deque_rot *const rots, uint32_t *part1, uint32_t *part2) {
  int16_t from, to;
  int16_t dial = 50;
  uint32_t landed_on_zero = 0;
  uint32_t passed_zero = 0;

  tlbt_deque_iterator_rot iter = {0};
  tlbt_deque_iterator_rot_init(&iter, rots);

  int16_t rot = 0;
  while (tlbt_deque_iterator_rot_iterate(&iter, &rot)) {
    from = dial;
    dial += rot;
    to = ((dial < 0 ? 100 : 0) + (dial % 100)) % 100;
    rot = abs(rot);

    if ((rot % 100) > 0) {
      if (to == 0) {
        landed_on_zero++;
      } else if ((to > from && dial <= 0 && from != 0) || (from > to && dial > 99)) {
        passed_zero++;
      }
    }
    if (rot >= 100) {
      passed_zero += (rot / 100);
    }

    dial = to;
  }

  *part1 = landed_on_zero;
  *part2 = passed_zero + landed_on_zero;
}

int main(int argc, char **argv) {
  if (argc != 2)
    return 1;

  char *input = NULL;
  size_t length = 0;
  if (!fileutils_read_all(argv[1], &input, &length))
    return 1;

  tlbt_deque_rot rots = {0};
  tlbt_deque_rot_create(&rots, 4096);
  parse_input(input, &rots);
  free(input);

  uint32_t part1, part2;
  solve(&rots, &part1, &part2);

  tlbt_deque_rot_destroy(&rots);

  printf("%u %u\n", part1, part2);
}

