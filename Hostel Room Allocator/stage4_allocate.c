#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "commonHeaderFile.h"

struct allocated *addAllocated(struct allocated **head, struct allocated *tail, int rollNo1, int rollNo2, int hostel, int floor, int checker){
    struct allocated *new = malloc(sizeof(struct allocated));
    new -> rollNos[0] = rollNo1;
    new -> rollNos[1] = rollNo2;
    new -> hostel = hostel;
    new -> floor = floor;
    new -> room = checker;
    if ((*head) == NULL){
        (*head) = new;
        tail = new;
        new -> next = NULL;
    }
    else{
        tail -> next = new;
        tail = new;
        new -> next = NULL;
    }
    return tail;
}

void allocateRooms(struct paired *lotteryArray[], int n, struct allocated **head, struct allocated **tail, struct paired **unallocated, int *unallocatedCount){
    (*unallocatedCount) = 0;
    int checker;
    for (int i = 0; i < n; i++){
        checker = 0;
        struct paired *temp = lotteryArray[i];
        for(int j = 0; j < 3; j++){
            struct choice *tempChoice = temp -> choices[j];
            checker = takeRoom(tempChoice -> hostel, tempChoice -> floor);
            if (checker){
                (*tail) = addAllocated(head, *tail, temp -> rollNos[0], temp -> rollNos[1], tempChoice -> hostel, tempChoice -> floor, checker);
                break;
            }
        }
        if (checker){
            continue;
        }
        unallocated[(*unallocatedCount)] = temp;
        (*unallocatedCount) += 1;
    }
    return;
}

void phase2Allocation(struct paired *unallocated[], int unallocatedCount, struct allocated **head, struct allocated **tail){
    for (int i = 0; i < unallocatedCount; i++){
        int checker = 0;
        struct paired *temp = unallocated[i];
        for(int j = 0; j < 3; j++){
            struct choice *tempChoice = temp -> choices[j];
            int hostel = tempChoice -> hostel;
            int floor = tempChoice -> floor;
            int k = 1;
            while (k < 4){
                if (freeRoomsOnFloor(hostel, floor + k)){
                    checker = takeRoom(hostel, floor + k);
                    (*tail) = addAllocated(head, *tail, temp -> rollNos[0], temp -> rollNos[1], hostel, floor + k, checker);
                    break;
                }
                if (freeRoomsOnFloor(hostel, floor - k)){
                    checker = takeRoom(hostel, floor - k);
                    (*tail) = addAllocated(head, *tail, temp -> rollNos[0], temp -> rollNos[1], hostel, floor - k, checker);
                    break;
                }
                k += 1;
            }
            if (checker){
                break;
            }
        }
        if (checker){
                continue;
        }
        for (int hostel = 0; hostel < 3; hostel++){
            int preferredHostel = 0;
            for (int j = 0; j < 3; j++){
                if (temp -> choices[j] -> hostel == hostel){
                    preferredHostel = 1;
                    break;
                }
            }
            if (preferredHostel){
                continue;
            }
            for (int floor = 0; floor < FLOORS; floor++){
                if (freeRoomsOnFloor(hostel, floor)){
                    checker = takeRoom(hostel, floor);
                    if (checker){
                        (*tail) = addAllocated(head, *tail, temp -> rollNos[0], temp -> rollNos[1], hostel, floor, checker);
                        break;
                    }
                }
            }
            if (checker){
                break;
            }
        }
    }
    return;
}

