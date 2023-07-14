#ifndef JSON_H
#define JSON_H

#include "main.h"

void write_json_tree(FILE *file, TreeNode *node, int indent);

void write_json_scores(FILE *file, ScoreInfo *score_infos,
                       int score_infos_count, int indent);

void write_json_score(FILE *file, ScoreInfo *score_info, int indent);

void write_json_score_info(FILE *file, TreeNode *sub_tree, Score score,
                           int indent);

void write_json_score_info_recursive(FILE *file, TreeNode *sub_tree,
                                     Score score, int indent);

void write_indent(FILE *file, int indent);

void print_json_tree(TreeNode *node, int indent);

void print_json_scores(ScoreInfo *score_infos, int score_infos_count,
                       int indent);

void print_json_score(ScoreInfo *score_info, int indent);

void print_json_score_info(TreeNode *sub_tree, Score score, int indent);

void print_json_score_info_recursive(TreeNode *sub_tree, Score score,
                                     int indent);

void print_indent(int indent);

#endif /* JSON_H */