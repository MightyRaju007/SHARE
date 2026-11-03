#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "commonHeadExchange.h"

// Functions implemented by Person 1.
int execute(Request *r);
void consentAccepted(Request *r);
void consentDeclined(Request *r, const char *reason);

// Find whether a student is an approver on a request.
int isApprover(const Request *r, long int roll)
{
    int i;
    // Unlock each approver and add a notice with the reason.
    for (i = 0; i < r->approverCount; i++) {
        if (r->approvers[i].roll == roll)
            return i;
    }
    // Not found: this student is not an approver.
    return -1;
}

// Create a mutual room exchange request (function used for exchange of room).
void createExchangeRoom(long int xRoll, int targetRoomId)
{
    // Get student and check whether a student with this rollno exists or not.
    Student *s = getStudent(xRoll);
    // Local variables: the two rooms, the new request and the list of affected students.
    Room *targetRoom, *presentRoom;
    Request *r;
    long int affected[MAX_APPROVERS];
    int affCount = 0, i;

    // The student must exist and must already have a room.
    if (s == NULL || s->room == NULL) {
        printf("Student with %ld rollno does not exist or has no room.\n", xRoll);
        return;
    }
    // Get the target room and check that it exists.
    targetRoom = getRoom(targetRoomId);
    presentRoom = s->room;
    if (targetRoom == NULL) {
        printf("Target room %d does not exist.\n", targetRoomId);
        return;
    }
    // A student cannot swap with their present room itself.
    if (presentRoom->roomId == targetRoomId) {
        printf("Target and present rooms cannot be the same.\n");
        return;
    }
    // Target room is empty; a student cannot exchange with a vacant room.
    if (roomOccupantCount(targetRoom) == 0) {
        printf("Target room %d is empty.\n", targetRoomId);
        return;
    }

    // All affected students in both rooms: store students of the present room and the target room.
    for (i = 0; i < BEDS_PER_ROOM; i++) {
        if (presentRoom->occupantsRollNo[i] != 0)
            affected[affCount++] = presentRoom->occupantsRollNo[i];
        if (targetRoom->occupantsRollNo[i] != 0)
            affected[affCount++] = targetRoom->occupantsRollNo[i];
    }
    // Check that all affected students are free: not committed and not involved in the pool.
    for (i = 0; i < affCount; i++) {
        Student *a = getStudent(affected[i]);
        if (a == NULL || a->commitmentRequest != 0 || a->poolState != NONE) {
            printf("Student with %ld rollno is already committed or in the pool.\n", affected[i]);
            return;
        }
    }

    // Build the request: ID, type, initiator, timestamps, status OPEN.
    // Also checks that there is space in the requests box.
    r = newRequest(REQ_ROOM_EXCHANGE, xRoll);
    if (r == NULL) {
        printf("Requests box is full or memory allocation failed.\n");
        return;
    }
    // Fill the moves of the request: present rooms occupants go to the target room.
    for (i = 0; i < BEDS_PER_ROOM; i++) {
        long int roll = presentRoom->occupantsRollNo[i];
        if (roll != 0 && !addMove(r, roll, presentRoom->roomId, targetRoomId))
            break;
    }
    // Target rooms occupants go to the present room.
    for (i = 0; i < BEDS_PER_ROOM; i++) {
        long int roll = targetRoom->occupantsRollNo[i];
        if (roll != 0 && !addMove(r, roll, targetRoomId, presentRoom->roomId))
            break;
    }
    // Every affected student must have a move, otherwise the request is incomplete.
    if (r->moveCount != affCount) {
        cancelRequest(r, "could not build room exchange request");
        return;
    }
    // Fill the approvers of the request.
    // X is ACCEPTED, and everyone else is PENDING and gets a notice.
    for (i = 0; i < affCount; i++) {
        ApproverState state = affected[i] == xRoll ? APPROVER_ACCEPTED : APPROVER_PENDING;
        if (!addApprover(r, affected[i], state)) {
            cancelRequest(r, "could not add request approvers");
            return;
        }
        if (affected[i] != xRoll)
            addNotice(affected[i], r->id, "Room exchange request waiting. Log in to respond.");
    }
    // Set X's commitmentRequest and add the request to both rooms lists.
    roomAddOpen(presentRoom->roomId, r->id);
    roomAddOpen(targetRoomId, r->id);
    s->commitmentRequest = r->id;
    // Save everything and tell the user the request was created.
    saveAll();
    printf("Room exchange request %d created.\n", r->id);
}

