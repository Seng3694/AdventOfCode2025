#include <ctype.h>
#include <math.h>
#include <stdio.h>
#include <stdint.h>

#include "../ext/toolbelt/src/assert.h"
#include "../common/fileutils.h"

#define TLBT_IMPLEMENTATION
#include "../ext/toolbelt/src/arena.h"

// 200 lines with 100 digits each
#define POWER_BANKS_MAX 200
#define POWER_BANK_BATTERIES_MAX 100

typedef struct power_bank {
  uint8_t *batteries;
  uint8_t count;
} power_bank;

static void parse_input(char *input, tlbt_arena *const a, power_bank *const banks, uint32_t *count) {
  uint32_t c = 0;
  for (;;) {
    switch (*input) {
    case '\0':
      *count = c;
      return;
    case '\n':
      ++input;
      break;
    default: {
      tlbt_assert_fmt(c + 1 <= POWER_BANKS_MAX, "there should not be more than %u power banks", POWER_BANKS_MAX);
      power_bank *b = &banks[c++];
      b->count = 0;
      b->batteries = tlbt_arena_malloc(sizeof(uint8_t) * POWER_BANK_BATTERIES_MAX, a);
      while (*input != '\0' && *input != '\n') {
        tlbt_assert_fmt(b->count + 1 <= POWER_BANK_BATTERIES_MAX,
                        "there should not be more than %u batteries per power bank", POWER_BANK_BATTERIES_MAX);
        tlbt_assert_fmt(isdigit(*input), "expected digit (actual: '%c' (%d))", *input, *input);
        b->batteries[b->count++] = (*input) - '0';
        ++input;
      }
      break;
    }
    }
  }
}

static uint64_t find_joltage(const power_bank *const b, uint8_t index, const uint8_t count, uint8_t found) {
  for (uint8_t i = 9; i > 0; --i) {
    uint64_t joltage = 0;
    for (uint8_t j = index; j < b->count; ++j) {
      if (b->batteries[j] == i) {
        found++;
        joltage = (uint64_t)pow(10, count - found) * (uint64_t)b->batteries[j];
        if (found == count) {
          return joltage;
        } else {
          uint64_t ret = find_joltage(b, j + 1, count, found);
          if (ret != 0) {
            return joltage + ret;
          } else {
            found--;
            goto next;
          }
        }
        tlbt_assert_msg(false, "should never reach");
      }
    }
  next:;
  }
  return 0;
}

uint64_t solve(const power_bank *banks, const uint32_t bank_count, const uint8_t digits) {
  uint64_t solution = 0;
  for (uint32_t i = 0; i < bank_count; ++i) {
    const power_bank *b = &banks[i];
    solution += find_joltage(b, 0, digits, 0);
  }
  return solution;
}

int main(int argc, char **argv) {
  if (argc != 2)
    return 1;

  char *input = NULL;
  size_t length = 0;
  if (!fileutils_read_all(argv[1], &input, &length))
    return 1;

  tlbt_arena a = {0};
  tlbt_arena_create(21000, &a); // roughly 200*100*1=20000. add some extra for padding

  power_bank banks[POWER_BANKS_MAX] = {0};
  uint32_t count = 0;
  parse_input(input, &a, banks, &count);
  free(input);

  uint64_t part1 = solve(banks, count, 2);
  uint64_t part2 = solve(banks, count, 12);

  printf("%lu\n", part1);
  printf("%lu\n", part2);

  tlbt_arena_destroy(&a);
}

