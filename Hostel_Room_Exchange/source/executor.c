#include <stdio.h>
#include <string.h>
#include "commonHeadExchange.h"
#include <stdlib.h>

/* a request touches at most 2 rooms per move */
/* largest number of distinct rooms one request can involve */
#define MAX_TOUCHED (2 * MAX_MOVES)

/* Copies the ids of every open request on a room into a new array and
   returns how many there are. The array is malloc'd and the caller must free
   it. This exists because cancelling a request removes it from the room's
   list, so you cannot cancel while walking the list itself. */
int copyOpenIds(const Room *room, int **out)
{
    const OpenRequestNode *n;
    int count = roomOpenCount(room), k = 0;

    *out = NULL;
    if (count == 0)
        return 0;
    *out = malloc(count * sizeof(int));
    if (*out == NULL) {
        fprintf(stderr, "out of memory copying open request ids\n");
        return 0;
    }
    for (n = room->openRequests; n != NULL; n = n->next)
        (*out)[k++] = n->requestId;
    return count;
}

/* Used whenever execute() has to give up. Prints the reason, cancels the
   request (which releases and notifies everyone) and returns 0 so the caller
   can just "return failRequest(...)". */
int failRequest(Request *r, const char *reason)
{
    printf("request %d could not be applied: %s\n", r->id, reason);
    cancelRequest(r, reason);
    return 0;
}

/* Takes a student out of a room by setting their bed to 0. Beds are not
   shifted, so the empty bed can be at either index. */
void roomRemove(Room *room, long int roll)
{
    int i;
    for (i = 0; i < BEDS_PER_ROOM; i++)
        if (room->occupantsRollNo[i] == roll)
            room->occupantsRollNo[i] = 0;
}

/* Puts a student into the first empty bed of a room. execute() has already
   checked there is space, so this cannot fail. */
void roomAdd(Room *room, long int roll)
{
    int i;
    for (i = 0; i < BEDS_PER_ROOM; i++) {
        if (room->occupantsRollNo[i] == 0) {
            room->occupantsRollNo[i] = roll;
            return;
        }
    }
}

/* Writes one line to the audit log for a completed request: the time, the
   request type and id, then each move as "roll from>to". Pieces that would
   not fit in the line buffer are dropped rather than overflowing it. */
void logRequest(const Request *r)
{
    char line[512], from[16], to[16], piece[64];
    int i;

    snprintf(line, sizeof(line), "%ld %s request %d", now(), requestTypeName(r->type), r->id);
    for (i = 0; i < r->moveCount; i++) {
        const Move *m = &r->moves[i];
        formatRoom(m->fromRoom, from);
        formatRoom(m->toRoom, to);
        snprintf(piece, sizeof(piece), " %ld %s>%s", m->roll, from, to);
        if (strlen(line) + strlen(piece) < sizeof(line))
            strcat(line, piece);
    }
    appendLog(line);
}

/* After a request is applied, releases every person involved from their
   commitment and sends each one a notice. Movers are told their new room.
   Approvers who did not move (roommates in a swap) just get "exchange done".
   A person can be both a mover and an approver, so the told[] list makes
   sure nobody gets two notices. */
void notifyDone(const Request *r)
{
    long int told[MAX_MOVES + MAX_APPROVERS];
    int nTold = 0, i, k, dup;
    char text[NOTICE_LEN], room[16];

    for (i = 0; i < r->moveCount; i++) {
        const Move *m = &r->moves[i];
        Student *s = getStudent(m->roll);
        if (s != NULL && s->commitmentRequest == r->id)
            s->commitmentRequest = 0;
        formatRoom(m->toRoom, room);
        snprintf(text, sizeof(text), "exchange done, you are now in %s", room);
        addNotice(m->roll, r->id, text);
        told[nTold++] = m->roll;
    }

    for (i = 0; i < r->approverCount; i++) {
        const Approver *a = &r->approvers[i];
        Student *s = getStudent(a->roll);
        if (s != NULL && s->commitmentRequest == r->id)
            s->commitmentRequest = 0;
        dup = 0;
        for (k = 0; k < nTold; k++)
            if (told[k] == a->roll)
                dup = 1;
        if (!dup)
            addNotice(a->roll, r->id, "exchange done");
    }
}

/* The one place in the program where students actually change rooms. Every
   feature (room exchange, roommate swap, pool ring) ends up here with a list
   of moves. The request is applied completely or not at all: all checks run
   first and nothing is touched until every one of them passes. Returns 1 on
   success. On failure the request is cancelled with a reason and 0 is
   returned. */
