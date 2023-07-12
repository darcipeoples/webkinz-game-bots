#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_CODE_LENGTH 10

typedef struct {
  int green;
  int red;
} Score;

// typedef struct {
//   int score;
//   int freq;
// } ScoreFrequency;

typedef struct {
  int freq;
  double prob;
  double info;
  double pinfo;
} ScoreInfo;

typedef struct {
  int remain;
  char guess[MAX_CODE_LENGTH + 1];
  ScoreInfo scores[MAX_CODE_LENGTH][MAX_CODE_LENGTH];
} TreeNode;

typedef struct {
  int num_digits;
  int has_repeats;
  int max_depth;
  char guesses[MAX_CODE_LENGTH][MAX_CODE_LENGTH + 1];
  Score responses[MAX_CODE_LENGTH];
} CalculationData;

void get_best_first_guess(int num_digits, int has_repeats, char *guess) {
  if (num_digits == 3) {
    strcpy(guess, "012");
  } else if (num_digits == 4) {
    strcpy(guess, "0123");
  } else if (num_digits == 5) {
    strcpy(guess, "01234");
  } else {
    strcpy(guess, "001234");
  }
}

Score compare_codes(const char *a, const char *b) {
  Score score = {0, 0};
  int count1[10] = {0};
  int count2[10] = {0};

  for (int i = 0; a[i] != '\0' && b[i] != '\0'; i++) {
    if (a[i] == b[i]) {
      score.green++;
    } else {
      count1[a[i] - '0']++;
      count2[b[i] - '0']++;
    }
  }

  for (int i = 0; i < 10; i++) {
    score.red += fmin(count1[i], count2[i]);
  }

  return score;
}

// void calc_entropy_distribution(const char *word, const char **possible,
//                                int possible_count, double *expected_info,
//                                ScoreInfo *distribution) {
//   int initial_count = possible_count;
//   *expected_info = 0;

//   for (int i = 0; i < possible_count; i++) {
//     const char *p = possible[i];
//     Score score = compare_codes(p, word);
//     distribution[score.green][score.red].freq++;
//   }

//   for (int i = 0; i < MAX_CODE_LENGTH; i++) {
//     for (int j = 0; j < MAX_CODE_LENGTH; j++) {
//       ScoreInfo *info = &distribution[i][j];
//       int freq = info->freq;
//       if (freq > 0) {
//         double prob = (double)freq / initial_count;
//         double log_prob = log2(1 / prob);
//         info->prob = prob;
//         info->info = log_prob;
//         info->pinfo = prob * log_prob;
//         *expected_info += prob * log_prob;
//       }
//     }
//   }
// }

double calc_expected_info(const char *word, const char **possible,
                          int possible_count) {
  int initial_count = possible_count;
  double expected_info = 0;
  int distribution[MAX_CODE_LENGTH][MAX_CODE_LENGTH] = {0};

  for (int i = 0; i < possible_count; i++) {
    const char *p = possible[i];
    Score score = compare_codes(p, word);
    distribution[score.green][score.red]++;
  }

  for (int i = 0; i < MAX_CODE_LENGTH; i++) {
    for (int j = 0; j < MAX_CODE_LENGTH; j++) {
      int freq = distribution[i][j];
      if (freq > 0) {
        double prob = (double)freq / initial_count;
        double log_prob = log2(1 / prob);
        expected_info += prob * log_prob;
      }
    }
  }

  // TODO: round to nearest 10th decimal?
  // return round(expected_info * 1000000000000) / 1000000000000;
  return expected_info;
}

