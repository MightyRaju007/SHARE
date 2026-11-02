#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "commonHeaderFile.h"
//AA = 0, AB = 1, GC = 2 (hostel codes)


static struct node *hostelInventoryFront[3][FLOORS]; //array of all rooms in the format of linked lists (per floor)
static struct node *hostelInventoryBack[3][FLOORS];

struct node *addNode(struct node **head, struct node *back, int val){ //addition of a node to linked list
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
        (*head) -> val = 1; //the first node of every linked list (per floor) points to the number of available free rooms per floor
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

int searchRoom(int code, int floor, int room){ //searching in the linked list corresponding to the hostel code, floor and room
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

int floorExists(int hostel, int floor){ //floors should range between 0 to 3
    if ((hostel > -1) && (hostel < 3)){
        if ((floor > -1) && (floor < FLOORS)){
            if(hostelInventoryFront[hostel][floor] != NULL){
                return 1;
            }
        }
    }
    return 0;
}

int hostelIndex(char code[]){ //converting hostel codes to there integer equivalents
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

void loadRooms(FILE *fp, FILE *fp2){ //loading all the rooms listed in the vacant_rooms.txt text file
    char line[60]; //each line storing buffer
    int lineNo = 0; //for error, comment or blank line detection logging
    while (fgets(line, sizeof(line), fp) != NULL){
        lineNo += 1;
        char code[8]; //hostel code (in char)
        int floor, room; 
        int n = sscanf(line, "%7[^,],%d, %d", code, &floor, &room); //reading with comma as a delimiter
        if ((line[0] == '#') || (line[0] == '\n')){ //if it is a comment or blank line we log it with line number
            fprintf(fp2, "vacant_rooms.txt line %d: comment or blank line\n", lineNo);
            continue;
        }
        if ((n==3) && (floor > -1) && (floor < FLOORS) &&(room > 0)){ //the number of fields read per line must be 3 
            int hostelNumber = hostelIndex(code); //conversion to integer
            if (hostelNumber != -1){ //fail check
                if (!searchRoom(hostelNumber, floor, room)){ //if the room isnt already added we add it to the inventory 
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

int takeRoom(int hostel, int floor){ //reserving a room for a candidate pair
    if(!floorExists(hostel, floor)){ //input check
        return 0;
    }
    struct node *temp = hostelInventoryFront[hostel][floor];
    struct node *allocation = temp -> next; //the first node points to the number of free rooms on the paticular floor
    if (allocation == NULL){ //empty list
        return 0;
    }
    temp -> next = allocation -> next; //deletion of the allocated room node
    temp -> val -= 1;
    if (temp -> next == NULL){ //if the allocated room was the last available room in the list 
        hostelInventoryBack[hostel][floor] = NULL;
    }
    int roomNo = allocation -> val; //returning the allocation result
    free(allocation);
    return roomNo;
}

int freeRoomsOnFloor(int hostel, int floor){ //first node of linked list
    if (floorExists(hostel, floor)){
        if(hostelInventoryFront[hostel][floor] != NULL){
            return hostelInventoryFront[hostel][floor] -> val;
        }
    }
    return 0;
}

void freeInventory(){ //frees the memory 
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

void initializeInventory(){ //sets the initial nodes to NULL to avoid garbage values
    for (int i = 0; i < 3; i++){
        for (int j = 0; j < FLOORS; j++){
            hostelInventoryFront[i][j] = NULL;
            hostelInventoryBack[i][j] = NULL;
        }
    }
}

void freeVacancy(void){ //prints all the number of available rooms per floor in each hostel
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
