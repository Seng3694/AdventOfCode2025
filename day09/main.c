#include <ctype.h>
#include <stdio.h>
#include <stdint.h>

#include "../ext/toolbelt/src/assert.h"
#include "../common/fileutils.h"

// wc -l day09/input.txt -> 497
#define MAX_VERTICES 500

// grep -Po '\d+' day09/input.txt | sort -nur | tail -n 1 -> 98471
typedef struct point {
  int32_t x;
  int32_t y;
} point;

void parse_input(char *input, point *const points, uint32_t *const point_count) {
  uint32_t c = 0;
  for (;;) {
    switch (*input) {
    case '\0':
      *point_count = c;
      return;
    case '\n':
      ++input;
      break;
    default:
      tlbt_assert_fmt(c < MAX_VERTICES, "too many vertices. max: %u", MAX_VERTICES);
      tlbt_assert_fmt(isdigit(*input), "expected digit, actual '%c' (%d)", *input, *input);
      point p = {0};
      p.x = strtoul(input, &input, 10);
      tlbt_assert_fmt(*input == ',', "expected ',', actual '%c' (%d)", *input, *input);
      tlbt_assert_fmt(isdigit(*(input + 1)), "expected digit, actual '%c' (%d)", *input, *input);
      p.y = strtoul(input + 1, &input, 10);
      tlbt_assert_fmt(*input == '\n' || *input == '\0', "expected new line or zero terminator, actual '%c' (%d)",
                      *input, *input);
      points[c++] = p;
      break;
    }
  }
}

static uint64_t solve_part1(const point *const points, const uint32_t point_count) {
  uint64_t biggest_area = 0;

  for (uint32_t i = 0; i < point_count - 1; ++i) {
    for (uint32_t j = i + 1; j < point_count; ++j) {
      const uint64_t width = abs(points[i].x - points[j].x) + 1;
      const uint64_t height = abs(points[i].y - points[j].y) + 1;
      const uint64_t area = width * height;
      if (area > biggest_area) {
        biggest_area = area;
      }
    }
  }
  return biggest_area;
}

#define MIN(x, y) ((x) < (y) ? (x) : (y))
#define MAX(x, y) ((x) > (y) ? (x) : (y))

typedef union edge {
  struct {
    int32_t xmin, xmax, y;
  };
  struct {
    int32_t ymin, ymax, x;
  };
} edge;

#define TLBT_T edge
#define TLBT_STATIC
#include "../ext/toolbelt/src/deque.h"

typedef struct rect {
  int32_t left, top, right, bottom;
} rect;

static inline bool line_intersect(const edge a, const edge b) {
  // assume they are perpendicular
  return a.x > b.xmin && a.x < b.xmax && b.y > a.ymin && b.y < a.ymax;
}

static bool is_rect_inside(const rect r, const tlbt_deque_edge *const vertical_edges,
                           const tlbt_deque_edge *const horizontal_edges) {
  // top and bottom edge of the rectangle
  const edge hor_edges[2] = {
      {.y = r.top, .xmin = r.left, .xmax = r.right},
      {.y = r.bottom, .xmin = r.left, .xmax = r.right},
  };
  // left and right edge of the rectangle
  const edge ver_edges[2] = {
      {.x = r.left, .ymin = r.top, .ymax = r.bottom},
      {.x = r.right, .ymin = r.top, .ymax = r.bottom},
  };

  // check if rectangle edges intersect with any polygon edges
  for (size_t i = 0; i < vertical_edges->count; ++i) {
    if (line_intersect(hor_edges[0], vertical_edges->data[i]) ||
        line_intersect(hor_edges[1], vertical_edges->data[i])) {
      return false;
    }
  }
  for (size_t i = 0; i < horizontal_edges->count; ++i) {
    if (line_intersect(ver_edges[0], horizontal_edges->data[i]) ||
        line_intersect(ver_edges[1], horizontal_edges->data[i])) {
      return false;
    }
  }

  // now cast a ray to two sides of every point and see how many intersections they have. they all have to be odd
  const edge vertical_rays[4] = {
      // from topleft point to top (0)
      {.x = r.left, .ymin = 0, .ymax = r.top},
      // from topright point to top (0)
      {.x = r.right, .ymin = 0, .ymax = r.top},
      // from bottomright point to bottom (max y)
      {.x = r.right, .ymin = r.bottom, .ymax = INT32_MAX},
      // from bottomleft point to bottom (max y)
      {.x = r.left, .ymin = r.bottom, .ymax = INT32_MAX},

  };
  const edge horizontal_rays[4] = {
      // from topleft point to left (0)
      {.y = r.top, .xmin = 0, .xmax = r.left},
      // from topright point to right (max x)
      {.y = r.top, .xmin = r.right, .xmax = INT32_MAX},
      // from bottomright point to right (max x)
      {.y = r.bottom, .xmin = r.right, .xmax = INT32_MAX},
      // from bottomleft point to left (0)
      {.y = r.bottom, .xmin = 0, .xmax = r.left},
  };

  for (uint32_t i = 0; i < 4; ++i) {
    uint32_t intersections = 0;
    for (size_t j = 0; j < horizontal_edges->count; ++j) {
      if (line_intersect(vertical_rays[i], horizontal_edges->data[j])) {
        // basically toggling from 0 to 1 etc
        intersections ^= 1;
      }
    }
    // if it's 0 (odd intersections) then another xor with 1 will result in 1 which is "true"
    if (intersections ^ 1) {
      return false;
    }
  }
  for (uint32_t i = 0; i < 4; ++i) {
    uint32_t intersections = 0;
    for (size_t j = 0; j < vertical_edges->count; ++j) {
      if (line_intersect(horizontal_rays[i], vertical_edges->data[j])) {
        intersections ^= 1;
      }
    }
    if (intersections ^ 1) {
      return false;
    }
  }

  return true;
}