char **dedupe_codes_to_patterns(const char **guesses, int guess_count,
                                const char **codes, int code_count,
                                int *deduped_count) {
  // Array of which digits 0-9 were seen in previous guesses
  char guess_chars[10] = {0};
  for (int i = 0; i < guess_count; i++) {
    const char *guess = guesses[i];
    while (*guess != '\0') {
      guess_chars[*guess - '0'] = *guess;
      guess++;
    }
  }

  // Array of unused characters
  char unused_chars[10] = {0};
  int unused_count = 0;
  for (int i = 0; i < 10; i++) {
    if (!guess_chars[i]) {
      unused_chars[unused_count++] = i + '0';
      // printf("i=%d, unused_char=%c, unused_count=%d\n", i,
      // unused_chars[unused_count-1], unused_count);
    }
  }

  int code_patterns_count = 0;
  char **deduped_codes = (char **)malloc(code_count * sizeof(char *));
  char **code_patterns = (char **)malloc(code_count * sizeof(char *));

  for (int i = 0; i < code_count; i++) {
    const char *code = codes[i];
    char char_map[10];
    memcpy(char_map, guess_chars, sizeof(guess_chars));
    // if (i == 0) {
    //     for  (int k = 0; i < 10; i++) {
    //         printf("i=%d, guess_chars=%d, char_map=%d\n", i, guess_chars[i],
    //         char_map[i]);
    //     }
    // }

    char code_pattern[MAX_CODE_LENGTH + 1] = {0};

    int is_ascending = 1;
    char prev_char = '\0';
    int unused_idx = 0;
    for (int j = 0; code[j] != '\0'; j++) {
      char c = code[j];
      if (!char_map[c - '0']) {
        char_map[c - '0'] = unused_chars[unused_idx++];
        // printf("code=%s, %c -> %c\n", code, c, char_map[c - '0']);
      }
      code_pattern[j] = char_map[c - '0'];
      // If no previous guesses, only keep code_patterns that are in ascending
      // order
      if (guess_count == 0) {
        if (prev_char != '\0' && code_pattern[j] < prev_char) {
          is_ascending = 0;
          break;
        }
        prev_char = code_pattern[j];
      }
    }
    if (guess_count == 0 && !is_ascending) {
      continue;
    }

    // TODO: This is terribly slow. Should use a set or dict.
    int is_duplicate = 0;
    for (int k = 0; k < code_patterns_count; k++) {
      if (strcmp(code_patterns[k], code_pattern) == 0) {
        is_duplicate = 1;
        break;
      }
    }

    // printf("i=%d, code=%s, code_pattern=%s, is_duplicate=%d\n", i, code,
    // code_pattern, is_duplicate);

    if (!is_duplicate) {
      code_patterns[code_patterns_count] =
          (char *)malloc((strlen(code_pattern) + 1) * sizeof(char));
      deduped_codes[code_patterns_count] =
          (char *)malloc((strlen(code_pattern) + 1) * sizeof(char));
      strcpy(code_patterns[code_patterns_count], code_pattern);
      strcpy(deduped_codes[code_patterns_count], code);
      code_patterns_count++;
    }
  }

  deduped_codes =
      (char **)realloc(deduped_codes, code_patterns_count * sizeof(char *));
  *deduped_count = code_patterns_count;

  for (int i = 0; i < code_patterns_count; i++) {
    free(code_patterns[i]);
  }
  free(code_patterns);

  return deduped_codes;
}

// char *calc_best_first_guess(int num_digits, int has_repeats) {
//   char *guess = (char *)malloc((num_digits + 1) * sizeof(char));
//   get_best_first_guess(num_digits, has_repeats, guess);
//   return guess;
// }

int unique_chars(const char *s) {
  int counts[10] = {0};

  for (int i = 0; s[i] != '\0'; i++) {
    int digit = s[i] - '0';
    counts[digit]++;
    if (counts[digit] > 1) {
      return 0;
    }
  }

  return 1;
}

typedef struct {
  char **possible;
  int possible_count;
  char **unused;
  int unused_count;
} PossibleUnused;

