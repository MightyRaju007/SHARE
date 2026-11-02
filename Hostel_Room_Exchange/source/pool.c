#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "commonHeadExchange.h"

/* large scratch arrays for the run, kept off the stack */
long int allRolls[MAX_STUDENTS];
long int activeRolls[MAX_STUDENTS];


/* free the choices and NULL the slots */
void clearPoolChoices(Student *s)
{
    int k;
    for (k = 0; k < NUM_CHOICES; k++) {
        free(s->poolchoices[k]);
        s->poolchoices[k] = NULL;
    }
}

/* copy the choices into fresh slots */
int storePoolChoices(Student *s, const PoolChoice *choices, int n)
{
    int k;
    clearPoolChoices(s);
    for (k = 0; k < n; k++) {
        PoolChoice *c = malloc(sizeof(PoolChoice));
        if (c == NULL) {
            clearPoolChoices(s);
            fprintf(stderr, "out of memory storing pool choices\n");
            return 0;
        }
        *c = choices[k];
        s->poolchoices[k] = c;
    }
    return 1;
}

/* join the pool with 1 to 3 ranked floors */
int enterPool(long int roll, const PoolChoice *choices, int numChoices)
{
    Student *s = getStudent(roll);
    const char *myHostel;
    int myFloor, k, m;
    long int mate;
    char text[NOTICE_LEN];

    if (s == NULL || s->room == NULL) {
        printf("roll %ld has no room in the allocation\n", roll);
        return 0;
    }
    if (s->commitmentRequest != 0) {
        printf("you have an open request (%d), withdraw it or wait for it to end\n",
               s->commitmentRequest);
        return 0;
    }
    if (s->poolState == IN_POOL) {
        printf("you are already in the pool\n");
        return 0;
    }
    if (s->poolState == WAITING_CONSENT) {
        printf("you are already waiting for your roommate's consent\n");
        return 0;
    }
    if (numChoices < 1 || numChoices > NUM_CHOICES) {
        printf("list 1 to %d floors\n", NUM_CHOICES);
        return 0;
    }

    myHostel = s->room->hostel;
    myFloor = s->room->floor;

    for (k = 0; k < numChoices; k++) {
        const PoolChoice *c = &choices[k];
        /* floor must exist */
        if (!floorExists(c->hostel, c->floor)) {
            printf("%s%d does not exist\n", c->hostel, c->floor);
            return 0;
        }
        /* must be an improvement, not the floor they are on */
        if (strcmp(c->hostel, myHostel) == 0 && c->floor == myFloor) {
            printf("%s%d is the floor you already live on\n", c->hostel, c->floor);
            return 0;
        }
        for (m = 0; m < k; m++) {
            if (sameFloor(c, &choices[m])) {
                printf("%s%d is listed twice\n", c->hostel, c->floor);
                return 0;
            }
        }
    }

    /* roommate must be free to answer */
    mate = roommateOf(roll);
    if (mate != 0) {
        Student *ms = getStudent(mate);
        if (ms != NULL && ms->commitmentRequest != 0) {
            printf("your roommate %s has an open request, try again when it has ended\n",
                   ms->name);
            return 0;
        }
    }

    if (!storePoolChoices(s, choices, numChoices))
        return 0;

    if (mate == 0) {
        /* solo room, nobody to ask */
        s->poolState = IN_POOL;
        printf("you are in the pool\n");
    } else {
        /* roommate has to agree first */
        Request *r = newRequest(REQ_POOL_CONSENT, roll);
        if (r == NULL) {
            clearPoolChoices(s);
            return 0;
        }
        addApprover(r, mate, APPROVER_PENDING);
        roomAddOpen(s->room->roomId, r->id);
        s->commitmentRequest = r->id;
        s->poolState = WAITING_CONSENT;

        snprintf(text, sizeof(text),
                 "%.30s wants to join the pool and may move out, log in to respond",
                 s->name);
        addNotice(mate, r->id, text);
        printf("consent request sent to your roommate %s, you join the pool once they accept\n",
               getStudent(mate) ? getStudent(mate)->name : "");
    }

    saveAll();
    return 1;
}

/* out before the run */
int leavePool(long int roll)
{
    Student *s = getStudent(roll);

    if (s == NULL) {
        printf("unknown roll %ld\n", roll);
        return 0;
    }
    if (s->poolState == NONE) {
        printf("you are not in the pool\n");
        return 0;
    }
    if (s->poolState == WAITING_CONSENT) {
        /* the consent request is still open; closing it resets the state */
        Request *r = getRequest(s->commitmentRequest);
        if (r != NULL)
            cancelRequest(r, "withdrawn by initiator");
    }
    s->poolState = NONE;
    clearPoolChoices(s);
    printf("you have left the pool\n");
    saveAll();
    return 1;
}

