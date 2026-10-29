#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "commonHeaderFile.h"

#define MAX_SCORE 16
#define NO_RANK 3

typedef struct {
    int i;
    int j;
    int score;
} Candidate;

/* rank (0,1,2) of a floor in a student's list, NO_RANK if absent */
int rankOf(const Preference *p, const Choice *c)
{
    int r;
    for (r = 0; r < NUM_CHOICES; r++) {
        if (sameFloor(&p->choices[r], c))
            return r;
    }
    return NO_RANK;
}

int cmpByRoll(const void *a, const void *b)
{
    long x = ((const Preference *)a)->rollNo;
    long y = ((const Preference *)b)->rollNo;
    return (x > y) - (x < y);
}

int cmpLong(const void *a, const void *b)
{
    long x = *(const long *)a;
    long y = *(const long *)b;
    return (x > y) - (x < y);
}

const Preference *findInPrefs(const Preference *prefs, int n, long roll)
{
    int k;
    for (k = 0; k < n; k++) {
        if (prefs[k].rollNo == roll)
            return &prefs[k];
    }
    return NULL;
}

void makePairUnit(Unit *u, long rollA, long rollB, const Choice choices[NUM_CHOICES])
{
    memset(u, 0, sizeof(*u));
    u->rollA = rollA;
    u->rollB = rollB;
    memcpy(u->choices, choices, sizeof(u->choices));
    u->numChoices = NUM_CHOICES;
    u->solo = 0;
}

void makeSoloUnit(Unit *u, long rollA, const Choice choices[NUM_CHOICES])
{
    memset(u, 0, sizeof(*u));
    u->rollA = rollA;
    u->rollB = 0;
    memcpy(u->choices, choices, sizeof(u->choices));
    u->numChoices = NUM_CHOICES;
    u->solo = 1;
}

int compatibilityScore(const Preference *a, const Preference *b)
{
    int i, j;
    int bestFloorBonus = -1;
    int bestHostelBonus = -1;

    for (i = 0; i < NUM_CHOICES; i++) {
        for (j = 0; j < NUM_CHOICES; j++) {
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

    if (bestFloorBonus >= 0)
        return 10 + bestFloorBonus;
    if (bestHostelBonus >= 0)
        return bestHostelBonus;
    return 0;
}

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
            cand[p].score = compatibilityScore(&pool[i], &pool[j]);
            p++;
        }
    }

    /* counting sort by score, highest first. Scores span only 0..16, so
       this is O(n) and stable: ties keep their (i, j) generation order. */
    memset(count, 0, sizeof(count));
    for (p = 0; p < numCand; p++)
        count[cand[p].score]++;

    start[MAX_SCORE] = 0;
    for (s = MAX_SCORE - 1; s >= 0; s--)
        start[s] = start[s + 1] + count[s + 1];

    for (p = 0; p < numCand; p++)
        order[start[cand[p].score]++] = (int)p;

    /* greedy: best candidate first, take it if both students are still free */
    for (p = 0; p < numCand; p++) {
        Candidate *c = &cand[order[p]];
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

/* both records must exist, be LOCKED and name each other */
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

        if (p->status == STATUS_LOCKED) {
            const Preference *q = findInPrefs(prefs, n, p->roommateRoll);

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

    k = pairRandoms(pool, poolSize, &out[nUnits]);
    nUnits += k;

    free(pool);

    (void)seed; /* Stage 3 handles lottery separately. */

    return nUnits;
}

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

const Unit *findUnit(const Unit *units, int nUnits, long roll)
{
    int i;
    for (i = 0; i < nUnits; i++) {
        if (units[i].rollA == roll || units[i].rollB == roll)
            return &units[i];
    }
    return NULL;
}

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
