#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "commonHeaderFile.h"

/* the highest compatibility score possible, used to size the counting sort */
#define MAX_SCORE 16
/* rank given to a floor a student did not list, one worse than third */
#define NO_RANK 3

/* one possible pairing from the random pool, i and j are indexes into the pool */
typedef struct {
    int i;
    int j;
    int score;
} Candidate;

/* Where a floor sits in a student's list. 0 means first choice, 1 second,
   2 third. NO_RANK (3) means the student did not list that floor at all,
   which is deliberately one worse than a third choice. */
int rankOf(const Preference *p, const Choice *c)
{
    int r;
    for (r = 0; r < NUM_CHOICES; r++) {
        if (sameFloor(&p->choices[r], c))
            return r;
    }
    return NO_RANK;
}

/* qsort comparator for Preference records, lowest roll number first. */
int cmpByRoll(const void *a, const void *b)
{
    long x = ((const Preference *)a)->rollNo;
    long y = ((const Preference *)b)->rollNo;
    return (x > y) - (x < y);
}

/* qsort comparator for plain roll numbers, lowest first. */
int cmpLong(const void *a, const void *b)
{
    long x = *(const long *)a;
    long y = *(const long *)b;
    return (x > y) - (x < y);
}

/* Looks up a student's record by roll number in the prefs array. Returns
   NULL if nobody has that roll. A simple scan is fine here because it is
   only used while checking locked pairs. */
const Preference *findInPrefs(const Preference *prefs, int n, long roll)
{
    int k;
    for (k = 0; k < n; k++) {
        if (prefs[k].rollNo == roll)
            return &prefs[k];
    }
    return NULL;
}

/* Fills in a Unit for two students who will share a room. The choices
   passed in become the unit's choices and solo is set to 0. */
void makePairUnit(Unit *u, long rollA, long rollB, const Choice choices[NUM_CHOICES])
{
    memset(u, 0, sizeof(*u));
    u->rollA = rollA;
    u->rollB = rollB;
    memcpy(u->choices, choices, sizeof(u->choices));
    u->numChoices = NUM_CHOICES;
    u->solo = 0;
}

/* Fills in a Unit for one student with no partner. rollB stays 0 and solo
   is set to 1 so the allocator knows only one bed is needed. */
void makeSoloUnit(Unit *u, long rollA, const Choice choices[NUM_CHOICES])
{
    memset(u, 0, sizeof(*u));
    u->rollA = rollA;
    u->rollB = 0;
    memcpy(u->choices, choices, sizeof(u->choices));
    u->numChoices = NUM_CHOICES;
    u->solo = 1;
}

/* How well two random students suit each other, as a number from 0 to 16.
   Every choice of a is compared with every choice of b. If they share a
   floor the score is 10 plus a bonus, if they only share a hostel it is
   just the bonus, otherwise 0. The bonus is bigger when both of them put
   that floor near the top of their lists. The best match found wins. */
int compatibilityScore(const Preference *a, const Preference *b)
{
    int i, j;
    int bestFloorBonus = -1;
    int bestHostelBonus = -1;

    for (i = 0; i < NUM_CHOICES; i++) {
        for (j = 0; j < NUM_CHOICES; j++) {
            /* bonus is 6 when both ranked it first, down to 2 for both third */
            int bonus = (3 - i) + (3 - j);

            if (sameFloor(&a->choices[i], &b->choices[j])) {
                if (bonus > bestFloorBonus)
                    bestFloorBonus = bonus;
            } else if (strcmp(a->choices[i].hostel, b->choices[j].hostel) == 0) {
                if (bonus > bestHostelBonus)
                    bestHostelBonus = bonus;
            }
        }
    }

    /* a shared floor always beats a shared hostel */
    if (bestFloorBonus >= 0)
        return 10 + bestFloorBonus;
    if (bestHostelBonus >= 0)
        return bestHostelBonus;
    return 0;
}

