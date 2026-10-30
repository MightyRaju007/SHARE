#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include "commonHeadExchange.h"
#include <string.h>
#include <time.h>
#include <ctype.h>

#define HASH_SIZE 503
#define MAX_REQUESTS 100
#define REQUEST_TTL (2L * 24 * 60 * 60)

static Request *requests[MAX_REQUESTS];
static int requestCount = 0;
static struct StudentNode *studentTableFront[HASH_SIZE];
static struct StudentNode *studentTableBack[HASH_SIZE];
static struct Room *roomArray[440];

int hashFunction(long int rollNo){
    int bucket = (int)(rollNo % HASH_SIZE);
    if (bucket < 0){
        bucket += HASH_SIZE;
    }
    return bucket;
}

struct Student *getStudent(long int rollNo){
    int bucket = hashFunction(rollNo);
    struct StudentNode *head = studentTableFront[bucket];
    if (head == NULL){
        return NULL;
    }
    struct StudentNode *temp = head;
    while (temp != NULL){
        if (temp -> student -> rollNo == rollNo){
            return temp -> student;
        }
        temp = temp -> next;
    }
    return NULL;
}

int addStudent(struct Student *student){
    if (student == NULL){
        return 0;
    }
    struct Student *temp = getStudent(student -> rollNo);
    if (temp != NULL){
        return 0;
    }
    struct StudentNode *new = malloc(sizeof(struct StudentNode));
    if (new == NULL){
        return 0;
    }
    new -> student = student;
    new -> next = NULL;
    int bucket = hashFunction(student -> rollNo);
    if (studentTableFront[bucket] == NULL){
        studentTableFront[bucket] = new;
        studentTableFront[bucket] -> next = NULL;
        studentTableBack[bucket] = new;
        return 1;
    }
    studentTableBack[bucket] -> next = new;
    studentTableBack[bucket] = new;
    return 1;
}

int deleteStudent(long int rollNo){
    struct Student *temp = getStudent(rollNo);
    if (temp == NULL){
        return 0;
    }
    int bucket = hashFunction(temp -> rollNo);
    struct StudentNode *transverser = studentTableFront[bucket];
    if (transverser -> student -> rollNo == rollNo){
        if(studentTableBack[bucket] == transverser){
            studentTableBack[bucket] = NULL;
        }
        studentTableFront[bucket] = transverser -> next;
        free(transverser);
        return 1;
    }
    while ((transverser != NULL) && (transverser -> next != NULL)){
        if (transverser -> next -> student -> rollNo == rollNo){
            struct StudentNode *deleted = transverser -> next;
            if (studentTableBack[bucket] == transverser -> next ){
                studentTableBack[bucket] = transverser;
            }
            transverser -> next = transverser -> next -> next;
            free(deleted);
            return 1;
        }
        transverser = transverser -> next;
    }
    return 0;
}

struct Student *createStudent(long int rollNo, char name[]){
    struct Student *new = malloc(sizeof(struct Student));
    if (new == NULL){
        return NULL;
    }
    new -> rollNo = rollNo;
    strcpy(new -> name, name);
    new -> room = NULL;
    new -> commitmentRequest = 0;
    new -> poolState = NONE;
    for(int i = 0; i < 3; i++){
        new -> poolchoices[i] = NULL;
        new -> notices[i] = NULL;
    }
    new -> poolSeedPos = -1;
    new -> noticeCount = 0;
    return new;
}

static void freeStudentData(struct Student *student){
    if (student == NULL){
        return;
    }
    for (int i = 0; i < NUM_CHOICES; i++){
        free(student -> poolchoices[i]);
        free(student -> notices[i]);
    }
    free(student);
}

void freeStudentTable(){
    for (int i = 0; i < HASH_SIZE; i++){
        struct StudentNode *temp = studentTableFront[i];
        while (temp != NULL){
            struct StudentNode *deleted = temp;
            temp = temp -> next;
            freeStudentData(deleted -> student);
            free(deleted);
        }
        studentTableFront[i] = NULL;
        studentTableBack[i] = NULL;
    }
    return;
}

const struct {
    const char *code;
    int floors;
    int offset;
    int firstFloorIndex;
} HOSTELS[NUM_HOSTELS] = {
    { "AA", 4, 0, 0 },
    { "AB", 4, 160, 4 },
    { "GC", 1, 320, 8 }
};

int hostelIndex(const char *hostel)
{
    int h;
    if (hostel == NULL)
        return -1;
    for (h = 0; h < NUM_HOSTELS; h++)
        if (strcmp(HOSTELS[h].code, hostel) == 0)
            return h;
    return -1;
}

const char *hostelCode(int index)
{
    return (index >= 0 && index < NUM_HOSTELS) ? HOSTELS[index].code : "?";
}

int hostelFloors(int index)
{
    return (index >= 0 && index < NUM_HOSTELS) ? HOSTELS[index].floors : 0;
}

int floorExists(const char *code, int floor)
{
    int hostel = hostelIndex(code);
    if (hostel < 0)
        return 0;
    if (hostel == 2)
        return floor == 2;
    return floor >= 0 && floor <= 3;
}

int roomId(char hostel[], int roomNo){
    int hostelNumber = hostelIndex(hostel);
    if (hostelNumber == 0){
        if ((roomNo < 1) || (roomNo > 160)){
            return -1;
        }
        return roomNo;
    }
    if (hostelNumber == 1){
        if ((roomNo < 1) || (roomNo > 160)){
            return -1;
        }
        return 160 + roomNo;
    }
    if (hostelNumber == 2){
        if ((roomNo < 80) || (roomNo > 119)){
            return -1;
        }
        return 320 + roomNo;
    }
    return -1;
}

