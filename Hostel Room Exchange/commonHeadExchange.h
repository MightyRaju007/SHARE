#ifndef COMMON_HEAD_EXCHANGE_H
#define COMMON_HEAD_EXCHANGE_H
#include <stdbool.h>
#define Max_Room_request 3
#define NUM_HOSTELS 3
#define ROOMS_PER_FLOOR 40
#define BEDS_PER_ROOM 2
#define NUM_CHOICES 3
#define HOSTEL_LEN 3
#define MAX_MOVES 9
#define MAX_APPROVERS 4
#define REASON_LEN 100
#define NOTICE_LEN 100
#define MAX_FLOORS 9
#define MAX_STUDENTS (MAX_FLOORS * ROOMS_PER_FLOOR * BEDS_PER_ROOM)
#define REQUEST_TTL (2L * 24 * 60 * 60)


typedef enum {
    NONE, WAITING_CONSENT, IN_POOL
} PoolState;

typedef struct OpenRequestNode OpenRequestNode;

struct Room{
    int roomId;
    char hostel[3];
    int floor;
    int roomNo;
    long int occupantsRollNo[2];
    OpenRequestNode *openRequests;
};

struct OpenRequestNode{
    int requestId;
    OpenRequestNode *next;
};

struct PoolChoice {
    char hostel[3];
    int floor;
};

struct Notice{
    int requestId;
    char info[100];
    bool seen;
};

struct Student {
    long int rollNo;
    char name[100];
    struct Room *room;
    int commitmentRequest;
    PoolState poolState;
    struct PoolChoice *poolchoices[3];
    int poolSeedPos;
    int noticeCount;
    struct Notice *notices[3];
};

struct StudentNode {
    struct Student *student;
    struct StudentNode *next;
};

typedef struct Room Room;
typedef struct PoolChoice PoolChoice;
typedef struct Notice Notice;
typedef struct Student Student;
typedef struct StudentNode StudentNode;

typedef enum {
    REQ_ROOM_EXCHANGE = 0,
    REQ_ROOMMATE_SWAP,
    REQ_POOL_CONSENT,
    REQ_POOL_TRADE
} RequestType;

typedef enum {
    REQ_OPEN = 0,
    REQ_COMPLETED,
    REQ_CANCELLED,
    REQ_EXPIRED
} RequestStatus;

typedef enum {
    APPROVER_PENDING = 0,
    APPROVER_ACCEPTED,
    APPROVER_REJECTED
} ApproverState;

typedef struct {
    long int roll;
    int fromRoom;
    int toRoom;
} Move;

typedef struct {
    long int roll;
    ApproverState state;
} Approver;

typedef struct {
    int id;
    RequestType type;
    long int initiator;
    int moveCount;
    Move moves[MAX_MOVES];
    int approverCount;
    Approver approvers[MAX_APPROVERS];
    long createdAt;
    long expiresAt;
    RequestStatus status;
    char reason[REASON_LEN];
} Request;

typedef struct {
    long int roll;
    int fromFloor;
    int toFloor;
    int rank;
} Edge;

typedef struct {
    Edge *items;
    int n;
    int cap;
} EdgeList;

void cancelConflicts(const int *rooms, int nRooms);

int hostelIndex(const char *code);

const char *hostelCode(int index);

int hostelFloors(int index);

int floorExists(const char *code, int floor);

int roomFloor(int roomId);

long int roommateOf(long int rollNo);

Student *getStudent(long int rollNo);

Room *getRoom(int roomId);

long now(void);

Request *newRequest(RequestType type, long int initiator);

Request *getRequest(int id);

int addMove(Request *req, long int rollNo, int fromRoom, int toRoom);

int addApprover(Request *req, long int rollNo, ApproverState state);

void roomAddOpen(int roomId, int requestId);

void roomRemoveOpen(int roomId, int requestId);

int listStudents(long int *out, int maxOut);

void saveAll(void);

void appendLog(const char *line);

void addNotice(long int rollNo, int requestId, const char *text);

int roomOccupantCount(const Room *room);

int roomHasStudent(const Room *room, long int rollNo);

long int roomOtherOccupant(const Room *room, long int rollNo);

int roomOpenCount(const Room *room);

int requestRooms(const Request *req, int *out, int maxOut);

int roomExists(int roomId);

int roomNumber(int roomId);


int makeRoomId(const char *hostel, int roomNo);

int hostelOfRoom(int roomId);

const char *roomHostel(int roomId);

int floorIndex(const char *hostel, int floor);

int floorIndexOfRoom(int roomId);

void floorFromIndex(int idx, PoolChoice *choice);

int parseRoom(const char *text);

int parseChoice(const char *text, PoolChoice *choice);

void formatRoom(int roomId, char *buf);

void formatChoice(const PoolChoice *choice, char *buf);

const char *requestTypeName(RequestType type);

const char *requestStatusName(RequestStatus status);

const char *poolStateName(PoolState state);

int sameFloor(const PoolChoice *a, const PoolChoice *b);

int poolChoiceCount(const Student *student);

Student *storeAddStudent(long int roll, const char *name, int roomId);

void storeReset(void);

void cancelRequest(Request *req, const char *reason);

void poolRequestClosed(Request *req);

void loadAll(void);

int importAllocatedRooms(void);

void createExchangeRoom(long int xRoll, int targetRoomId);

void createRoomateSwap(long int xRoll, long int yRoll);

void respond(int reqId, long int roll, ApproverState answer);

void withdraw(long int roll);

void expireStaleRequest(long int roll);

void login(long int roll);

int execute(Request *req);

int copyOpenIds(const Room *room, int **out);

void shuffleRolls(long int *rolls, int n, unsigned int seed);

int enterPool(long int roll, const PoolChoice *choices, int numChoices);

int leavePool(long int roll);

int revokeConsent(long int roommateRoll);

int runPool(unsigned int seed);

#endif