PossibleUnused calc_initial_possible_unused(int num_digits, int has_repeats) {
  int total_codes = pow(10, num_digits);

  char **possible = (char **)malloc(total_codes * sizeof(char *));
  char **unused = (char **)malloc(total_codes * sizeof(char *));

  int possible_count = 0;
  for (int i = 0; i < total_codes; i++) {
    unused[i] = (char *)malloc((num_digits + 1) * sizeof(char));
    sprintf(unused[i], "%0*d", num_digits, i);

    if (!has_repeats) {
      if (unique_chars(unused[i])) {
        possible[possible_count] =
            (char *)malloc((num_digits + 1) * sizeof(char));
        strcpy(possible[possible_count], unused[i]);
        possible_count += 1;
      }
    } else {
      possible[i] = (char *)malloc((num_digits + 1) * sizeof(char));
      strcpy(possible[i], unused[i]);
    }
  }

  if (has_repeats) {
    possible_count = total_codes;
  } else {
    // TODO: do we need to do this? Does it slow it down?
    possible = (char **)realloc(possible, possible_count * sizeof(char *));
  }
  int unused_count = total_codes;

  // TODO: should I return by modifying parameter or return struct?
  PossibleUnused poss_unused = {0};
  poss_unused.possible = possible;
  poss_unused.possible_count = possible_count;
  poss_unused.unused = unused;
  poss_unused.unused_count = unused_count;
  return poss_unused;
}

void calc_best_next_guess(const char **search_set, int search_set_count,
                          int max_search_time, const char **possible,
                          int possible_count, char *suggestion) {
  if (max_search_time > 0) {
    printf("Searching (for up to %d seconds)...\n", max_search_time);
  } else {
    printf("Searching (no time limit)...\n");
  }

  int best_idx = 0;
  double best_expected_info = 0;
  const char *best_guess;

  for (int i = 0; i < search_set_count - 1; i++) {
    const char *guess = search_set[i];
    double expected_info = calc_expected_info(guess, possible, possible_count);
    if (expected_info > best_expected_info) {
      best_idx = i;
      best_expected_info = expected_info;
      best_guess = guess;
    }

    if (i % 1000 == 0) {
      printf("iter %d/%d (%.1f%%): best_idx = %d, best guess = '%s', expected"
             "info = %f\n",
             i, search_set_count, (double)i / search_set_count * 100, best_idx,
             search_set[best_idx], best_expected_info);
    }

    // if (max_search_time > 0 && i + 1 >= max_search_time) {
    //   printf("INFO: Ran out of time to keep searching");
    //   break;
    // }
  }

  strcpy(suggestion, search_set[best_idx]);
  printf("Found  best_idx = %d, best_guess = '%s', best_expected_info = %f\n",
         best_idx, search_set[best_idx], best_expected_info);
}

// void calc_tree_helper(const CalculationData *data, TreeNode *tree, int
// depth)
// {
//   int num_digits = data->num_digits;
//   int has_repeats = data->has_repeats;
//   int max_depth = data->max_depth;
//   int guess_count = depth;
//   int possible_count;
//   const char **possible =
//       calc_initial_possible_unused(num_digits, has_repeats,
//       &possible_count);

//   if (depth == 0) {
//     strcpy(tree->guess, get_best_first_guess(num_digits, has_repeats));
//   } else {
//     int search_set_count = possible_count;
//     char **search_set = (char **)malloc(search_set_count * sizeof(char *));

//     for (int i = 0; i < search_set_count; i++) {
//       search_set[i] = (char *)malloc((num_digits + 1) * sizeof(char));
//       strcpy(search_set[i], possible[i]);
//     }

//     calc_best_next_guess((const char **)search_set, search_set_count, 0,
//                          (const char **)possible, possible_count,
//                          tree->guess);

//     for (int i = 0; i < search_set_count; i++) {
//       free(search_set[i]);
//     }
//     free(search_set);
//   }

//   tree->remain = possible_count;

//   if (depth + 1 <= max_depth) {
//     for (int i = 0; i < possible_count; i++) {
//       const char *p = possible[i];
//       Score score = compare_codes(p, tree->guess);
//       calc_tree_helper(data, &tree->scores[score.green][score.red], depth +
//       1);
//     }
//   }

//   for (int i = 0; i < possible_count; i++) {
//     free((void *)possible[i]);
//   }
//   free(possible);
// }