int execute(Request *r)
{
    int rooms[MAX_TOUCHED], nRooms, i, k;
    char why[REASON_LEN], roomText[16];

    if (r == NULL)
        return 0;
    /* a request that already ended cannot be applied again */
    if (r->status != REQ_OPEN)
        return failRequest(r, "request is not open");

    /* everyone must have agreed (a pool trade has no approvers) */
    for (i = 0; i < r->approverCount; i++) {
        if (r->approvers[i].state != APPROVER_ACCEPTED) {
            snprintf(why, sizeof(why), "%ld has not agreed", r->approvers[i].roll);
            return failRequest(r, why);
        }
    }

    /* the distinct rooms this request touches */
    nRooms = requestRooms(r, rooms, MAX_TOUCHED);
    if (r->moveCount == 0 || nRooms == 0)
        return failRequest(r, "nothing to move");

    /* step 1: stale and overflow checks, nothing changes yet */
    for (i = 0; i < r->moveCount; i++) {
        const Move *m = &r->moves[i];
        Student *s = getStudent(m->roll);
        Room *from = getRoom(m->fromRoom);
        Room *to = getRoom(m->toRoom);

        if (s == NULL) {
            snprintf(why, sizeof(why), "unknown student %ld", m->roll);
            return failRequest(r, why);
        }
        if (from == NULL || to == NULL) {
            snprintf(why, sizeof(why), "unknown room in move for %ld", m->roll);
            return failRequest(r, why);
        }
        /* stale if the student is no longer where the move says */
        /* both the student record and the room's bed list must agree */
        if (s->room != from || !roomHasStudent(from, m->roll)) {
            snprintf(why, sizeof(why), "stale: %ld has moved", m->roll);
            return failRequest(r, why);
        }
    }

    /* overflow check per touched room */
    for (i = 0; i < nRooms; i++) {
        Room *room = getRoom(rooms[i]);
        int leavers = 0, joiners = 0;
        for (k = 0; k < r->moveCount; k++) {
            if (r->moves[k].fromRoom == rooms[i]) leavers++;
            if (r->moves[k].toRoom == rooms[i]) joiners++;
        }
        /* occupants after the request must still fit in the beds */
        if (roomOccupantCount(room) - leavers + joiners > BEDS_PER_ROOM) {
            formatRoom(rooms[i], roomText);
            snprintf(why, sizeof(why), "room %s would overflow", roomText);
            return failRequest(r, why);
        }
    }

    /* step 2: apply the moves in two passes. Everyone leaves first and
       enters second, so a ring of 3 or more rooms never has a room sitting
       at 3 people part way through. */
    for (i = 0; i < r->moveCount; i++)
        roomRemove(getRoom(r->moves[i].fromRoom), r->moves[i].roll);

    /* pass 2: into the new rooms, and update each student's record */
    for (i = 0; i < r->moveCount; i++) {
        Room *to = getRoom(r->moves[i].toRoom);
        Student *s = getStudent(r->moves[i].roll);
        roomAdd(to, r->moves[i].roll);
        s->room = to;
    }

    /* step 3: the request is done, now tidy up around it. Our own id comes
       off the rooms first, otherwise cancelConflicts would cancel us. */
    r->status = REQ_COMPLETED;
    for (i = 0; i < nRooms; i++)
        roomRemoveOpen(rooms[i], r->id);

    /* release and inform the people, cancel rival requests, log, save */
    notifyDone(r);
    cancelConflicts(rooms, nRooms);
    logRequest(r);
    saveAll();
    return 1;
}

/* Called after a request is applied, for every room it touched. Any other
   request still open on those rooms is now based on a situation that no
   longer exists, so it is cancelled and its people are told. This is how two
   requests that wanted the same room are settled: the first one fully
   accepted goes through, the other one is dropped. */
void cancelConflicts(const int *rooms, int nRooms)
{
    int i, k, n;

    for (i = 0; i < nRooms; i++) {
        Room *room = getRoom(rooms[i]);
        int *ids;

        if (room == NULL)
            continue;

        /* copy ids out first, cancelRequest unlinks nodes */
        n = copyOpenIds(room, &ids);
        for (k = 0; k < n; k++) {
            Request *other = getRequest(ids[k]);
            if (other != NULL && other->status == REQ_OPEN)
                cancelRequest(other, "room changed by another exchange");
        }
        free(ids);
    }
}
