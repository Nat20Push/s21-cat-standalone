#include <stdbool.h>
#include <stdio.h>
#include <string.h>

typedef struct {
  bool number_lines;
  bool number_nonempty_lines;
  bool show_nonprinting;
  bool show_tabs;
  bool show_ends;
  bool squeeze_blank;
} Flags;

static int check_flag(const char *argument, Flags *flags) {
  if (strcmp(argument, "--number") == 0) {
    flags->number_lines = true;
  } else if (strcmp(argument, "--number-nonblank") == 0) {
    flags->number_nonempty_lines = true;
  } else if (strcmp(argument, "--squeeze-blank") == 0) {
    flags->squeeze_blank = true;
  } else if (argument[0] == '-' && argument[1] != '\0') {
    for (int i = 1; argument[i] != '\0'; i++) {
      switch (argument[i]) {
      case 'b':
        flags->number_nonempty_lines = true;
        break;
      case 'e':
        flags->show_ends = true;
        flags->show_nonprinting = true;
        break;
      case 'n':
        flags->number_lines = true;
        break;
      case 's':
        flags->squeeze_blank = true;
        break;
      case 't':
        flags->show_tabs = true;
        flags->show_nonprinting = true;
        break;
      case 'v':
        flags->show_nonprinting = true;
        break;
      case 'E':
        flags->show_ends = true;
        break;
      case 'T':
        flags->show_tabs = true;
        break;
      default:
        return 0;
      }
    }
  } else {
    return 0;
  }

  return 1;
}

static void print_nonprinting(int c) {
  if (c < 32) {
    putchar('^');
    putchar(c == 0 ? '@' : c + 'A' - 1);
  } else if (c == 127) {
    printf("^?");
  } else if (c >= 128) {
    printf("M-");
    if (c < 160) {
      putchar('^');
      putchar(c - 128 + 'A' - 1);
    } else if (c == 255) {
      printf("^?");
    } else {
      putchar(c - 128);
    }
  } else {
    putchar(c);
  }
}

static void print_character(int c, const Flags *flags) {
  if (c == '\n' && flags->show_ends) {
    printf("$\n");
  } else if (c == '\t' && flags->show_tabs) {
    printf("^I");
  } else if (flags->show_nonprinting && c != '\n' && c != '\t') {
    print_nonprinting(c);
  } else {
    putchar(c);
  }
}

static void print_line_number(int *line_number) {
  printf("%6d\t", *line_number);
  (*line_number)++;
}

static int process_stream(FILE *stream, const Flags *flags, int *line_number,
                          bool *new_line, int *blank_lines) {
  int c;

  while ((c = fgetc(stream)) != EOF) {
    if (flags->squeeze_blank && c == '\n' && *new_line) {
      if (*blank_lines >= 1) {
        continue;
      }
      (*blank_lines)++;
    } else if (c != '\n') {
      *blank_lines = 0;
    }

    if (*new_line) {
      if (flags->number_nonempty_lines && c != '\n') {
        print_line_number(line_number);
      } else if (flags->number_lines && !flags->number_nonempty_lines) {
        print_line_number(line_number);
      }
    }

    if (c == '\r' && flags->show_ends && !flags->show_nonprinting) {
      int next = fgetc(stream);
      if (next == '\n') {
        fputs("^M", stdout);
        c = '\n';
      } else if (next != EOF) {
        ungetc(next, stream);
      }
    }

    print_character(c, flags);

    if (c == '\n') {
      *new_line = true;
    } else {
      *new_line = false;
    }
  }

  return ferror(stream) ? 1 : 0;
}

static int print_file(const char *filename, const Flags *flags,
                      int *line_number, bool *new_line, int *blank_lines) {
  FILE *file = fopen(filename, "rb");

  if (file == NULL) {
    fprintf(stderr, "s21_cat: cannot open '%s'\n", filename);
    return 1;
  }

  int result = process_stream(file, flags, line_number, new_line, blank_lines);

  fclose(file);
  return result;
}

int main(int argc, char *argv[]) {
  Flags flags = {0};
  int line_number = 1;
  int blank_lines = 0;
  bool new_line = true;
  bool has_file = false;
  int result = 0;

  /* Сначала собираем флаги. */
  for (int i = 1; i < argc; i++) {
    check_flag(argv[i], &flags);
  }

  /* Затем обрабатываем все аргументы, которые не являются флагами. */
  for (int i = 1; i < argc; i++) {
    if (check_flag(argv[i], &(Flags){0})) {
      continue;
    }

    has_file = true;

    if (strcmp(argv[i], "-") == 0) {
      if (process_stream(stdin, &flags, &line_number, &new_line,
                         &blank_lines) != 0) {
        result = 1;
      }
    } else {
      if (print_file(argv[i], &flags, &line_number, &new_line, &blank_lines) !=
          0) {
        result = 1;
      }
    }
  }

  /* Если имён файлов не было, читаем из stdin. */
  if (!has_file) {
    if (process_stream(stdin, &flags, &line_number, &new_line, &blank_lines) !=
        0) {
      result = 1;
    }
  }

  return result;
}