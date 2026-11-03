#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <stdbool.h>
#include <string.h>
#include "commonHeaderFile.h"
#define RANDOM STATUS_RANDOM
#define PENDING STATUS_PENDING
#define LOCKED STATUS_LOCKED



#define Rollsize 9
#define Namesize 30
#define hashsize 100
#define hostelcodesize 3

bool registeration_window_open = true;

typedef struct Student{

        unsigned long rollno;
        char name[Namesize];
        unsigned long roomaterollno;
        Status status;
        bool registered;
        Choice choice[3];
        struct Student *hnext;
        struct Student *next; 

}Student;


// Linked List that store student database linealy.
Student *head=NULL;

// Hash table implementation
Student *buckets[hashsize];

// Hash functrion that return key of hash table.
int hash(unsigned long int n){

        return n%hashsize;

}

// Hash searching function to find if a student is in hash table or not.
Student* hashsearching(unsigned long int roll){

         int idx=hash(roll);
         Student* s = buckets[idx];

         while(s!=NULL){
                if(s->rollno == roll){
                    return s;
                }
                s=s->hnext;
        }

        return NULL;

}

// Insert student into hash table and linked list.
void addstudent(unsigned long int roll,const char* name){

        Student* s=hashsearching(roll);

        // Student s already present in Hash table.
        if(s!=NULL){
            return;
        }

        int idx=hash(roll);

        Student* newstudent = calloc(1,sizeof(Student));
        newstudent->rollno=roll;
        snprintf(newstudent->name,Namesize,"%s",name);
        newstudent->status=RANDOM;
        newstudent->registered=false;

        // Insert student into Hash table.
        Student* temp=buckets[idx];
        newstudent->hnext=temp;
        buckets[idx]=newstudent;

        // Insert student into Linked List.
        newstudent->next=head;
        head=newstudent;

}

// Helper function to change status to string
const char *statusToString(Status status){
    switch(status){
        case RANDOM :
            return "RANDOM";
        case PENDING :
            return "PENDING";
        case LOCKED :
            return "LOCKED";
        default : 
            return "RANDOM";
    }
} 

// Reverse of previous function
bool stringToStatus(const char *str,Status* status){
    if(strcmp(str,"RANDOM")==0){
        *status = RANDOM;
    }
    else if(strcmp(str,"PENDING")==0){
        *status = PENDING;
    }
    else if(strcmp(str,"LOCKED")==0){
        *status = LOCKED;
    }
    else{
        return false;
    }
    return true;
}

// Load data of students who are eligible from eligiblity file
void load_data(){

        FILE *fptr = fopen("EligibleStudentfile.txt","r");

        if(fptr==NULL){
            printf("Please provide a list of eligible students to continue.\n");
            return;
        }

        char line[100];

        while(fgets(line,sizeof(line),fptr)!=NULL){
            unsigned long int roll;
            char name[Namesize];

            // sacn line in format Rollno | Name
            int n = sscanf(line,"%lu|%29[^\n]",&roll,name);

            if(n!=2){
                continue;
            }

            addstudent(roll,name);
    
        }

        fclose(fptr);


}

// You entered three choice in preference order.
// Check Is this choices valid or not.
bool HostelValidation(Choice choice[],int n){
        
        for(int i=0;i<3;i++){
            // Incorrect hostel name code.
            if(strcmp(choice[i].hostel,"AA")!=0 && strcmp(choice[i].hostel,"AB")!=0 && strcmp(choice[i].hostel,"GC")!=0){
                    printf("Hostel %d code name is incorrect.\n",i+1);
                    return false;
            }

            int hostel = hostelIndex(choice[i].hostel);
            int floor = choice[i].floor;

            // Entered floor does not exits.
            if(!floorExists(hostel,floor)){
                    printf("Hostel %s and floor %d does not exist.\n",choice[i].hostel,choice[i].floor);
                    return false;
            }
            // Free rooms on one of the entered floor does not exits.
            if(freeRoomsOnFloor(hostel,floor)<=0){
                    printf("Hostel %s and floor %d exists but has zero vacant room.\n",choice[i].hostel,choice[i].floor);
                    return false; 
            }

            // Same (hostel,floor) combination entered more than once.
            for(int j=0;j<i;j++){
                if(strcmp(choice[i].hostel,choice[j].hostel)==0 && choice[i].floor==choice[j].floor){
                    printf("The same hostel %s and floor %d is chosen twice.\n",choice[i].hostel,choice[i].floor);
                    return false;
                }
            }
        }
      
        return true;

}