// Create a roommate swap request between two different rooms (function to swap roommates).
void createRoomateSwap(long int xRoll, long int yRoll)
{
    Student *x, *y;
    Room *xRoom, *yRoom;
    Request *r;
    long int affected[MAX_APPROVERS];
    int affCount = 0, i;

    // X is not Y.
    if (xRoll == yRoll) {
        printf("Invalid swap: both students cannot be the same.\n");
        return;
    }
    // X and Y must exist.
    x = getStudent(xRoll);
    y = getStudent(yRoll);
    if (x == NULL || y == NULL) {
        printf("One or both student roll numbers do not exist.\n");
        return;
    }
    // X and Y must both have a room.
    if (x->room == NULL || y->room == NULL) {
        printf("Both students must have an allocated room.\n");
        return;
    }
    // Get the rooms of X and Y.
    xRoom = x->room;
    yRoom = y->room;
    // Y is not X's roommate.
    if (xRoom->roomId == yRoom->roomId) {
        printf("Both students are in the same room. They cannot swap.\n");
        return;
    }

    // Collect all the affected students.
    for (i = 0; i < BEDS_PER_ROOM; i++) {
        if (xRoom->occupantsRollNo[i] != 0)
            affected[affCount++] = xRoom->occupantsRollNo[i];
        if (yRoom->occupantsRollNo[i] != 0)
            affected[affCount++] = yRoom->occupantsRollNo[i];
    }
    // Check that no affected student is committed or in the pool.
    for (i = 0; i < affCount; i++) {
        Student *a = getStudent(affected[i]);
        if (a == NULL || a->commitmentRequest != 0 || a->poolState != NONE) {
            printf("Student %ld is already committed or in the pool.\n", affected[i]);
            return;
        }
    }

    // Build a request with status OPEN (also checks space in the requests box).
    r = newRequest(REQ_ROOMMATE_SWAP, xRoll);
    if (r == NULL) {
        printf("Requests box is full or memory allocation failed.\n");
        return;
    }
    // Set moves: X from X's room to Y's room, and Y from Y's room to X's room.
    if (!addMove(r, xRoll, xRoom->roomId, yRoom->roomId) ||
        !addMove(r, yRoll, yRoom->roomId, xRoom->roomId)) {
        cancelRequest(r, "could not build roommate swap request");
        return;
    }
    // Fill the approvers: X is ACCEPTED and others are PENDING.
    // Others get a notice.
    for (i = 0; i < affCount; i++) {
        ApproverState state = affected[i] == xRoll ? APPROVER_ACCEPTED : APPROVER_PENDING;
        if (!addApprover(r, affected[i], state)) {
            cancelRequest(r, "could not add request approvers");
            return;
        }
        if (affected[i] != xRoll)
            addNotice(affected[i], r->id, "Roommate swap request waiting. Log in to respond.");
    }
    // X is committed to the request; register the request on both rooms.
    x->commitmentRequest = r->id;
    roomAddOpen(xRoom->roomId, r->id);
    roomAddOpen(yRoom->roomId, r->id);
    // Save everything and tell the user the request was created.
    saveAll();
    printf("Roommate swap request %d created.\n", r->id);
}

// Remove a request from every room it touches (function to remove an open request from the rooms).
void unlinkRequestRooms(Request *r)
{
    int rooms[2 * MAX_MOVES];
    // Collect the rooms touched by the request's moves.
    int nRooms = requestRooms(r, rooms, 2 * MAX_MOVES);
    int i;
    // Remove the request id from each of those rooms' open request lists.
    for (i = 0; i < nRooms; i++)
        roomRemoveOpen(rooms[i], r->id);
}

