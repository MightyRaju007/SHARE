#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "commonHeadExchange.h"

/* Functions implemented by Person 1. */
int execute(Request *r);
void consentAccepted(Request *r);
void consentDeclined(Request *r, const char *reason);

/* Find whether a student is an approver on a request. */
int isApprover(const Request *r, long int roll)
{
    int i;
    for (i = 0; i < r->approverCount; i++) {
        if (r->approvers[i].roll == roll)
            return i;
    }
    return -1;
}

/* Create a mutual room exchange request. */
void createExchangeRoom(long int xRoll, int targetRoomId)
{
    Student *s = getStudent(xRoll);
    Room *targetRoom, *presentRoom;
    Request *r;
    long int affected[MAX_APPROVERS];
    int affCount = 0, i;

    if (s == NULL || s->room == NULL) {
        printf("Student with %ld rollno does not exist or has no room.\n", xRoll);
        return;
    }
    targetRoom = getRoom(targetRoomId);
    presentRoom = s->room;
    if (targetRoom == NULL) {
        printf("Target room %d does not exist.\n", targetRoomId);
        return;
    }
    if (presentRoom->roomId == targetRoomId) {
        printf("Target and present rooms cannot be the same.\n");
        return;
    }
    if (roomOccupantCount(targetRoom) == 0) {
        printf("Target room %d is empty.\n", targetRoomId);
        return;
    }

    for (i = 0; i < BEDS_PER_ROOM; i++) {
        if (presentRoom->occupantsRollNo[i] != 0)
            affected[affCount++] = presentRoom->occupantsRollNo[i];
        if (targetRoom->occupantsRollNo[i] != 0)
            affected[affCount++] = targetRoom->occupantsRollNo[i];
    }
    for (i = 0; i < affCount; i++) {
        Student *a = getStudent(affected[i]);
        if (a == NULL || a->commitmentRequest != 0 || a->poolState != NONE) {
            printf("Student with %ld rollno is already committed or in the pool.\n", affected[i]);
            return;
        }
    }

    r = newRequest(REQ_ROOM_EXCHANGE, xRoll);
    if (r == NULL) {
        printf("Requests box is full or memory allocation failed.\n");
        return;
    }
    for (i = 0; i < BEDS_PER_ROOM; i++) {
        long int roll = presentRoom->occupantsRollNo[i];
        if (roll != 0 && !addMove(r, roll, presentRoom->roomId, targetRoomId))
            break;
    }
    for (i = 0; i < BEDS_PER_ROOM; i++) {
        long int roll = targetRoom->occupantsRollNo[i];
        if (roll != 0 && !addMove(r, roll, targetRoomId, presentRoom->roomId))
            break;
    }
    if (r->moveCount != affCount) {
        cancelRequest(r, "could not build room exchange request");
        return;
    }
    for (i = 0; i < affCount; i++) {
        ApproverState state = affected[i] == xRoll ? APPROVER_ACCEPTED : APPROVER_PENDING;
        if (!addApprover(r, affected[i], state)) {
            cancelRequest(r, "could not add request approvers");
            return;
        }
        if (affected[i] != xRoll)
            addNotice(affected[i], r->id, "Room exchange request waiting. Log in to respond.");
    }
    roomAddOpen(presentRoom->roomId, r->id);
    roomAddOpen(targetRoomId, r->id);
    s->commitmentRequest = r->id;
    saveAll();
    printf("Room exchange request %d created.\n", r->id);
}