/* Builds the three floors a new random pair will ask for. Every floor that
   either student listed is a candidate. Each candidate is scored as rankA
   plus rankB (a floor missing from one list counts as 3), lower is better,
   and the three lowest go out in order. */
void sharedChoices(const Preference *a, const Preference *b, Choice out[NUM_CHOICES])
{
    /* union of both lists: at least 3 floors, at most 6. Floors in neither
       list would total 6, worse than anything here, so they are never needed */
    Choice cand[2 * NUM_CHOICES];
    int total[2 * NUM_CHOICES];
    int n = 0;
    int k, m;

    for (k = 0; k < NUM_CHOICES; k++)
        cand[n++] = a->choices[k];

    for (k = 0; k < NUM_CHOICES; k++) {
        int dup = 0;
        for (m = 0; m < n; m++) {
            if (sameFloor(&cand[m], &b->choices[k])) {
                dup = 1;
                break;
            }
        }
        if (!dup)
            cand[n++] = b->choices[k];
    }

    for (k = 0; k < n; k++)
        total[k] = rankOf(a, &cand[k]) + rankOf(b, &cand[k]);

    /* insertion sort: lowest total first, ties by hostel then floor so the
       result does not depend on which student is a and which is b */
    for (k = 1; k < n; k++) {
        Choice c = cand[k];
        int t = total[k];
        m = k - 1;
        while (m >= 0) {
            int cmp = total[m] - t;
            if (cmp == 0) cmp = strcmp(cand[m].hostel, c.hostel);
            if (cmp == 0) cmp = cand[m].floor - c.floor;
            if (cmp <= 0)
                break;
            cand[m + 1] = cand[m];
            total[m + 1] = total[m];
            m--;
        }
        cand[m + 1] = c;
        total[m + 1] = t;
    }

    for (k = 0; k < NUM_CHOICES; k++)
        out[k] = cand[k];
}

/* Pairs up everyone in the random pool and writes the resulting units to
   out. Every possible pair is scored, the pairs are sorted by score with the
   best first, and then taken greedily, a pair is kept if both students are
   still free. Anyone left over at the end becomes a solo unit. Returns the
   number of units written. */
int pairRandoms(Preference *pool, int n, Unit *out)
{
    long numCand, p;
    Candidate *cand;
    int *order;
    int count[MAX_SCORE + 1];
    int start[MAX_SCORE + 1];
    int *paired;
    int i, j, s, made = 0;

    if (n <= 0)
        return 0;

    /* records arrive in hash table order, which is arbitrary. Sorting by
       roll number first makes the tie-breaking below reproducible. */
    qsort(pool, n, sizeof(Preference), cmpByRoll);

    if (n == 1) {
        makeSoloUnit(&out[0], pool[0].rollNo, pool[0].choices);
        return 1;
    }

    /* every pair i<j, that is n choose 2 candidates */
    numCand = (long)n * (n - 1) / 2;
    cand = malloc(numCand * sizeof(Candidate));
    order = malloc(numCand * sizeof(int));
    paired = calloc(n, sizeof(int));
    if (!cand || !order || !paired) {
        fprintf(stderr, "out of memory scoring %ld candidates\n", numCand);
        free(cand); free(order); free(paired);
        return 0;
    }

    p = 0;
    for (i = 0; i < n; i++) {
        for (j = i + 1; j < n; j++) {
            cand[p].i = i;
            cand[p].j = j;
            /* score every possible pair once */
            cand[p].score = compatibilityScore(&pool[i], &pool[j]);
            p++;
        }
    }

    /* counting sort by score, highest first. Scores span only 0..16, so
       this is O(n) and stable: ties keep their (i, j) generation order. */
    memset(count, 0, sizeof(count));
    for (p = 0; p < numCand; p++)
        count[cand[p].score]++;

    /* start[s] is where score s begins in the sorted order, highest score first */
    start[MAX_SCORE] = 0;
    for (s = MAX_SCORE - 1; s >= 0; s--)
        start[s] = start[s + 1] + count[s + 1];

    for (p = 0; p < numCand; p++)
        order[start[cand[p].score]++] = (int)p;

    /* greedy: best candidate first, take it if both students are still free */
    for (p = 0; p < numCand; p++) {
        Candidate *c = &cand[order[p]];
        /* skip if either student was already taken by a better pair */
        if (paired[c->i] || paired[c->j])
            continue;
        paired[c->i] = paired[c->j] = 1;

        {
            Choice shared[NUM_CHOICES];
            sharedChoices(&pool[c->i], &pool[c->j], shared);
            makePairUnit(&out[made], pool[c->i].rollNo, pool[c->j].rollNo, shared);
            made++;
        }
    }

    for (i = 0; i < n; i++) {
        /* whoever is still free after the greedy pass has no partner */
        if (!paired[i]) {
            makeSoloUnit(&out[made], pool[i].rollNo, pool[i].choices);
            made++;
        }
    }

    free(cand);
    free(order);
    free(paired);
    return made;
}

