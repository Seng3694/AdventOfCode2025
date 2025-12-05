#include <ctype.h>
#include <stdio.h>
#include <stdint.h>

#include "../ext/toolbelt/src/assert.h"
#include "../common/fileutils.h"

// grep -P '\d+-' day05/input.txt | wc -l
// 174 ranges in my input. go with 200 for max size just in case
#define MAX_RANGE_COUNT 200

// grep -P '^\d+$' day05/input.txt | wc -l
// 1000 ids in my input. assume it's the max
#define MAX_ID_COUNT 1000

// grep -Po '\d+' day05/input.txt | sort -nu | tail -n 1
// biggest number in my input is 562421429314384. definitely need a 64 bit int for that
typedef struct range {
  int64_t from;
  int64_t to;
} range;

// note that this function is not compatible with stdlib qsort
static inline int64_t range_compare(const range left, const range right) {
  const int64_t from_diff = left.from - right.from;
  const int64_t to_diff = left.to - right.to;
  return from_diff != 0 ? from_diff : to_diff;
}

#ifndef NDEBUG
#define assert_sorted_ranges(ranges, count)                                                                            \
  do {                                                                                                                 \
    for (uint32_t i = 0; i < count - 1; ++i) {                                                                         \
      tlbt_assert_fmt(                                                                                                 \
          range_compare(ranges[i], ranges[i + 1]) <= 0,                                                                \
          "expected sorted ranges: left range (%ld-%ld) at [%u] should be smaller than right range (%ld-%ld) at [%u]", \
          ranges[i].from, ranges[i].to, i, ranges[i + 1].from, ranges[i + 1].to, i + 1);                               \
    }                                                                                                                  \
  } while (0)

#else
#define assert_sorted_ranges(ranges, count)
#endif

void parse_input(char *input, range *const ranges, uint32_t *const range_count, int64_t *const ids,
                 uint32_t *const id_count) {
  uint32_t rc = 0;
  uint32_t ic = 0;
  while (*input != '\n') {
    tlbt_assert_fmt(rc + 1 <= MAX_RANGE_COUNT, "too many ranges. max: %u", MAX_RANGE_COUNT);
    tlbt_assert_fmt(isdigit(*input), "digit expected, actual '%c' (%d)", *input, *input);
    range r = {0};
    r.from = strtoul(input, &input, 10);
    tlbt_assert_fmt(*input == '-', "'-' expected, actual '%c' (%d)", *input, *input);
    tlbt_assert_fmt(isdigit(*(input + 1)), "digit expected, actual '%c' (%d)", *input, *input);
    r.to = strtoul(input + 1, &input, 10);

    uint32_t current = rc;
    ranges[rc++] = r;

    // bubble new range up so I don't have to sort later
    for (int i = current; i > 0; --i) {
      if (range_compare(ranges[i - 1], ranges[i]) > 0) {
        range tmp = ranges[i];
        ranges[i] = ranges[i - 1];
        ranges[i - 1] = tmp;
      }
    }

    tlbt_assert_fmt(*input == '\n', "new line expected, actual '%c' (%d)", *input, *input);
    ++input; // skip new line
    // either the next id range starts now or another new line. if it's a new line, then it will break out of this loop
  }

  for (;;) {
    switch (*input) {
    case '\0':
      *range_count = rc;
      *id_count = ic;
      assert_sorted_ranges(ranges, rc);
      return;
    case '\n':
      ++input;
      break;
    default: {
      tlbt_assert_fmt(ic + 1 <= MAX_ID_COUNT, "too many ids. max: %u", MAX_ID_COUNT);
      tlbt_assert_fmt(isdigit(*input), "digit expected, actual '%c' (%d)", *input, *input);
      ids[ic++] = strtoul(input, &input, 10);
      tlbt_assert_fmt(*input == '\n' || *input == '\0', "new line or zero terminator expected, actual '%c' (%d)",
                      *input, *input);
      break;
    }
    }
  }
}

static uint32_t merge_ranges(range *const ranges, uint32_t range_count) {
  assert_sorted_ranges(ranges, range_count);
  uint32_t merged = 0;
  uint32_t left = 0;
  for (uint32_t right = 1; right < range_count; ++right) {
    tlbt_assert_fmt(ranges[left].from <= ranges[right].from,
                    "left(%u).from (%ld) should be smaller or equal to right(%u).from (%ld)", left, ranges[left].from,
                    right, ranges[right].from);

    if (ranges[left].to >= ranges[right].from) {
      // can merge -> left.from-right.to or left.from-left.to if left.to is bigger
      ranges[left].to = ranges[left].to > ranges[right].to ? ranges[left].to : ranges[right].to;
      // this will be used to sort them later
      ranges[right].from = INT64_MAX;
      merged++;
      // now i increases and this current range will be compared to the next one
    } else {
      // can't merge so move current to i
      left = right;
    }
  }

  // remove the empty slots ("sort")
  const uint32_t new_count = range_count - merged;
  for (uint32_t i = 0; i < new_count; ++i) {
    if (ranges[i].from == INT64_MAX) {
      for (uint32_t j = i + 1; j < range_count; ++j) {
        if (ranges[j].from != INT64_MAX) {
          ranges[i] = ranges[j];
          ranges[j].from = INT64_MAX;
          break;
        }
      }
    }
  }

  assert_sorted_ranges(ranges, new_count);
  return new_count;
}

static uint32_t solve_part1(const range *const ranges, const uint32_t range_count, const int64_t *const ids,
                            const uint32_t id_count) {
  uint32_t solution = 0;

  for (uint32_t i = 0; i < id_count; ++i) {
    for (uint32_t j = 0; j < range_count; ++j) {
      if (ids[i] >= ranges[j].from && ids[i] <= ranges[j].to) {
        solution++;
        break;
      }
    }
  }

  return solution;
}

static uint64_t solve_part2(const range *const ranges, const uint32_t range_count) {
  uint64_t solution = 0;

  for (uint32_t i = 0; i < range_count; ++i)
    solution += ((ranges[i].to - ranges[i].from) + 1); // +1 because they are inclusive ranges

  return solution;
}

int main(int argc, char **argv) {
  if (argc != 2)
    return 1;

  char *input = NULL;
  size_t length = 0;
  if (!fileutils_read_all(argv[1], &input, &length))
    return 1;

  uint32_t range_count = 0;
  uint32_t id_count = 0;
  range ranges[MAX_RANGE_COUNT] = {0};
  int64_t ids[MAX_ID_COUNT] = {0};

  parse_input(input, ranges, &range_count, ids, &id_count);
  free(input);

  range_count = merge_ranges(ranges, range_count);

  uint32_t part1 = solve_part1(ranges, range_count, ids, id_count);
  uint64_t part2 = solve_part2(ranges, range_count);

  printf("%u\n", part1);
  printf("%lu\n", part2);
}

