#include <algorithm>
#include <cmath>
#include <iostream>
#include <map>
#include <set>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

using namespace std;

struct Score {
  int green;
  int red;

  bool operator==(const Score &other) const {
    return green == other.green && red == other.red;
  }
};

struct score_hash {
  size_t operator()(const Score &s) const {
    return hash<int>{}(s.green) ^ hash<int>{}(s.red);
  }
};

struct VectorHash {
  size_t operator()(const vector<int> &vec) const {
    size_t seed = vec.size();
    for (const int &i : vec) {
      seed ^= i + 0x9e3779b9 + (seed << 6) + (seed >> 2);
    }
    return seed;
  }
};

struct VectorEqual {
  bool operator()(const vector<int> &vec1, const vector<int> &vec2) const {
    return vec1 == vec2;
  }
};

typedef int Digit;
typedef vector<Digit> Code;
typedef vector<Code> CodeList;

void printCode(const Code &code) {
  for (Digit digit : code) {
    cout << digit << "";
  }
}

void printCodeList(const CodeList &codeList, int max_to_print = 10) {
  int num_to_print = min(max_to_print, static_cast<int>(codeList.size()));
  cout << "(" << num_to_print << "/" << codeList.size() << ") [";
  for (int i = 0; i < num_to_print; ++i) {
    Code code = codeList[i];
    printCode(code);
    if (i != num_to_print - 1) {
      cout << ", ";
    }
  }
  cout << "]" << endl;
}

Score compare_codes(const Code &a, const Code &b) {
  int green = 0;
  vector<int> count1(10, 0);
  vector<int> count2(10, 0);

  auto it1 = a.begin();
  auto it2 = b.begin();

  for (; it1 != a.end() && it2 != b.end(); ++it1, ++it2) {
    if (*it1 == *it2) {
      green++;
    } else {
      count1[*it1]++;
      count2[*it2]++;
    }
  }

  int red = 0;
  for (int i = 0; i < 10; ++i) {
    red += min(count1[i], count2[i]);
  }

  return Score{green, red};
}

double calc_expected_info(const Code &code, const CodeList &possible) {
  int initial_count = possible.size();
  double expected_info = 0.0;
  unordered_map<Score, int, score_hash> distribution;

  for (const auto &p : possible) {
    Score score = compare_codes(p, code);
    distribution[score]++;
  }

  for (const auto &pair : distribution) {
    Score score = pair.first;
    int freq = pair.second;
    double prob = static_cast<double>(freq) / initial_count;
    double info = log2(1.0 / prob);
    expected_info += prob * info;
  }

  return round(expected_info * pow(10, 12)) / pow(10, 12);
}

CodeList dedupe_codes_to_patterns(const CodeList &guesses,
                                  const CodeList &codes) {
  // Map of digits used in previous guesses (e.g. {0: 0, 1: 1, 2: 2})
  unordered_map<Digit, Digit> guess_digits;
  for (const Code &guess : guesses) {
    for (Digit d : guess) {
      guess_digits[d] = d;
    }
  }

  // List of digits not used in previous guesses (e.g. [3,4,5,6,7,8,9])
  vector<Digit> unused_digits;
  for (Digit i = 0; i < 10; i++) {
    if (guess_digits.find(i) == guess_digits.end()) {
      unused_digits.push_back(i);
    }
  }
  // The list of deduplicated codes (e.g. 210, 987, 886)
  CodeList deduped_codes;
  // The code patterns we've seen (e.g. 210, 345, 334)
  unordered_set<Code, VectorHash, VectorEqual> code_patterns;

  for (const Code &code : codes) {
    // Map of code digits to what we should replace it with in the pattern
    unordered_map<Digit, Digit> digit_map = guess_digits;
    int unused_idx = 0;

    Code code_pattern;

    bool is_ascending = true;
    Digit prev_patt_digit = -1;
    for (Digit d : code) {
      if (digit_map.find(d) == digit_map.end()) {
        digit_map[d] = unused_digits[unused_idx++];
      }
      // If no previous guesses, determine if in ascending order
      if (guesses.empty()) {
        if (digit_map[d] < prev_patt_digit) {
          is_ascending = false;
          break;
        }
        prev_patt_digit = digit_map[d];
      }
      code_pattern.push_back(digit_map[d]);
    }
    // If no previous guesses, only keep those in ascending order
    // E.g. 0120 and 0012 give the same expected info as the first guess
    if (guesses.empty() && !is_ascending) {
      continue;
    }
    if (code_patterns.find(code_pattern) == code_patterns.end()) {
      code_patterns.insert(code_pattern);
      deduped_codes.push_back(code);
    }
  }

  return deduped_codes;
}

