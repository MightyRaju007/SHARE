#include <stdio.h>
#include "commonHeaderFile.h"

/* Counts how many units put this floor as their first choice. Units with no
   preferences (unregistered students) are not counted. */
static int firstChoiceDemand(const Unit *units, int nUnits, const Choice *floorChoice)
{
    int i, d = 0;
    for (i = 0; i < nUnits; i++) {
        if (units[i].numChoices > 0 && sameFloor(&units[i].choices[0], floorChoice))
            d++;
    }
    return d;
}

/* Prints a table for the admin after the window closes. For every floor it
   shows how many rooms are free and how many units want it first, and flags
   the floors where demand is more than the rooms. Nothing is decided here,
   it is only information. Ends with the overall unit and room totals. */
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

            /* GC has only one floor, so skip the floor numbers it does not have */
            if (!floorExists(hostel, floor))
                continue;

            rooms = freeRoomsOnFloor(hostel, floor);
            floorChoice.floor = floor;
            snprintf(floorChoice.hostel, sizeof(floorChoice.hostel), "%s", hostelCodes[hostel]);
            demand = firstChoiceDemand(units, nUnits, &floorChoice);

            totalRooms += rooms;
            printf("%-8s %-6d %-10d %d", floorChoice.hostel, floor, rooms, demand);
            /* more first choices than rooms means some of them will not get it */
            if (demand > rooms)
                printf("  (over by %d)", demand - rooms);
            printf("\n");
        }
    }

    /* reusing the floor variable as a plain counter here */
    for (floor = 0; floor < nUnits; floor++) {
        if (units[floor].numChoices == 0)
            noPrefs++;
    }

    printf("units %d (%d without preferences), rooms %d\n", nUnits, noPrefs, totalRooms);
    if (nUnits > totalRooms)
        printf("warning: %d more units than rooms\n", nUnits - totalRooms);
}