int main(void){
    FILE *fp = fopen("registrations_paired_final", "r");
    if (fp == NULL){
        printf("Error opening registrations_paired_final\n");
        return 1;
    }
    FILE *rooms = fopen("vacant_rooms.txt", "r");
    if (rooms == NULL){
        printf("Error opening vacant_rooms.txt\n");
        fclose(fp);
        return 1;
    }
    FILE *errors = fopen("vacant_rooms_errors.txt", "w");
    if (errors == NULL){
        printf("Error opening vacant_rooms_errors.txt\n");
        fclose(fp);
        fclose(rooms);
        return 1;
    }
    initializeInventory();
    loadRooms(rooms, errors);
    fclose(rooms);
    fclose(errors);
    struct paired **lotteryArray = NULL;
    int n = 0;
    char line[200];
    while (fgets(line, sizeof(line), fp) != NULL){
        long int rollA, rollB;
        char choiceText[3][16];
        if (sscanf(line, "%ld|%ld|%15[^|]|%15[^|]|%15[^|]", &rollA, &rollB, choiceText[0], choiceText[1], choiceText[2]) != 5){
            printf("Skipping invalid lottery-order line: %s", line);
            continue;
        }
        struct paired *new = malloc(sizeof(struct paired));
        if (new == NULL){
            printf("Memory allocation failed\n");
            fclose(fp);
            free(lotteryArray);
            freeInventory();
            return 1;
        }
        new -> rollNos[0] = rollA;
        new -> rollNos[1] = rollB;
        new -> next = NULL;
        for (int i = 0; i < 3; i++){
            new -> choices[i] = NULL;
        }
        int valid = 1;
        for (int i = 0; i < 3; i++){
            new -> choices[i] = malloc(sizeof(struct choice));
            if (new -> choices[i] == NULL){
                valid = 0;
                break;
            }
            char hostelCode[3];
            hostelCode[0] = choiceText[i][0];
            hostelCode[1] = choiceText[i][1];
            hostelCode[2] = '\0';
            int hostel = hostelIndex(hostelCode);
            if (hostel == -1 || choiceText[i][2] < '0' || choiceText[i][2] > '9'){
                valid = 0;
                break;
            }
            new -> choices[i] -> hostel = hostel;
            new -> choices[i] -> floor = choiceText[i][2] - '0';
        }
        if (!valid){
            for (int i = 0; i < 3; i++){
                free(new -> choices[i]);
            }
            free(new);
            printf("Skipping invalid choices for pair %ld, %ld\n", rollA, rollB);
            continue;
        }
        struct paired **temp = realloc(lotteryArray, (n + 1) * sizeof(struct paired *));
        if (temp == NULL){
            for (int i = 0; i < 3; i++){
                free(new -> choices[i]);
            }
            free(new);
            printf("Memory allocation failed\n");
            fclose(fp);
            free(lotteryArray);
            freeInventory();
            return 1;
        }
        lotteryArray = temp;
        lotteryArray[n] = new;
        n++;
    }
    fclose(fp);
    struct allocated *allocatedHead = NULL;
    struct allocated *allocatedTail = NULL;
    struct paired **unallocated = malloc((n > 0 ? n : 1) * sizeof(struct paired *));
    if (unallocated == NULL){
        printf("Memory allocation failed\n");
        free(lotteryArray);
        freeInventory();
        return 1;
    }
    int unallocatedCount = 0;
    allocateRooms(lotteryArray, n, &allocatedHead, &allocatedTail, unallocated, &unallocatedCount);
    phase2Allocation(unallocated, unallocatedCount, &allocatedHead, &allocatedTail);
    FILE *out = fopen("allocated_rooms.txt", "w");
    if (out == NULL){
        printf("Error opening allocated_rooms.txt\n");
    }
    else{
        fprintf(out, "rollNo1|rollNo2|hostel|floor|room\n");
        struct allocated *temp = allocatedHead;
        while (temp != NULL){
            const char *hostel = "??";
            if (temp -> hostel == 0) hostel = "AA";
            else if (temp -> hostel == 1) hostel = "AB";
            else if (temp -> hostel == 2) hostel = "GC";
            fprintf(out, "%ld|%ld|%s|%d|%d\n", temp -> rollNos[0], temp -> rollNos[1], hostel, temp -> floor, temp -> room);
            temp = temp -> next;
        }
        fclose(out);
        printf("Final allocated list written to allocated_rooms.txt\n");
        int allocatedCount = 0;
        struct allocated *countTemp = allocatedHead;
        while (countTemp != NULL){
            allocatedCount++;
            countTemp = countTemp -> next;
        }
        printf("Allocated pairs: %d; pairs sent to phase 2: %d\n", allocatedCount, unallocatedCount);
    }
    struct allocated *allocatedTemp = allocatedHead;
    while (allocatedTemp != NULL){
        struct allocated *next = allocatedTemp -> next;
        free(allocatedTemp);
        allocatedTemp = next;
    }
    for (int i = 0; i < n; i++){
        for (int j = 0; j < 3; j++){	
            free(lotteryArray[i] -> choices[j]);
        }
        free(lotteryArray[i]);
    }
    free(lotteryArray);
    free(unallocated);
    freeInventory();
} 