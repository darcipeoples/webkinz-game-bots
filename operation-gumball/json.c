#include "json.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void write_json_tree(FILE *file, TreeNode *node, int indent) {
  write_indent(file, indent);
  fprintf(file, "{\n");

  write_indent(file, indent + 2);
  fprintf(file, "\"remain\": %d,\n", node->remain);

  if (strlen(node->guess) > 0) {
    write_indent(file, indent + 2);
    fprintf(file, "\"guess\": \"%s\",\n", node->guess);
  }

  write_indent(file, indent + 2);
  fprintf(file, "\"scores\": {\n");
  write_json_scores(file, node->score_infos, node->score_infos_count,
                    indent + 4);

  write_indent(file, indent + 2);
  fprintf(file, "}\n");

  write_indent(file, indent);
  fprintf(file, "}\n");
}

void write_json_scores(FILE *file, ScoreInfo *score_infos,
                       int score_infos_count, int indent) {
  for (int i = 0; i < score_infos_count; i++) {
    write_indent(file, indent);
    fprintf(file, "\"(%d, %d)\": {\n", score_infos[i].score.green,
            score_infos[i].score.red);

    write_indent(file, indent + 2);
    fprintf(file, "\"remain\": %d,\n", score_infos[i].sub_tree->remain);

    if (strlen(score_infos[i].sub_tree->guess) > 0) {
      write_indent(file, indent + 2);
      fprintf(file, "\"guess\": \"%s\",\n", score_infos[i].sub_tree->guess);
    }

    if (score_infos[i].sub_tree->score_infos_count > 0) {
      write_indent(file, indent + 2);
      fprintf(file, "\"scores\": {\n");
      write_json_scores(file, score_infos[i].sub_tree->score_infos,
                        score_infos[i].sub_tree->score_infos_count, indent + 4);
      write_indent(file, indent + 2);
      fprintf(file, "}\n");
    }

    write_indent(file, indent);
    fprintf(file, "}");

    if (i < score_infos_count - 1) {
      fprintf(file, ",");
    }

    fprintf(file, "\n");
  }
}

void write_json_score(FILE *file, ScoreInfo *score_info, int indent) {
  write_indent(file, indent);
  fprintf(file, "\"(%d, %d)\": {\n", score_info->score.green,
          score_info->score.red);

  write_indent(file, indent + 2);
  fprintf(file, "\"remain\": %d,\n", score_info->sub_tree->remain);

  if (strlen(score_info->sub_tree->guess) > 0) {
    write_indent(file, indent + 2);
    fprintf(file, "\"guess\": \"%s\",\n", score_info->sub_tree->guess);
  }

  if (score_info->sub_tree->score_infos_count > 0) {
    write_indent(file, indent + 2);
    fprintf(file, "\"scores\": {\n");
    write_json_scores(file, score_info->sub_tree->score_infos,
                      score_info->sub_tree->score_infos_count, indent + 4);
    write_indent(file, indent + 2);
    fprintf(file, "}\n");
  }

  write_indent(file, indent);
  fprintf(file, "}");
}

void write_json_score_info(FILE *file, TreeNode *sub_tree, Score score,
                           int indent) {
  write_indent(file, indent);
  fprintf(file, "\"remain\": %d,\n", sub_tree->remain);

  if (strlen(sub_tree->guess) > 0) {
    write_indent(file, indent);
    fprintf(file, "\"guess\": \"%s\",\n", sub_tree->guess);
  }

  if (sub_tree->score_infos_count > 0) {
    write_indent(file, indent);
    fprintf(file, "\"scores\": {\n");
    write_json_scores(file, sub_tree->score_infos, sub_tree->score_infos_count,
                      indent + 2);
    write_indent(file, indent);
    fprintf(file, "}\n");
  }
}

void write_json_score_info_recursive(FILE *file, TreeNode *sub_tree,
                                     Score score, int indent) {
  write_indent(file, indent);
  fprintf(file, "\"remain\": %d,\n", sub_tree->remain);

  if (strlen(sub_tree->guess) > 0) {
    write_indent(file, indent);
    fprintf(file, "\"guess\": \"%s\",\n", sub_tree->guess);
  }

  if (sub_tree->score_infos_count > 0) {
    write_indent(file, indent);
    fprintf(file, "\"scores\": {\n");
    write_json_scores(file, sub_tree->score_infos, sub_tree->score_infos_count,
                      indent + 2);
    write_indent(file, indent);
    fprintf(file, "}\n");
  }
}

void write_indent(FILE *file, int indent) {
  for (int i = 0; i < indent; i++) {
    fprintf(file, " ");
  }
}

void print_json_tree(TreeNode *node, int indent) {
  print_indent(indent);
  printf("{\n");

  print_indent(indent + 2);
  printf("\"remain\": %d,\n", node->remain);

  if (strlen(node->guess) > 0) {
    print_indent(indent + 2);
    printf("\"guess\": \"%s\",\n", node->guess);
  }

  print_indent(indent + 2);
  printf("\"scores\": {\n");
  print_json_scores(node->score_infos, node->score_infos_count, indent + 4);

  print_indent(indent + 2);
  printf("}\n");

  print_indent(indent);
  printf("}\n");
}

