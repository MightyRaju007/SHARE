/*
 * Stage 2: load Stage 1 files, build paired/solo units, report demand,
 * and write the paired records for Stage 3.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "commonHeaderFile.h"

/* input from Stage 1 and output for Stage 3 are the same file, it is rewritten in place */
#define REGISTRATION_FILE "registrations_paired.txt"
#define UNREGISTERED_FILE "NotRegisteredStudent.txt"
#define VACANT_ROOMS_FILE "vacant_rooms.txt"
#define INVENTORY_ERRORS_FILE "inventory_errors.txt"
#define PAIRED_FILE "registrations_paired.txt"

/* the three big arrays live here rather than on the stack */
Preference prefs[MAX_STUDENTS];
Unit units[MAX_UNITS];
long unregistered[MAX_STUDENTS];

/* Reads the two files Stage 1 produced. The registration file gives one
   student per line with roll, name, three choices, status and roommate, and
   the unregistered file gives the rolls of students who never registered.
   Every line is checked and the whole load stops with -1 on the first bad
   one, since a half loaded list is worse than none. Returns the number of
   registrations read, and the unregistered count through numUnregistered. */
int loadStage1Files(Preference *out, int maxOut,
                    long *unregisteredRolls, int maxUnreg, int *numUnregistered)
{
    FILE *fp;
    char line[512];
    int nPrefs = 0;

    *numUnregistered = 0;
    fp = fopen(REGISTRATION_FILE, "r");
    if (fp == NULL) {
        fprintf(stderr, "cannot open %s\n", REGISTRATION_FILE);
        return -1;
    }

    while (fgets(line, sizeof(line), fp) != NULL) {
        /* one line has exactly seven fields separated by | */
        char *fields[7];
        char *p = line;
        char *end;
        long roll, roommateRoll = 0;
        int i, count = 0;
        Preference pref;

        /* drop the newline, works for both Unix and Windows line endings */
        line[strcspn(line, "\r\n")] = '\0';
        fields[count++] = p;
        /* split the line in place by turning each | into a string terminator */
        while ((p = strchr(p, '|')) != NULL && count < 7) {
            *p = '\0';
            fields[count++] = ++p;
        }
        if (count != 7) {
            fprintf(stderr, "invalid registration record: %s\n", line);
            fclose(fp);
            return -1;
        }
        if (nPrefs >= maxOut) {
            fprintf(stderr, "too many registrations\n");
            fclose(fp);
            return -1;
        }

        /* end must land on the terminator, otherwise the roll had junk in it */
        roll = strtol(fields[0], &end, 10);
        if (fields[0][0] == '\0' || *end != '\0') {
            fprintf(stderr, "invalid roll number in registration record\n");
            fclose(fp);
            return -1;
        }

        memset(&pref, 0, sizeof(pref));
        pref.rollNo = roll;
        snprintf(pref.name, sizeof(pref.name), "%s", fields[1]);
        for (i = 0; i < NUM_CHOICES; i++) {
            if (!parseChoice(fields[i + 2], &pref.choices[i])) {
                fprintf(stderr, "invalid preference for roll number %ld\n", roll);
                fclose(fp);
                return -1;
            }
        }

        if (strcmp(fields[5], "RANDOM") == 0)
            pref.status = STATUS_RANDOM;
        else if (strcmp(fields[5], "PENDING") == 0)
            pref.status = STATUS_PENDING;
        else if (strcmp(fields[5], "LOCKED") == 0)
            pref.status = STATUS_LOCKED;
        else {
            fprintf(stderr, "invalid status for roll number %ld\n", roll);
            fclose(fp);
            return -1;
        }

        /* the roommate column is empty for random students */
        if (fields[6][0] != '\0') {
            roommateRoll = strtol(fields[6], &end, 10);
            if (*end != '\0') {
                fprintf(stderr, "invalid roommate roll number for %ld\n", roll);
                fclose(fp);
                return -1;
            }
        }
        pref.roommateRoll = roommateRoll;
        out[nPrefs++] = pref;
    }
    fclose(fp);

    /* second file, just a roll number per line */
    fp = fopen(UNREGISTERED_FILE, "r");
    if (fp == NULL) {
        fprintf(stderr, "cannot open %s\n", UNREGISTERED_FILE);
        return -1;
    }
    while (fgets(line, sizeof(line), fp) != NULL) {
        char *end;
        long roll = strtol(line, &end, 10);
        if (end == line || (*end != '|' && *end != '\0' && *end != '\r' && *end != '\n')) {
            fprintf(stderr, "invalid unregistered student record: %s", line);
            fclose(fp);
            return -1;
        }
        if (*numUnregistered >= maxUnreg) {
            fprintf(stderr, "too many unregistered students\n");
            fclose(fp);
            return -1;
        }
        unregisteredRolls[(*numUnregistered)++] = roll;
    }
    fclose(fp);
    return nPrefs;
}

/* Opens the vacant rooms file and an errors file, then hands both to the
   inventory module to load. Returns 1 if everything opened, 0 otherwise. */
static int initializeRoomInventory(void)
{
    FILE *rooms = fopen(VACANT_ROOMS_FILE, "r");
    FILE *errors = fopen(INVENTORY_ERRORS_FILE, "w");

    if (rooms == NULL) {
        fprintf(stderr, "cannot open %s\n", VACANT_ROOMS_FILE);
        if (errors != NULL) fclose(errors);
        return 0;
    }
    if (errors == NULL) {
        fprintf(stderr, "cannot open %s\n", INVENTORY_ERRORS_FILE);
        fclose(rooms);
        return 0;
    }

    initializeInventory();
    loadRooms(rooms, errors);
    fclose(rooms);
    fclose(errors);
    return 1;
}

/* Stage 2 from start to finish. Load the Stage 1 files, turn registrations
   into units, add the students who never registered, print the units and
   the demand report, then write the paired file for Stage 3 to pick up. */
int main(void)
{
    int nPrefs, nUnreg = 0, nUnits, i, pairs = 0, solos = 0;

    nPrefs = loadStage1Files(prefs, MAX_STUDENTS, unregistered, MAX_STUDENTS, &nUnreg);
    if (nPrefs < 0)
        return 1;
    printf("registrations %d, never registered %d\n", nPrefs, nUnreg);

    /* the seed argument is unused in this stage, the lottery is Stage 3 */
    nUnits = buildUnits(prefs, nPrefs, 1, units);
    for (i = 0; i < nUnits; i++) {
        if (units[i].solo)
            solos++;
        else
            pairs++;
    }
    printf("units %d (%d pairs, %d solo)\n\n", nUnits, pairs, solos);
    /* after the counts so the pairs and solos line refers only to registered students */
    nUnits = appendUnregistered(units, nUnits, unregistered, nUnreg);

    printUnits(units, nUnits);
    if (!initializeRoomInventory())
        return 1;
    demandReport(units, nUnits);

    if (!writePairedFile(prefs, nPrefs, units, nUnits, PAIRED_FILE))
        return 1;
    printf("\nwritten %s for Stage 3\n", PAIRED_FILE);
    freeInventory();
    return 0;
}
