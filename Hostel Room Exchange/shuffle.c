#include "commonHeadExchange.h"

/* own generator so a seed gives the same order everywhere */
unsigned long long randomState = 1;

void seedRandom(unsigned int seed)
{
    randomState = (unsigned long long)seed * 2654435761ULL + 1;
}

/* 0..bound-1 */
unsigned int nextRandom(unsigned int bound)
{
    randomState = randomState * 6364136223846793005ULL + 1442695040888963407ULL;
    if (bound == 0)
        return 0;
    return (unsigned int)((randomState >> 33) % bound);
}

/* Fisher-Yates from the end */
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
