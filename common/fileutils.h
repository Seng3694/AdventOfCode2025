#pragma once

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

static bool fileutils_read_all(const char *file_name, char **out, size_t *length) {
  FILE *f = fopen(file_name, "r");
  if (!f) {
    fprintf(stderr, "couldn't open file '%s'\n", file_name);
    return false;
  }
  fseek(f, 0, SEEK_END);
  const size_t size = ftell(f);
  rewind(f);
  char *buffer = malloc(size + 1);
  fread(buffer, size, 1, f);
  fclose(f);
  buffer[size] = '\0';
  *out = buffer;
  *length = size + 1;
  return true;
}

