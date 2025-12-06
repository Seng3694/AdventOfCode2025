#include <ctype.h>
#include <math.h>
#include <stdio.h>
#include <stdint.h>

#include "../ext/toolbelt/src/assert.h"
#include "../common/fileutils.h"

// awk '{print NF}' day06/input.txt | sort -u | tail -n 1
// -> 1000 numbers per line
#define MAX_TOKENS_PER_LINE 2001 // with spaces in between

// wc -l
// -> 5 lines (4 number lines, 1 operator line)
#define MAX_LINES 5

typedef enum equation_type {
  EQUATION_TYPE_ADD,
  EQUATION_TYPE_MUL,
} equation_type;

typedef enum equation_alignment {
  EQUATION_ALIGNMENT_LEFT,
  EQUATION_ALIGNMENT_RIGHT,
} equation_alignment;

// grep -Po '\d+' day06/input.txt | sort -nu | tail -n 1
// -> 9986 biggest number (no negative numbers)
typedef struct equation {
  uint8_t type;      // equation_type
  uint8_t alignment; // equation_alignment
  uint16_t values[MAX_LINES];
  uint16_t reverse[MAX_LINES];
} equation;

typedef enum token_type {
  TOKEN_TYPE_SPACE,
  TOKEN_TYPE_NUMBER,
  TOKEN_TYPE_OPERATOR,
} token_type;

typedef struct token {
  uint8_t type; // token_type
  char *start;
  uint8_t length;
} token;

typedef struct token_line {
  token tokens[MAX_TOKENS_PER_LINE];
  uint16_t count;
} token_line;

void tokenize_input(char *input, token_line *const lines, uint16_t *const line_count) {
  uint16_t line = 0;
  uint32_t i = 0;

  // prepend a space token on every line with a length of 1
  // do it for the first line here
  lines[0].tokens[i].type = TOKEN_TYPE_SPACE;
  lines[0].tokens[i++].length = 1;

  char *start = input;
  for (;;) {
    switch (*input) {
    case '\0':
      *line_count = line;
      return;
    case '\n':
      do {
        ++input;
      } while (*input == '\n');
      // append a space token if there isn't one already
      tlbt_assert(i > 0);
      if (lines[line].tokens[i - 1].type != TOKEN_TYPE_SPACE) {
        lines[line].tokens[i].type = TOKEN_TYPE_SPACE;
        lines[line].tokens[i].length = 0;
        ++i;
      }

      lines[line].count = i;
      ++line;
      if (*input != '\0') {
        i = 0;
        lines[line].tokens[i].type = TOKEN_TYPE_SPACE;
        lines[line].tokens[i++].length = 1;
      }
      break;
    case ' ':
      start = input;
      while (*input == ' ') {
        ++input;
      }
      // if previous token is a space token, then add data in there
      tlbt_assert(i > 0);
      if (lines[line].tokens[i - 1].type == TOKEN_TYPE_SPACE) {
        lines[line].tokens[i - 1].type = TOKEN_TYPE_SPACE;
        lines[line].tokens[i - 1].length += input - start;
      } else {
        lines[line].tokens[i].type = TOKEN_TYPE_SPACE;
        lines[line].tokens[i].length = input - start;
        ++i;
      }
      break;
    case '+':
    case '*':
      lines[line].tokens[i].type = TOKEN_TYPE_OPERATOR;
      lines[line].tokens[i].start = input;
      lines[line].tokens[i].length = 1;
      ++input;
      ++i;
      break;
    default:
      start = input;
      while (isdigit(*input)) {
        ++input;
      }
      lines[line].tokens[i].type = TOKEN_TYPE_NUMBER;
      lines[line].tokens[i].start = start;
      lines[line].tokens[i].length = input - start;
      ++i;
      break;
    }
  }
}

static bool parse_number(char *input, uint16_t *forward, uint16_t *backward) {
  if (!isdigit(*input))
    return false;

  char *start = input;
  while (isdigit(*input))
    input++;

  const size_t len = input - start;
  uint16_t fwd = 0;
  uint16_t bwd = 0;
  for (size_t i = 0; i < len; ++i) {
    fwd += (start[i] - '0') * (uint16_t)pow(10, len - i - 1);
    bwd += (start[i] - '0') * (uint16_t)pow(10, i);
  }
  *forward = fwd;
  *backward = bwd;
  return true;
}

