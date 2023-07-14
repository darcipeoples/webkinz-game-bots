#ifndef MAIN_H
#define MAIN_H

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_CODE_LENGTH 6

typedef struct {
  int green;
  int red;
} Score;

typedef struct TreeNode TreeNode;

typedef struct {
  TreeNode *sub_tree;
  Score score;
} ScoreInfo;

struct TreeNode {
  int remain;
  char guess[MAX_CODE_LENGTH + 1];
  ScoreInfo *score_infos;
  int score_infos_count; // TODO rename x_count variables to num_x
};

// TODO: use CodeList?
typedef struct {
  const char **values;
  int count;
} CodeList;

typedef struct {
  Score score;
  char **new_possible;
  int new_possible_count;
} RemainInfo;

typedef struct {
  double expected_info;
  RemainInfo *distribution;
  int score_count;
} Result;

typedef struct {
  char **possible;
  int possible_count;
  char **unused;
  int unused_count;
} PossibleUnused;

int scores_equal(Score a, Score b);

int score_equals(Score a, int green, int red);

void get_best_first_guess(int num_digits, int has_repeats, char *guess);

Score compare_codes(const char *a, const char *b);

Result *calc_score_distribution(const char *guess, const char **possible,
                                int possible_count);

double calc_expected_info(const char *guess, const char **possible,
                          int possible_count);

char **dedupe_codes_to_patterns(const char **guesses, int guess_count,
                                const char **codes, int code_count,
                                int *deduped_count);

int unique_chars(const char *s);

PossibleUnused filter_possible_unused(const char **possible, int possible_count,
                                      const char **unused, int unused_count,
                                      const char *attempt, Score score);

PossibleUnused calc_initial_possible_unused(int num_digits, int has_repeats);

void calc_best_next_guess(const char **search_set, int search_set_count,
                          int max_search_time, const char **possible,
                          int possible_count, char *suggestion);

void print_string_array(const char **string_array, int string_count,
                        int max_to_print);

int array_contains(const char *target, const char *array[], int size);

char **merge_arrays(const char **list_a, int list_a_count, const char **list_b,
                    int list_b_count, int *merged_count);

char **copy_array_without_element(const char **array, int array_count,
                                  const char *element, int *new_array_count);

void score_to_string(const Score score, char *output_buffer);

void get_guesses_responses_string(const char **guesses, int guesses_count,
                                  const Score *responses, int responses_count,
                                  char *output_buffer);

int save_string_to_file(const char *string, const char *filename);

char *json_dumps_tree(const TreeNode *tree);

TreeNode *_calc_tree(int num_digits, int has_repeats, const char **guesses,
                     int guesses_count, const Score *responses,
                     int responses_count, int depth, int max_depth,
                     const char **possible, int possible_count,
                     const char **unused, int unused_count);

TreeNode *calc_tree(int num_digits, int has_repeats, int max_depth);

void calc_save_tree(int num_digits, int has_repeats, int max_depth);

int output_tree_to_json_file(TreeNode *tree, char *filename);

#endif /* MAIN_H */