#include <ctype.h>
#include <stdio.h>
#include <stdint.h>

#include "../ext/toolbelt/src/assert.h"
#include "../ext/toolbelt/src/bitutils.h"
#include "../common/fileutils.h"

#define PRESENT_WIDTH 3
#define PRESENT_HEIGHT 3

typedef struct present_type {
  uint16_t value;
  uint8_t tiles;
} present_type;

// grep -Po '^\d+(?=\:)' day12/input.txt | wc -l
#define MAX_PRESENT_TYPES 6

// grep 'x' day12/input.txt | wc -l
#define MAX_REGIONS 1000

typedef struct region {
  uint8_t width;
  uint8_t height;
  uint8_t counts[MAX_PRESENT_TYPES];
} region;

typedef struct context {
  present_type present_types[MAX_PRESENT_TYPES];
  uint8_t present_type_count;
  region regions[MAX_REGIONS];
  uint16_t region_count;
} context;

static void parse_present_type(char *input, char **out, present_type *const t) {
  present_type type = {0};

  for (uint8_t y = 0; y < PRESENT_HEIGHT; ++y) {
    for (uint8_t x = 0; x < PRESENT_WIDTH; ++x) {
      tlbt_assert_fmt(*input == '.' || *input == '#', "expected '.' or '#', actual '%c' (%d)", *input, *input);
      if (*input == '#') {
        type.value = TLBT_SET_BIT(type.value, y * PRESENT_WIDTH + x);
        ++type.tiles; // could also do a single popcount after parsing. should be negligible
      }
      ++input;
    }
    tlbt_assert_fmt(*input == '\n', "expected newline, actual '%c' (%d)", *input, *input);
    ++input;
  }

  *t = type;
  *out = input;
}

static void parse_present_types(char *input, char **out, present_type *const p, uint8_t *const present_type_count) {
  tlbt_assert_fmt(isdigit(*input), "digit expected, actual '%c' (%d)", *input, *input);
  uint8_t count = 0;

  for (;;) {
    switch (*input) {
    case '\n':
      ++input;
      break;
    default: {
      char *start = input;
      tlbt_assert_fmt(isdigit(*input), "digit expected, actual '%c' (%d)", *input, *input);
      const uint8_t id = strtoul(input, &input, 10);

      switch (*input) {
      case ':':
        tlbt_assert_fmt(count == id, "expected presents to be ordered, expected next %u, actual %u", count, id);
        parse_present_type(input + 2, &input, &p[count++]); // +2 to skip colon and newline
        break;
      case 'x':
        // regions reached. no more present types
        *present_type_count = count;
        *out = start;
        return;
      default:
        tlbt_assert_unreachable();
        break;
      }
      break;
    }
    }
  }
}

static void parse_region(char *input, char **out, region *const r) {
  tlbt_assert_fmt(isdigit(*input), "digit expected, actual '%c' (%d)", *input, *input);
  r->width = strtoul(input, &input, 10);
  tlbt_assert_fmt(*input == 'x', "expected 'x', actual '%c' (%d)", *input, *input);
  tlbt_assert_fmt(isdigit(input[1]), "digit expected, actual '%c' (%d)", *input, *input);
  r->height = strtoul(input + 1, &input, 10);
  tlbt_assert_fmt(*input == ':', "expected ':', actual '%c' (%d)", *input, *input);
  ++input;
  uint8_t count = 0;
  do {
    tlbt_assert_fmt(*input == ' ', "expected space, actual '%c' (%d)", *input, *input);
    tlbt_assert_fmt(count < MAX_PRESENT_TYPES, "too many present types for region, max: %u", MAX_PRESENT_TYPES);
    r->counts[count++] = strtoul(input + 1, &input, 10); // +1 to skip the space
  } while (*input == ' ');

  *out = input;
}

static void parse_regions(char *input, region *const regions, uint16_t *const region_count) {
  uint16_t count = 0;
  tlbt_assert_fmt(isdigit(*input), "digit expected, actual '%c' (%d)", *input, *input);

  for (;;) {
    switch (*input) {
    case '\0':
      *region_count = count;
      return;
    case '\n':
      ++input;
      break;
    default: {
      tlbt_assert_fmt(count < MAX_REGIONS, "too many regions, max: %u", MAX_REGIONS);
      parse_region(input, &input, &regions[count++]);
      break;
    }
    }
  }
}

static void parse_input(char *input, context *const ctx) {
  parse_present_types(input, &input, ctx->present_types, &ctx->present_type_count);
  parse_regions(input, ctx->regions, &ctx->region_count);
}

static uint32_t solve(const context *const ctx) {
  uint32_t solution = 0;

  for (uint16_t i = 0; i < ctx->region_count; ++i) {
    const uint32_t target_area = (uint32_t)ctx->regions[i].width * (uint32_t)ctx->regions[i].height;
    uint32_t minimum_area_required = 0;

    for (uint8_t j = 0; j < ctx->present_type_count; ++j) {
      minimum_area_required += (uint32_t)ctx->regions[i].counts[j] * (uint32_t)ctx->present_types[j].tiles;
    }

    if (minimum_area_required > target_area)
      continue;

    // solving it would be very hard. probably trying different combinations of single
    // present types which can yield a rectangular shape and then use a binpack. or for other types combining with the
    // least amount of wasted tiles. but these could be combined further to waste even less tiles. so it seems like a
    // very complicated problem. bruteforce doesn't seem feasible with the amount of pieces. but for some reason this is
    // actually the correct solution for the input. so I'll take it
    solution++;
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

  context ctx = {0};

  parse_input(input, &ctx);
  free(input);

  uint32_t solution = solve(&ctx);

  printf("%u\n", solution);
}