// Helper function to input a string.
void inputstring(char* s,int n){

        if(fgets(s,n,stdin)==NULL){
            s[0]='\0';
            return;
        }

        // Replaced new line to null.
        s[strcspn(s,"\n")]='\0';
        return;

}

// Function that take hostel choices as input. 
void inputhostelchoice(Choice choice[],int n){
    
        while(1){
            for(int i=0;i<3;i++){
                char a[hostelcodesize];
                int f;
                // Print which hostel and floor have vacant rooms.
                freeVacancy();
                printf("\n1 for Aquamarine A\n2 for Aquamarine B\n3 for Garnet C\n\n");
                printf("Enter hostel %d code : ",i+1);
                while(1){
                int ch;
                if(scanf("%d",&ch)!=1){
                    int c;
                    while((c = getchar())!='\n' && c!= EOF){
                        printf("INVALID INPUT Enter 1 2 or 3\n");
                        continue;
                    }
                }
                if(ch==1){
                    strcpy(a,"AA");
                    break;
                }
                else if(ch==2){
                    strcpy(a,"AB");
                    break;
                }
                else if(ch==3){
                    strcpy(a,"GC");
                    break;
                }
                else{
                    printf("INVALID INPUT\n");
                    printf("\n1 for Aquamarine A\n2 for Aquamarine B\n3 for Garnet C\n\n");
                    printf("Enter hostel %d code : ",i+1);
                }
                }
                printf("Enter floor : ");
                scanf("%d",&f);

                strcpy(choice[i].hostel,a);
                choice[i].floor=f;
    
            }

            // Check whether entered hostel choice is valid or not.
            if(HostelValidation(choice,3)){
                 break;
            }

            // It will coutinue till you do not enter correct choice.
            printf("Please enter all the preferences again.\n");

        }
        printf("Hostel preferences successfully submitted.\n");

}


// Check your status, update your status according to your current roomate status.
bool setroomate(unsigned long int x,unsigned long int y){

        // Enter your rollno in roomate roll no. 
        if(x==y){
            printf("You cannot choose yourself as roomate.\n");
            return false;
        }

        // Student s is the roomate.
        Student* s = hashsearching(y);

        // Enter roomate is not eligible.
        if(s==NULL){
            printf("Entered roomate not in eligible list.\n");
            return false;
        }

        // If s has not registered yet,then your current status is PENDING.
        if(!s->registered){
            // Here s1 is student itself.
            Student *s1=hashsearching(x);
            s1->status=PENDING;
            s1->roomaterollno=y;
            printf("Waiting for y to select you as roomate.\n");
            return true;
        }

        // If s is already LOCKED with some other student,then immediatly return.
        // You have to enter your roomate rollno again.
        if(s->status == LOCKED && s->roomaterollno!=x){
            printf("Entered roomate is already locked.\n");
            return false;
        }

        // If student s also registered s1 as his desire roomate.
        // Then locked both the roomate.
        // The pair now shared s(roomate) preference list.
        if(s->roomaterollno == x){
            s->status = LOCKED;
            Student* s1=hashsearching(x);
            s1->status = LOCKED;
            s1->roomaterollno=y;
            // Copy roomate preference list to s prefernce list.
            for(int i=0;i<3;i++){
                strcpy(s1->choice[i].hostel,s->choice[i].hostel);
                s1->choice[i].floor=s->choice[i].floor;
            }
            return true;
        }

        // If s status is either RANDOM or (PENDING with roomaterollno is not s)
        // Then choices saved and waiting for roomate select you.
        if(s->status == RANDOM || (s->status == PENDING && s->roomaterollno!=x)){
            Student* roomate=hashsearching(x);
            roomate->status = PENDING;
            roomate->roomaterollno = y;
            printf("Roomate request is RANDOM or PENDING.\n");
            printf("Waiting for roomate select you.\n");
            return true;
        }


        return false;

}

