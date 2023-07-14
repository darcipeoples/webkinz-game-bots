#include "main.h"
#include "json.h"
#include "test.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int scores_equal(Score a, Score b) {
  return a.green == b.green && a.red == b.red;
}

int score_equals(Score a, int green, int red) {
  return a.green == green && a.red == red;
}

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

Result *calc_score_distribution(const char *guess, const char **possible,
                                int possible_count) {
  int num_digits = strlen(guess);
  RemainInfo **distribution = malloc(MAX_CODE_LENGTH * sizeof(RemainInfo *));
  for (int green = 0; green < MAX_CODE_LENGTH; green++) {
    distribution[green] = malloc(MAX_CODE_LENGTH * sizeof(RemainInfo));
    for (int red = 0; red < MAX_CODE_LENGTH; red++) {
      RemainInfo *info = &distribution[green][red];
      info->new_possible_count = 0;
      // TODO can hit an issue here
      info->new_possible = malloc(possible_count * sizeof(char *));
      info->score.green = green;
      info->score.red = red;
    }
  }

  for (int i = 0; i < possible_count; i++) {
    const char *p = possible[i];
    Score score = compare_codes(p, guess);
    RemainInfo *info = &distribution[score.green][score.red];
    // TODO: make a copy of possible elements or just point to them?
    info->new_possible[info->new_possible_count] =
        malloc((num_digits + 1) * sizeof(char));
    strcpy(info->new_possible[info->new_possible_count++], p);
  }

  Result *result = malloc(sizeof(Result));
  result->distribution =
      malloc(MAX_CODE_LENGTH * MAX_CODE_LENGTH * sizeof(RemainInfo));
  result->score_count = 0;
  result->expected_info = 0;
  for (int i = 0; i < MAX_CODE_LENGTH; i++) {
    for (int j = 0; j < MAX_CODE_LENGTH; j++) {
      RemainInfo *info = &distribution[i][j];
      int new_possible_count = info->new_possible_count;
      if (new_possible_count > 0) {
        double prob = (double)new_possible_count / possible_count;
        double log_prob = log2(1 / prob);
        result->expected_info += prob * log_prob;
        // Shrink the remaining array
        info->new_possible =
            realloc(info->new_possible, new_possible_count * sizeof(char *));
        result->distribution[result->score_count++] = *info;
      } else {
        free(info->new_possible);
        info->new_possible = 0;
      }
    }
    free(distribution[i]);
    distribution[i] = 0;
  }
  free(distribution);
  distribution = 0;
  result->distribution =
      realloc(result->distribution, result->score_count * sizeof(RemainInfo));

  return result;
}