// TreeNode *calc_tree(int num_digits, int has_repeats, int max_depth) {
//   CalculationData data;
//   data.num_digits = num_digits;
//   data.has_repeats = has_repeats;
//   data.max_depth = max_depth;
//   strcpy(data.guesses[0], "");
//   data.responses[0].green = 0;
//   data.responses[0].red = 0;

//   TreeNode *tree = (TreeNode *)malloc(sizeof(TreeNode));
//   calc_tree_helper(&data, tree, 0);
//   return tree;
// }

// void calc_save_tree(int num_digits, int has_repeats, int max_depth) {
//   TreeNode *tree = calc_tree(num_digits, has_repeats, max_depth);
//   char filename[100];
//   sprintf(filename, "output-%d-%d-%d.json", num_digits, has_repeats,
//   max_depth); FILE *file = fopen(filename, "w+"); if (file == NULL) {
//     debug("Failed to create file");
//     return;
//   }
//   char *json = tree_to_json(tree);
//   fprintf(file, "%s", json);
//   fclose(file);
//   free(json);
// }

void print_string_array(char **string_array, int string_count,
                        int max_to_print) {
  int num_to_print = fmin(max_to_print, string_count);
  printf(" (%d/%d elements) [", num_to_print, string_count);
  for (int i = 0; i < num_to_print; i++) {
    printf("%s", string_array[i]);
    if (i < num_to_print - 1) {
      printf(", ");
    }
  }
  printf("]\n");
}

void test_dedupe_codes_by_pattern() {
  char *guesses[] = {"708"};
  char *codes[] = {"117", "210", "345", "510", "558", "654"};
  // // should equal ["117", "210", "345", "558"]

  //   char *guesses[] = {"012"};
  //   char *codes[] = {"117", "210", "345", "510", "558", "654"};
  // // should equal ["117", "210", "345", "510", "558"]

  //   char *guesses[] = {"708", "059"};
  //   char *codes[] = {"117", "210", "345", "510", "558", "654"};
  // // should equal ["117", "210", "345", "510", "558", "654"]

  //   char *guesses[] = {"001234"};
  //   char *codes[] = {"05678", "08765"};
  // // should equal ["05678"]

  //   char *guesses[] = {"53052"};
  //   char *codes[] = {"97237"};
  // // should equal ["97237"]

  int num_guesses = sizeof(guesses) / sizeof(guesses[0]);
  int num_codes = sizeof(codes) / sizeof(codes[0]);
  int result_count = 0;
  char **result =
      dedupe_codes_to_patterns((const char **)guesses, num_guesses,
                               (const char **)codes, num_codes, &result_count);

  printf("Deduplicated codes");
  print_string_array(result, result_count, 10);
}

int main() {
  // calc_save_tree(3, 0, 3);
  //   char a[] = "001234";
  //   char b[] = "400214";
  //   Score score = compare_codes(a, b);
  //   printf("(%d, %d)\n", score.green, score.red);

  int num_digits = 5;
  int has_repeats = 0;

  //   char *guess = (char *)malloc((num_digits + 1) * sizeof(char));
  //   get_best_first_guess(num_digits, 0, guess);
  //   printf("%s\n", guess);

  //   char c[] = "012345";
  //   printf("%d, %d, %d\n", unique_chars(a), unique_chars(b),
  //   unique_chars(c));

  PossibleUnused poss_unused =
      calc_initial_possible_unused(num_digits, has_repeats);

  char suggestion[MAX_CODE_LENGTH];

  //   printf("UNUSED");
  //   print_string_array(poss_unused.unused, poss_unused.unused_count, 10);

  //   printf("POSSIBLE");
  //   print_string_array(poss_unused.possible, poss_unused.possible_count, 10);

  calc_best_next_guess((const char **)poss_unused.unused,
                       poss_unused.unused_count, 0,
                       (const char **)poss_unused.possible,
                       poss_unused.possible_count, suggestion);
  printf("%s\n", suggestion);

  return 0;
}
