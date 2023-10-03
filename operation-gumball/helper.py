import json
import time
import math

from collections import defaultdict
from multiprocessing import Pool
from functools import partial

import numpy as np

verbose = True
break_program = False

# Calculated using minimax (best worst eliminations) and expected information (entropy)
LEVEL_BEST_FIRST_GUESS = {
    'minimax': {
        False: ['012', '0123', '01234', '012345'], 
        True: ['012', '0123', '00123', '001123']
    },
    'entropy': {
        False: ['012', '0123', '01234', '001234'], 
        True: ['012', '0123', '01234', '001234']
    }
}

def debug(*args, **kwargs):
    if verbose:
        print(*args, **kwargs)

def get_best_first_guess(num_digits, has_repeats, method='entropy'):
    return LEVEL_BEST_FIRST_GUESS[method][has_repeats][num_digits - 3]

# Returns the number of red & green gumballs
# TODO: possible to use LRU cache here or elsewhere? Or I/O too large?
def compare_codes(a, b):
    green = 0
    count1 = [0] * 10
    count2 = [0] * 10
    for dig1, dig2 in zip(a, b):
        if dig1 == dig2:
            green += 1
        else:
            count1[int(dig1)] += 1
            count2[int(dig2)] += 1
    red = sum(map(min, count1, count2))
    return green, red

def calc_entropy_distribution(word, possible):
    initial_count = len(possible)
    expected_info = 0
    distribution = defaultdict(list)
    for p in possible:
        score = compare_codes(p, word)
        distribution[score].append(p)
    for score, new_possible in distribution.items():
        freq = len(new_possible)
        prob = freq/initial_count
        info = math.log2(1/prob)
        expected_info += prob * info
    return {'expected_info': round(expected_info, 12), 'distribution': distribution}

def calc_expected_info(word, possible):
    initial_count = len(possible)
    expected_info = 0
    distribution = defaultdict(int)
    for p in possible:
        score = compare_codes(p, word)
        distribution[score] += 1
    for score, freq in distribution.items():
        prob = freq/initial_count
        info = math.log2(1/prob)
        expected_info += prob * info
    return round(expected_info, 12)

def calc_worst_elims(word, possible):
    initial_count = len(possible)
    worst_elims = initial_count
    distribution = defaultdict(int)
    for p in possible:
        score = compare_codes(p, word)
        distribution[score] += 1
    for score, freq in distribution.items():
        num_elims = len(possible) - freq
        worst_elims = min(worst_elims, num_elims)
    return worst_elims

# Given a list prev. guesses and potential next guesses, dedupe those with the same pattern.
# E.g. [234] [567, 619, 233, 211] -> {015: [567, 619], 233: [233], 200: [211]} -> [567, 233, 211]
# E.g. [] [987, 234, 336, 991, 388, 112, 555, 888, 737, 181] -> {012: [987, 234], 001: [336, 991], 011: [388, 112], 000: [555, 888], 010: [737, 181]} -> [987, 336, 388, 555]
# See test_dedupe_codes_to_patterns for more test cases
def dedupe_codes_to_patterns(guesses, codes):
    guess_chars = dict()
    for guess in guesses:
        for c in guess:
            guess_chars[c] = c
    unused_chars = [str(x) for x in range(10) if str(x) not in guess_chars]
    code_patterns = set()
    deduped_codes = []
    for code in codes:
        char_map = guess_chars.copy()
        unused_idx = 0
        last_pattern_c = None
        is_ascending = True
        code_pattern = ''
        for c in code:
            if c not in char_map:
                char_map[c] = unused_chars[unused_idx]
                unused_idx += 1
            code_pattern += char_map[c]

            # If no prev. guesses, track if ascending
            if guesses == []:
                if last_pattern_c is not None and char_map[c] < last_pattern_c:
                    is_ascending = False
                    break
                last_pattern_c = char_map[c]
        # If no prev guesses, only keep codes in asc. order
        if guesses == [] and not is_ascending:
            continue

        if code_pattern not in code_patterns:
            code_patterns.add(code_pattern)
            deduped_codes.append(code)

    return deduped_codes