/* Create a roommate swap request between two different rooms. */
void createRoomateSwap(long int xRoll, long int yRoll)
{
    Student *x, *y;
    Room *xRoom, *yRoom;
    Request *r;
    long int affected[MAX_APPROVERS];
    int affCount = 0, i;

    if (xRoll == yRoll) {
        printf("Invalid swap: both students cannot be the same.\n");
        return;
    }
    x = getStudent(xRoll);
    y = getStudent(yRoll);
    if (x == NULL || y == NULL) {
        printf("One or both student roll numbers do not exist.\n");
        return;
    }
    if (x->room == NULL || y->room == NULL) {
        printf("Both students must have an allocated room.\n");
        return;
    }
    xRoom = x->room;
    yRoom = y->room;
    if (xRoom->roomId == yRoom->roomId) {
        printf("Both students are in the same room. They cannot swap.\n");
        return;
    }

    for (i = 0; i < BEDS_PER_ROOM; i++) {
        if (xRoom->occupantsRollNo[i] != 0)
            affected[affCount++] = xRoom->occupantsRollNo[i];
        if (yRoom->occupantsRollNo[i] != 0)
            affected[affCount++] = yRoom->occupantsRollNo[i];
    }
    for (i = 0; i < affCount; i++) {
        Student *a = getStudent(affected[i]);
        if (a == NULL || a->commitmentRequest != 0 || a->poolState != NONE) {
            printf("Student %ld is already committed or in the pool.\n", affected[i]);
            return;
        }
    }

    r = newRequest(REQ_ROOMMATE_SWAP, xRoll);
    if (r == NULL) {
        printf("Requests box is full or memory allocation failed.\n");
        return;
    }
    if (!addMove(r, xRoll, xRoom->roomId, yRoom->roomId) ||
        !addMove(r, yRoll, yRoom->roomId, xRoom->roomId)) {
        cancelRequest(r, "could not build roommate swap request");
        return;
    }
    for (i = 0; i < affCount; i++) {
        ApproverState state = affected[i] == xRoll ? APPROVER_ACCEPTED : APPROVER_PENDING;
        if (!addApprover(r, affected[i], state)) {
            cancelRequest(r, "could not add request approvers");
            return;
        }
        if (affected[i] != xRoll)
            addNotice(affected[i], r->id, "Roommate swap request waiting. Log in to respond.");
    }
    x->commitmentRequest = r->id;
    roomAddOpen(xRoom->roomId, r->id);
    roomAddOpen(yRoom->roomId, r->id);
    saveAll();
    printf("Roommate swap request %d created.\n", r->id);
}

/* Remove a request from every room it touches. */
void unlinkRequestRooms(Request *r)
{
    int rooms[2 * MAX_MOVES];
    int nRooms = requestRooms(r, rooms, 2 * MAX_MOVES);
    int i;
    for (i = 0; i < nRooms; i++)
        roomRemoveOpen(rooms[i], r->id);
}

/* Expire one request and release its students. */
void expireRequest(Request *r)
{
    int i;
    if (r == NULL || r->status != REQ_OPEN)
        return;
    r->status = REQ_EXPIRED;
    snprintf(r->reason, sizeof(r->reason), "Request expired after two days.");
    for (i = 0; i < r->approverCount; i++) {
        Student *s = getStudent(r->approvers[i].roll);
        if (s != NULL && s->commitmentRequest == r->id)
            s->commitmentRequest = 0;
        addNotice(r->approvers[i].roll, r->id, r->reason);
    }
    for (i = 0; i < r->moveCount; i++) {
        Student *s = getStudent(r->moves[i].roll);
        if (s != NULL && s->commitmentRequest == r->id)
            s->commitmentRequest = 0;
    }
    unlinkRequestRooms(r);
    poolRequestClosed(r);
    saveAll();
}

/* Check all requests when a student logs in; expiry is not timer-based. */
void expireStaleRequest(long int roll)
{
    int id;
    (void)roll;
    for (id = 1; ; id++) {
        Request *r = getRequest(id);
        if (r == NULL)
            break;
        if (r->status == REQ_OPEN && now() > r->expiresAt)
            expireRequest(r);
    }
}

/* Respond to a request. answer must be APPROVER_ACCEPTED or APPROVER_REJECTED. */
void respond(int reqId, long int roll, ApproverState answer)
{
    Request *r = getRequest(reqId);
    Student *s = getStudent(roll);
    int k, i, allAccepted = 1;

    if (r == NULL || r->status != REQ_OPEN) {
        printf("Request %d is not open or does not exist.\n", reqId);
        return;
    }
    if (s == NULL) {
        printf("Student with rollno %ld does not exist.\n", roll);
        return;
    }
    k = isApprover(r, roll);
    if (k < 0) {
        printf("Student %ld is not an approver of request %d.\n", roll, reqId);
        return;
    }
    if (now() > r->expiresAt) {
        expireRequest(r);
        printf("Request %d has expired.\n", reqId);
        return;
    }
    if (answer == APPROVER_REJECTED) {
        snprintf(r->reason, sizeof(r->reason), "Rejected by student %ld.", roll);
        if (r->type == REQ_POOL_CONSENT)
            consentDeclined(r, r->reason);
        else
            cancelRequest(r, r->reason);
        saveAll();
        return;
    }
    if (answer != APPROVER_ACCEPTED) {
        printf("Invalid response.\n");
        return;
    }
    if (s->commitmentRequest != 0 && s->commitmentRequest != reqId) {
        printf("Student is committed to another request.\n");
        return;
    }

    s->commitmentRequest = reqId;
    r->approvers[k].state = APPROVER_ACCEPTED;
    if (r->type == REQ_POOL_CONSENT) {
        consentAccepted(r);
        saveAll();
        return;
    }
    for (i = 0; i < r->approverCount; i++) {
        if (r->approvers[i].state != APPROVER_ACCEPTED) {
            allAccepted = 0;
            break;
        }
    }
    if (allAccepted)
        execute(r);
    saveAll();
}