/* roommate pulls the student back out */
int revokeConsent(long int roommateRoll)
{
    long int roll = roommateOf(roommateRoll);
    Student *s = roll ? getStudent(roll) : NULL;
    Student *me = getStudent(roommateRoll);
    char text[NOTICE_LEN];

    if (me == NULL) {
        printf("unknown roll %ld\n", roommateRoll);
        return 0;
    }
    if (s == NULL || s->poolState == NONE) {
        printf("your roommate is not in the pool\n");
        return 0;
    }

    if (s->poolState == WAITING_CONSENT) {
        /* same as rejecting the open request */
        Request *r = getRequest(s->commitmentRequest);
        if (r != NULL)
            cancelRequest(r, "consent revoked by roommate");
    } else {
        s->poolState = NONE;
        clearPoolChoices(s);
        snprintf(text, sizeof(text), "%.40s withdrew consent, you are out of the pool", me->name);
        addNotice(roll, 0, text);
    }
    printf("%s has been removed from the pool\n", s->name);
    saveAll();
    return 1;
}


/* roommate said yes, student goes IN_POOL */
void consentAccepted(Request *r)
{
    Student *s;
    int rooms[2], nRooms, i;

    if (r == NULL || r->type != REQ_POOL_CONSENT || r->status != REQ_OPEN)
        return;

    s = getStudent(r->initiator);
    nRooms = requestRooms(r, rooms, 2);

    r->status = REQ_COMPLETED;
    for (i = 0; i < r->approverCount; i++) {
        Student *mate = getStudent(r->approvers[i].roll);
        r->approvers[i].state = APPROVER_ACCEPTED;
        /* consenting costs the roommate nothing, so they are released at once */
        if (mate != NULL && mate->commitmentRequest == r->id)
            mate->commitmentRequest = 0;
    }
    for (i = 0; i < nRooms; i++)
        roomRemoveOpen(rooms[i], r->id);

    if (s != NULL) {
        if (s->commitmentRequest == r->id)
            s->commitmentRequest = 0;
        if (s->poolState == WAITING_CONSENT)
            s->poolState = IN_POOL;
        addNotice(s->rollNo, r->id, "your roommate agreed, you are in the exchange pool");
    }
    saveAll();
}

/* roommate said no, or initiator withdrew */
void consentDeclined(Request *r, const char *reason)
{
    /* cancelRequest releases everyone and calls poolRequestClosed */
    cancelRequest(r, reason);
}

/* consent request ended without a yes */
void poolRequestClosed(Request *r)
{
    Student *s;
    char text[NOTICE_LEN];

    if (r == NULL || r->type != REQ_POOL_CONSENT)
        return;
    s = getStudent(r->initiator);
    if (s == NULL || s->poolState != WAITING_CONSENT)
        return;

    s->poolState = NONE;
    clearPoolChoices(s);
    if (s->commitmentRequest == r->id)
        s->commitmentRequest = 0;
    snprintf(text, sizeof(text), "your pool request ended: %.70s", r->reason[0] ? r->reason : "no consent");
    addNotice(s->rollNo, r->id, text);
}


void edgeListInit(EdgeList *list)
{
    list->items = NULL;
    list->n = 0;
    list->cap = 0;
}

void edgeListFree(EdgeList *list)
{
    free(list->items);
    edgeListInit(list);
}

/* grow by doubling */
int edgeListPush(EdgeList *list, Edge e)
{
    if (list->n == list->cap) {
        int cap = list->cap == 0 ? 64 : list->cap * 2;
        Edge *grown = realloc(list->items, cap * sizeof(Edge));
        if (grown == NULL) {
            fprintf(stderr, "out of memory for edges\n");
            return 0;
        }
        list->items = grown;
        list->cap = cap;
    }
    list->items[list->n++] = e;
    return 1;
}

int cmpRoll(const void *a, const void *b)
{
    long int x = *(const long int *)a;
    long int y = *(const long int *)b;
    return (x > y) - (x < y);
}