// Expire one request and release its students.
// It marks the request EXPIRED with a reason, unlocks its students, notifies the approvers,
// and removes the request from every room's request list.
void expireRequest(Request *r)
{
    int i;
    // Only an OPEN request can expire.
    if (r == NULL || r->status != REQ_OPEN)
        return;
    // Mark the request EXPIRED and store the reason.
    r->status = REQ_EXPIRED;
    snprintf(r->reason, sizeof(r->reason), "Request expired after two days.");
    // Check whether every approver has accepted.
    for (i = 0; i < r->approverCount; i++) {
        Student *s = getStudent(r->approvers[i].roll);
        if (s != NULL && s->commitmentRequest == r->id)
            s->commitmentRequest = 0;
        addNotice(r->approvers[i].roll, r->id, r->reason);
    }
    // Also unlock the students being moved, in case they are not approvers.
    for (i = 0; i < r->moveCount; i++) {
        Student *s = getStudent(r->moves[i].roll);
        if (s != NULL && s->commitmentRequest == r->id)
            s->commitmentRequest = 0;
    }
    // Remove the request from the rooms' request lists.
    unlinkRequestRooms(r);
    // Tell the pool module that this request is closed.
    poolRequestClosed(r);
    // Save the changes.
    saveAll();
}

// Check all requests when a student logs in; expiry is not timer-based.
// Requests not completed within two days are cleaned up: every OPEN request
// whose expiry time has passed is expired.
void expireStaleRequest(long int roll)
{
    int id;
    // roll is unused because all requests are checked, not only this student's.
    (void)roll;
    // Go through requests from id 1 until getRequest returns NULL.
    for (id = 1; ; id++) {
        Request *r = getRequest(id);
        if (r == NULL)
            break;
        // Expire only OPEN requests whose time has passed.
        if (r->status == REQ_OPEN && now() > r->expiresAt)
            expireRequest(r);
    }
}

// Respond to a request. answer must be APPROVER_ACCEPTED or APPROVER_REJECTED.
void respond(int reqId, long int roll, ApproverState answer)
{
    // Find the request and check it is still open.
    Request *r = getRequest(reqId);
    Student *s = getStudent(roll);
    int k, i, allAccepted = 1;

    if (r == NULL || r->status != REQ_OPEN) {
        printf("Request %d is not open or does not exist.\n", reqId);
        return;
    }
    // The student must exist.
    if (s == NULL) {
        printf("Student with rollno %ld does not exist.\n", roll);
        return;
    }
    // Check the student is an approver on this request.
    k = isApprover(r, roll);
    if (k < 0) {
        printf("Student %ld is not an approver of request %d.\n", roll, reqId);
        return;
    }
    // If the request has expired, mark it EXPIRED and stop.
    if (now() > r->expiresAt) {
        expireRequest(r);
        printf("Request %d has expired.\n", reqId);
        return;
    }
    // If the answer is REJECTED, cancel the request.
    if (answer == APPROVER_REJECTED) {
        snprintf(r->reason, sizeof(r->reason), "Rejected by student %ld.", roll);
        // Pool consent requests have their own decline handling.
        if (r->type == REQ_POOL_CONSENT)
            consentDeclined(r, r->reason);
        else
            cancelRequest(r, r->reason);
        // Save the changes made by the rejection.
        saveAll();
        return;
    }
    // Anything other than ACCEPTED or REJECTED is invalid.
    if (answer != APPROVER_ACCEPTED) {
        printf("Invalid response.\n");
        return;
    }
    // If the answer is ACCEPTED, check the student is not committed to another request.
    if (s->commitmentRequest != 0 && s->commitmentRequest != reqId) {
        printf("Student is committed to another request.\n");
        return;
    }

    // Mark the student committed; if all approvers have accepted, then execute.
    s->commitmentRequest = reqId;
    // Record this approver's acceptance.
    r->approvers[k].state = APPROVER_ACCEPTED;
    // For pool consent requests, hand over to consentAccepted.
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
    // All approvers accepted: execute the request.
    if (allAccepted)
        execute(r);
    saveAll();
}

