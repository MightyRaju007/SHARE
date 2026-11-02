#include "commonHeadExchange.h"

/* The generator's current state. We use our own generator instead of
   rand() so that a seed gives the same order on every machine. */
unsigned long long randomState = 1;

/* Resets the random number generator from a seed. Calling this with the
   same seed makes every later nextRandom() call give the same sequence, which
   is how a pool run or a lottery can be repeated exactly. */
void seedRandom(unsigned int seed)
{
    randomState = (unsigned long long)seed * 2654435761ULL + 1;
}

/* Returns the next random number in the range 0 to bound-1. The state is
   updated with a linear congruential formula (multiply, add, wrap around) and
   the high bits are used because they are the better mixed ones. */
unsigned int nextRandom(unsigned int bound)
{
    randomState = randomState * 6364136223846793005ULL + 1442695040888963407ULL;
    if (bound == 0)
        return 0;
    return (unsigned int)((randomState >> 33) % bound);
}

/* Shuffles an array of roll numbers in place using the seed. This is the
   Fisher-Yates shuffle, walk from the last slot down to the second, and swap
   each slot with a randomly chosen slot at or before it. Every ordering is
   equally likely, and the same seed always gives the same ordering. */
void shuffleRolls(long int *rolls, int n, unsigned int seed)
{
    int i;
    seedRandom(seed);
    for (i = n - 1; i > 0; i--) {
        int j = (int)nextRandom((unsigned int)i + 1);
        long t = rolls[i];
        rolls[i] = rolls[j];
        rolls[j] = t;
    }
}