int hostelNumber(int roomId){
    if ((roomId >= 1) && (roomId <= 160)){
        return 0;
    }
    if ((roomId >= 161) && (roomId <= 320)){
        return 1;
    }
    if ((roomId >= 400) && (roomId <= 439)){
        return 2;
    }
    return -1;
}

int roomNo(int roomId){
    if ((roomId >= 1) && (roomId <= 160)){
        return roomId;
    }
    if ((roomId >= 161) && (roomId <= 320)){
        return roomId - 160;
    }
    if ((roomId >= 400) && (roomId <= 439)){
        return roomId - 320;
    }
    return -1;
}

int roomNumber(int roomId);

int roomFloor(int roomId)
{
    int h = hostelNumber(roomId);
    int n = roomNumber(roomId);
    if (h == 2){
        return n >= 80 && n <= 119 ? 2 : 0;
    }
    return n == 0 ? -1 : (n - 1) / ROOMS_PER_FLOOR;
}


int roomOffset(const char *hostel)
{
    int h = hostelIndex(hostel);
    return h < 0 ? -1 : HOSTELS[h].offset;
}

int makeRoomId(const char *hostel, int roomNo)
{
    int h = hostelIndex(hostel);
    if (h < 0)
        return 0;
    if (h == 2){
        if (roomNo < 80 || roomNo > 119)
            return 0;
        return HOSTELS[h].offset + roomNo;
    }
    if (roomNo < 1 || roomNo > HOSTELS[h].floors * ROOMS_PER_FLOOR)
        return 0;
    return HOSTELS[h].offset + roomNo;
}

int hostelOfRoom(int roomId)
{
    int h, found = -1;
    if (roomId < 1)
        return -1;
    for (h = 0; h < NUM_HOSTELS; h++)
        if (roomId > HOSTELS[h].offset)
            found = h;
    if (found < 0)
        return -1;
    int localRoom = roomId - HOSTELS[found].offset;
    if (found == 2){
        if (localRoom < 80 || localRoom > 119)
            return -1;
    }
    else if (localRoom < 1 || localRoom > HOSTELS[found].floors * ROOMS_PER_FLOOR){
        return -1;
    }
    return found;
}

int roomExists(int roomId)
{
    return hostelOfRoom(roomId) >= 0;
}

const char *roomHostel(int roomId)
{
    return hostelCode(hostelOfRoom(roomId));
}

int roomNumber(int roomId)
{
    int h = hostelOfRoom(roomId);
    return h < 0 ? 0 : roomId - HOSTELS[h].offset;
}

int floorIndex(const char *hostel, int floor)
{
    int h = hostelIndex(hostel);
    if (h < 0 || !floorExists(hostel, floor))
        return -1;
    return HOSTELS[h].firstFloorIndex + (h == 2 ? 0 : floor);
}

int floorIndexOfRoom(int roomId)
{
    int h = hostelNumber(roomId);
    if (!roomExists(roomId))
        return -1;
    if (h == 2)
        return 8;
    return (roomId - 1) / ROOMS_PER_FLOOR;
}

void floorFromIndex(int idx, PoolChoice *c)
{
    int h;
    strcpy(c->hostel, "?");
    c->floor = 0;
    for (h = 0; h < NUM_HOSTELS; h++) {
        int first = HOSTELS[h].firstFloorIndex;
        if (idx >= first && idx < first + HOSTELS[h].floors) {
            strcpy(c->hostel, HOSTELS[h].code);
            c->floor = idx - first + (h == 2 ? 2 : 0);
            return;
        }
    }
}

void formatRoom(int roomId, char *buf)
{
    if (!roomExists(roomId))
        sprintf(buf, "room?%d", roomId);
    else
        sprintf(buf, "%s-%d", roomHostel(roomId), roomNumber(roomId));
}

int parseRoom(const char *text)
{
    char hostel[HOSTEL_LEN];
    const char *p = text;

    while (isspace((unsigned char)*p))
        p++;

    if (isdigit((unsigned char)*p)) {
        int id = atoi(p);
        return roomExists(id) ? id : 0;
    }

    if (!isupper((unsigned char)p[0]) || !isupper((unsigned char)p[1]))
        return 0;
    hostel[0] = p[0];
    hostel[1] = p[1];
    hostel[2] = '\0';
    p += 2;
    if (*p == '-')
        p++;
    if (!isdigit((unsigned char)*p))
        return 0;
    return makeRoomId(hostel, atoi(p));
}

int sameFloor(const PoolChoice *a, const PoolChoice *b)
{
    return a->floor == b->floor && strcmp(a->hostel, b->hostel) == 0;
}

int parseChoice(const char *text, PoolChoice *c)
{
    if (!isupper((unsigned char)text[0]) ||
        !isupper((unsigned char)text[1]) ||
        !isdigit((unsigned char)text[2]))
        return 0;

    c->hostel[0] = text[0];
    c->hostel[1] = text[1];
    c->hostel[2] = '\0';
    c->floor = atoi(text + 2);
    return floorExists(c->hostel, c->floor);
}

void formatChoice(const PoolChoice *c, char *buf)
{
    sprintf(buf, "%s%d", c->hostel, c->floor);
}

const char *requestTypeName(RequestType t)
{
    switch (t) {
        case REQ_ROOM_EXCHANGE: return "ROOM_EXCHANGE";
        case REQ_ROOMMATE_SWAP: return "ROOMMATE_SWAP";
        case REQ_POOL_CONSENT: return "POOL_CONSENT";
        case REQ_POOL_TRADE: return "POOL_TRADE";
    }
    return "?";
}

const char *requestStatusName(RequestStatus s)
{
    switch (s) {
        case REQ_OPEN: return "OPEN";
        case REQ_COMPLETED: return "COMPLETED";
        case REQ_CANCELLED: return "CANCELLED";
        case REQ_EXPIRED: return "EXPIRED";
    }
    return "?";
}