// Cancel an open request only when its initiator withdraws it.
void withdraw(long int roll)
{
    Student *s = getStudent(roll);
    Request *r;
    // Nothing to withdraw if the student does not exist or has no committed request.
    if (s == NULL || s->commitmentRequest == 0)
        return;
    // Find the student's committed request; it must still be open.
    r = getRequest(s->commitmentRequest);
    if (r == NULL || r->status != REQ_OPEN)
        return;
    // Only the initiator can withdraw the request.
    if (r->initiator != roll) {
        printf("Only the initiator can withdraw request %d.\n", r->id);
        return;
    }
    // Cancel and give reason.
    cancelRequest(r, "Withdrawn by initiator.");
    // Save the changes made by the withdrawal.
    saveAll();
}

// Helper function to print the hostel name for a room id.
// The hostel code comes from roomHostel(); AA, AB and GC map to full names.
void printHostelName(int roomId)
{
    // Get the hostel code for this room id.
    const char *hostel = roomHostel(roomId);
    // Print the full hostel name for the code.
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

// Return the room number inside its hostel (RoomId minus the hostel offset).
int printRoomNo(int roomId)
{
    // roomNumber() gives the room number inside its hostel.
    return roomNumber(roomId);
}

// Login: show notices and pending approvals for this student.
void login(long int roll)
{
    Student *s = getStudent(roll);
    int i, k = 0, id;
    // The student must exist.
    if (s == NULL) {
        printf("Student with rollNo %ld does not exist.\n", roll);
        return;
    }
    printf("---- ENTER DETAILS ----\n");
    // Show the roll number and the room (if one is allocated).
    if (s->room != NULL)
        printf("Roll no : %ld Room no : %d\n", s->rollNo, s->room->roomNo);
    else
        printf("Roll no : %ld Room : not allocated\n", s->rollNo);

    // Check whether any request has expired.
    expireStaleRequest(roll);

    // Show unseen notices.
    for (i = 0; i < s->noticeCount; i++) {
        // Print the notice and mark it as seen.
        if (s->notices[i] != NULL && !s->notices[i]->seen) {
            printf("---- NOTICE BOX ----\n%s\n", s->notices[i]->info);
            s->notices[i]->seen = true;
            k = 1;
        }
    }
    // If no unseen notice was printed, say the inbox is empty.
    if (!k)
        printf("NOTICE Inbox is EMPTY.\n");

    // Show requests waiting for this student: OPEN requests where the
    // student is a PENDING approver.
    for (id = 1; ; id++) {
        Request *r = getRequest(id);
        int a;
        if (r == NULL)
            break;
        // Skip requests that are not OPEN.
        if (r->status != REQ_OPEN)
            continue;
        // Skip requests where this student is not a PENDING approver.
        a = isApprover(r, roll);
        if (a < 0 || r->approvers[a].state != APPROVER_PENDING)
            continue;

        // Print the request details.
        printf("RequestId : %d\n", r->id);
        printf("Request's initiator rollNo : %ld\n", r->initiator);
        printf("Request type : %s\n", requestTypeName(r->type));
        // Show the first move: from hostel and room to hostel and room.
        if (r->moveCount > 0) {
            printf("From Hostel ");
            printHostelName(r->moves[0].fromRoom);
            printf(" %d to Hostel ", printRoomNo(r->moves[0].fromRoom));
            printHostelName(r->moves[0].toRoom);
            printf(" %d\n", printRoomNo(r->moves[0].toRoom));
        }
        // Show the time left before the request expires.
        printf("Remaining Time : %ld seconds.\n", r->expiresAt - now());
        // Get the answer and either call respond or skip for later.
        printf("--- Choice Menu ---\n1.ACCEPT\n2.REJECT\n3.SKIP FOR LATER\n");
        {
            int ch;
            // Read the choice; stop if the input is invalid.
            if (scanf("%d", &ch) != 1) {
                printf("Invalid input.\n");
                return;
            }
            // 1 = accept, 2 = reject, 3 = skip for later; anything else is skipped.
            if (ch == 1)
                respond(r->id, roll, APPROVER_ACCEPTED);
            else if (ch == 2)
                respond(r->id, roll, APPROVER_REJECTED);
            else if (ch != 3)
                printf("INVALID Choice, SKIPPING FOR NOW.\n");
        }
    }
    // Save any changes made during login.
    saveAll();
}