/* collect, cancel strays, sort, shuffle, stamp seed positions */
int prepareRun(unsigned int seed, long int *active, int maxActive)
{
    int nAll, n = 0, i, k, nIds;
    int *ids;
    char line[96];

    /* everyone who is IN_POOL and has a room */
    nAll = listStudents(allRolls, MAX_STUDENTS);
    for (i = 0; i < nAll && n < maxActive; i++) {
        Student *s = getStudent(allRolls[i]);
        if (s != NULL && s->poolState == IN_POOL && s->room != NULL)
            active[n++] = s->rollNo;
    }

    /* open requests on pool rooms are cancelled before trading */
    for (i = 0; i < n; i++) {
        Room *room = getStudent(active[i])->room;

        /* ids are copied out first because cancelRequest unlinks them */
        nIds = copyOpenIds(room, &ids);
        for (k = 0; k < nIds; k++) {
            Request *r = getRequest(ids[k]);
            if (r != NULL && r->status == REQ_OPEN)
                cancelRequest(r, "cancelled for the pool run");
        }
        free(ids);
    }

    /* sort first so the shuffle always starts from the same order */
    qsort(active, n, sizeof(long int), cmpRoll);
    shuffleRolls(active, n, seed);
    for (i = 0; i < n; i++)
        getStudent(active[i])->poolSeedPos = i;

    snprintf(line, sizeof(line), "%ld POOL_RUN seed %u students %d", now(), seed, n);
    appendLog(line);
    return n;
}

/* -1 if the student did not list it */
int rankOfFloor(const Student *s, int floorIdx)
{
    int k;
    for (k = 0; k < NUM_CHOICES && s->poolchoices[k] != NULL; k++)
        if (floorIndex(s->poolchoices[k]->hostel, s->poolchoices[k]->floor) == floorIdx)
            return k;
    return -1;
}

/* one edge per (student, listed floor) up to allowedRank */
int buildEdges(const long int *active, int nActive, int allowedRank, EdgeList *out)
{
    int i, k;

    out->n = 0;
    for (i = 0; i < nActive; i++) {
        const Student *s = getStudent(active[i]);
        Edge e;
        if (s == NULL || s->room == NULL)
            continue;
        e.fromFloor = floorIndexOfRoom(s->room->roomId);
        e.roll = s->rollNo;
        for (k = 0; k <= allowedRank && k < NUM_CHOICES && s->poolchoices[k] != NULL; k++) {
            e.toFloor = floorIndex(s->poolchoices[k]->hostel, s->poolchoices[k]->floor);
            e.rank = k;
            if (e.fromFloor >= 0 && e.toFloor >= 0)
                edgeListPush(out, e);
        }
    }
    return out->n;
}

/* search state: best[f][g] is the lowest rank of edge f -> g, -1 if none */
typedef struct {
    int best[MAX_FLOORS][MAX_FLOORS];
    int colour[MAX_FLOORS]; /* 0 white, 1 grey (on the path), 2 black (done) */
    int path[MAX_FLOORS];
    int top;
    int *cycle;
} Search;

/* depth-first step; returns the cycle length when it closes one */
int visitFloor(Search *st, int f)
{
    int targets[MAX_FLOORS], nT = 0, g, i, j, len;

    st->colour[f] = 1;
    st->path[st->top++] = f;

    /* targets sorted by rank, then floor index */
    for (g = 0; g < MAX_FLOORS; g++)
        if (st->best[f][g] >= 0)
            targets[nT++] = g;
    for (i = 1; i < nT; i++) {
        int t = targets[i];
        j = i - 1;
        while (j >= 0 && (st->best[f][targets[j]] > st->best[f][t]
                          || (st->best[f][targets[j]] == st->best[f][t] && targets[j] > t))) {
            targets[j + 1] = targets[j];
            j--;
        }
        targets[j + 1] = t;
    }

    for (i = 0; i < nT; i++) {
        g = targets[i];
        if (st->colour[g] == 1) {
            /* g is on the path: the ring is g ... f */
            for (j = 0; j < st->top; j++)
                if (st->path[j] == g)
                    break;
            len = st->top - j;
            memcpy(st->cycle, &st->path[j], len * sizeof(int));
            return len;
        }
        if (st->colour[g] == 0) {
            len = visitFloor(st, g);
            if (len > 0)
                return len;
        }
    }

    st->colour[f] = 2;
    st->top--;
    return 0;
}

/* colours: 0 white, 1 grey, 2 black */
int findCycle(const EdgeList *edges, int *cycle)
{
    Search st;
    int f, g, i, len;

    /* collapse edges to one best rank per (from, to) */
    for (f = 0; f < MAX_FLOORS; f++)
        for (g = 0; g < MAX_FLOORS; g++)
            st.best[f][g] = -1;
    for (i = 0; i < edges->n; i++) {
        const Edge *e = &edges->items[i];
        if (st.best[e->fromFloor][e->toFloor] < 0 || e->rank < st.best[e->fromFloor][e->toFloor])
            st.best[e->fromFloor][e->toFloor] = e->rank;
    }

    /* start from floors in sorted order so results repeat */
    memset(st.colour, 0, sizeof(st.colour));
    st.top = 0;
    st.cycle = cycle;

    for (f = 0; f < MAX_FLOORS; f++) {
        if (st.colour[f] == 0) {
            st.top = 0;
            len = visitFloor(&st, f);
            if (len > 0)
                return len;
        }
    }
    return 0;
}