double calc_expected_info(const char *guess, const char **possible,
                          int possible_count) {
  int distribution[MAX_CODE_LENGTH][MAX_CODE_LENGTH] = {0};
  for (int i = 0; i < possible_count; i++) {
    const char *p = possible[i];
    Score score = compare_codes(p, guess);

    distribution[score.green][score.red]++;
  }

  double expected_info = 0;
  for (int i = 0; i < MAX_CODE_LENGTH; i++) {
    for (int j = 0; j < MAX_CODE_LENGTH; j++) {
      int freq = distribution[i][j];
      if (freq > 0) {
        double prob = (double)freq / possible_count;
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

    char code_pattern[MAX_CODE_LENGTH + 1] = {0};

    int is_ascending = 1;
    char prev_char = '\0';
    int unused_idx = 0;
    for (int j = 0; code[j] != '\0'; j++) {
      char c = code[j];
      if (!char_map[c - '0']) {
        char_map[c - '0'] = unused_chars[unused_idx++];
      }
      code_pattern[j] = char_map[c - '0'];
      // If no prev. guesses, only keep code_patterns in ascending order
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

PossibleUnused filter_possible_unused(const char **possible, int possible_count,
                                      const char **unused, int unused_count,
                                      const char *attempt, Score score) {
  const char **filtered_possible =
      (const char **)malloc(possible_count * sizeof(char *));
  const char **filtered_unused =
      (const char **)malloc(unused_count * sizeof(char *));

  int filtered_possible_count = 0;
  int filtered_unused_count = 0;

  // Iterate through the possible codes
  for (int i = 0; i < possible_count; i++) {
    // TODO: should we exclude attempt from the new possible?
    if (scores_equal(compare_codes(possible[i], attempt), score)) {
      // Add the code to the filtered possible codes array
      filtered_possible[filtered_possible_count++] = possible[i];
    }
  }

  for (int i = 0; i < unused_count; i++) {
    if (strcmp(unused[i], attempt) != 0) {
      filtered_unused[filtered_unused_count++] = unused[i];
    }
  }

  filtered_possible =
      realloc(filtered_possible, filtered_possible_count * sizeof(char *));
  filtered_unused =
      realloc(filtered_unused, filtered_unused_count * sizeof(char *));

  PossibleUnused poss_unused = {0};
  poss_unused.possible = filtered_possible;
  poss_unused.possible_count = filtered_possible_count;
  poss_unused.unused = filtered_unused;
  poss_unused.unused_count = filtered_unused_count;
  return poss_unused;
}

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
  printf("\tEnter calc_best_next_guess(%d, %d, %d)\n", search_set_count,
         possible_count, max_search_time);

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
      printf("\t\titer %d/%d(%d) (%.1f%%): best_idx = %d, best guess = '%s', "
             "expected"
             "info = %f\n",
             i, search_set_count, possible_count,
             (double)i / search_set_count * 100, best_idx, search_set[best_idx],
             best_expected_info);
    }

    // if (max_search_time > 0 && i + 1 >= max_search_time) {
    //   printf("INFO: Ran out of time to keep searching");
    //   break;
    // }
  }

  strcpy(suggestion, search_set[best_idx]);
  printf(
      "\t\tFound best_guess = '%s', best_expected_info = %f, best_idx = %d\n",
      search_set[best_idx], best_expected_info, best_idx);

  printf("\tExit calc_best_next_guess(%d, %d, %d)\n", search_set_count,
         possible_count, max_search_time);
}

void print_string_array(const char **string_array, int string_count,
                        int max_to_print) {
  int num_to_print = fmin(max_to_print, string_count);
  printf(" (%-7d items)\t[", string_count);
  for (int i = 0; i < num_to_print; i++) {
    printf("%s", string_array[i]);
    if (i < num_to_print - 1) {
      printf(", ");
    } else if (num_to_print < string_count) {
      printf("...");
    }
  }
  printf("]\n");
}

// {
//       "remain": 720,
//       "guess": "012",
//       "scores": {
//         "(0, 0)": {
//           "remain": 210,
//           "guess": "345",
//           "scores": {

char *six_false_second_guesses[MAX_CODE_LENGTH][MAX_CODE_LENGTH] = {
    {NULL, "562178", "562177", "560347", "560043", "156043"},
    {"056789", "051567", "002156", "002356", "052146"},
    {"051367", "051267", "015136", "012536"},
    {"051367", "015267", "051326"},
    {"051367", "015267"},
    {"050067"},
};

int array_contains(const char *target, const char *array[], int size) {
  for (int i = 0; i < size; i++) {
    if (strcmp(target, array[i]) == 0) {
      return 1;
    }
  }
  return 0;
}

// Returns an array with all list a elements followed by all list b that aren't
// in a Note that only list b is deduplicated.
char **merge_arrays(const char **list_a, int list_a_count, const char **list_b,
                    int list_b_count, int *merged_count) {
  char **array = malloc((list_a_count + list_b_count) * sizeof(char *));
  int count = 0;
  for (int i = 0; i < list_a_count; i++) {
    array[count] = malloc((strlen(list_a[i]) + 1) * sizeof(char));
    strcpy(array[count++], list_a[i]);
  }
  for (int i = 0; i < list_b_count; i++) {
    if (!array_contains(list_b[i], (const char **)array, count)) {
      array[count] = malloc((strlen(list_b[i]) + 1) * sizeof(char));
      strcpy(array[count++], list_b[i]);
    }
  }
  *merged_count = count;
  return array;
}

char **copy_array_without_element(const char **unused, int unused_count,
                                  const char *guess, int *new_unused_count) {
  char **new_array = malloc((unused_count - 1) * sizeof(char *));
  int new_count = 0;
  for (int i = 0; i < unused_count; i++) {
    if (strcmp(unused[i], guess) != 0) {
      // TODO: not sure yet whether to malloc the code strings e.g. "001234", or
      //    try pointers. Prob less bugs if just copy.
      new_array[new_count] = malloc((strlen(unused[i]) + 1) * sizeof(char));
      // TODO: change all assignments from = to strcpy!
      strcpy(new_array[new_count], unused[i]);
      new_count++;
    }
  }
  *new_unused_count = new_count;
  return new_array;
}

// TODO: more function should pass a score pointer instead of a Score
void score_to_string(const Score score, char *output_buffer) {
  sprintf(output_buffer, "(%d, %d)", score.green, score.red);
}

void get_guesses_responses_string(const char **guesses, int guesses_count,
                                  const Score *responses, int responses_count,
                                  char *output_buffer) {
  char score_str[100] = {0};
  int offset = 0;
  strcpy(&output_buffer[offset++], "[");
  for (int i = 0; i < fmin(responses_count, guesses_count); i++) {
    const Score score = responses[i];
    score_to_string(score, score_str);
    const char *guess = guesses[i];
    offset += sprintf(output_buffer + offset, "('%s', %s), ", guess, score_str);
  }
  if (strlen(output_buffer) > 1) {
    offset -= 2;
  }
  strcpy(&output_buffer[offset++], "]");
}

int save_string_to_file(const char *string, const char *filename) {
  FILE *file = fopen(filename, "w+");
  if (file != NULL) {
    fprintf(file, "%s", string);
    fclose(file);
    return 0;
  }
  return 1;
}

TreeNode *_calc_tree(int num_digits, int has_repeats, const char **guesses,
                     int guesses_count, const Score *responses,
                     int responses_count, int depth, int max_depth,
                     const char **possible, int possible_count,
                     const char **unused, int unused_count) {
  int CHECKPOINT_DEPTH = 1;
  char guess_resp_str[300];
  get_guesses_responses_string(guesses, guesses_count, responses,
                               responses_count, guess_resp_str);
  printf("Starting _calc_tree(dig=%d, reps=%d, guess/resps='%s', dep=%d, "
         "max_dep=%d, poss_cnt=%d, unused_cnt=%d\n",
         num_digits, has_repeats, guess_resp_str, depth, max_depth,
         possible_count, unused_count);
  if (depth >= max_depth) {
    return NULL;
  }

  TreeNode *tree = malloc(sizeof(TreeNode));
  tree->remain = possible_count;

  char guess[MAX_CODE_LENGTH + 1] = {0};

  if (depth == 0) {
    get_best_first_guess(num_digits, has_repeats, guess);
  } else if (num_digits == 6 && !has_repeats && depth == 1 &&
             responses_count == 1) {
    Score score = responses[0];
    strcpy(guess, six_false_second_guesses[score.green][score.red]);
  } else {
    int search_set_count;
    char **search_set =
        merge_arrays((const char **)possible, possible_count,
                     (const char **)unused, unused_count, &search_set_count);
    int deduped_count;
    char **deduped = dedupe_codes_to_patterns(
        (const char **)guesses, guesses_count, (const char **)search_set,
        search_set_count, &deduped_count);
    calc_best_next_guess((const char **)deduped, deduped_count, -1,
                         (const char **)possible, possible_count, guess);
  }
  strcpy(tree->guess, guess);
  int new_unused_count;
  char **new_unused = copy_array_without_element(unused, unused_count, guess,
                                                 &new_unused_count);
  char **new_guesses = malloc(guesses_count + 1 * sizeof(char *));
  for (int i = 0; i < guesses_count; i++) {
    new_guesses[i] = malloc(strlen(guesses[i]) * sizeof(char));
    strcpy(new_guesses[i], guesses[i]);
  }
  new_guesses[guesses_count] = guess;
  Score *new_responses = malloc(responses_count + 1 * sizeof(Score));
  for (int i = 0; i < responses_count; i++) {
    new_responses[i] = responses[i];
  }

  Result *result =
      calc_score_distribution(guess, (const char **)possible, possible_count);
  int score_count = result->score_count;
  tree->score_infos = malloc(score_count * sizeof(ScoreInfo));

  for (int i = 0; i < score_count; i++) {
    ScoreInfo *si = &tree->score_infos[i];
    RemainInfo info = result->distribution[i];
    Score score = info.score;
    new_responses[responses_count] = score;
    int new_possible_count = info.new_possible_count;
    const char **new_possible = info.new_possible;
    if (score_equals(score, num_digits, 0)) {
      // TODO
      if (new_possible_count != 1) {
        // TODO: include function params in warning
        printf("WARNING: new_possible_count (%d) != 1 for (N, 0) response: ",
               new_possible_count);
      }
      // TODO: move this out into a function
      si->score = score;
      si->sub_tree = malloc(sizeof(TreeNode));
      si->sub_tree->remain = new_possible_count;
      continue;
    }
    if (depth + 1 >= max_depth) {
      si->score = score;
      si->sub_tree = malloc(sizeof(TreeNode));
      si->sub_tree->remain = new_possible_count;
      continue;
    }
    si->score = score;
    si->sub_tree = _calc_tree(
        num_digits, has_repeats, (const char **)new_guesses, guesses_count + 1,
        new_responses, responses_count + 1, depth + 1, max_depth,
        (const char **)new_possible, new_possible_count,
        (const char **)new_unused, new_unused_count);
    if (si->sub_tree != NULL) {
      if (si->sub_tree->remain != new_possible_count) {
        printf("WARNING: tree['scores'][score_str]['remain'] "
               "(%d) !=  freq (%d):",
               si->sub_tree->remain, new_possible_count);
      }
    }
  }

  if (depth <= CHECKPOINT_DEPTH) {
    printf("Finished _calc_tree(dig=%d, reps=%d, guess/resps='%s', dep=%d, "
           "max_dep=%d, poss_cnt=%d, unused_cnt=%d\n",
           num_digits, has_repeats, guess_resp_str, depth, max_depth,
           possible_count, unused_count);
    char filename[100];
    char guesses_responses_str[100];
    get_guesses_responses_string((const char **)guesses, guesses_count,
                                 responses, responses_count,
                                 guesses_responses_str);
    sprintf(filename, "temp-output-%d-%d-%d-%d-%s.json", num_digits,
            has_repeats, max_depth, depth, guesses_responses_str);
    output_tree_to_json_file(tree, filename);
  }

  return tree;
}

TreeNode *calc_tree(int num_digits, int has_repeats, int max_depth) {
  PossibleUnused pu = calc_initial_possible_unused(num_digits, has_repeats);
  TreeNode *tree =
      _calc_tree(num_digits, has_repeats, NULL, 0, NULL, 0, 0, max_depth,
                 (const char **)pu.possible, pu.possible_count,
                 (const char **)pu.unused, pu.unused_count);
  return tree;
}

int output_tree_to_json_file(TreeNode *tree, char *filename) {
  FILE *file = fopen(filename, "w+");
  if (file == NULL) {
    printf("Failed to open file '%s'\n", filename);
    return 1;
  }
  write_json_tree(file, tree, 0);
  fclose(file);
  printf("Saved tree to '%s'.\n", filename);
  return 0;
}

void calc_save_tree(int num_digits, int has_repeats, int max_depth) {
  TreeNode *tree = calc_tree(num_digits, has_repeats, max_depth);
  char filename[100];
  sprintf(filename, "strategies/output-%d-%d-%d.json", num_digits, has_repeats,
          max_depth);
  output_tree_to_json_file(tree, filename);
}

void free_string_array(char **array, int array_count) {
  for (int i = 0; i < array_count; i++) {
    free(array[i]);
  }
  free(array);
}

int main() {
  int num_digits = 3;
  int has_repeats = 0;

  run_all_tests();

  // calc_save_tree(num_digits, has_repeats, 4);
  // return 0;

  // save_string_to_file("Testing", "test.txt");

  // char guess[MAX_CODE_LENGTH + 1] = {0};
  // get_best_first_guess(num_digits, 0, guess);
  // printf("%s\n", guess);

  // char c[] = "012345";
  // printf("%d, %d, %d\n", unique_chars(a), unique_chars(b), unique_chars(c));

  PossibleUnused poss_unused =
      calc_initial_possible_unused(num_digits, has_repeats);
  char **possible = poss_unused.possible;
  char **unused = poss_unused.unused;
  int possible_count = poss_unused.possible_count;
  int unused_count = poss_unused.unused_count;
  printf("UNUSED");
  print_string_array((const char **)unused, unused_count, 10);
  printf("POSSIBLE");
  print_string_array((const char **)possible, possible_count, 10);

  // Score score;
  // score.green = 0;
  // score.red = 3;
  // PossibleUnused filtered = filter_possible_unused(
  //     possible, possible_count, unused, unused_count, "001234", score);
  // char **f_possible = filtered.possible;
  // char **f_unused = filtered.unused;
  // int f_possible_count = filtered.possible_count;
  // int f_unused_count = filtered.unused_count;
  // printf("FILTERED UNUSED");
  // print_string_array((const char **)f_unused, f_unused_count, 10);
  // printf("FILTERED POSSIBLE");
  // print_string_array((const char **)f_possible, f_possible_count, 10);

  // int deduped_count = 0;
  // char **deduped_codes = dedupe_codes_to_patterns(
  //     NULL, 0, (const char **)unused, unused_count, &deduped_count);
  // printf("DEDUPED");
  // print_string_array(deduped_codes, deduped_count, 10);

  // char suggestion[MAX_CODE_LENGTH];
  // calc_best_next_guess((const char **)deduped_codes, deduped_count, 0,
  //                      (const char **)possible, possible_count, suggestion);
  // calc_best_next_guess((const char **)unused, unused_count, 0,
  //                      (const char **)possible, possible_count, suggestion);
  // printf("%s\n", suggestion);

  // double expected_info =
  //     calc_expected_info("001234", (const char **)possible, possible_count);
  // printf("%f\n", expected_info);
  Result *result = calc_score_distribution("001234", (const char **)possible,
                                           possible_count);
  printf("%f\n", result->expected_info);
  int score_count = result->score_count;
  for (int i = 0; i < score_count; i++) {
    RemainInfo remainInfo = result->distribution[i];
    Score score = remainInfo.score;
    printf("(%d, %d): ", score.green, score.red);
    print_string_array((const char **)remainInfo.new_possible,
                       remainInfo.new_possible_count, 10);
  }

  run_all_tests();

  return 0;
}
