#include "main.h"

// TODO: make a string array comparison function
int _test_dedupe_codes_by_pattern(const char **guesses, int num_guesses,
                                  const char **codes, int num_codes,
                                  const char **expected, int num_expected) {
  int num_results;
  char **results =
      dedupe_codes_to_patterns((const char **)guesses, num_guesses,
                               (const char **)codes, num_codes, &num_results);
  printf("GUESSES\t\t");
  print_string_array((const char **)guesses, num_guesses, 10);
  printf("CODES\t\t");
  print_string_array((const char **)codes, num_codes, 10);
  printf("EXPECTED\t");
  print_string_array((const char **)expected, num_expected, 10);
  printf("ACTUAL\t\t");
  print_string_array((const char **)results, num_results, 10);
}

int test_dedupe_codes_by_pattern() {
  char *guesses[] = {"708"};
  char *codes[] = {"117", "210", "345", "510", "558", "654"};
  char *expected[] = {"117", "210", "345", "558"};

  // char *guesses[] = {"012"};
  // char *codes[] = {"117", "210", "345", "510", "558", "654"};
  // char *expected[] = {"117", "210", "345", "510", "558"};

  // char *guesses[] = {"708", "059"};
  // char *codes[] = {"117", "210", "345", "510", "558", "654"};
  // char *expected[] = {"117", "210", "345", "510", "558", "654"};

  // char *guesses[] = {"001234"};
  // char *codes[] = {"05678", "08765"};
  // char *expected[] = {"05678"};

  // char *guesses[] = {"53052"};
  // char *codes[] = {"97237"};
  // char *expected[] = {"97237"};

  int num_guesses = sizeof(guesses) / sizeof(guesses[0]);
  int num_codes = sizeof(codes) / sizeof(codes[0]);
  int num_expected = sizeof(expected) / sizeof(expected[0]);

  _test_dedupe_codes_by_pattern((const char **)guesses, num_guesses,
                                (const char **)codes, num_codes,
                                (const char **)expected, num_expected);

  return 0;
}

int test_scores_equal() {
  Score a = {0, 3};
  Score b = {1, 2};
  Score c = {2, 1};
  Score d = {0, 3};

  // Expected: all ? 1
  printf("scores_equal(a, b) == 0 ? %d\n", scores_equal(a, b) == 0);
  printf("scores_equal(a, c) == 0 ? %d\n", scores_equal(a, c) == 0);
  printf("scores_equal(a, d) == 1 ? %d\n", scores_equal(a, d) == 1);
  printf("scores_equal(a, a) == 1 ? %d\n", scores_equal(a, a) == 1);
  printf("scores_equal(b, c) == 1 ? %d\n", scores_equal(b, c) == 0);

  return 0;
}

int test_compare_codes() {
  char code_1[] = "001234";
  char code_2[] = "400214";
  Score score = {3, 2};
  char score_str[10];
  score_to_string(score, score_str);

  Score score_result = compare_codes(code_1, code_2);
  int are_equal = scores_equal(score_result, score);

  return 0;
  // Expected: == (3, 2) ? 1
  printf("compare_codes(%s, %s) == %s ? %d", code_1, code_2, score_str,
         are_equal == 1);

  return 0;
}

int test_array_contains() {
  const char *array_1[] = {"001", "004", "003", "004"};
  int array_1_count = sizeof(array_1) / sizeof(array_1[0]);
  const char *array_2[] = {"004", "001", "002", "001", "000"};
  int array_2_count = sizeof(array_2) / sizeof(array_2[0]);

  // Expected: == 0 ? 1
  int contains = array_contains("000", array_1, array_1_count);
  printf("array_contains('000', {'001', '004', '003, '004'}) == 0 ? %d\n",
         contains == 0);
  contains = array_contains("000", array_2, array_2_count);
  // Expected: == 1 ? 1
  printf(
      "array_contains('000', {'004', '001', '002', '001', '000'}) == 1 ? %d\n",
      contains == 1);

  return 0;
}

// TODO print expected so easy to check
int test_merge_array() {
  const char *array_1[] = {"001", "004", "003", "004"};
  int array_1_count = sizeof(array_1) / sizeof(array_1[0]);
  const char *array_2[] = {"004", "001", "002", "001", "000"};
  int array_2_count = sizeof(array_2) / sizeof(array_2[0]);
  int merged_count;
  char **merged = merge_arrays(array_1, array_1_count, array_2, array_2_count,
                               &merged_count);
  printf("MERGED"); // Expected: [001, 004, 003, 004, 002, 000]
  print_string_array((const char **)merged, merged_count, 10);

  return 0;
}