# This narrows down the search set by using info about the possible set.
# E.g. if the possible codes = [887878, 777787, 878778, ...], the code can only contain a subset of {7, 8}
# So, guessing a code that has neither 7 nor 8 is useless.
# And the answer won't contain {0,1,2,3,4,5,6,9}, so we can just includes codes with {0}.
# E.g. 007788 and 117788 give the same info.
def dedupe_codes_using_patterns_possible(search_set, possible):
    if len(search_set) == len(possible) and search_set == possible:
        return search_set
    possible_chars = set()
    for p in possible:
        for c in p:
            possible_chars.add(c)
        if len(possible_chars) == 10:
            return search_set
    impossible_chars = [str(x) for x in range(10) if str(x) not in possible_chars]
    code_patterns = set()
    deduped_codes = []
    has_a_possible_char = False
    for code in search_set:
        code_pattern = ''
        for c in code:
            if c not in possible_chars:
                code_pattern += impossible_chars[0]
            else:
                code_pattern += c
                has_a_possible_char = True
        if not has_a_possible_char:
            continue

        if code_pattern not in code_patterns:
            code_patterns.add(code_pattern)
            deduped_codes.append(code)

    return deduped_codes

# Only includes codes that contain possible letters.
# [UNUSED] WARNING: causes ever-so-slightly worse average guesses, though calculation is faster.
def dedupe_codes_only_possible_chars(search_set, possible):
    if len(search_set) == len(possible) and search_set == possible:
        return search_set
    possible_chars = set()
    for p in possible:
        for c in p:
            possible_chars.add(c)
        if len(possible_chars) == 10:
            return search_set
    return [code for code in search_set if all([x in possible_chars for x in code])]

def test_dedupe_codes_using_patterns_possible():
    tests = [
        {'input': (["778", "878", "012", "007", "008", "018", "801"], ["778", "878"]), 'output': ['778', '878', '007', '008', '801']},
    ]
    for test in tests:
        input = test['input']
        actual = dedupe_codes_using_patterns_possible(*input)
        expected = test['output']
        assert actual == expected, f"dedupe_codes_using_patterns_possible({input}) = {actual} != {expected}"

def test_dedupe_codes_to_patterns():
    tests = [
        {'input': (["708"], ['117', '210', '345', '510', '558', '654']), 'output': ['117', '210', '345', '558']},
        {'input': (["012"], ['117', '210', '345', '510', '558', '654']), 'output': ['117', '210', '345', '510', '558']},
        {'input': (["708", "059"], ['117', '210', '345', '510', '558', '654']), 'output': ['117', '210', '345', '510', '558', '654']},
        {'input': (["001234"], ["05678", "08765"]), 'output': ['05678']},
        {'input': (["53052"], ["97237"]), 'output': ['97237']},
        {'input': (['234'], ['567', '619', '233', '211']), 'output': ['567', '233', '211']},
        {'input': ([], ['987', '234', '336', '991', '388', '112', '555', '888', '737', '181']), 'output': ['987', '336', '388', '555']}
    ]
    for test in tests:
        input = test['input']
        actual = dedupe_codes_to_patterns(*input)
        expected = test['output']
        assert actual == expected, f"dedupe_codes_to_patterns({input}) = {actual} != {expected}"

def filter_possible_unused(possible, unused, attempt, score):
    # TODO: should we be removing p from possible??
    # possible = [p for p in possible if compare_codes(p, attempt) == score and p != attempt]
    possible = [p for p in possible if compare_codes(p, attempt) == score]
    if attempt in unused:
        unused.remove(attempt)
    return possible, unused

def calc_initial_possible_unused(num_digits, has_repeats):
    possible = [str(i).zfill(num_digits) for i in range(10**(num_digits))]
    unused = possible
    if not has_repeats:
        possible = [code for code in possible if unique_chars(code)]
    return possible, unused

# TODO: extend to make tree of decisions. E.g. what's best to guess after 001234 [1, 2]?
def calc_best_first_guess(num_digits, has_repeats):
    possible, unused = calc_initial_possible_unused(num_digits, has_repeats)
    search_set = dedupe_codes_to_patterns([], unused)
    suggestion = calc_best_next_guess(search_set, 600, possible)
    return suggestion

def unique_chars(s):
    return len(set(s)) == len(s)