/* Checks that a LOCKED student really has a valid partner. The partner's
   record must exist, also be LOCKED, and name this student back. If their
   choice lists differ a warning is printed and the first student's list is
   used. Returns 1 when the pair is sound, 0 when it is not. */
int lockedPairOk(const Preference *p, const Preference *q)
{
    int k;

    if (q == NULL) {
        fprintf(stderr, "%ld is LOCKED with %ld but that record does not exist\n",
                p->rollNo, p->roommateRoll);
        return 0;
    }
    if (q->status != STATUS_LOCKED) {
        fprintf(stderr, "%ld is LOCKED with %ld but %ld is %s\n",
                p->rollNo, q->rollNo, q->rollNo, statusName(q->status));
        return 0;
    }
    if (q->roommateRoll != p->rollNo) {
        fprintf(stderr, "%ld is LOCKED with %ld but %ld names %ld\n",
                p->rollNo, q->rollNo, q->rollNo, q->roommateRoll);
        return 0;
    }
    for (k = 0; k < NUM_CHOICES; k++) {
        if (!sameFloor(&p->choices[k], &q->choices[k])) {
            fprintf(stderr, "warning: %ld and %ld are locked but their lists differ, using %ld's\n",
                    p->rollNo, q->rollNo, p->rollNo);
            break;
        }
    }
    return 1;
}

/* Turns the closed registration list into units, the things the lottery and
   allocator work with. Locked pairs become one unit each. Everyone else,
   including students whose locked partner turned out to be invalid and
   students still PENDING at close, goes into the random pool and is paired
   by pairRandoms. Returns the number of units written to out. */
int buildUnits(Preference *prefs, int n, unsigned int seed, Unit *out)
{
    Preference *pool;
    int poolSize = 0;
    int nUnits = 0;
    int i, k;

    pool = malloc((n > 0 ? n : 1) * sizeof(Preference));
    if (!pool) {
        fprintf(stderr, "out of memory\n");
        return 0;
    }

    for (i = 0; i < n; i++) {
        Preference *p = &prefs[i];

        /* locked students bring their partner with them */
        if (p->status == STATUS_LOCKED) {
            const Preference *q = findInPrefs(prefs, n, p->roommateRoll);

            /* a broken lock falls back to the random pool instead of losing the student */
            if (!lockedPairOk(p, q)) {
                pool[poolSize] = *p;
                pool[poolSize].status = STATUS_RANDOM;
                pool[poolSize].roommateRoll = 0;
                poolSize++;
                continue;
            }

            /* the pair is created once, from the lower roll number */
            if (p->rollNo < q->rollNo) {
                makePairUnit(&out[nUnits], p->rollNo, q->rollNo, p->choices);
                nUnits++;
            }
        } else {
            if (p->status == STATUS_PENDING)
                fprintf(stderr, "%ld is still PENDING after close, treating as RANDOM\n",
                        p->rollNo);
            pool[poolSize] = *p;
            pool[poolSize].status = STATUS_RANDOM;
            pool[poolSize].roommateRoll = 0;
            poolSize++;
        }
    }

    /* now the random pool, appended after the locked units */
    k = pairRandoms(pool, poolSize, &out[nUnits]);
    nUnits += k;

    free(pool);

    (void)seed; /* Stage 3 handles lottery separately. */

    return nUnits;
}

