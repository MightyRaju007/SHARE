#ifndef COMMONHEADERFILE_H
#define COMMONHEADERFILE_H

#include <stdio.h>

#define FLOORS 4

#define NUM_CHOICES 3
#define NUM_HOSTELS 3
#define HOSTEL_LEN 3
#define MAX_STUDENTS 10000
#define MAX_UNITS 10000

typedef struct {
    char hostel[HOSTEL_LEN];
    int floor;
} Choice;

typedef enum {
    STATUS_RANDOM,
    STATUS_PENDING,
    STATUS_LOCKED
} Status;

typedef struct {
    long rollNo;
    char name[100];
    Choice choices[NUM_CHOICES];
    Status status;
    long roommateRoll;
} Preference;

typedef struct {
    long rollA;
    long rollB;
    Choice choices[NUM_CHOICES];
    int numChoices;
    int solo;
} Unit;

struct node{
    int val;
    struct node *next;
};

struct paired{
    long int rollNos[2];
    struct choice *choices[3];
    struct paired *next;
};

struct choice{
    int hostel;
    int floor;
};

struct record{
    long int smallRollNo;
    long int largeRollNo;
    struct choice *choices[3];
    struct record *next;
};

struct allocated{
    long int rollNos[2];
    int hostel;
    int floor;
    int room;
    struct allocated *next;
};

struct node *addNode(struct node **head, struct node *back, int val);

void initializeInventory();

int searchRoom(int code, int floor, int room);

int floorExists(int hostel, int floor);

int hostelIndex(char code[]);

void loadRooms(FILE *fp, FILE *fp2);

int takeRoom(int hostel, int floor);

int freeRoomsOnFloor(int hostel, int floor);

void freeInventory();

void freeVacancy(void);

void lottery();

struct allocated *addAllocated(struct allocated **head, struct allocated *tail, int rollNo1, int rollNo2, int hostel, int floor, int checker);

void allocateRooms(struct paired *lotteryArray[], int n, struct allocated **head, struct allocated **tail, struct paired **unallocated, int *unallocatedCount);

void phase2Allocation(struct paired *unallocated[], int unallocatedCount, struct allocated **head, struct allocated **tail);

//stage 2

int sameFloor(const Choice *a, const Choice *b);

const char *statusName(Status s);

int parseChoice(const char *text, Choice *c);

void formatChoice(const Choice *c, char *buf);

int compatibilityScore(const Preference *a, const Preference *b);

void sharedChoices(const Preference *a, const Preference *b, Choice out[NUM_CHOICES]);

int buildUnits(Preference *prefs, int n, unsigned int seed, Unit *out);

int appendUnregistered(Unit *units, int nUnits, long *rolls, int nRolls);

void printUnits(const Unit *units, int n);

int writePairedFile(const Preference *prefs, int n, const Unit *units, int nUnits, const char *path);

void demandReport(const Unit *units, int nUnits);

#endif