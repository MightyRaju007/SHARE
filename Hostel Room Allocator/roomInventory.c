#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "commonHeaderFile.h"
//AA = 0, AB = 1, GC = 2 (hostel codes)


static struct node *hostelInventoryFront[3][FLOORS];
static struct node *hostelInventoryBack[3][FLOORS];

struct node *addNode(struct node **head, struct node *back, int val){
    struct node *new = malloc(sizeof(struct node));
    if (new == NULL){
        printf("Node memory allocation failed.\n");
        return NULL;
    }
    new -> val = val;
    if ((*head) == NULL){
        (*head) = malloc(sizeof(struct node));
        if ((*head) == NULL){
            printf("Node memory allocation failed.\n");
            return NULL;
        }
        (*head) -> val = 1;
        (*head) -> next = new;
        back = new;
        new -> next = NULL;
        return back;
    }
    (*head) -> val += 1;
    back -> next = new;
    back = new;
    new -> next = NULL;
    return back;
}

int searchRoom(int code, int floor, int room){
    struct node *temp = hostelInventoryFront[code][floor];
    if (temp == NULL){
        return 0;
    }
    temp = temp -> next;
    while(temp != NULL){
        if(temp -> val == room){
            return 1;
        }
        temp = temp -> next;
    }
    return 0;
}

int floorExists(int hostel, int floor){
    if ((hostel > -1) && (hostel < 3)){
        if ((floor > -1) && (floor < FLOORS)){
            if(hostelInventoryFront[hostel][floor] != NULL){
                return 1;
            }
        }
    }
    return 0;
}

int hostelIndex(char code[]){
    if (strcmp(code, "AA") == 0){
        return 0;
    }
    else if (strcmp(code, "AB") == 0){
        return 1;
    }
    else if (strcmp(code, "GC") == 0){
        return 2;
    }
    return -1;
}

void loadRooms(FILE *fp, FILE *fp2){
    char line[60];
    int lineNo = 0;
    while (fgets(line, sizeof(line), fp) != NULL){
        lineNo += 1;
        char code[8];
        int floor, room;
        int n = sscanf(line, "%7[^,],%d, %d", code, &floor, &room);
        if ((line[0] == '#') || (line[0] == '\n')){
            fprintf(fp2, "vacant_rooms.txt line %d: comment or blank line\n", lineNo);
            continue;
        }
        if ((n==3) && (floor > -1) && (floor < FLOORS) &&(room > 0)){
            int hostelNumber = hostelIndex(code);
            if (hostelNumber != -1){
                if (!searchRoom(hostelNumber, floor, room)){
                    struct node *temp = addNode(&hostelInventoryFront[hostelNumber][floor], hostelInventoryBack[hostelNumber][floor], room);
                    if (temp != NULL){
                        hostelInventoryBack[hostelNumber][floor] = temp;
                    }
                }
                else{
                    fprintf(fp2, "vacant_rooms.txt line %d: duplicate rooms\n", lineNo);
                }
            }
            else{
                fprintf(fp2, "vacant_rooms.txt line %d: bad format\n", lineNo);
            }
        }
        else{
            fprintf(fp2, "vacant_rooms.txt line %d: bad format\n", lineNo);
        }
    }
}

int takeRoom(int hostel, int floor){
    if(!floorExists(hostel, floor)){
        return 0;
    }
    struct node *temp = hostelInventoryFront[hostel][floor];
    struct node *allocation = temp -> next;
    if (allocation == NULL){
        return 0;
    }
    temp -> next = allocation -> next;
    temp -> val -= 1;
    if (temp -> next == NULL){
        hostelInventoryBack[hostel][floor] = NULL;
    }
    int roomNo = allocation -> val;
    free(allocation);
    return roomNo;
}

int freeRoomsOnFloor(int hostel, int floor){
    if (floorExists(hostel, floor)){
        if(hostelInventoryFront[hostel][floor] != NULL){
            return hostelInventoryFront[hostel][floor] -> val;
        }
    }
    return 0;
}

void freeInventory(){
    for (int i = 0; i < 3; i++){
        for (int j = 0; j < FLOORS; j++){
            struct node *temp = hostelInventoryFront[i][j];
            while (temp != NULL){
                struct node *next = temp -> next;
                free(temp);
                temp = next;
            }
            hostelInventoryFront[i][j] = NULL;
            hostelInventoryBack[i][j] = NULL;
        }
    }
}

void initializeInventory(){
    for (int i = 0; i < 3; i++){
        for (int j = 0; j < FLOORS; j++){
            hostelInventoryFront[i][j] = NULL;
            hostelInventoryBack[i][j] = NULL;
        }
    }
}




void freeVacancy(void){
    char *hostelCodes[3] = {"AA", "AB", "GC"};

    printf("\nAvailable vacant rooms:\n");
    for (int hostel = 0; hostel < 3; hostel++){
        for (int floor = 0; floor < FLOORS; floor++){
            int rooms = freeRoomsOnFloor(hostel, floor);
            if (rooms > 0){
                printf("%s floor %d: %d rooms\n", hostelCodes[hostel], floor, rooms);
            }
        }
    }
}