const char *poolStateName(PoolState s)
{
    switch (s) {
        case NONE: return "NONE";
        case WAITING_CONSENT: return "WAITING_CONSENT";
        case IN_POOL: return "IN_POOL";
    }
    return "?";
}

int roomOccupantCount(const Room *room)
{
    int i, n = 0;
    for (i = 0; i < BEDS_PER_ROOM; i++)
        if (room->occupantsRollNo[i] != 0)
            n++;
    return n;
}

int roomHasStudent(const Room *room, long int roll)
{
    int i;
    if (roll == 0)
        return 0;
    for (i = 0; i < BEDS_PER_ROOM; i++)
        if (room->occupantsRollNo[i] == roll)
            return 1;
    return 0;
}

long int roomOtherOccupant(const Room *room, long int roll)
{
    int i;
    for (i = 0; i < BEDS_PER_ROOM; i++)
        if (room->occupantsRollNo[i] != 0 &&
            room->occupantsRollNo[i] != roll)
            return room->occupantsRollNo[i];
    return 0;
}

int roomOpenCount(const Room *room)
{
    int count = 0;
    OpenRequestNode *n = room -> openRequests;
    while (n != NULL){
        count++;
        n = n -> next;
    }
    return count;
}

int poolChoiceCount(const Student *s)
{
    int k = 0;
    while (k < NUM_CHOICES && s->poolchoices[k] != NULL)
        k++;
    return k;
}

static int addRoomOnce(int *out, int n, int maxOut, int room)
{
    int i;
    if (room == 0)
        return n;
    for (i = 0; i < n; i++)
        if (out[i] == room)
            return n;
    if (n < maxOut)
        out[n++] = room;
    return n;
}

int requestRooms(const Request *r, int *out, int maxOut)
{
    int i, n = 0;

    for (i = 0; i < r->moveCount; i++) {
        n = addRoomOnce(out, n, maxOut, r->moves[i].fromRoom);
        n = addRoomOnce(out, n, maxOut, r->moves[i].toRoom);
    }

    if (r->type == REQ_POOL_CONSENT && r->initiator != 0) {
        const Student *s = getStudent(r->initiator);
        if (s != NULL && s->room != NULL)
            n = addRoomOnce(out, n, maxOut, s->room->roomId);
    }
    return n;
}

struct Room *getRoom(int roomId){
    if ((roomId >= 1) && (roomId < 440)){
        if((roomId > 319) && (roomId < 400)){
            return NULL;
        }
        return roomArray[roomId];
    }
    return NULL;
}

int addRoom(struct Room *room){
    if (room == NULL){
        return 0;
    }
    int id = roomId(room -> hostel, room -> roomNo);
    if (id == -1){
        return 0;
    }
    if (roomArray[id] == NULL){
        roomArray[id] = room;
        return 1;
    }
    return 0;
}

long int roommateOf(long int rollNo){
    struct Student *temp = getStudent(rollNo);
    if (temp == NULL){
        return 0;
    }
    struct Room *room = temp -> room;
    if (room == NULL){
        return 0;
    } 
    for (int i = 0; i < 2; i++){
        if (room -> occupantsRollNo[i] != rollNo){
            return room -> occupantsRollNo[i];
        }
    }
    return 0;
}

long now(void){
    return (long)time(NULL);
}

void cancelRequest(Request *req, const char *reason){
    int rooms[2 * MAX_MOVES];
    int nRooms, i;
    if (req == NULL || req->status != REQ_OPEN)
        return;

    req->status = REQ_CANCELLED;
    if (reason != NULL)
        snprintf(req->reason, sizeof(req->reason), "%s", reason);

    for (i = 0; i < req->approverCount; i++){
        Student *student = getStudent(req->approvers[i].roll);
        if (student != NULL && student->commitmentRequest == req->id)
            student->commitmentRequest = 0;
    }
    for (i = 0; i < req->moveCount; i++){
        Student *student = getStudent(req->moves[i].roll);
        if (student != NULL && student->commitmentRequest == req->id)
            student->commitmentRequest = 0;
    }

    nRooms = requestRooms(req, rooms, 2 * MAX_MOVES);
    for (i = 0; i < nRooms; i++)
        roomRemoveOpen(rooms[i], req->id);

    poolRequestClosed(req);
}

