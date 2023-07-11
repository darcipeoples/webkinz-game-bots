import json
import time
import math

from collections import defaultdict
from multiprocessing import Pool
from functools import partial

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
    return int(green), int(red)

def calc_entropy_distribution(word, possible):
    initial_count = len(possible)
    expected_info = 0
    distribution = dict()
    for p in possible:
        score = compare_codes(p, word)
        if score not in distribution:
        #     distribution[score] = {'new_possible': []}
        # distribution[score]['new_possible'].append(p)
            distribution[score] = {'freq': 0}
        distribution[score]['freq'] += 1
    for score, info in distribution.items():
        # new_possible = info['new_possible']
        # freq = len(new_possible)
        freq = info['freq']
        prob = freq/initial_count
        info = math.log2(1/prob)
        expected_info += prob * info
        # distribution[score]['freq'] = freq
        distribution[score]['prob'] = prob
        distribution[score]['info'] = info
        distribution[score]['pinfo'] = prob * info
        # distribution[score]['new_possible'] = []
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

# Given a list of codes and previous guesses, will deduplicate them into ascending ABC paterns (convert to pattern, dedupe)
# E.g. [000, 001, 002, 010, 011, 100, 101, 110, 111] -> [000, 001, 001, 010, 011, 011, 010, 001, 000] -> [000, 001, 010, 011] -> [000, 001, 011]
# Useful for deduping options for the next guess (first, second)
def dedupe_codes_to_patterns(guesses, codes):
    guess_chars = set(''.join(guesses))
    # guess_chars = set(guesses)
    all_unused_chars = sorted([str(x) for x in range(10) if str(x) not in guess_chars])
    new_codes = defaultdict(list)
    for code in codes:
        unused_chars = all_unused_chars[:]
        char_map = defaultdict(int)
        new_code = ''
        for c in code:
            # If the digit is in the OG guess, keep it as is
            if c in guess_chars:
                new_code += c
                continue
            # If the digit is brand new, assign it a new value using unused_chars
            if c not in char_map:
                char_map[c] = unused_chars.pop(0)
            # If digit isn't in OG guess, use it's mapped value
            new_code += char_map[c]
        new_codes[new_code].append(code)
    # TODO: are we able to return non ascending for second guesses?
    new_codes = list([cs[0] for cs in new_codes.values()])
    if len(guesses) == 0:
        new_codes = [x for x in new_codes if ''.join(sorted(x)) == x]
    return new_codes

def filter_possible_unused(possible, unused, attempt, score):
    # TODO: should we be removing p from possible??
    possible = [p for p in possible if compare_codes(p, attempt) == score and p != attempt]
    # possible = [p for p in possible if compare_codes(p, attempt) == score]
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
def calc_best_next_guess(search_set, max_search_time, possible):
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
                debug(f"iter {i}/{len(search_set)} ({i/len(search_set)*100:.1f}%, {time.time() - search_start:.1f}s): best_idx = {best_idx}, best guess = '{suggestion}', expected info = {best_value}")
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

def dedupe_list(seq):
    seen = set()
    return [x for x in seq if not (x in seen or seen.add(x))]

def _calc_tree(num_digits, has_repeats, guesses, responses, depth, max_depth):
    if depth <= 2:
        print(f'Calculating _calc_tree({num_digits}, {has_repeats}, {guesses}, {responses}, {depth}, {max_depth})')
    if depth >= max_depth:
        return {}
    tree = {} # TODO: can we read in already calculated bit to keep building?

    # Set remain & calculate possible
    # TODO: can we pass possible down instead of recalc?
    possible, unused = calc_initial_possible_unused(num_digits, has_repeats)
    new_possible = possible[:]
    new_unused = unused[:]
    for guess, response in zip(guesses, responses):
        new_possible, new_unused = filter_possible_unused(new_possible, new_unused, guess, response)
    remain = len(new_possible)
    tree['remain'] = remain

    # Set guess
    if depth == 0:
        guess = get_best_first_guess(num_digits, has_repeats, 'entropy')
    else:
        search_set = dedupe_list(new_possible + new_unused)
        search_set = dedupe_codes_to_patterns(guesses, search_set)
        guess = calc_best_next_guess(search_set, None, new_possible)
    tree['guess'] = guess

    # Set scores
    results = calc_entropy_distribution(guess, new_possible)
    tree['scores'] = {}
    for score, info in sorted(results['distribution'].items()):
        score_str = str(score)
        freq = info['freq']
        if score == (num_digits, 0):
            if (freq != 1):
                print(f"WARNING: remaining ({freq}) != 1 for (N, 0) response: {num_digits}, {has_repeats}, {guesses}, {responses}, {depth}")  
            tree['scores'][score_str] = {'remain': freq}
            continue
        if depth + 1 >= max_depth:
            tree['scores'][score_str] = {'remain': freq}
            continue
        tree['scores'][score_str] = _calc_tree(num_digits, has_repeats, guesses + [guess], responses + [score], depth + 1, max_depth)
        if tree['scores'][score_str]['remain'] != freq:
            print(f"WARNING: tree['scores'][score_str]['remain'] ({tree['scores'][score_str]['remain']}) !=  freq ({freq}): {num_digits}, {has_repeats}, {guesses}, {responses}, {depth}, {score_str}")  
    
    # if depth <= 2:
    #     print(f'Calculated _calc_tree({num_digits}, {has_repeats}, {guesses}, {responses}, {depth}, {max_depth})')
    #     with open(f"temp-output-{num_digits}-{has_repeats}-{max_depth}-{depth}-[{', '.join([str(x) for x in zip(guesses, responses)])}]", "w+") as f:
    #         f.write(json.dumps(tree, indent=2))
    return tree

def calc_tree(num_digits, has_repeats, guesses, responses, max_depth):
    tree = {}
    has_repeats_str = str(has_repeats)
    num_digits_str = str(num_digits)
    if num_digits not in tree:
        tree[num_digits_str] = {}
    if has_repeats_str not in tree:
        tree[num_digits_str][has_repeats_str] = _calc_tree(num_digits, has_repeats, guesses, responses, depth=0, max_depth=max_depth)
    return tree

def calc_save_tree(num_digits, has_repeats, max_depth):
    tree = calc_tree(num_digits, has_repeats, [], [], max_depth)
    pretty_json = json.dumps(tree, indent=2)
    with open(f"strategies/output-{num_digits}-{has_repeats}-{max_depth}.json", "w+") as f:
        f.write(pretty_json)
    # print(pretty_json)

def main():
    # TODO: See if there is a way to further dedupe the search_set
    calc_save_tree(num_digits=3, has_repeats=False, max_depth=3)

if __name__ == '__main__':
    main()