// Release your old roomate
// If s edit his sumbission,then we must release his old roomate.
void releaseoldroomate(Student *s){

        // If s is LOCKED with someone,then update his roomate status to PENDING.
        if(s->status == LOCKED){
              unsigned long int y=s->roomaterollno;
              Student* s2 = hashsearching(y);
              if(s2 != NULL){
                 s2->status=PENDING;
              }

        }      
              // Set s->status to RANDOM and rollno to 0.
              s->status = RANDOM;
              s->roomaterollno=0;
              return;

}

// Register a student when a registeration window is open.
void registerStudent(){
        if(!registeration_window_open){
            printf("Registration window is closed :(, apply next year.\n");
            return;
        }


        // Input rollno from the user.
        unsigned long int roll;
        printf("Enter roll number (9 digits) : ");
        scanf("%lu",&roll);

        // Search that entered rollno is in hash or not. 
        // If entered rollno is not in hash,then student is not eligible.
        Student *s = hashsearching(roll);
        if(s == NULL){
            printf("Entered roll number is not eligible.\n");
            return;
        }

        // If s is editing his previous sumbission,then release his old roomate.
        if(s->registered){
             releaseoldroomate(s);
        }

        // 1 for RANDOM roomate, 2 for enter roomate roll no.
        int ch;
        printf("Enter 1 for RANDOM ROOMATE, and 2(or any) for ROOMATE ROLL NUMBER: ");
        scanf("%d",&ch);

        

        if(ch!=1){
            unsigned long int roomroll;
            printf("Enter roomate roll number (%d digits) : ",Rollsize);
            scanf("%lu",&roomroll);

            while(!setroomate(roll,roomroll)){
                printf("Enter roomate roll number again.\n");
                if(scanf("%lu",&roomroll)!=1){
                    printf("Invalid input.\n");
                    return;
                }
            
            }

        }

        // If s->status is locked,and do not want change his roomate preference list
        // Then save his registration.
        if(s->status == LOCKED){
                printf("You are locked with your desire roomate.\n");
                printf("Do you want to change the hostel preferences for you and your roomate.\n");
                int change;
                printf("Press 1 to change preferences or anyother key to continue without changing.\n");
                scanf("%d",&change);
                if(change!=1){
                    Student* s1 = hashsearching(s->roomaterollno);
                    for(int i=0;i<3;i++){
                        strcpy(s->choice[i].hostel,s1->choice[i].hostel);
                        s->choice[i].floor = s1->choice[i].floor;

                    }
                    printf("Registration is completed successfully.\n");
                    s->registered=true; // Update registration status to TRUE.
                    return;
                }
        }
        
        // If status is either PENDING or (LOCKED with updating roomate preference list)
        // Then enter preference list.
        Choice c[3];
        printf("\nEntered hostel and floor in higher priority order.\n");
        inputhostelchoice(c,3);

        for(int i=0;i<3;i++){
            strcpy(s->choice[i].hostel,c[i].hostel);
            s->choice[i].floor = c[i].floor;
        }

        if(s->status == LOCKED){
            Student* s1 = hashsearching(s->roomaterollno);
                    for(int i=0;i<3;i++){
                        strcpy(s1->choice[i].hostel,s->choice[i].hostel);
                        s1->choice[i].floor = s->choice[i].floor;

                    }
        }

        s->registered=true;
        printf("Registration successfully completed.\n");

}