/* Students who never registered at all are added at the end as units with
   no preferences, two to a unit in roll number order, with a solo unit for
   the odd one. They get whatever rooms are left. Returns the new unit count. */
int appendUnregistered(Unit *units, int nUnits, long *rolls, int nRolls)
{
    int i;

    if (nRolls <= 0)
        return nUnits;

    qsort(rolls, nRolls, sizeof(long), cmpLong);

    for (i = 0; i + 1 < nRolls && nUnits < MAX_UNITS; i += 2) {
        Unit *u = &units[nUnits++];
        memset(u, 0, sizeof(*u));
        u->rollA = rolls[i];
        u->rollB = rolls[i + 1];
        u->numChoices = 0;
        u->solo = 0;
    }
    if (i < nRolls && nUnits < MAX_UNITS) {
        Unit *u = &units[nUnits++];
        memset(u, 0, sizeof(*u));
        u->rollA = rolls[i];
        u->numChoices = 0;
        u->solo = 1;
    }
    return nUnits;
}

/* Finds the unit a student ended up in, by roll number. NULL if none. */
const Unit *findUnit(const Unit *units, int nUnits, long roll)
{
    int i;
    for (i = 0; i < nUnits; i++) {
        if (units[i].rollA == roll || units[i].rollB == roll)
            return &units[i];
    }
    return NULL;
}

/* Writes the registration list back out in the same file format, but with
   every random student now shown as LOCKED to the partner they were given
   and with the pair's shared choices. Solo students stay RANDOM with no
   partner. Returns 1 on success, 0 if the file could not be opened. */
int writePairedFile(const Preference *prefs, int n, const Unit *units, int nUnits,
                    const char *path)
{
    FILE *fp = fopen(path, "w");
    int i, k;

    if (fp == NULL) {
        fprintf(stderr, "cannot write %s\n", path);
        return 0;
    }

    for (i = 0; i < n; i++) {
        const Preference *p = &prefs[i];
        const Unit *u = findUnit(units, nUnits, p->rollNo);
        const Choice *choices = p->choices;
        const char *status = statusName(p->status);
        long partner = p->roommateRoll;
        char buf[HOSTEL_LEN + 4];

        /* if the student is in a unit, the unit's partner and choices override the original record */
        if (u != NULL) {
            choices = u->choices;
            if (u->solo) {
                status = "RANDOM";
                partner = 0;
            } else {
                status = "LOCKED";
                partner = (u->rollA == p->rollNo) ? u->rollB : u->rollA;
            }
        }

        fprintf(fp, "%ld|%s", p->rollNo, p->name);
        for (k = 0; k < NUM_CHOICES; k++) {
            formatChoice(&choices[k], buf);
            fprintf(fp, "|%s", buf);
        }
        fprintf(fp, "|%s|", status);
        if (partner != 0)
            fprintf(fp, "%ld", partner);
        fprintf(fp, "\n");
    }

    fclose(fp);
    return 1;
}

/* Prints the unit list as a table, one unit per line with both roll numbers
   (or "(solo)") and the choices. */
void printUnits(const Unit *units, int n)
{
    int i, k;
    char buf[HOSTEL_LEN + 4];

    printf("%-4s %-10s %-10s %s\n", "#", "rollA", "rollB", "choices");
    for (i = 0; i < n; i++) {
        const Unit *u = &units[i];
        printf("%-4d %-10ld ", i + 1, u->rollA);
        if (u->solo)
            printf("%-10s ", "(solo)");
        else
            printf("%-10ld ", u->rollB);
        if (u->numChoices == 0) {
            printf("none");
        } else {
            for (k = 0; k < u->numChoices; k++) {
                formatChoice(&u->choices[k], buf);
                printf("%s%s", k ? " " : "", buf);
            }
        }
        printf("\n");
    }
}