// TODO print expected so easy to check
int test_filter_possible_unused() {
  const char *array_1[] = {"001", "004", "003", "004"};
  int array_1_count = sizeof(array_1) / sizeof(array_1[0]);
  const char *array_2[] = {"004", "001", "002", "001", "000"};
  int array_2_count = sizeof(array_2) / sizeof(array_2[0]);

  Score c = {2, 0};
  PossibleUnused pu = filter_possible_unused(array_1, array_1_count, array_2,
                                             array_2_count, "001", c);
  printf("FILTERED POSSIBLE"); // Expected: [004, 003, 004]
  print_string_array((const char **)pu.possible, pu.possible_count, 10);
  printf("FILTERED UNUSED"); // Expected: [004, 002, 000]
  print_string_array((const char **)pu.unused, pu.unused_count, 10);

  Score d = {3, 0};
  pu = filter_possible_unused(array_1, array_1_count, array_2, array_2_count,
                              "001", d);
  printf("FILTERED POSSIBLE"); // Expected: [001]
  print_string_array((const char **)pu.possible, pu.possible_count, 10);
  printf("FILTERED UNUSED"); // Expected: [004, 002, 000]
  print_string_array((const char **)pu.unused, pu.unused_count, 10);

  return 0;
}

int test_copy_array_without_element() {
  int new_array_count;
  char *array[] = {"001", "002", "003", "001"};
  int array_count = sizeof(array) / sizeof(array[0]);
  char **new_array = copy_array_without_element(
      (const char **)array, array_count, "001", &new_array_count);
  printf("NEW ARRAY"); // Expected: [002, 003]
  print_string_array((const char **)new_array, new_array_count, 10);

  char **new_array2 = copy_array_without_element(
      (const char **)array, array_count, "004", &new_array_count);
  printf("NEW ARRAY 2"); // Expected: [001, 002, 003, 001]
  print_string_array((const char **)new_array2, new_array_count, 10);

  return 0;
}

int test_get_guesses_responses_string() {
  char *guesses[] = {"001234", "562178"};
  int guesses_count = sizeof(guesses) / sizeof(guesses[0]);
  Score a = {0, 1};
  Score b = {4, 0};
  Score responses[] = {a, b};
  int responses_count = sizeof(responses) / sizeof(responses[0]);

  char output[100];
  get_guesses_responses_string((const char **)guesses, guesses_count,
                               (const Score *)responses, responses_count,
                               output);
  // Expected: '[('001234', (0, 1)), ('562178', (4, 0))]'
  printf("get_guesses_responses_string: '%s' == '[('001234', (0, 1)), "
         "('562178', (4, 0))]' ? %d\n",
         output,
         strcmp(output, "[('001234', (0, 1)), ('562178', (4, 0))]") == 0);

  return 0;
}

int test_score_to_string() {
  Score a = {0, 1};
  char score_str[10];
  score_to_string(a, score_str); // Expected: (0, 1)
  printf("score_to_string: %s == (0, 1) ? %d\n", score_str,
         strcmp(score_str, "(0, 1)") == 0);

  return 0;
}

int test_possible_unused() {
  int num_digits = 6;
  int has_repeats = 0;

  PossibleUnused poss_unused =
      calc_initial_possible_unused(num_digits, has_repeats);
  char **possible = poss_unused.possible;
  char **unused = poss_unused.unused;
  int possible_count = poss_unused.possible_count;
  int unused_count = poss_unused.unused_count;
  printf("UNUSED"); // Expected: 1000000
  print_string_array((const char **)unused, unused_count, 10);
  printf("POSSIBLE"); // Expected: 151200
  print_string_array((const char **)possible, possible_count, 10);

  return 0;
}

int run_all_tests() {
  test_dedupe_codes_by_pattern();
  test_scores_equal();
  test_compare_codes();
  test_array_contains();
  test_merge_array();
  test_filter_possible_unused();
  test_copy_array_without_element();
  test_get_guesses_responses_string();
  test_score_to_string();
  test_possible_unused();

  return 0;
}

int main_test() {
  run_all_tests();

  return 0;
}