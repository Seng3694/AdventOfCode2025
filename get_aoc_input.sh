#!/bin/bash

DAY="$1"
OUTDIR="$2"



for profile in ~/.librewolf/*/; do
  if [ -f "$profile/cookies.sqlite" ]; then
    cp "$profile/cookies.sqlite" /tmp/aoc_cookies.sqlite
    SESSION=$(sqlite3 /tmp/aoc_cookies.sqlite \
      "SELECT value FROM moz_cookies \
       WHERE host = '.adventofcode.com' AND name = 'session' \
       ORDER BY lastAccessed DESC LIMIT 1;")
    if [ -n "$SESSION" ]; then
      curl -s --cookie "session=$SESSION" \
        "https://adventofcode.com/2025/day/$DAY/input" \
        -o "$OUTDIR/input.txt"
      break
    fi
  fi
done

rm /tmp/aoc_cookies.sqlite

exit 0

