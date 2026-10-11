#!/bin/sh

set -u
LC_ALL=C
export LC_ALL

SCRIPT_DIR=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
PROGRAM="$SCRIPT_DIR/s21_cat"

if [ ! -x "$PROGRAM" ]; then
  echo "Build s21_cat first: make -C '$SCRIPT_DIR'" >&2
  exit 1
fi

if [ -n "${CAT_REFERENCE:-}" ]; then
  REFERENCE=$CAT_REFERENCE
elif command -v gcat >/dev/null 2>&1; then
  REFERENCE=$(command -v gcat)
else
  REFERENCE=$(command -v cat)
fi

TEMP_DIR=$(mktemp -d "${TMPDIR:-/tmp}/s21_cat_test.XXXXXX") || exit 1
trap 'rm -rf "$TEMP_DIR"' EXIT HUP INT TERM

printf 'one\n\n\n\tTabbed\n\001\177\200\377\nlast' > "$TEMP_DIR/main.txt"
printf 'second\n\nend\n' > "$TEMP_DIR/second.txt"
printf 'a\r\nb\r\n' > "$TEMP_DIR/crlf.txt"
: > "$TEMP_DIR/empty.txt"

passed=0
failed=0
skipped=0

run_test() {
  name=$1
  shift

  if "$REFERENCE" "$@" < "$TEMP_DIR/main.txt" > "$TEMP_DIR/expected" 2> "$TEMP_DIR/reference_error"; then
    reference_status=0
  else
    reference_status=$?
  fi
  if [ "$reference_status" -ne 0 ]; then
    printf 'SKIP  %s (reference cat does not support these arguments)\n' "$name"
    skipped=$((skipped + 1))
    return
  fi

  if "$PROGRAM" "$@" < "$TEMP_DIR/main.txt" > "$TEMP_DIR/actual" 2> "$TEMP_DIR/program_error"; then
    program_status=0
  else
    program_status=$?
  fi

  if [ "$program_status" -eq "$reference_status" ] &&
     cmp -s "$TEMP_DIR/expected" "$TEMP_DIR/actual"; then
    printf 'PASS  %s\n' "$name"
    passed=$((passed + 1))
  else
    printf 'FAIL  %s (reference exit=%s, s21_cat exit=%s)\n' \
      "$name" "$reference_status" "$program_status"
    printf '  expected bytes: '
    od -An -tx1 "$TEMP_DIR/expected" | head -3
    printf '  actual bytes:   '
    od -An -tx1 "$TEMP_DIR/actual" | head -3
    failed=$((failed + 1))
  fi
}

run_test 'file without flags' "$TEMP_DIR/main.txt"
run_test 'stdin without flags'
run_test 'stdin as -' -
run_test 'empty file' "$TEMP_DIR/empty.txt"
run_test 'multiple files' "$TEMP_DIR/main.txt" "$TEMP_DIR/second.txt"
run_test '-b' -b "$TEMP_DIR/main.txt"
run_test '--number-nonblank' --number-nonblank "$TEMP_DIR/main.txt"
run_test '-n' -n "$TEMP_DIR/main.txt"
run_test '--number' --number "$TEMP_DIR/main.txt"
run_test '-s' -s "$TEMP_DIR/main.txt"
run_test '--squeeze-blank' --squeeze-blank "$TEMP_DIR/main.txt"
run_test '-e' -e "$TEMP_DIR/main.txt"
run_test '-E' -E "$TEMP_DIR/main.txt"
run_test '-t' -t "$TEMP_DIR/main.txt"
run_test '-T' -T "$TEMP_DIR/main.txt"
run_test '-v' -v "$TEMP_DIR/main.txt"
run_test '-bn' -bn "$TEMP_DIR/main.txt"
run_test '-ens' -ens "$TEMP_DIR/main.txt"
run_test '-E on CRLF' -E "$TEMP_DIR/crlf.txt"
run_test '-e on CRLF' -e "$TEMP_DIR/crlf.txt"
run_test 'flags after first file' "$TEMP_DIR/second.txt" -n "$TEMP_DIR/main.txt"

printf '\nResult: %s passed, %s failed, %s skipped\n' "$passed" "$failed" "$skipped"
[ "$failed" -eq 0 ]