static void parse_tokens(token_line *const lines, const uint8_t line_count, equation *const equations,
                         uint16_t *const equation_count) {
#ifndef NDEBUG
  {
    // assert that every line starts and ends with a space token and that all lines have the same length
    uint16_t line_length = lines[0].count;
    for (uint8_t i = 0; i < line_count; ++i) {
      tlbt_assert_msg(lines[i].tokens[0].type == TOKEN_TYPE_SPACE, "expected line to start with space token");
      tlbt_assert_msg(lines[i].tokens[lines[i].count - 1].type == TOKEN_TYPE_SPACE,
                      "expected line to end with space token");
      tlbt_assert_fmt(lines[i].count == line_length, "expected same line length '%u', actual '%u'", line_length,
                      lines[i].count);
    }
  }
#endif

  const uint8_t operator_line_index = line_count - 1;
  const uint8_t line_count_wo_op_line = operator_line_index;
  uint16_t line_length_wo_spaces = lines[0].count / 2;
  *equation_count = line_length_wo_spaces;
  for (uint32_t i = 0; i < line_length_wo_spaces; ++i) {
    const uint32_t token_index = i * 2;
    bool is_right_aligned = false;
    for (uint8_t j = 0; j < line_count_wo_op_line; ++j) {
      if (lines[j].tokens[token_index].length > 1) {
        is_right_aligned = true;
        break;
      }
    }
    equations[i].alignment = is_right_aligned ? EQUATION_ALIGNMENT_RIGHT : EQUATION_ALIGNMENT_LEFT;
    equations[i].type =
        *lines[operator_line_index].tokens[token_index + 1].start == '*' ? EQUATION_TYPE_MUL : EQUATION_TYPE_ADD;
    for (uint8_t j = 0; j < line_count_wo_op_line; ++j) {
      parse_number(lines[j].tokens[token_index + 1].start, &equations[i].values[j], &equations[i].reverse[j]);
      if (equations[i].alignment == EQUATION_ALIGNMENT_LEFT) {
        // if alignment is left, then subtract the width of the equation from the next space token
        // the operator always starts at the beginning. so the distance to the next operator is the width
        const int16_t total_length = lines[operator_line_index].tokens[token_index + 2].length;
        const int16_t token_length = lines[j].tokens[token_index + 1].length;
        lines[j].tokens[token_index + 2].length -= total_length - token_length;
      }
    }
  }
}

static uint64_t solve_part1(const equation *const equations, const uint16_t equation_count,
                            const uint8_t operand_count) {
  uint64_t solution = 0;
  for (uint16_t i = 0; i < equation_count; ++i) {
    if (equations[i].type == EQUATION_TYPE_ADD) {
      uint64_t r = 0;
      for (uint16_t j = 0; j < operand_count; ++j) {
        r += equations[i].values[j];
      }
      solution += r;
    } else {
      uint64_t r = 1;
      for (uint16_t j = 0; j < operand_count; ++j) {
        r *= equations[i].values[j];
      }
      solution += r;
    }
  }
  return solution;
}

static uint64_t solve_part2(equation *const equations, const uint16_t equation_count, const uint8_t operand_count) {
  uint64_t solution = 0;

  for (uint16_t i = 0; i < equation_count; ++i) {
    uint16_t n = 0;
    // parsed the number both normally and in reverse order. depending on alignment use the reversed one
    uint16_t *v = equations[i].alignment == EQUATION_ALIGNMENT_LEFT ? equations[i].reverse : equations[i].values;

    // code duplication is a bit annoying in this case but besides macros I'm not aware of any other zero cost
    // abstractions. can only hope on compiler optimizations but some are not aggressive enough. so macro it is
#define CASE_X(name, init, op)                                                                                         \
  case name: {                                                                                                         \
    uint64_t r = init;                                                                                                 \
    for (;;) {                                                                                                         \
      n = 0;                                                                                                           \
      for (uint8_t j = 0; j < operand_count; ++j) {                                                                    \
        uint8_t digit = v[j] % 10;                                                                                     \
        v[j] /= 10;                                                                                                    \
        if (digit == 0) {                                                                                              \
          n /= 10;                                                                                                     \
        } else {                                                                                                       \
          n += digit * pow(10, operand_count - j - 1);                                                                 \
        }                                                                                                              \
      }                                                                                                                \
      if (n == 0)                                                                                                      \
        break;                                                                                                         \
      r = r op n;                                                                                                      \
    }                                                                                                                  \
    solution += r;                                                                                                     \
    break;                                                                                                             \
  }

    switch (equations[i].type) {
      CASE_X(EQUATION_TYPE_ADD, 0, +)
      CASE_X(EQUATION_TYPE_MUL, 1, *)
    }
  }

#undef CASE_X
  return solution;
}

int main(int argc, char **argv) {
  if (argc != 2)
    return 1;

  char *input = NULL;
  size_t length = 0;
  if (!fileutils_read_all(argv[1], &input, &length))
    return 1;

  token_line lines[MAX_LINES] = {0};
  uint16_t line_count = 0;
  tokenize_input(input, lines, &line_count);

  equation equations[MAX_TOKENS_PER_LINE / 2] = {0};
  uint16_t equation_count = 0;
  parse_tokens(lines, line_count, equations, &equation_count);
  free(input);

  // line_count - 1 because line_count included the operator_line
  uint64_t part1 = solve_part1(equations, equation_count, line_count - 1);
  uint64_t part2 = solve_part2(equations, equation_count, line_count - 1);

  printf("%lu\n", part1);
  printf("%lu\n", part2);
}

