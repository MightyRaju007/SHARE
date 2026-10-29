#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "commonHeaderFile.h"

struct record *addRecord(struct record **head, struct record *tail, long int rollNo, long int roomateRollNo, struct choice *choices[]){
    struct record *new = malloc(sizeof(struct record));
    if (rollNo < roomateRollNo){
        new -> smallRollNo = rollNo;
        new -> largeRollNo = roomateRollNo;
    }
    else{
        new -> smallRollNo = roomateRollNo;
        new -> largeRollNo = rollNo;
    }
    for (int i = 0; i < 3; i++){
        new -> choices[i] = choices[i];
    }
    if ((*head) == NULL){
        (*head) = new;
        tail = new;
        new -> next = NULL;
        return tail;
    }
    tail -> next = new;
    tail = new;
    new -> next = NULL;
    return tail;
}

int hostelChoice(char choice[]){
    if (choice[0] == 'A'){
        if (choice[1] == 'A'){
            return 0;
        }
        else if (choice[1] == 'B'){
            return 1;
        }
    }
    else if (choice [0] == 'G'){
        if (choice[1] == 'C'){
            return 2;
        }
    }
    return -1;
}

char *hostelName(int hostel){
    if (hostel == 0){
        return "AA";
    }
    else if (hostel == 1){
        return "AB";
    }
    else if (hostel == 2){
        return "GC";
    }
    return "??";
}

void splitList(struct record *head, struct record **left, struct record **right){
    struct record *slow = head;
    struct record *fast = head -> next;
    while (fast != NULL){
        fast = fast -> next;
        if (fast != NULL){
            slow = slow -> next;
            fast = fast -> next;
        }
    }
    *left = head;
    *right = slow -> next;
    slow -> next = NULL;
}

struct record *merge(struct record *left, struct record *right){
    struct record *head = NULL;
    struct record *tail = NULL;
    while ((left != NULL) && (right != NULL)){
        struct record *temp;
        if ((left -> smallRollNo < right -> smallRollNo) ||
            ((left -> smallRollNo == right -> smallRollNo) &&
             (left -> largeRollNo <= right -> largeRollNo))){

            temp = left;
            left = left -> next;
        }
        else{
            temp = right;
            right = right -> next;
        }
        if (head == NULL){
            head = temp;
            tail = temp;
        }
        else{
            tail -> next = temp;
            tail = temp;
        }
    }
    if (left != NULL){
        tail -> next = left;
    }
    else if (right != NULL){
        tail -> next = right;
    }
    return head;
}

struct record *mergeSort(struct record *head){
    if ((head == NULL) || (head -> next == NULL)){
        return head;
    }
    struct record *left = NULL;
    struct record *right = NULL;
    splitList(head, &left, &right);
    left = mergeSort(left);
    right = mergeSort(right);
    return merge(left, right);
}

struct paired *createPairs(struct record *head){
    struct paired *pairedHead = NULL;
    struct paired *pairedTail = NULL;
    while (head != NULL){
        struct record *first = head;
        struct record *second = head -> next;
        if (second == NULL){
            printf("Unmatched record: %ld, %ld\n",first -> smallRollNo, first -> largeRollNo);
            break;
        }
        if ((first -> smallRollNo != second -> smallRollNo) || (first -> largeRollNo != second -> largeRollNo)){
            printf("Missing partner for pair: %ld, %ld\n", first -> smallRollNo, first -> largeRollNo);
            head = head -> next;
            continue;
        }
        struct paired *new = malloc(sizeof(struct paired));
        new -> rollNos[0] = first -> smallRollNo;
        new -> rollNos[1] = first -> largeRollNo;
        for (int i = 0; i < 3; i++){
            new -> choices[i] = first -> choices[i];
        }
        new -> next = NULL;
        if (pairedHead == NULL){
            pairedHead = new;
            pairedTail = new;
        }
        else{
            pairedTail -> next = new;
            pairedTail = new;
        }
        head = second -> next;
    }
    return pairedHead;
}

void swap(struct paired **i, struct paired **j){
    struct paired *temp = *j;
    *j = *i;
    *i = temp;
    return;
}

void lottery(){
    srand(time(NULL));
    FILE *fp = fopen("registrations_paired.txt", "r");
    char line[200];
    int lineNo = 0;
    struct choice *choices[3];
    struct record *head = NULL;
    struct record *tail = NULL;
    while (fgets(line, sizeof(line), fp) != NULL){
        lineNo += 1;
        char *ptr = strchr(line, '|');
        *ptr = '\0';
        long int rollNo = strtol(line, NULL, 10);
        ptr = ptr + 1;
        ptr = strchr(ptr, '|');
        *ptr = '\0';
        for (int i = 0; i < 3; i++){
            char *choice = ptr + 1;
            ptr = strchr(choice, '|');
            *ptr = '\0';
            int hostel = hostelChoice(choice);
            int floor = choice[2] - '0';
            struct choice *new = malloc(sizeof(struct choice));
            new -> hostel = hostel;
            new -> floor = floor;
            choices[i] = new;
        }
        ptr = ptr + 1;
        ptr = strchr(ptr, '|');
        long int roomateRollNo = strtol(ptr + 1, NULL, 10);
        tail = addRecord(&head, tail, rollNo, roomateRollNo, choices);
    }
    fclose(fp);
    head = mergeSort(head);
    struct paired *pairedHead = createPairs(head);
    int nodeNumber = 0;
    struct paired *temp = pairedHead;
    while (temp != NULL){
        nodeNumber += 1;
        temp = temp -> next;
    }
    struct paired *arr[nodeNumber];
    temp = pairedHead;
    for (int i = 0; i < nodeNumber; i++){
        arr[i] = temp;
        temp = temp -> next;
    }
    for (int i = nodeNumber - 1; i > 0; i--){
        int j = rand() % (i+1);
        swap(&arr[i], &arr[j]);
    }
    FILE *fp2 = fopen("registrations_paired_final", "w");
    if (fp2 == NULL){
    printf("Error opening output file\n");
    return;
    }
    temp = pairedHead;
    for (int j = 0; j < nodeNumber; j++){
        fprintf(fp2, "%ld|%ld", arr[j] -> rollNos[0], arr[j] -> rollNos[1]);
        for (int i = 0; i < 3; i++){
            fprintf(fp2, "|%s%d", hostelName(arr[j] -> choices[i] -> hostel),arr[j] -> choices[i] -> floor);
        }
        fprintf(fp2, "\n");
    }
    fclose(fp2);
}

int main(){
    lottery();
}