/* Cancel an open request only when its initiator withdraws it. */
void withdraw(long int roll)
{
    Student *s = getStudent(roll);
    Request *r;
    if (s == NULL || s->commitmentRequest == 0)
        return;
    r = getRequest(s->commitmentRequest);
    if (r == NULL || r->status != REQ_OPEN)
        return;
    if (r->initiator != roll) {
        printf("Only the initiator can withdraw request %d.\n", r->id);
        return;
    }
    cancelRequest(r, "Withdrawn by initiator.");
    saveAll();
}

void printHostelName(int roomId)
{
    const char *hostel = roomHostel(roomId);
    if (hostel == NULL)
        printf("INVALID RoomId");
    else if (strcmp(hostel, "AA") == 0)
        printf("Aquamarine A");
    else if (strcmp(hostel, "AB") == 0)
        printf("Aquamarine B");
    else if (strcmp(hostel, "GC") == 0)
        printf("Garnet C");
    else
        printf("%s", hostel);
}

int printRoomNo(int roomId)
{
    return roomNumber(roomId);
}

/* Login: show notices and pending approvals for this student. */
void login(long int roll)
{
    Student *s = getStudent(roll);
    int i, k = 0, id;
    if (s == NULL) {
        printf("Student with rollNo %ld does not exist.\n", roll);
        return;
    }
    printf("---- ENTER DETAILS ----\n");
    if (s->room != NULL)
        printf("Roll no : %ld Room no : %d\n", s->rollNo, s->room->roomNo);
    else
        printf("Roll no : %ld Room : not allocated\n", s->rollNo);

    expireStaleRequest(roll);

    for (i = 0; i < s->noticeCount; i++) {
        if (s->notices[i] != NULL && !s->notices[i]->seen) {
            printf("---- NOTICE BOX ----\n%s\n", s->notices[i]->info);
            s->notices[i]->seen = true;
            k = 1;
        }
    }
    if (!k)
        printf("NOTICE Inbox is EMPTY.\n");

    for (id = 1; ; id++) {
        Request *r = getRequest(id);
        int a;
        if (r == NULL)
            break;
        if (r->status != REQ_OPEN)
            continue;
        a = isApprover(r, roll);
        if (a < 0 || r->approvers[a].state != APPROVER_PENDING)
            continue;

        printf("RequestId : %d\n", r->id);
        printf("Request's initiator rollNo : %ld\n", r->initiator);
        printf("Request type : %s\n", requestTypeName(r->type));
        if (r->moveCount > 0) {
            printf("From Hostel ");
            printHostelName(r->moves[0].fromRoom);
            printf(" %d to Hostel ", printRoomNo(r->moves[0].fromRoom));
            printHostelName(r->moves[0].toRoom);
            printf(" %d\n", printRoomNo(r->moves[0].toRoom));
        }
        printf("Remaining Time : %ld seconds.\n", r->expiresAt - now());
        printf("--- Choice Menu ---\n1.ACCEPT\n2.REJECT\n3.SKIP FOR LATER\n");
        {
            int ch;
            if (scanf("%d", &ch) != 1) {
                printf("Invalid input.\n");
                return;
            }
            if (ch == 1)
                respond(r->id, roll, APPROVER_ACCEPTED);
            else if (ch == 2)
                respond(r->id, roll, APPROVER_REJECTED);
            else if (ch != 3)
                printf("INVALID Choice, SKIPPING FOR NOW.\n");
        }
    }
    saveAll();
}