static uint64_t solve_part2(const point *const points, const uint32_t point_count) {
  uint64_t biggest_area = 0;

  edge vert_edge_buffer[MAX_VERTICES / 2] = {0};
  edge hor_edge_buffer[MAX_VERTICES / 2] = {0};
  tlbt_deque_edge vertical_edges = {0};
  tlbt_deque_edge horizontal_edges = {0};

  tlbt_deque_edge_init(&vertical_edges, MAX_VERTICES / 2, vert_edge_buffer);
  tlbt_deque_edge_init(&horizontal_edges, MAX_VERTICES / 2, hor_edge_buffer);

  for (uint32_t i = 0; i < point_count; ++i) {
    uint32_t j = (i + 1) % point_count;
    point a = points[i];
    point b = points[j];
    if (a.x == b.x) {
      tlbt_deque_edge_push_back(&vertical_edges, (edge){.x = a.x, .ymin = MIN(a.y, b.y), .ymax = MAX(a.y, b.y)});
    } else if (a.y == b.y) {
      tlbt_deque_edge_push_back(&horizontal_edges, (edge){.y = a.y, .xmin = MIN(a.x, b.x), .xmax = MAX(a.x, b.x)});
    } else {
      tlbt_assert_unreachable();
    }
  }

  // this only works because there was no push/pop front used (head == 0)
  tlbt_assert(vertical_edges.head == 0);
  tlbt_assert(horizontal_edges.head == 0);

  for (uint32_t i = 0; i < point_count - 1; ++i) {
    for (uint32_t j = i + 1; j < point_count; ++j) {
      rect r = {.left = MIN(points[i].x, points[j].x),
                .top = MIN(points[i].y, points[j].y),
                .right = MAX(points[i].x, points[j].x),
                .bottom = MAX(points[i].y, points[j].y)};

      // deflate rectangle by 1 for handling weird edge cases
      rect r2 = {.left = r.left + 1, .top = r.top + 1, .right = r.right - 1, .bottom = r.bottom - 1};
      if (r2.left > r2.right || r2.top > r2.bottom)
        continue;

      if (is_rect_inside(r2, &vertical_edges, &horizontal_edges)) {
        const uint64_t width = (r.right - r.left) + 1;
        const uint64_t height = (r.bottom - r.top) + 1;
        const uint64_t area = width * height;
        if (area > biggest_area) {
          biggest_area = area;
        }
      }
    }
  }

  return biggest_area;
}

int main(int argc, char **argv) {
  if (argc != 2)
    return 1;

  char *input = NULL;
  size_t length = 0;
  if (!fileutils_read_all(argv[1], &input, &length))
    return 1;

  point points[MAX_VERTICES] = {0};
  uint32_t point_count = 0;

  parse_input(input, points, &point_count);
  free(input);

  uint64_t part1 = solve_part1(points, point_count);
  uint64_t part2 = solve_part2(points, point_count);

  printf("%lu\n", part1);
  printf("%lu\n", part2);
}