void saveAll(void){
    FILE *studentFile = NULL;
    FILE *studentCheck = fopen("students.txt", "r");
    FILE *studentStateFile = fopen("student_state.txt", "w");
    FILE *roomFile = fopen("rooms.txt", "w");
    FILE *noticeFile = fopen("notices.txt", "w");
    FILE *requestFile = fopen("requests.txt", "w");
    if (studentCheck == NULL){
        studentFile = fopen("students.txt", "w");
    }
    else{
        fclose(studentCheck);
    }
    if (((studentCheck == NULL) && (studentFile == NULL)) ||
        (studentStateFile == NULL) || (roomFile == NULL) ||
        (noticeFile == NULL) || (requestFile == NULL)){
        if (studentFile != NULL) fclose(studentFile);
        if (studentStateFile != NULL) fclose(studentStateFile);
        if (roomFile != NULL) fclose(roomFile);
        if (noticeFile != NULL) fclose(noticeFile);
        if (requestFile != NULL) fclose(requestFile);
        printf("Error saving data!\n");
        return;
    }
    for (int i = 0; i < HASH_SIZE; i++){
        struct StudentNode *temp = studentTableFront[i];
        while (temp != NULL){
            struct Student *student = temp -> student;
            /*
             * Create students.txt only if it does not exist.
             * Existing master roster is never overwritten.
             */
            if (studentFile != NULL){
                int room = -1;
                if (student -> room != NULL){
                    room = student -> room -> roomId;
                }
                fprintf(studentFile, "%ld,%s,%d,%d,%d,%d",
                    student -> rollNo,
                    student -> name,
                    room,
                    student -> commitmentRequest,
                    student -> poolState,
                    student -> poolSeedPos);
                for (int j = 0; j < NUM_CHOICES; j++){
                    if (student -> poolchoices[j] != NULL){
                        fprintf(studentFile, ",%s,%d",
                            student -> poolchoices[j] -> hostel,
                            student -> poolchoices[j] -> floor);
                    }
                    else{
                        fprintf(studentFile, ",-1,-1");
                    }
                }
                fprintf(studentFile, "\n");
            }
            /*
             * Save changing student information separately.
             * Format:
             * rollNo,commitmentRequest,poolState,poolSeedPos,
             * hostel1,floor1,hostel2,floor2,hostel3,floor3
             */
            fprintf(studentStateFile, "%ld,%d,%d,%d",
                student -> rollNo,
                student -> commitmentRequest,
                student -> poolState,
                student -> poolSeedPos);
            for (int j = 0; j < NUM_CHOICES; j++){
                if (student -> poolchoices[j] != NULL){
                    fprintf(studentStateFile, ",%s,%d",
                        student -> poolchoices[j] -> hostel,
                        student -> poolchoices[j] -> floor);
                }
                else{
                    fprintf(studentStateFile, ",-1,-1");
                }
            }
            fprintf(studentStateFile, "\n");
            temp = temp -> next;
        }
    }
    for (int i = 0; i < 440; i++){
        struct Room *room = roomArray[i];
        if (room != NULL){
            fprintf(roomFile, "%d,%s,%d,%d,%ld,%ld,%d",
                room -> roomId,
                room -> hostel,
                room -> floor,
                room -> roomNo,
                room -> occupantsRollNo[0],
                room -> occupantsRollNo[1],
                roomOpenCount(room));
            for (OpenRequestNode *n = room -> openRequests;
                 n != NULL; n = n -> next){
                fprintf(roomFile, ",%d", n -> requestId);
            }
            fprintf(roomFile, "\n");
        }
    }
    for (int i = 0; i < HASH_SIZE; i++){
        struct StudentNode *temp = studentTableFront[i];
        while (temp != NULL){
            struct Student *student = temp -> student;
            for (int j = 0; j < student -> noticeCount; j++){
                if (student -> notices[j] != NULL){
                    fprintf(noticeFile, "%ld,%d,%s,%d\n",
                        student -> rollNo,
                        student -> notices[j] -> requestId,
                        student -> notices[j] -> info,
                        student -> notices[j] -> seen);
                }
            }
            temp = temp -> next;
        }
    }
    for (int i = 0; i < requestCount; i++){
        Request *req = requests[i];
        if (req == NULL)
            continue;
        fprintf(requestFile, "%d,%d,%ld,%ld,%ld,%d,%d,%d,%s",
            req -> id,
            req -> type,
            req -> initiator,
            req -> createdAt,
            req -> expiresAt,
            req -> status,
            req -> moveCount,
            req -> approverCount,
            req -> reason);
        for (int j = 0; j < req -> moveCount; j++){
            fprintf(requestFile, ",%ld:%d:%d",
                req -> moves[j].roll,
                req -> moves[j].fromRoom,
                req -> moves[j].toRoom);
        }
        for (int j = 0; j < req -> approverCount; j++){
            fprintf(requestFile, ",%ld:%d",
                req -> approvers[j].roll,
                req -> approvers[j].state);
        }
        fprintf(requestFile, "\n");
    }
    if (studentFile != NULL) fclose(studentFile);
    fclose(studentStateFile);
    fclose(roomFile);
    fclose(noticeFile);
    fclose(requestFile);
    printf("Data saved successfully!\n");
}

