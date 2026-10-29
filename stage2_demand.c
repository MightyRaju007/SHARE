#include <stdio.h>
#include "commonHeaderFile.h"

static int firstChoiceDemand(const Unit *units, int nUnits, const Choice *floorChoice)
{
    int i, d = 0;
    for (i = 0; i < nUnits; i++) {
        if (units[i].numChoices > 0 && sameFloor(&units[i].choices[0], floorChoice))
            d++;
    }
    return d;
}

void demandReport(const Unit *units, int nUnits)
{
    const char *hostelCodes[NUM_HOSTELS] = {"AA", "AB", "GC"};
    int hostel, floor, totalRooms = 0, noPrefs = 0;

    printf("\nDemand report\n");
    printf("%-8s %-6s %-10s %s\n", "hostel", "floor", "rooms", "first choice");

    for (hostel = 0; hostel < NUM_HOSTELS; hostel++) {
        for (floor = 0; floor < FLOORS; floor++) {
            int rooms;
            int demand;
            Choice floorChoice;

            if (!floorExists(hostel, floor))
                continue;

            rooms = freeRoomsOnFloor(hostel, floor);
            floorChoice.floor = floor;
            snprintf(floorChoice.hostel, sizeof(floorChoice.hostel), "%s", hostelCodes[hostel]);
            demand = firstChoiceDemand(units, nUnits, &floorChoice);

            totalRooms += rooms;
            printf("%-8s %-6d %-10d %d", floorChoice.hostel, floor, rooms, demand);
            if (demand > rooms)
                printf("  (over by %d)", demand - rooms);
            printf("\n");
        }
    }

    for (floor = 0; floor < nUnits; floor++) {
        if (units[floor].numChoices == 0)
            noPrefs++;
    }

    printf("units %d (%d without preferences), rooms %d\n", nUnits, noPrefs, totalRooms);
    if (nUnits > totalRooms)
        printf("warning: %d more units than rooms\n", nUnits - totalRooms);
}