/* one student per floor of the ring */
int chooseStudents(const int *cycle, int len, const long int *active, int nActive,
                   int allowedRank, Request *r, long int *chosen)
{
    int i, k, picked = 0;

    for (i = 0; i < len; i++) {
        int here = cycle[i];
        int next = cycle[(i + 1) % len];
        const Student *best = NULL;
        int bestRank = NUM_CHOICES;

        /* lowest rank for the next floor wins, then seeded position */
        for (k = 0; k < nActive; k++) {
            const Student *s = getStudent(active[k]);
            int rank;
            if (s == NULL || s->room == NULL || floorIndexOfRoom(s->room->roomId) != here)
                continue;
            rank = rankOfFloor(s, next);
            if (rank < 0 || rank > allowedRank)
                continue;
            if (best == NULL || rank < bestRank
                || (rank == bestRank && s->poolSeedPos < best->poolSeedPos)) {
                best = s;
                bestRank = rank;
            }
        }
        if (best == NULL)
            return picked; /* should not happen: the edge came from someone here */
        chosen[picked++] = best->rollNo;
    }

    /* each one moves into the next chosen student's room */
    for (i = 0; i < picked; i++) {
        const Student *from = getStudent(chosen[i]);
        const Student *to = getStudent(chosen[(i + 1) % picked]);
        addMove(r, from->rollNo, from->room->roomId, to->room->roomId);
    }
    return picked;
}

/* remove and keep order */
void dropFromActive(long int *active, int *nActive, long int roll)
{
    int i, k;
    for (i = 0; i < *nActive; i++) {
        if (active[i] == roll) {
            for (k = i; k + 1 < *nActive; k++)
                active[k] = active[k + 1];
            (*nActive)--;
            return;
        }
    }
}

void printRing(const Request *r)
{
    int i;
    char from[16], to[16];
    printf("ring applied (request %d):\n", r->id);
    for (i = 0; i < r->moveCount; i++) {
        const Move *m = &r->moves[i];
        const Student *s = getStudent(m->roll);
        formatRoom(m->fromRoom, from);
        formatRoom(m->toRoom, to);
        printf("  %ld %-20s %s -> %s\n", m->roll, s ? s->name : "", from, to);
    }
}

/* the admin run */
int runPool(unsigned int seed)
{
    int n, rank, len, i, picked, trades = 0, moved = 0;
    int cycle[MAX_FLOORS];
    long int chosen[MAX_FLOORS];
    EdgeList edges;
    char text[NOTICE_LEN];

    n = prepareRun(seed, activeRolls, MAX_STUDENTS);
    printf("pool run, seed %u, %d student(s) in the pool\n", seed, n);

    /* nothing to trade with 0 or 1 */
    if (n < 2) {
        printf("nothing to trade\n");
        for (i = 0; i < n; i++) {
            Student *s = getStudent(activeRolls[i]);
            s->poolState = NONE;
            clearPoolChoices(s);
            addNotice(s->rollNo, 0, "no match found in the pool, you stay in your room");
        }
        saveAll();
        return 0;
    }

    edgeListInit(&edges);

    /* first choices before second choices */
    for (rank = 0; rank < NUM_CHOICES; rank++) {
        for (;;) {
            Request *r;

            buildEdges(activeRolls, n, rank, &edges);
            len = findCycle(&edges, cycle);
            if (len == 0)
                break;

            /* each cycle becomes one request and goes through the executor */
            r = newRequest(REQ_POOL_TRADE, 0);
            if (r == NULL)
                break;
            picked = chooseStudents(cycle, len, activeRolls, n, rank, r, chosen);
            if (picked != len) {
                cancelRequest(r, "could not fill the ring");
                break;
            }

            if (execute(r)) {
                trades++;
                moved += picked;
                printRing(r);
            } else {
                /* ring failed, skip these students */
                snprintf(text, sizeof(text), "pool trade could not be applied: %.60s", r->reason);
                for (i = 0; i < picked; i++)
                    addNotice(chosen[i], r->id, text);
            }

            for (i = 0; i < picked; i++) {
                Student *s = getStudent(chosen[i]);
                s->poolState = NONE;
                clearPoolChoices(s);
                dropFromActive(activeRolls, &n, chosen[i]);
            }
        }
    }

    /* leftovers stay where they are */
    for (i = 0; i < n; i++) {
        Student *s = getStudent(activeRolls[i]);
        s->poolState = NONE;
        clearPoolChoices(s);
        addNotice(s->rollNo, 0, "no match found in the pool, you stay in your room");
    }

    edgeListFree(&edges);
    saveAll();
    printf("done: %d ring(s), %d student(s) moved, %d without a match\n", trades, moved, n);
    return trades;
}