bool hasUniqueDigits(const Code &code) {
  unordered_set<Digit> digitSet;
  for (const auto &digit : code) {
    if (digitSet.find(digit) != digitSet.end()) {
      return false;
    }
    digitSet.insert(digit);
  }
  return true;
}

pair<CodeList, CodeList> calcInitialPossibleUnused(int numDigits,
                                                   bool hasRepeats) {
  CodeList possible;
  CodeList unused;

  for (int i = 0; i < pow(10, numDigits); i++) {
    Code code;
    bool code_has_repeats = false;
    unordered_set<Digit> seen_digits;
    for (int j = numDigits - 1; j >= 0; j--) {
      Digit digit = (i / static_cast<int>(pow(10, j))) % 10;
      code.push_back(digit);
      // Track if this code has repeats, if we don't want that
      if (!hasRepeats) {
        if (seen_digits.find(digit) != seen_digits.end()) {
          code_has_repeats = true;
        }
        seen_digits.insert(digit);
      }
    }
    // If we don't want repeat codes, don't include them
    if (hasRepeats || !code_has_repeats) {
      possible.push_back(code);
    }
    unused.push_back(code);
  }

  return pair<CodeList, CodeList>{possible, unused};
}

Code calc_best_next_guess(const CodeList &search_set,
                          const CodeList &possible) {
  printf("Searching (no time limit)...\n");

  int best_idx = 0;
  double best_expected_info = 0;
  Code best_guess;

  for (int i = 0; i < search_set.size(); i++) {
    const Code guess = search_set[i];
    double expected_info = calc_expected_info(guess, possible);
    if (expected_info > best_expected_info) {
      best_idx = i;
      best_expected_info = expected_info;
      best_guess = guess;
    }

    if (i % 1000 == 0) {
      cout << "iter " << i << "/" << search_set.size() << " ("
           << static_cast<double>(i) / search_set.size() * 100
           << "%): best_idx = " << best_idx << ", best guess = '";
      printCode(best_guess);
      cout << "', expected info = " << best_expected_info << endl;
    }

    // if (max_search_time > 0 && i + 1 >= max_search_time) {
    //     printf("INFO: Ran out of time to keep searching");
    //     break;
    // }
  }

  cout << "Found best_idx = " << best_idx << ", best_guess = '";
  printCode(best_guess);
  cout << "', best_expected_info = " << best_expected_info << "\n";

  return best_guess;
}

int main() {
  // Code a = {0, 0, 1, 2, 3, 4};
  // Code b = {4, 0, 0, 2, 1, 4};

  // Score score = compare_codes(a, b);

  // cout << "Green: " << score.green << endl;
  // cout << "Red: " << score.red << endl;

  int numDigits = 6;
  bool hasRepeats = true;

  pair<CodeList, CodeList> result =
      calcInitialPossibleUnused(numDigits, hasRepeats);
  CodeList possible = result.first;
  CodeList unused = result.second;

  CodeList guesses = {};

  cout << "POSSIBLE ";
  printCodeList(possible, 10);

  cout << "UNUSED ";
  printCodeList(unused, 10);

  CodeList deduped = dedupe_codes_to_patterns(guesses, unused);
  cout << "DEDUPED ";
  printCodeList(deduped, 10);

  Code suggestion = calc_best_next_guess(deduped, possible);
  cout << "Suggestion:\t";
  printCode(suggestion);
  cout << endl;

  return 0;
}