# TODO: if following time limit, need to shuffle the search set
# TODO: could I stop searching if I hit the max possible entropy?
def calc_best_next_guess(search_set, max_search_time, possible):
    if 1 <= len(possible) <= 2:
        return possible[0]
    if max_search_time is not None:
        debug(f"Searching ({max_search_time:.1f}s limit)...")
    else:
        debug(f"Searching (no time limit)...")
    results = []
    if len(search_set) == 0:
        debug("Something's rotten in Denmark, search_set is empty")
        return None
    
    temp_filename = f"temp-{len(search_set[0])}-{len(possible)}-{len(search_set)}-{int(time.time())}.csv"

    search_start = time.time()

    with Pool(16) as p:
        # TODO: instead of returning just the expected info, also return the score distribution.
        for i, result in enumerate(p.imap(partial(calc_expected_info, possible=possible), search_set, chunksize=1)):
            if result is None or break_program:
                debug("Broke program, cancel calc_best_next_guess")
                p.terminate()
                return None
            results.append(result)
            if i % 2000 == 0:
                best_value = max(results)
                best_idx = results.index(best_value)
                suggestion = search_set[best_idx]
                debug(f"iter {i}/{len(search_set)}({len(possible)}) ({i/len(search_set)*100:.1f}%, {time.time() - search_start:.1f}s): best_idx = {best_idx}, best guess = '{suggestion}', expected info = {best_value}")
                # with open(temp_filename, "w+") as f:
                #     f.write('guess, expected_info\n')
                #     f.write('\n'.join([f"{x[0]}, {x[1]}" for x in zip(search_set, results)]))
            if max_search_time is not None and time.time() - search_start >= max_search_time:
                debug("INFO: ran out of time to keep searching")
                p.terminate()
                break
    
    debug(f"searched {len(results)/len(search_set)*100:.1f}% ({len(results)}/{len(search_set)}) guesses in {time.time() - search_start:.2f}s")
    best_value = max(results)
    # with open(temp_filename, "w+") as f:
    #     f.write('guess, expected_info\n')
    #     f.write('\n'.join([f"{x[0]}, {x[1]}" for x in zip(search_set, results)]))
    best_idx = results.index(best_value)
    suggestion = search_set[best_idx]
    debug(f"guess = {suggestion} has the best expected info = {best_value}")
    
    return suggestion

# TODO: since we're using dedupe list to just put possible before rest of unused, can prob make a better impl that uses possible as a set
# TODO: dicts are also like ordered sets. Prob should be using ordered sets instead of hopeful lists
def dedupe_list(seq):
    seen = set()
    return filter(lambda x: not (x in seen or seen.add(x)), seq)

STRAT = None
def _calc_tree(num_digits, has_repeats, guesses, responses, depth, max_depth, possible, unused):
    global STRAT
    # if len(responses) >= 1:
    #     if num_digits == 6 and max_depth == 100:
    #         if has_repeats and responses[0] in [(0,0),(0,1)]:
    #             return {'remain': -1}
    #         if has_repeats and responses[0] == (0,2) and len(responses) >= 2 and responses[1] in [(0,0),(0,1),(0,2),(0,3),(0,4),(0,5),(0,6),(1,0),(1,1)]:
    #             return {'remain': -1}
    CHECKPOINT_DEPTH = 2
    if num_digits == 6:
        CHECKPOINT_DEPTH = 2
    if depth <= CHECKPOINT_DEPTH:
        print(f'Calculating _calc_tree({num_digits}, {has_repeats}, {guesses}, {responses}, {depth}, {max_depth}, {len(possible)}, {len(unused)})')
    if depth >= max_depth:
        return {}
    tree = {} # TODO: can we read in already calculated bit to keep building?

    # Set remain
    tree['remain'] = len(possible)

    # Set guess
    guess = None
    # if depth == 0:
    #     guess = get_best_first_guess(num_digits, has_repeats, 'entropy')

    if guess is None and STRAT is not None:
        # TODO: finish this. Should read the best guesses from the JSON, if available
        substrat = STRAT[str(num_digits)][str(has_repeats)]
        known_guess = True
        for code, response in zip(guesses, responses):
            str_response = str(response)
            if "guess" in substrat and substrat["guess"] == code:
                if "scores" in substrat and str_response in substrat["scores"]:
                    substrat = substrat["scores"][str_response]
                    continue
            known_guess = False
            break
        if known_guess:
            if 'guess' in substrat:
                guess = substrat['guess']
                print(f"\t\tSUCCESS:\tFound pre-calculated guess for {list(zip(guesses, responses))}: {guess}")
            else:
                print(f"\t\tWARNING:\tGuess not in substrat for {list(zip(guesses, responses))}")
        else:
            print(f"\t\tWARNING:\tNo known_guess for{list(zip(guesses, responses))} ")
    else:
        print(f"\t\tERROR:\tSTRAT is None")

    if guess is None:
        if 1 <= len(possible) <= 2:
            guess = possible[0]
        else:
            search_set = dedupe_list(possible + unused)
            search_set = dedupe_codes_to_patterns(guesses, search_set)
            search_set = dedupe_codes_using_patterns_possible(search_set, possible)
            guess = calc_best_next_guess(search_set, None, possible)
    tree['guess'] = guess
    new_unused = [x for x in unused if x != guess] # TODO: faster way?

    # Set scores
    tree['scores'] = {}
    results = calc_entropy_distribution(guess, possible)
    for score, new_possible in sorted(results['distribution'].items()):
        score_str = str(score)
        freq = len(new_possible)
        if score == (num_digits, 0):
            if (freq != 1):
                print(f"WARNING: new_possible ({freq}) != 1 for (N, 0) response: {num_digits}, {has_repeats}, {guesses}, {responses}, {depth}, {len(possible)}, {len(unused)}, {len(new_possible)}, {len(new_unused)}")  
            tree['scores'][score_str] = {'remain': freq}
            continue
        if depth + 1 >= max_depth:
            tree['scores'][score_str] = {'remain': freq}
            continue
        tree['scores'][score_str] = _calc_tree(num_digits, has_repeats, guesses + [guess], responses + [score], depth + 1, max_depth, new_possible, new_unused)
        if tree['scores'][score_str]['remain'] != freq:
            print(f"WARNING: tree['scores'][score_str]['remain'] ({tree['scores'][score_str]['remain']}) !=  freq ({freq}): {num_digits}, {has_repeats}, {guesses}, {responses}, {depth}, {score_str}, {len(possible)}, {len(unused)}, {len(new_possible)}, {len(new_unused)}")  
    
    if depth <= CHECKPOINT_DEPTH:
        print(f'Calculated _calc_tree({num_digits}, {has_repeats}, {guesses}, {responses}, {depth}, {max_depth}, {len(possible)}, {len(unused)}, {len(new_possible)}, {len(new_unused)})')
        with open(f"temp-output-{num_digits}-{has_repeats}-{max_depth}-{depth}-[{', '.join([str(x) for x in zip(guesses, responses)])}]", "w+") as f:
            f.write(json.dumps(tree, indent=2))
    return tree

