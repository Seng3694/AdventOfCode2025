#include <ctype.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>

#include "../ext/toolbelt/src/assert.h"
#include "../ext/toolbelt/src/bitutils.h"
#include "../common/fileutils.h"

// max number of lights -> 10
// awk '{print length($1)-2}' day10/input.txt | sort -nu | tail -n1
// -2 to substract the square brackets
#define MAX_LIGHTS 10
// this means there are a max of 2^10 states of lights
#define MAX_LIGHT_STATES (1 << MAX_LIGHTS)

// max number of machines -> 159 (increase to 200)
// wc -l day10/input.txt
#define MAX_MACHINES 200

// max number of buttons per machine -> 13 (increase to 16)
// awk '{print NF-2}' day10/input.txt | sort -nu | tail -n1
// -2 to substract the light and joltage fields
#define MAX_BUTTONS 16

// max affected lights per button -> 9
// grep -Po '(?<=\()[\d,]+(?=\))' day10/input.txt | tr -cd ",\n" | awk '{print length($1)+1}' | sort -nu | tail -n 1
// +1 because I count commas -> (1,2) -> 1 comma + 1
#define MAX_AFFECTED_LIGHTS_PER_BUTTON 9

// max joltage requirements -> 11
// grep -Po '(?<=\{)[\d,]+(?=\})' day10/input.txt | tr -cd ",\n" | awk '{print length($1)+1}' | sort -nu | tail -n 1
// +1 because I count commas -> {1,2} -> 1 comma + 1
#define MAX_JOLTAGE_REQUIREMENTS 11

// max joltage requirement value is 269. so uint16_t as well
// grep -Po '(?<=\{)[\d,]+(?=\})' day10/input.txt | grep -Po '\d+' | sort -nu | tail -n1
#define MAX_JOLTAGE_REQUIREMENT_VALUE 269

// I can put the light settings in an uint16_t and use the bits to represent the lights
// while parsing the button settings, I can create bitmasks of the affected bits and just xor them
typedef struct machine {
  uint16_t lights;
  uint16_t buttons[MAX_BUTTONS];
  uint8_t button_count;
  uint16_t joltage_requirements[MAX_JOLTAGE_REQUIREMENTS];
  uint8_t joltage_requirement_count;
} machine;

static uint16_t parse_lights(char *input, char **out) {
  tlbt_assert_fmt(*input == '[', "expected '[', actual '%c' (%d)", *input, *input);
  ++input;
  uint16_t lights = 0;
  for (uint8_t count = 0; *input != ']'; ++count, ++input) {
    tlbt_assert_fmt(count < MAX_LIGHTS, "too many lights, expected <= %u", MAX_LIGHTS);
    tlbt_assert_fmt(*input == '.' || *input == '#', "expected '.' or '#', actual '%c' (%d)", *input, *input);
    if (*input == '#')
      lights = TLBT_SET_BIT(lights, count);
  }
  tlbt_assert_fmt(*input == ']', "expected ']', actual '%c' (%d)", *input, *input);
  *out = input + 1;
  return lights;
}

inline static void skip_spaces(char *input, char **out) {
  while (*input == ' ')
    ++input;
  *out = input;
}

static uint16_t parse_button(char *input, char **out) {
  uint16_t button = 0;
  tlbt_assert_fmt(*input == '(', "expected '(', actual '%c' (%d)", *input, *input);

  do {
    ++input; // either the opening brace or a comma
    tlbt_assert_fmt(isdigit(*input), "expected digit, actual '%c' (%d)", *input, *input);
    uint8_t affected_light = strtoul(input, &input, 10);
    button = TLBT_SET_BIT(button, affected_light);
    tlbt_assert_fmt(*input == ')' || *input == ',', "expected ')' or ',', actual '%c' (%d)", *input, *input);
  } while (*input == ',');

  tlbt_assert_fmt(*input == ')', "expected ')', actual '%c' (%d)", *input, *input);
  *out = input + 1;
  return button;
}

static uint8_t parse_buttons(char *input, char **out, uint16_t *const buttons) {
  uint8_t count = 0;
  skip_spaces(input, &input);
  tlbt_assert_fmt(*input == '(', "expected '(', actual '%c' (%d)", *input, *input);
  while (*input == '(') {
    tlbt_assert_fmt(count < MAX_BUTTONS, "too many buttons, max: %u", MAX_BUTTONS);
    buttons[count++] = parse_button(input, &input);
    skip_spaces(input, &input);
  }
  *out = input;
  return count;
}

static uint8_t parse_joltage_requirements(char *input, char **out, uint16_t *const joltage_requirements) {
  uint8_t count = 0;
  tlbt_assert_fmt(*input == '{', "expected '{', actual '%c' (%d)", *input, *input);
  do {
    ++input; // either the opening brace or a comma
    tlbt_assert_fmt(count < MAX_JOLTAGE_REQUIREMENTS, "too many joltage requirements, max: %u",
                    MAX_JOLTAGE_REQUIREMENTS);
    tlbt_assert_fmt(isdigit(*input), "expected digit, actual '%c' (%d)", *input, *input);
    joltage_requirements[count++] = strtoul(input, &input, 10);
    tlbt_assert_fmt(joltage_requirements[count - 1] <= MAX_JOLTAGE_REQUIREMENT_VALUE,
                    "joltage too big. expected <= %u, actual %u", MAX_JOLTAGE_REQUIREMENT_VALUE,
                    joltage_requirements[count - 1]);
    tlbt_assert_fmt(*input == '}' || *input == ',', "expected '}' or ',', actual '%c' (%d)", *input, *input);
  } while (*input == ',');

  tlbt_assert_fmt(*input == '}', "expected '}', actual '%c' (%d)", *input, *input);
  *out = input + 1;
  return count;
}

