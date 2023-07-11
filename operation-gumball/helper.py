from collections import defaultdict
from math import log2

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


def calc_expected_info(word, possible):
    initial_count = len(possible)
    expected_info = 0
    distribution = defaultdict(int)
    for p in possible:
        score = compare_codes(p, word)
        distribution[score] += 1
    for score, freq in distribution.items():
        prob = freq/initial_count
        info = log2(1/prob)
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