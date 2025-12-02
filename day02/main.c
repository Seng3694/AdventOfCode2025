#include <ctype.h>
#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include "../common/fileutils.h"
#include "../ext/toolbelt/src/assert.h"

// biggest input number `grep -Po '[0-9]+' day02/input.txt | sort -nu | tail -n 1` -> 6_868_700_146
// gotta use 64 bit integers then
typedef struct range {
  uint64_t from;
  uint64_t to;
} range;

#define TLBT_T range
#define TLBT_BASE2_CAPACITY
#define TLBT_STATIC
#include "../ext/toolbelt/src/deque.h"

void parse_input(char *input, tlbt_deque_range *const ranges) {
  for (;;) {
    switch (*input) {
    case '\0':
      return;
    case '\n':
    case ',':
      ++input;
      break;

    default: {
      range r = {0};
      tlbt_assert_fmt(isdigit(*input), "expected digit found '%c' (%d)", *input, *input);
      r.from = strtoull(input, &input, 10);
      tlbt_assert_fmt(*input == '-', "expected `-` found `%c` (%d)", *input, *input);
      ++input; // skip minus
      tlbt_assert_fmt(isdigit(*input), "expected digit found '%c' (%d)", *input, *input);
      r.to = strtoull(input, &input, 10);
      tlbt_deque_range_push_back(ranges, r);
      break;
    }
    }
  }
}

inline static uint32_t get_digit_count(const uint64_t n) {
  return (uint32_t)log10(n) + 1;
}

inline static void subdivide(uint64_t input, uint64_t divisor, uint64_t digit_count, uint64_t *const output) {
  tlbt_assert_fmt(divisor > 1, "divisor should be bigger than 1 (actual: %lu)", divisor);
  tlbt_assert_fmt(divisor <= 6, "divisor should be smaller than 7 (actual: %lu)", divisor);
  tlbt_assert_fmt(digit_count % divisor == 0, "digit_count should be divisible by divisor (%lu %% %lu = %lu)",
                  digit_count, divisor, digit_count % divisor);

  uint64_t groups = digit_count / divisor;
  uint64_t divisor_pow = (uint64_t)pow(10, divisor);
  for (int64_t i = groups - 1; i >= 0; --i) {
    output[i] = input % divisor_pow;
    input /= divisor_pow;
  }
}

inline static uint64_t repeat(uint64_t input, uint64_t digit_count, uint64_t repetitions) {
  uint64_t result = 0;
  const uint64_t power = pow(10, digit_count);
  for (uint64_t i = 0; i < repetitions; ++i)
    result = result * power + input;
  return result;
}

typedef struct divisor_values {
  uint32_t divisors[3];
  uint32_t count;
} divisor_values;

// could have done that dynamically but eh
const static divisor_values divisors[2][11] = {
    {
        // part 1
        [0] = {{0}, 0},
        [1] = {{0}, 0},
        [2] = {{1}, 1},
        [3] = {{0}, 0},
        [4] = {{1, 2}, 2},
        [5] = {{0}, 0},
        [6] = {{1, 3}, 2},
        [7] = {{0}, 0},
        [8] = {{1, 4}, 2},
        [9] = {{0}, 0},
        [10] = {{1, 5}, 2},
    },
    {
        // part 2
        [0] = {{0}, 0},
        [1] = {{0}, 0},
        [2] = {{1}, 1},
        [3] = {{1}, 1},
        [4] = {{1, 2}, 2},
        [5] = {{1}, 1},
        [6] = {{1, 2, 3}, 3},
        [7] = {{1}, 1},
        [8] = {{1, 4}, 2},
        [9] = {{1, 3}, 2},
        [10] = {{1, 2, 5}, 3},
    },
};