static void parse_machine(char *input, char **out, machine *const m) {
  m->lights = parse_lights(input, &input);
  m->button_count = parse_buttons(input, &input, m->buttons);
  m->joltage_requirement_count = parse_joltage_requirements(input, &input, m->joltage_requirements);
  *out = input;
}

static void parse_input(char *input, machine *const machines, uint8_t *const machine_count) {
  uint8_t c = 0;
  for (;;) {
    switch (*input) {
    case '\0':
      *machine_count = c;
      return;
    case '\n':
      ++input;
      break;
    case '[':
      tlbt_assert_fmt(c < MAX_MACHINES, "too many machines, max: %u", MAX_MACHINES);
      parse_machine(input, &input, &machines[c++]);
      tlbt_assert_fmt(*input == '\n' || *input == '\0', "expected new line or zero terminator, actual '%c' (%d)",
                      *input, *input);
      break;
    default:
      tlbt_assert_unreachable();
      break;
    }
  }
}

static void print_machines(const machine *const machines, const uint8_t machine_count) {
  for (uint8_t i = 0; i < machine_count; ++i) {
    putchar('[');
    uint8_t count = 16 - (__builtin_clz((uint32_t)machines[i].lights) - 16);
    for (int16_t j = 0; j < count; j++) {
      putchar(TLBT_CHECK_BIT(machines[i].lights, j) ? '#' : '.');
    }
    printf("] ");

    for (uint8_t j = 0; j < machines[i].button_count; ++j) {
      count = 16 - (__builtin_clz((uint32_t)machines[i].buttons[j]) - 16);
      putchar('(');
      for (int16_t k = count - 1; k >= 0; --k) {
        if (TLBT_CHECK_BIT(machines[i].buttons[j], k))
          printf("%u,", k);
      }
      putchar(')');
      putchar(' ');
    }

    putchar('{');
    for (uint8_t j = 0; j < machines[i].joltage_requirement_count; ++j) {
      printf("%u,", machines[i].joltage_requirements[j]);
    }
    printf("}\n");
  }
}

inline static uint32_t light_state_hash(const uint16_t s) {
  const uint32_t hash = 36591911;
  return ((hash << 5) + hash) + (uint32_t)s;
}

inline static bool light_state_equals(const uint16_t left, const uint16_t right) {
  return left == right;
}

#define TLBT_KEY_T uint16_t
#define TLBT_KEY_T_NAME light_state
#define TLBT_HASH_FUNC light_state_hash
#define TLBT_EQUALS_FUNC light_state_equals
#define TLBT_BASE2_CAPACITY
#define TLBT_DYNAMIC_MEMORY
#define TLBT_STATIC
#include "../ext/toolbelt/src/hashmap.h"

#define TLBT_T uint16_t
#define TLBT_T_NAME light_state
#define TLBT_DYNAMIC_MEMORY
#define TLBT_STATIC
#include "../ext/toolbelt/src/deque.h"

static uint32_t solve_part1(const machine *const machines, const uint8_t machine_count) {
  uint32_t solution = 0;

  tlbt_set_light_state visited = {0};
  tlbt_set_light_state_create(&visited, 256);

  tlbt_deque_light_state states = {0};
  tlbt_deque_light_state_create(&states, 256);

  for (uint8_t i = 0; i < machine_count; ++i) {
    tlbt_set_light_state_clear(&visited);
    tlbt_deque_light_state_clear(&states);

    uint16_t start = 0;
    const uint16_t goal = machines[i].lights;
    tlbt_set_light_state_insert(&visited, start);
    tlbt_deque_light_state_push_back(&states, start);
    uint32_t moves = 0;
    while (states.count > 0) {
      const size_t state_count = states.count;
      for (size_t j = 0; j < state_count; ++j) {
        const uint16_t current = *tlbt_deque_light_state_peek_front(&states);
        tlbt_deque_light_state_pop_front(&states);

        for (uint8_t b = 0; b < machines[i].button_count; ++b) {
          const uint16_t next = current ^ machines[i].buttons[b];
          if (next == goal) {
            solution += (moves + 1);
            goto next_machine;
          }
          const uint32_t hash = light_state_hash(next);
          if (!tlbt_set_light_state_contains_ph(&visited, next, hash)) {
            tlbt_set_light_state_insert_ph(&visited, next, hash);
            tlbt_deque_light_state_push_back(&states, next);
          }
        }
      }
      ++moves;
    }
  next_machine:;
  }

  tlbt_deque_light_state_destroy(&states);
  tlbt_set_light_state_destroy(&visited);
  return solution;
}

int main(int argc, char **argv) {
  if (argc != 2)
    return 1;

  char *input = NULL;
  size_t length = 0;
  if (!fileutils_read_all(argv[1], &input, &length))
    return 1;

  machine machines[MAX_MACHINES] = {0};
  uint8_t machine_count = 0;

  parse_input(input, machines, &machine_count);
  free(input);

  uint32_t part1 = solve_part1(machines, machine_count);

  printf("%u\n", part1);
}