void loadAll(void){
    FILE *studentFile = fopen("students.txt", "r");
    int rosterMissing = (studentFile == NULL);
    int loadedStudents = 0;
    FILE *roomFile = fopen("rooms.txt", "r");
    FILE *noticeFile = fopen("notices.txt", "r");
    FILE *requestFile = fopen("requests.txt", "r");
    FILE *studentStateFile = fopen("student_state.txt", "r");
    char line[4096];
    char *token;
    storeReset();
    if (studentFile != NULL){
        while (fgets(line, sizeof(line), studentFile) != NULL){
            Student *student;
            long int rollNo;
            char name[100];
            int room, commitment, poolState, seedPos;
            token = strtok(line, ",\n");
            if (token == NULL)
                continue;
            rollNo = atol(token);
            token = strtok(NULL, ",\n");
            if (token == NULL)
                continue;
            strcpy(name, token);
            token = strtok(NULL, ",\n");
            if (token == NULL)
                continue;
            room = atoi(token);
            token = strtok(NULL, ",\n");
            if (token == NULL)
                continue;
            commitment = atoi(token);
            token = strtok(NULL, ",\n");
            if (token == NULL)
                continue;
            poolState = atoi(token);
            token = strtok(NULL, ",\n");
            if (token == NULL)
                continue;
            seedPos = atoi(token);
            student = createStudent(rollNo, name);
            if (student == NULL)
                continue;
            student -> commitmentRequest = commitment;
            student -> poolState = poolState;
            student -> poolSeedPos = seedPos;
            for (int i = 0; i < NUM_CHOICES; i++){
                char hostel[3];
                int floor;
                token = strtok(NULL, ",\n");
                if (token == NULL)
                    break;
                strcpy(hostel, token);
                token = strtok(NULL, ",\n");
                if (token == NULL)
                    break;
                floor = atoi(token);
                if (floor != -1){
                    student -> poolchoices[i] = malloc(sizeof(PoolChoice));
                    if (student -> poolchoices[i] != NULL){
                        strcpy(student -> poolchoices[i] -> hostel, hostel);
                        student -> poolchoices[i] -> floor = floor;
                    }
                }
            }
            if (addStudent(student) == 0){
                freeStudentData(student);
            }
            else{
                loadedStudents++;
            }
        }
        fclose(studentFile);
    }
    if (studentStateFile != NULL){
    while (fgets(line, sizeof(line), studentStateFile) != NULL){
        Student *student;
        long int rollNo;
        int commitment, poolState, seedPos;
        token = strtok(line, ",\n");
        if (token == NULL)
            continue;
        rollNo = atol(token);
        student = getStudent(rollNo);
        if (student == NULL)
            continue;
        token = strtok(NULL, ",\n");
        if (token == NULL)
            continue;
        commitment = atoi(token);
        token = strtok(NULL, ",\n");
        if (token == NULL)
            continue;
        poolState = atoi(token);
        token = strtok(NULL, ",\n");
        if (token == NULL)
            continue;
        seedPos = atoi(token);
        student -> commitmentRequest = commitment;
        student -> poolState = poolState;
        student -> poolSeedPos = seedPos;
        for (int i = 0; i < NUM_CHOICES; i++){
            char hostel[3];
            int floor;
            token = strtok(NULL, ",\n");
            if (token == NULL)
                break;
            strcpy(hostel, token);
            token = strtok(NULL, ",\n");
            if (token == NULL)
                break;
            floor = atoi(token);
            if (floor != -1){
                if (student -> poolchoices[i] == NULL){
                    student -> poolchoices[i] =
                        malloc(sizeof(PoolChoice));
                }
                if (student -> poolchoices[i] != NULL){
                    strcpy(student -> poolchoices[i] -> hostel, hostel);
                    student -> poolchoices[i] -> floor = floor;
                }
            }
            else{
                free(student -> poolchoices[i]);
                student -> poolchoices[i] = NULL;
            }
        }
    }
    fclose(studentStateFile);
    }
    if (roomFile != NULL){
        while (fgets(line, sizeof(line), roomFile) != NULL){
            Room *room;
            int id, floor, number, openCount;
            long int occupant1, occupant2;
            char hostel[3];
            token = strtok(line, ",\n");
            if (token == NULL)
                continue;
            id = atoi(token);
            token = strtok(NULL, ",\n");
            if (token == NULL)
                continue;
            strcpy(hostel, token);
            token = strtok(NULL, ",\n");
            if (token == NULL)
                continue;
            floor = atoi(token);
            token = strtok(NULL, ",\n");
            if (token == NULL)
                continue;
            number = atoi(token);
            token = strtok(NULL, ",\n");
            if (token == NULL)
                continue;
            occupant1 = atol(token);
            token = strtok(NULL, ",\n");
            if (token == NULL)
                continue;
            occupant2 = atol(token);
            token = strtok(NULL, ",\n");
            if (token == NULL)
                continue;
            openCount = atoi(token);
            if (id < 1 || id >= 440)
                continue;
            room = roomArray[id];
            if (room == NULL){
                room = calloc(1, sizeof(Room));
                if (room == NULL)
                    continue;
                roomArray[id] = room;
            }
            room -> roomId = id;
            strcpy(room -> hostel, hostel);
            room -> floor = floor;
            room -> roomNo = number;
            room -> occupantsRollNo[0] = occupant1;
            room -> occupantsRollNo[1] = occupant2;
            room -> openRequests = NULL;
            if (occupant1 != 0){
                Student *student = getStudent(occupant1);
                if (student != NULL)
                    student -> room = room;
            }
            if (occupant2 != 0){
                Student *student = getStudent(occupant2);
                if (student != NULL)
                    student -> room = room;
            }
            (void)openCount;
        }
        fclose(roomFile);
    }
    if (noticeFile != NULL){
        while (fgets(line, sizeof(line), noticeFile) != NULL){
            long int rollNo;
            int requestId, seen;
            char info[100];
            token = strtok(line, ",\n");
            if (token == NULL)
                continue;
            rollNo = atol(token);
            token = strtok(NULL, ",\n");
            if (token == NULL)
                continue;
            requestId = atoi(token);
            token = strtok(NULL, ",\n");
            if (token == NULL)
                continue;
            strcpy(info, token);
            token = strtok(NULL, ",\n");
            if (token == NULL)
                continue;
            seen = atoi(token);
            addNotice(rollNo, requestId, info);
            Student *student = getStudent(rollNo);
            if (student != NULL && student -> noticeCount > 0){
                student -> notices[student -> noticeCount - 1] -> seen = seen;
            }
        }
        fclose(noticeFile);
    }
    if (requestFile != NULL){
        while (fgets(line, sizeof(line), requestFile) != NULL){
            Request *req;
            int id, type, status, moveCount, approverCount;
            long int initiator, createdAt, expiresAt;
            char reason[REASON_LEN];
            token = strtok(line, ",\n");
            if (token == NULL)
                continue;
            id = atoi(token);
            token = strtok(NULL, ",\n");
            if (token == NULL)
                continue;
            type = atoi(token);
            token = strtok(NULL, ",\n");
            if (token == NULL)
                continue;
            initiator = atol(token);
            token = strtok(NULL, ",\n");
            if (token == NULL)
                continue;
            createdAt = atol(token);
            token = strtok(NULL, ",\n");
            if (token == NULL)
                continue;
            expiresAt = atol(token);
            token = strtok(NULL, ",\n");
            if (token == NULL)
                continue;
            status = atoi(token);
            token = strtok(NULL, ",\n");
            if (token == NULL)
                continue;
            moveCount = atoi(token);
            token = strtok(NULL, ",\n");
            if (token == NULL)
                continue;
            approverCount = atoi(token);
            token = strtok(NULL, ",\n");
            if (token == NULL)
                continue;
            strcpy(reason, token);
            req = newRequest(type, initiator);
            if (req == NULL)
                continue;
            req -> id = id;
            req -> createdAt = createdAt;
            req -> expiresAt = expiresAt;
            req -> status = status;
            strcpy(req -> reason, reason);
            for (int i = 0; i < moveCount && i < MAX_MOVES; i++){
                long int roll;
                int fromRoom, toRoom;
                char *moveData;
                token = strtok(NULL, ",\n");
                if (token == NULL)
                    break;
                moveData = strtok(token, ":");
                if (moveData == NULL)
                    continue;
                roll = atol(moveData);
                moveData = strtok(NULL, ":");
                if (moveData == NULL)
                    continue;
                fromRoom = atoi(moveData);
                moveData = strtok(NULL, ":");
                if (moveData == NULL)
                    continue;
                toRoom = atoi(moveData);
                addMove(req, roll, fromRoom, toRoom);
            }
            for (int i = 0; i < approverCount && i < MAX_APPROVERS; i++){
                long int roll;
                int state;
                char *approverData;
                token = strtok(NULL, ",\n");
                if (token == NULL)
                    break;
                approverData = strtok(token, ":");
                if (approverData == NULL)
                    continue;
                roll = atol(approverData);
                approverData = strtok(NULL, ":");
                if (approverData == NULL)
                    continue;
                state = atoi(approverData);

                addApprover(req, roll, state);
            }
        }
        fclose(requestFile);
    }
    roomFile = fopen("rooms.txt", "r");
    if (roomFile != NULL){
        while (fgets(line, sizeof(line), roomFile) != NULL){
            int id, openCount;

            token = strtok(line, ",\n");
            if (token == NULL)
                continue;
            id = atoi(token);

            for (int i = 0; i < 6; i++){
                token = strtok(NULL, ",\n");
                if (token == NULL)
                    break;
            }

            token = strtok(NULL, ",\n");
            if (token == NULL)
                continue;
            openCount = atoi(token);

            for (int i = 0; i < openCount; i++){
                token = strtok(NULL, ",\n");
                if (token == NULL)
                    break;
                roomAddOpen(id, atoi(token));
            }
        }
        fclose(roomFile);
    }

    /* If the master roster is missing, build the initial student table
       from allocated_rooms.txt. Existing rosters are never replaced. */
    if (rosterMissing || loadedStudents == 0){
        int imported = importAllocatedRooms();
        if (imported > 0){
            printf("No students were loaded from students.txt. Initialized %d room allocations from allocated_rooms.txt.\n", imported);
        }
        else{
            printf("No students were loaded, and no valid allocations were imported. Check allocated_rooms.txt and its format.\n");
        }
    }
}