// Save data of registered student.
void write_data(){
        FILE *fptr = fopen("registeredstudentdata.txt","w");
        FILE *lotteryFile = fopen("registrations_paired.txt","w");

        if(fptr == NULL){
            printf("Error opening registeredstudentdata.txt.\n");
        }
        if(lotteryFile == NULL){
            printf("Error opening registrations_paired.txt.\n");
        }
        if(fptr == NULL && lotteryFile == NULL){
            return;
        }

        Student* s = head;
        while(s != NULL){
            // Register data of all student who have registered.
            if(s->registered){
                // Keep Person 1's saved format for loading registrations back.
                if(fptr != NULL){
                    fprintf(fptr,"%lu|%s|%s|%d|%s|%d|%s|%d|%s|%lu|%s\n",s->rollno,s->name,
                        s->choice[0].hostel,s->choice[0].floor,s->choice[1].hostel,s->choice[1].floor,s->choice[2].hostel,s->choice[2].floor,
                        statusToString(s->status),s->roomaterollno," ");
                }

                // Lottery input: roll|name|AA1|AB2|GC3|STATUS|roommate
                if(lotteryFile != NULL){
                    fprintf(lotteryFile,"%lu|%s|%s%d|%s%d|%s%d|%s|%lu\n",s->rollno,s->name,
                        s->choice[0].hostel,s->choice[0].floor,
                        s->choice[1].hostel,s->choice[1].floor,
                        s->choice[2].hostel,s->choice[2].floor,
                        statusToString(s->status),s->roomaterollno);
                }
            }
            s=s->next;
        }

        if(fptr != NULL){
            fclose(fptr);
        }
        if(lotteryFile != NULL){
            fclose(lotteryFile);
        }
        printf("Registered data saved.\n");

}


//load data of registered student from registerstudentdata file
void loadregistereddata(){
        FILE *fptr = fopen("registeredstudentdata.txt","r");

        if(fptr == NULL){
            printf("File does not exits.\n");
            return;
        }

        // Declare a line that take scan a line from file
        char line[180];

        // Till line is not NULL
        while(fgets(line,sizeof(line),fptr)!=NULL){

            unsigned long int roll;
            char name[Namesize];
            char statusText[10];
            unsigned long int roomroll;
            char hn0[hostelcodesize];
            char hn1[hostelcodesize];
            char hn2[hostelcodesize];
            int f0,f1,f2;

            // Scan from registerstudentdata file in order Roll no | Name |  H1 | F1 | H2 | F2 | H3 | F3 | Status | Roomaterollno 
            int n = sscanf(line,"%lu|%29[^|]|%2[^|]|%d|%2[^|]|%d|%2[^|]|%d|%9[^|]|%lu",&roll,name,hn0,&f0,hn1,&f1,hn2,&f2,statusText,&roomroll);

            // If scan entries is not 10,then skip that line.
            if(n!=10){
                continue;
            }

            // Search roll in hash,if finds name it as Student s.
            Student* s = hashsearching(roll);
            if(s == NULL){
                continue;
            }

            // Load value like registered,status,choices in hash.
            if(!stringToStatus(statusText,&s->status)){
                continue;
            }
            s->registered = true;
            s->roomaterollno = roomroll;
            strcpy(s->choice[0].hostel,hn0);
            s->choice[0].floor = f0;
            strcpy(s->choice[1].hostel,hn1);
            s->choice[1].floor = f1;
            strcpy(s->choice[2].hostel,hn2);
            s->choice[2].floor = f2;


        }

        fclose(fptr);

}

// Helper function to print string.
void printstring(char *a,int n){
            int i=0;
            while(a[i]!='\0' && i<n){
                printf("%c",a[i]);
                i++;
            }
            printf("\n");
}