void print_json_scores(ScoreInfo *score_infos, int score_infos_count,
                       int indent) {
  for (int i = 0; i < score_infos_count; i++) {
    print_indent(indent);
    printf("\"(%d, %d)\": {\n", score_infos[i].score.green,
           score_infos[i].score.red);

    print_indent(indent + 2);
    printf("\"remain\": %d,\n", score_infos[i].sub_tree->remain);

    if (strlen(score_infos[i].sub_tree->guess) > 0) {
      print_indent(indent + 2);
      printf("\"guess\": \"%s\",\n", score_infos[i].sub_tree->guess);
    }

    if (score_infos[i].sub_tree->score_infos_count > 0) {
      print_indent(indent + 2);
      printf("\"scores\": {\n");
      print_json_scores(score_infos[i].sub_tree->score_infos,
                        score_infos[i].sub_tree->score_infos_count, indent + 4);
      print_indent(indent + 2);
      printf("}\n");
    }

    print_indent(indent);
    printf("}");

    if (i < score_infos_count - 1) {
      printf(",");
    }

    printf("\n");
  }
}

void print_json_score(ScoreInfo *score_info, int indent) {
  print_indent(indent);
  printf("\"(%d, %d)\": {\n", score_info->score.green, score_info->score.red);

  print_indent(indent + 2);
  printf("\"remain\": %d,\n", score_info->sub_tree->remain);

  if (strlen(score_info->sub_tree->guess) > 0) {
    print_indent(indent + 2);
    printf("\"guess\": \"%s\",\n", score_info->sub_tree->guess);
  }

  if (score_info->sub_tree->score_infos_count > 0) {
    print_indent(indent + 2);
    printf("\"scores\": {\n");
    print_json_scores(score_info->sub_tree->score_infos,
                      score_info->sub_tree->score_infos_count, indent + 4);
    print_indent(indent + 2);
    printf("}\n");
  }

  print_indent(indent);
  printf("}");
}

void print_json_score_info(TreeNode *sub_tree, Score score, int indent) {
  print_indent(indent);
  printf("\"remain\": %d,\n", sub_tree->remain);

  if (strlen(sub_tree->guess) > 0) {
    print_indent(indent);
    printf("\"guess\": \"%s\",\n", sub_tree->guess);
  }

  if (sub_tree->score_infos_count > 0) {
    print_indent(indent);
    printf("\"scores\": {\n");
    print_json_scores(sub_tree->score_infos, sub_tree->score_infos_count,
                      indent + 2);
    print_indent(indent);
    printf("}\n");
  }
}

void print_json_score_info_recursive(TreeNode *sub_tree, Score score,
                                     int indent) {
  print_indent(indent);
  printf("\"remain\": %d,\n", sub_tree->remain);

  if (strlen(sub_tree->guess) > 0) {
    print_indent(indent);
    printf("\"guess\": \"%s\",\n", sub_tree->guess);
  }

  if (sub_tree->score_infos_count > 0) {
    print_indent(indent);
    printf("\"scores\": {\n");
    print_json_scores(sub_tree->score_infos, sub_tree->score_infos_count,
                      indent + 2);
    print_indent(indent);
    printf("}\n");
  }
}

void print_indent(int indent) {
  for (int i = 0; i < indent; i++) {
    printf(" ");
  }
}

int main_json() {
  // Create the tree structure
  TreeNode tree;
  tree.remain = 3;
  tree.guess[0] = '\0'; // Initialize the guess field

  ScoreInfo score_infos[2];

  TreeNode sub_tree1 = {0};
  ;
  sub_tree1.remain = 1;
  strcpy(sub_tree1.guess, "95138");
  sub_tree1.score_infos_count = 1;

  ScoreInfo sub_score_infos1[1];
  TreeNode sub_sub_tree1 = {0};
  ;
  sub_sub_tree1.remain = 1;
  sub_sub_tree1.score_infos_count = 0;
  sub_score_infos1[0].sub_tree = &sub_sub_tree1;
  sub_score_infos1[0].score.green = 5;
  sub_score_infos1[0].score.red = 0;

  sub_tree1.score_infos = sub_score_infos1;

  TreeNode sub_tree2;
  sub_tree2.remain = 1;
  strcpy(sub_tree2.guess, "85931");
  sub_tree2.score_infos_count = 1;

  ScoreInfo sub_score_infos2[1];
  TreeNode sub_sub_tree2 = {0};
  sub_sub_tree2.remain = 1;
  sub_sub_tree2.score_infos_count = 0;
  sub_score_infos2[0].sub_tree = &sub_sub_tree2;
  sub_score_infos2[0].score.green = 5;
  sub_score_infos2[0].score.red = 0;

  sub_tree2.score_infos = sub_score_infos2;

  score_infos[0].sub_tree = &sub_tree1;
  score_infos[0].score.green = 1;
  score_infos[0].score.red = 4;

  score_infos[1].sub_tree = &sub_tree2;
  score_infos[1].score.green = 2;
  score_infos[1].score.red = 3;

  tree.score_infos = score_infos;
  tree.score_infos_count = 2;

  // Open the output file
  FILE *file = fopen("output.json", "w");
  if (file == NULL) {
    printf("Failed to open the output file.\n");
    return 1;
  }

  // Write the JSON representation of the tree to the file
  write_json_tree(file, &tree, 0);

  // Close the output file
  fclose(file);

  printf("JSON written to file: output.json\n");

  return 0;
}