int importAllocatedRooms(void){
    FILE *allocationFile = fopen("allocated_rooms.txt", "r");
    char line[256];
    long int roll1, roll2;
    char hostel[3];
    int floor, number, id, imported = 0;
    if (allocationFile == NULL){
        return 0;
    }
    for (int i = 0; i < HASH_SIZE; i++){
        StudentNode *node = studentTableFront[i];
        while (node != NULL){
            node -> student -> room = NULL;
            node = node -> next;
        }
    }
    for (int i = 0; i < 440; i++){
        if (roomArray[i] != NULL){
            roomArray[i] -> occupantsRollNo[0] = 0;
            roomArray[i] -> occupantsRollNo[1] = 0;
        }
    }
    while (fgets(line, sizeof(line), allocationFile) != NULL){
        if (sscanf(line, "%ld|%ld|%2[^|]|%d|%d", &roll1, &roll2, hostel, &floor, &number) != 5){
            continue;
        }

        /* On first initialization, allocated_rooms.txt is the only
           available roster source, so create placeholder student names. */
        Student *student1 = getStudent(roll1);
        Student *student2 = getStudent(roll2);
        char generatedName[100];
        if (student1 == NULL){
            snprintf(generatedName, sizeof(generatedName), "Student_%ld", roll1);
            student1 = createStudent(roll1, generatedName);
            if (student1 != NULL && addStudent(student1) == 0){
                freeStudentData(student1);
                student1 = NULL;
            }
        }
        if (student2 == NULL){
            snprintf(generatedName, sizeof(generatedName), "Student_%ld", roll2);
            student2 = createStudent(roll2, generatedName);
            if (student2 != NULL && addStudent(student2) == 0){
                freeStudentData(student2);
                student2 = NULL;
            }
        }

        id = roomId(hostel, number);
        if (id <= 0 || id >= 440){
            continue;
        }
        if (student1 == NULL || student2 == NULL){
            continue;
        }
        Room *room = roomArray[id];
        if (room == NULL){
            room = calloc(1, sizeof(Room));
            if (room == NULL){
                continue;
            }
            roomArray[id] = room;
        }
        room -> roomId = id;
        strcpy(room -> hostel, hostel);
        room -> floor = floor;
        room -> roomNo = number;
        room -> occupantsRollNo[0] = roll1;
        room -> occupantsRollNo[1] = roll2;
        student1 -> room = room;
        student2 -> room = room;
        imported += 1;
    }
    fclose(allocationFile);
    return imported;
}

void appendLog(const char *line){
    FILE *logFile = fopen("audit.log", "a");
    if (logFile == NULL){
        return;
    }
    fprintf(logFile, "%s\n", line);
    fclose(logFile);
}