static uint64_t solve(tlbt_deque_range *const ranges, const divisor_values *const digit_div_lookup) {
  uint64_t result = 0;

  static const uint64_t ones[11] = {
      [0] = 0,      [1] = 1,       [2] = 11,       [3] = 111,       [4] = 1111,        [5] = 11111,
      [6] = 111111, [7] = 1111111, [8] = 11111111, [9] = 111111111, [10] = 1111111111,
  };

  tlbt_deque_iterator_range iter = {0};
  tlbt_deque_iterator_range_init(&iter, ranges);
  range r = {0};

  while (tlbt_deque_iterator_range_iterate(&iter, &r)) {
    uint64_t from = r.from;
    uint64_t to = r.to;
    uint32_t from_digit_count = get_digit_count(from);
    uint32_t to_digit_count = get_digit_count(to);

    // theoretically ranges could go from a number with two digits smaller than the upper end. but I analyzed the input
    // and that's not the case. for example 100-10000
    tlbt_assert_fmt(to_digit_count - from_digit_count <= 1,
                    "digit counts should not be further apart than 1 (%u - %u = %u)", to_digit_count, from_digit_count,
                    to_digit_count - from_digit_count);

    // this means there can only be 2 ranges maximum. add the default one
    range new_ranges[2] = {{from, to}};
    uint32_t new_range_digits[2] = {from_digit_count, to_digit_count};
    uint8_t new_range_count = 1;
    if (from_digit_count != to_digit_count) {
      // this means something like 1000-20000
      // separate into two ranges like 1000-9999 and 10000-20000
      new_ranges[0] = (range){from, pow(10, from_digit_count) - 1};
      new_ranges[1] = (range){new_ranges[0].to + 1, to};
      new_range_count = 2;
    }

    for (uint8_t i = 0; i < new_range_count; ++i) {
      from = new_ranges[i].from;
      to = new_ranges[i].to;
      const uint32_t digit_count = new_range_digits[i];
      if (digit_count <= 1)
        continue;

      const uint32_t divisor_count = digit_div_lookup[digit_count].count;
      // divisor count should always be at least 1 and the first one should be 1
      if (divisor_count == 0)
        continue;

      uint64_t subdivisions[12] = {0};
      for (uint32_t j = 0; j < divisor_count; ++j) {
        const uint32_t divisor = digit_div_lookup[digit_count].divisors[j];
        if (divisor == 1) {
          // handle the 1 case differently than the others. the others can include the cases handled by one. that's why
          // we have the ones array to check if the current number is dividable by the ones. if it is, skip it because
          // it was handled here already
          uint64_t current = ones[digit_count];
          // for digit_count 3 it would be 111. add 111 for the next step etc
          // find starting point
          while (current < from) {
            current += ones[digit_count];
          }
          // now it's either the same as current or bigger. so for 100 it would be 111
          // if the range goal is 110, then it won't do anything in the next step

          while (current <= to) {
            result += current;
            current += ones[digit_count];
          }
        } else {
          const uint32_t groups = digit_count / divisor;
          const uint32_t group_digit_count = digit_count / groups;
          subdivide(from, divisor, digit_count, subdivisions);

          uint64_t m = subdivisions[0] >= subdivisions[1] ? subdivisions[0] : subdivisions[0] + 1;
          uint64_t current = repeat(m, group_digit_count, groups);
          while (current <= to) {
            if (current % ones[digit_count] != 0) {
              result += current;
            }
            m++;
            current = repeat(m, group_digit_count, groups);
          }
        }
      }
    }
  }

  return result;
}

int main(int argc, char **argv) {
  if (argc != 2)
    return 1;

  char *input = NULL;
  size_t length = 0;
  if (!fileutils_read_all(argv[1], &input, &length))
    return 1;

  range buffer[32] = {0};
  tlbt_deque_range ranges = {0};
  tlbt_deque_range_init(&ranges, 32, buffer);

  parse_input(input, &ranges);
  free(input);

  printf("%lu\n", solve(&ranges, divisors[0]));
  printf("%lu\n", solve(&ranges, divisors[1]));
}