def calc_tree(num_digits, has_repeats, guesses, responses, max_depth):
    tree = {}
    possible, unused = calc_initial_possible_unused(num_digits, has_repeats)
    for guess, response in zip(guesses, responses):
        possible, unused = filter_possible_unused(possible, unused, guess, response)

    tree[str(num_digits)] = {}
    tree[str(num_digits)][str(has_repeats)] = _calc_tree(num_digits, has_repeats, guesses, responses, 0, max_depth, possible, unused)
    return tree
    
def calc_save_tree(num_digits, has_repeats, max_depth):
    tree = calc_tree(num_digits, has_repeats, [], [], max_depth)
    pretty_json = json.dumps(tree, indent=2)
    with open(f"strategies/output-{num_digits}-{has_repeats}-{max_depth}.json", "w+") as f:
        f.write(pretty_json)
    # print(pretty_json)

def print_strategy_stats(filename, num_digits):
    with open(filename) as f:
        lines = f.readlines()
    soln_lines = [x for x in lines if '"({}, 0)": {{'.format(num_digits) in x]
    soln_guesses = [x.find(x.strip())//4-1 for x in soln_lines]
    print(filename)
    print("Count:\t\t", len(soln_guesses))
    print("Average:\t", np.mean(soln_guesses))
    print("Std. Dev.:\t", np.std(soln_guesses))
    print("Distribution:\t", dict(enumerate(np.bincount(soln_guesses))))

def main():
    # TODO: See if there is a way to further dedupe the search_set
    global STRAT

    num_digits = 6
    has_repeats = True
    depth_loaded = 6
    max_depth = 100
    
    # with open(f"strategies/output-{num_digits}-{has_repeats}-{depth_loaded}-entropy.json") as f:
    #     STRAT = json.load(f)
    #     print(STRAT)

    # start = time.time()
    # calc_save_tree(num_digits=num_digits, has_repeats=has_repeats, max_depth=max_depth)
    # end = time.time()
    # with open("temp.csv", "a+") as f:
    #     f.write(f"{num_digits}, {has_repeats}, {max_depth}, {depth_loaded}, {end - start}\n")

    max_depth = 100
    num_digits = 6
    has_repeats = True
    filename = f'strategies/output-{num_digits}-{has_repeats}-{max_depth}-entropy.json'
    print_strategy_stats(filename, num_digits)

if __name__ == '__main__':
    main()