void addNotice(long int rollNo, int requestId, const char *text){
    struct Student *student = getStudent(rollNo);
    if (student == NULL){
        return;
    }
    struct Notice *new = calloc(1, sizeof(struct Notice));
    if (new == NULL){
        return;
    }
    new -> requestId = requestId;
    strncpy(new -> info, text, sizeof(new -> info) - 1);
    new -> seen = false;

    if (student -> noticeCount >= NUM_CHOICES){
        free(student -> notices[0]);
        for (int i = 0; i < NUM_CHOICES - 1; i++){
            student -> notices[i] = student -> notices[i + 1];
        }
        student -> noticeCount = NUM_CHOICES - 1;
    }
    student -> notices[student -> noticeCount] = new;
    student -> noticeCount += 1;
}

Request *newRequest(RequestType type, long int initiator){
    if (requestCount >= MAX_REQUESTS){
        return NULL;
    }
    Request *new = calloc(1, sizeof(Request));
    if (new == NULL){
        return NULL;
    }
    new -> id = requestCount + 1;
    new -> type = type;
    new -> initiator = initiator;
    new -> createdAt = now();
    new -> expiresAt = new -> createdAt + REQUEST_TTL;
    new -> status = REQ_OPEN;
    requests[requestCount] = new;
    requestCount += 1;
    return new;
}

Request *getRequest(int id){
    if (id < 1 || id > requestCount){
        return NULL;
    }
    return requests[id - 1];
}

int addMove(Request *req, long int rollNo, int fromRoom, int toRoom){
    Move *m;
    if (req == NULL || req -> moveCount >= MAX_MOVES){
        return 0;
    }
    m = &req -> moves[req -> moveCount];
    req -> moveCount += 1;
    m -> roll = rollNo;
    m -> fromRoom = fromRoom;
    m -> toRoom = toRoom;
    return 1;
}

int addApprover(Request *req, long int roll, ApproverState state){
    Approver *a;
    if (req == NULL || req -> approverCount >= MAX_APPROVERS){
        return 0;
    }
    a = &req -> approvers[req -> approverCount];
    req -> approverCount += 1;
    a -> roll = roll;
    a -> state = state;
    return 1;
}

void roomAddOpen(int roomId, int requestId){
    Room *room = getRoom(roomId);
    OpenRequestNode *n;
    if (room == NULL || getRequest(requestId) == NULL){
        return;
    }
    for (n = room -> openRequests; n != NULL; n = n -> next){
        if (n -> requestId == requestId){
            return;
        }
    }
    if (roomOpenCount(room) >= Max_Room_request){
        return;
    }
    n = malloc(sizeof(OpenRequestNode));
    if (n == NULL){
        return;
    }
    n -> requestId = requestId;
    n -> next = room -> openRequests;
    room -> openRequests = n;
}

void roomRemoveOpen(int roomId, int requestId){
    Room *room = getRoom(roomId);
    OpenRequestNode **p;
    if (room == NULL){
        return;
    }
    for (p = &room -> openRequests; *p != NULL; p = &(*p) -> next){
        if ((*p) -> requestId == requestId){
            OpenRequestNode *temp = *p;
            *p = temp -> next;
            free(temp);
            return;
        }
    }
}

int listStudents(long int *out, int maxOut){
    int b, n = 0;
    StudentNode *node;
    for (b = 0; b < HASH_SIZE && n < maxOut; b++){
        for (node = studentTableFront[b]; node != NULL && n < maxOut; node = node -> next){
            out[n] = node -> student -> rollNo;
            n += 1;
        }
    }
    return n;
}

void storeReset(void){
    for (int i = 0; i < HASH_SIZE; i++){
        StudentNode *n = studentTableFront[i];
        while (n != NULL){
            StudentNode *next = n -> next;
            freeStudentData(n -> student);
            free(n);
            n = next;
        }
        studentTableFront[i] = NULL;
        studentTableBack[i] = NULL;
    }

    for (int i = 0; i < 440; i++){
        Room *room = roomArray[i];
        if (room != NULL){
            OpenRequestNode *n = room -> openRequests;
            while (n != NULL){
                OpenRequestNode *next = n -> next;
                free(n);
                n = next;
            }
            room -> openRequests = NULL;
            room -> occupantsRollNo[0] = 0;
            room -> occupantsRollNo[1] = 0;
        }
    }

    for (int i = 0; i < requestCount; i++){
        free(requests[i]);
        requests[i] = NULL;
    }
    requestCount = 0;
}

Student *storeAddStudent(long int roll, const char *name, int roomId){
    Room *room = getRoom(roomId);
    Student *student;
    int bed;
    if (room == NULL || roomOccupantCount(room) >= BEDS_PER_ROOM){
        return NULL;
    }
    if (getStudent(roll) != NULL){
        return NULL;
    }
    student = createStudent(roll, (char *)name);
    if (student == NULL){
        return NULL;
    }
    if (addStudent(student) == 0){
        freeStudentData(student);
        return NULL;
    }
    student -> room = room;
    bed = room -> occupantsRollNo[0] == 0 ? 0 : 1;
    room -> occupantsRollNo[bed] = roll;
    return student;
}