// Display my submission
void viewmysubmission(){

            unsigned long int roll;
            printf("Enter rollno (%d digits) : ",Rollsize);
            scanf("%lu",&roll);

            Student* s = hashsearching(roll);
            // If s is NULL,then entered rollno is not in eligible list.
            if(s == NULL){
                printf("%lu is not an eligible roll number\n",roll);
                return;
            }

            // If rollno has not registered yet.
            if(!s->registered){
                printf("%lu has not registered yet.\n",s->rollno);
                return;
            }

            printf("Student Rollno : %lu\n",s->rollno);
            printf("Student name : ");
            printstring(s->name,Namesize);

            printf("Hostel 1 : ");
            printstring(s->choice[0].hostel,hostelcodesize);
            printf("Hostel 1's floor : %d\n",s->choice[0].floor);

            printf("Hostel 2 : ");
            printstring(s->choice[1].hostel,hostelcodesize);
            printf("Hostel 2's floor : %d\n",s->choice[1].floor);

            printf("Hostel 3 : ");
            printstring(s->choice[2].hostel,hostelcodesize);
            printf("Hostel 3's floor : %d\n",s->choice[2].floor);
            
            // Check if status is RANDOM or other.
            // If other, then print roomate rollno also.
            if(s->status == RANDOM){
                printf("Status : RANDOM\n");
            }
            else{
                if(s->status == PENDING){
                      printf("Status : PENDING\n");
                }
                else{
                      printf("Status : LOCKED\n");
                }
                printf("Roomate rollno : %lu\n",s->roomaterollno);
            }

            printf("\n");
     

}

// Close window
void closeWindow(){
            // Change registeration_window_open to false.
            registeration_window_open = false;

            Student* s = head;

            // Change all PENDING status to RANDOM.
            while(s!=NULL){
                    if(s->status == PENDING){
                        s->status = RANDOM;
                        s->roomaterollno = 0;
                    }
                    s = s->next;
            }

            printf("Registration closed now.\n");

            s = head;

            FILE *fptr;
            // Create a file that stores student data who has not registered yet.
            fptr = fopen("NotRegisteredStudent.txt","w");

            if(fptr == NULL){
                printf("Not registered is not present.\n");
            }

            while(s!=NULL){
                if(s->registered){
                     s=s->next;
                     continue;
                }
                // Store in this form rollno | name | " "
                fprintf(fptr,"%lu|%s|%s\n",s->rollno,s->name," ");
                s = s->next;

            }

            fclose(fptr);

            // Write and save data in registeredstudentfile. 
            write_data();


}

int main(void) {
    initializeInventory();
    FILE *fp = fopen("vacant_rooms.txt", "r");
    FILE *fp2 = fopen("vacant_rooms_errors.txt", "w");
    if (fp == NULL || fp2 == NULL) {
        printf("Error opening room inventory files.\n");
        if (fp != NULL) {
            fclose(fp);
        }
        if (fp2 != NULL) {
            fclose(fp2);
        }
        freeInventory();
        return 1;
    }
    loadRooms(fp, fp2);
    fclose(fp);
    fclose(fp2);
    load_data();
    loadregistereddata();
    int choice;
    while (1) {
        printf("\n===== HOSTEL ROOM ALLOCATOR =====\n");
        printf("1. Register / Edit submission\n");
        printf("2. View submission\n");
        printf("3. List eligible students\n");
        printf("4. Close registration window\n");
        printf("5. Exit\n");
        printf("Enter choice: ");

        if (scanf("%d", &choice) != 1) {
            printf("Invalid input.\n");

            int ch;
            while ((ch = getchar()) != '\n' && ch != EOF) {}
            continue;
        }

        switch (choice) {
            case 1:
                registerStudent();
                break;

            case 2:
                viewmysubmission();
                break;

            case 3:
                for (Student *s = head; s; s = s->next) {
                    printf("%lu | %s \n",
                           s->rollno,
                           s->name);
                }
                break;

            case 4:
                closeWindow();
                break;

            case 5:
                write_data();

                while (head) {
                    Student *temp = head;
                    head = head->next;
                    free(temp);
                }

                return 0;

            default:
                printf("Invalid choice.\n");
        }
    }
}