void studentMenu(long int roll){
    int choice;
    while (1){
        printf("\n===== STUDENT MENU =====\n");
        printf("1. Start Room Exchange\n");
        printf("2. Start Roommate Swap\n");
        printf("3. Join Pool\n");
        printf("4. Leave Pool\n");
        printf("5. View Request\n");
        printf("6. Withdraw Request\n");
        printf("0. Exit\n");
        printf("Enter your choice: ");
        if (scanf("%d", &choice) != 1){
            printf("Invalid input!\n");
            while (getchar() != '\n');
            continue;
        }
        switch (choice){
            case 1:{
                char targetText[32];
                int targetRoom;
                printf("Enter target room (for example AA-12 or GC-80): ");
                if (scanf("%31s", targetText) != 1){
                    printf("Invalid input!\n");
                    break;
                }
                targetRoom = parseRoom(targetText);
                if (targetRoom == 0)
                    printf("Invalid room!\n");
                else
                    createExchangeRoom(roll, targetRoom);
                break;
            }
            case 2:{
                long int otherRoll;
                printf("Enter the other student's roll number: ");
                if (scanf("%ld", &otherRoll) != 1){
                    printf("Invalid input!\n");
                    break;
                }
                createRoomateSwap(roll, otherRoll);
                break;
            }
            case 3:{
                PoolChoice choices[NUM_CHOICES];
                char choiceText[32];
                int numChoices;
                printf("How many floor choices (1-%d)? ", NUM_CHOICES);
                if (scanf("%d", &numChoices) != 1 ||
                    numChoices < 1 || numChoices > NUM_CHOICES){
                    printf("Invalid number of choices!\n");
                    break;
                }
                int valid = 1;
                for (int i = 0; i < numChoices; i++){
                    printf("Enter choice %d (for example AA0 or GC2): ", i + 1);
                    if (scanf("%31s", choiceText) != 1 ||
                        !parseChoice(choiceText, &choices[i])){
                        printf("Invalid hostel/floor choice!\n");
                        valid = 0;
                        break;
                    }
                }
                if (valid)
                    enterPool(roll, choices, numChoices);
                break;
            }
            case 4:
                leavePool(roll);
                break;

            case 5:{
                Student *student = getStudent(roll);
                Request *req;
                if (student == NULL || student->commitmentRequest == 0){
                    printf("You do not have an open request.\n");
                    break;
                }
                req = getRequest(student->commitmentRequest);
                if (req == NULL){
                    printf("Request not found.\n");
                    break;
                }
                printf("Request ID: %d\n", req->id);
                printf("Type: %s\n", requestTypeName(req->type));
                printf("Status: %s\n", requestStatusName(req->status));
                printf("Initiator: %ld\n", req->initiator);
                printf("Expires at: %ld\n", req->expiresAt);
                printf("Details:\n");
                for (int i = 0; i < req->moveCount; i++){
                    char from[16], to[16];
                    formatRoom(req->moves[i].fromRoom, from);
                    formatRoom(req->moves[i].toRoom, to);
                    printf("  %ld: %s -> %s\n",
                           req->moves[i].roll, from, to);
                }
                for (int i = 0; i < req->approverCount; i++){
                    printf("  Approver %ld: %s\n",
                           req->approvers[i].roll,
                           req->approvers[i].state == APPROVER_ACCEPTED ? "Accepted" :
                           req->approvers[i].state == APPROVER_REJECTED ? "Rejected" : "Pending");
                }
                break;
            }
            case 6:
                withdraw(roll);
                break;

            case 0:
                return;

            default:
                printf("Invalid choice!\n");
        }
    }
}

void adminMenu(){
    int choice;
    while (1){
        printf("\n===== ADMIN MENU =====\n");
        printf("1. Close Exchange Window\n");
        printf("2. Run Expiry Check\n");
        printf("3. Run Pool with Seed\n");
        printf("4. Print Report\n");
        printf("5. View Audit Log\n");
        printf("0. Exit\n");
        printf("Enter your choice: ");
        if (scanf("%d", &choice) != 1){
            printf("Invalid input!\n");
            while (getchar() != '\n');
            continue;
        }
        switch (choice){
            case 1:
                printf("Closing the exchange window is not implemented in the current data model.\n");
                break;

            case 2:{
                long int rolls[MAX_STUDENTS];
                int n = listStudents(rolls, MAX_STUDENTS);
                for (int i = 0; i < n; i++)
                    expireStaleRequest(rolls[i]);
                printf("Expiry check completed.\n");
                break;
            }
            case 3:{
                unsigned int seed;
                printf("Enter seed: ");
                if (scanf("%u", &seed) != 1){
                    printf("Invalid seed!\n");
                    break;
                }
                runPool(seed);
                break;
            }
            case 4:
                printf("Student and room report:\n");
                {
                    long int rolls[MAX_STUDENTS];
                    int n = listStudents(rolls, MAX_STUDENTS);
                    for (int i = 0; i < n; i++){
                        Student *student = getStudent(rolls[i]);
                        if (student != NULL){
                            char roomText[16];
                            if (student->room != NULL)
                                formatRoom(student->room->roomId, roomText);
                            else
                                strcpy(roomText, "Unallocated");
                            printf("%ld  %-25s  %s  Pool: %s\n",
                                   student->rollNo, student->name, roomText,
                                   poolStateName(student->poolState));
                        }
                    }
                }
                break;

            case 5:{
                FILE *logFile = fopen("audit.log", "r");
                char line[512];
                if (logFile == NULL){
                    printf("No audit log found.\n");
                    break;
                }
                while (fgets(line, sizeof(line), logFile) != NULL)
                    printf("%s", line);
                fclose(logFile);
                break;
            }
            case 0:
                return;

            default:
                printf("Invalid choice!\n");
        }
    }
}

int main(){
    int choice;
    loadAll();
    while (1){
        printf("\n===== ROOM EXCHANGE MODULE =====\n");
        printf("1. Student Login\n");
        printf("2. Admin Login\n");
        printf("3. Exit\n");
        printf("Enter your choice: ");
        if (scanf("%d", &choice) != 1){
            printf("Invalid input!\n");
            while (getchar() != '\n');
            continue;
        }
        switch (choice){
            case 1:{
                long int roll;
                printf("Enter your roll number: ");
                if (scanf("%ld", &roll) != 1){
                    printf("Invalid roll number!\n");
                    break;
                }
                if (getStudent(roll) == NULL){
                    printf("Student not found. Check students.txt.\n");
                    break;
                }
                login(roll);
                studentMenu(roll);
                break;
            }
            case 2:
                adminMenu();
                break;
            case 3:
                saveAll();
                return 0;
            default:
                printf("Invalid choice!\n");
        }
    }
}
