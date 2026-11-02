# SHARE: Hostel Room Allocator and Room Exchange
## User manual

Team Coffee Overflow: Sagar Aggarwal (106125099), Rohit (106125097),
Sanapala Rahul (106125101)
CSLR31 Data Structures Laboratory, Department of CSE, NIT Tiruchirappalli

---------------------------------------------------------------------------

## 1. What the program does

SHARE is a console program in C for allotting hostel rooms and for changing
them afterwards. It has two subsystems that run one after the other.

1. The Hostel Room Allocator takes student registrations (three ranked floor
   choices and an optional roommate), pairs the students who asked for a
   random roommate, draws a lottery, and places every pair in a room. Its
   output is the allocation table.
2. The Room Exchange Module reads that table and lets students change rooms
   or roommates with the agreement of everyone affected.

The hostels in the data are three boys' hostels: Aquamarine A (code AA,
floors 0 to 3), Aquamarine B (code AB, floors 0 to 3) and Garnet C (code GC,
floor 2 only). Every floor has 40 rooms and every room has 2 beds. Floors are
numbered from 0.

Rooms are numbered inside each hostel. In AA and AB, rooms 1 to 40 are on
floor 0, 41 to 80 on floor 1, 81 to 120 on floor 2 and 121 to 160 on
floor 3. In GC the rooms are 80 to 119, all on floor 2. On screen a room is
written as the hostel code and room number, for example `AA-81` is
Aquamarine A, floor 2, room 81.

The work was split so that each member owned one part of each subsystem.

| Member | Hostel Room Allocator | Room Exchange Module |
|---|---|---|
| Rohit | Registration and student database (Stage 1) | Request engine and login flow |
| S. Rahul | Pairing of the random pool (Stage 2) | Pool engine and executor |
| Sagar | Lottery, room inventory, allocation and output (Stages 3 and 4) | Data layer, persistence, log and menus |

---------------------------------------------------------------------------

## 2. Folders and building

```
Hostel Room Allocator/    stage1.c, stage2_*.c, stage3_*.c, stage4_*.c,
                          roomInventory.c, commonHeaderFile.h,
                          EligibleStudentfile.txt, vacant_rooms.txt
Hostel Room Exchange/     inventoryMenus.c, requestEngineLogin.c, pool.c,
                          executor.c, shuffle.c, commonHeadExchange.h,
                          allocated_rooms.txt
Previous Runs/            output files from earlier runs, for reference
```

Everything needs only a C compiler and the standard library. We built with
gcc on macOS and Linux. Run every command from inside the subsystem's own
folder, because the programs open their data files by name in the current
directory.

The allocator is four separate programs, run in order:

```
gcc stage1.c roomInventory.c -o stage1
gcc stage2_pairing.c stage2_contract.c stage2_demand.c stage2_main.c roomInventory.c -o stage2
gcc stage3_pairedListMergerLottery.c roomInventory.c -o stage3
gcc stage4_allocate.c roomInventory.c -o stage4
```

The exchange module is one program, built the plain way so it comes out
as `a.out`:

```
gcc inventoryMenus.c requestEngineLogin.c pool.c executor.c shuffle.c
```

---------------------------------------------------------------------------

## 3. Hostel Room Allocator

### 3.1 Input files

- `EligibleStudentfile.txt`: one line per student allowed to register,
  `rollNo|name`. 720 students in the sample.
- `vacant_rooms.txt`: one line per free room, `hostel,floor,room`, for
  example `AA,0,1`. Lines starting with `#` are comments. 360 rooms in the
  sample.

### 3.2 Stage 1: registration (`./stage1`)

Stage 1 is the registration window. It loads the eligible list and shows a
menu:

```
1. Register / Edit submission
2. View submission
3. List eligible students
4. Close registration window
5. Exit
```

Registering: choose 1, enter your 9-digit roll number (it must be on the
eligible list), then choose 1 for a random roommate or 2 to name one, then
enter your three choices in order of preference, each as a hostel code
(1 for Aquamarine A, 2 for Aquamarine B, 3 for Garnet C) and a floor number.

Roommates: if you name a roommate, your status is PENDING until that
student registers and names you back; then both of you are LOCKED and
share one set of choices. If you asked for a random roommate your status is
RANDOM. A LOCKED student who edits their submission can change the shared
preferences for both. Choose 2 at any time to see your current submission.

Closing the window: choose 4. Nobody can register or edit after this.
Every student still PENDING becomes RANDOM, and the roll numbers of eligible
students who never registered are written to `NotRegisteredStudent.txt`.
Choose 5 to exit; this writes the registrations to
`registeredstudentdata.txt` (Stage 1's own format, reloaded on the next
start) and `registrations_paired.txt` (the file Stage 2 reads). The
registration file has one line per student:

```
rollNo|name|choice1|choice2|choice3|status|roommateRoll
106125001|Aarav Sharma|AA0|AA1|AA2|LOCKED|106125002
106125021|Aditya Singh|AA2|AA3|AB0|RANDOM|0
```

### 3.3 Stage 2: pairing (`./stage2`)

Reads `registrations_paired.txt` and `NotRegisteredStudent.txt`. Checks
every LOCKED pair (both records exist, both LOCKED, each names the other; a
broken pair falls back to RANDOM). Pairs the RANDOM students by how well
their choices overlap: every possible pair gets a score from 0 to 16, a
shared floor scoring more than a shared hostel and higher-ranked matches
scoring more than lower, and pairs are taken greedily from the best score
down. Each new pair gets a shared list of three floors built from both
students' lists. The number of students is assumed to be even.

It prints the list of units (pairs) and then the demand report, which shows
for each floor the free rooms against the number of pairs that listed it
first. It then rewrites `registrations_paired.txt` with every student now
LOCKED to their partner.

### 3.4 Stage 3: lottery (`./stage3`)

Reads `registrations_paired.txt`, which has two lines per pair, merges them
into one record per pair sorted by roll number, and shuffles the pairs with
a Fisher-Yates shuffle. The shuffle is seeded from the clock, so each run
gives a different order. Writes `registrations_paired_final`, one line per
pair:

```
rollNo1|rollNo2|choice1|choice2|choice3
```

### 3.5 Stage 4: allocation (`./stage4`)

Reads `registrations_paired_final` and `vacant_rooms.txt`. Phase 1 walks
the pairs in lottery order and gives each its first choice floor if it has
a free room, else its second, else its third, taking the lowest-numbered
free room. Pairs whose three choices are all full are set aside for phase
2, which tries the nearest floors above and below each choice in the same
hostel, and failing that any free floor in a hostel the pair did not list.
It prints how many pairs were allotted and how many went through phase 2,
and writes `allocated_rooms.txt`:

```
rollNo1|rollNo2|hostel|floor|room
106125001|106125002|AA|0|1
```

Lines of `vacant_rooms.txt` that could not be read are listed in
`vacant_rooms_errors.txt`.

Copy `allocated_rooms.txt` into the `Hostel Room Exchange` folder before
starting the exchange module.

---------------------------------------------------------------------------

## 4. Room Exchange Module (`./a.out`)

### 4.1 Starting

On the first start the program finds no saved state, reads
`allocated_rooms.txt` and reports how many room allocations it loaded. From
then on it keeps its own files in the folder (`students.txt`,
`student_state.txt`, `rooms.txt`, `requests.txt`, `notices.txt`) and saves
them after every action. The audit log is `audit.log`.

The first menu is:

```
1. Student Login
2. Admin Login
3. Exit
```

### 4.2 What a student can do

- Mutual room exchange: your room and another room swap. Both pairs keep
  their roommate. Everyone in both rooms (up to 4) has to agree.
- Roommate swap: you and a named student trade places, so you end up with
  their roommate and they with yours. All four have to agree.
- Random pool: list 1 to 3 floors you would rather be on and let the
  program find a ring of students who can rotate. Your roommate has to
  agree to you joining.

Rules that apply to all of them:

- You can have one open request at a time, whether you started it or
  accepted it. While it is open you cannot start another or accept a
  second one (you may reject it). The lock ends when the request
  completes, is cancelled, rejected, withdrawn or expires.
- A request expires 2 days after it is made. Expiry is checked when you log
  in and when the admin runs the expiry check, not by a timer.
- Nothing is applied unless the whole set of moves can be applied together.
- Every completed exchange and every pool run is one line in the audit log.

### 4.3 Logging in

Choose 1 and enter your roll number. The program shows your room, expires
any of your requests past their time, shows your unread notices once, and
then shows each open request waiting for your answer:

```
RequestId : 1
Request's initiator rollNo : 106125019
Request type : ROOM_EXCHANGE
From Hostel Aquamarine A 2 to Hostel Aquamarine B 81
Remaining Time : 172800 seconds.
--- Choice Menu ---
1.ACCEPT
2.REJECT
3.SKIP FOR LATER
```

Accept marks you as agreed; when the last person accepts, the exchange is
applied at once. Reject cancels the request for everyone. Skip leaves it
open for your next login. Then the student menu appears:

```
1. Start Room Exchange
2. Start Roommate Swap
3. Join Pool
4. Leave Pool
5. View Request
6. Withdraw Request
0. Exit
```

Start Room Exchange asks for the target room as hostel code and room
number, for example `AB-81`. It is refused if the room does not exist, is
your own, is empty, or if anyone in either room already has an open request
or is in the pool.

Start Roommate Swap asks for the other student's roll number. Refused if
they do not exist, are already your roommate, or if any of the four people
is busy.

Join Pool asks how many floors (1 to 3) and then each floor as hostel code
and floor number, best first, for example `AA2` or `GC2`. Refused if a floor
does not exist, is the floor you already live on, or is repeated, or if you
or your roommate has an open request. If you live alone you are in the pool
at once. If you have a roommate, a consent request goes to them and you are
in the pool once they accept at their login; until then you cannot start
anything else.

Leave Pool takes you out before the admin runs it. View Request shows your
open request with its approvers and their answers. Withdraw Request cancels
a request you started.

### 4.4 Admin menu

Choose 2 at the first menu:

```
1. Close Exchange Window
2. Run Expiry Check
3. Run Pool with Seed
4. Print Report
5. View Audit Log
0. Exit
```

Run Expiry Check expires every open request past its 2 days, so nothing
stays open just because its people never logged in. Run Pool asks for a
seed and runs the pool once (see below). Print Report lists every student
with their room and pool state. View Audit Log prints `audit.log`. Close
Exchange Window is on the menu but is not implemented in this version; it
prints a message and does nothing.

### 4.5 The pool run

The admin runs the pool once, after students have had time to join. The
program builds a graph whose nodes are floors and whose arrows go from each
pool student's floor to the floors they listed. A cycle in that graph is a
ring of students who can each move into the next one's room with no floor
gaining or losing anybody. First choices are matched before second choices,
and second before third. On a floor with more than one candidate the
student who ranked the target higher goes; on a tie the seeded order
decides. Each ring is applied through the same executor as every other
request. Students left without a match are told and stay where they are.
Running the same joins with the same seed gives the same result. Example:

```
pool run, seed 7, 3 student(s) in the pool
ring applied (request 4):
  106125011 Student_106125011    AA-41 -> AA-81
  106125003 Student_106125003    AA-81 -> AB-1
  106125005 Student_106125005    AB-1 -> AA-41
done: 1 ring(s), 3 student(s) moved, 0 without a match
```

### 4.6 A walkthrough

Anil is in AA-2 with Bhaskar and wants AA floor 2.

1. Anil logs in, chooses Join Pool, enters 1 floor, `AA2`. The program says
   a consent request has gone to Bhaskar.
2. Bhaskar logs in. The consent request is shown; he chooses 1 (accept).
   Anil is now in the pool.
3. Anil logs in later and sees the notice that his roommate agreed.
4. The admin chooses Run Pool with Seed and enters a seed. If Anil is part
   of a ring, the run prints it.
5. Anil logs in and sees a notice: exchange done, you are now in AA-81.
   Bhaskar sees one too, since his roommate changed.

If Bhaskar had rejected instead, Anil would have been told and would be
free to try something else.

---------------------------------------------------------------------------

## 5. Known limitations

- The pool is greedy. It applies the first ring it finds and never looks
  for a better one, so it does not promise the largest number of trades.
- On a floor with two pool students only one of them is in a given ring.
- Expiry depends on the machine clock. There is no timer.
- Only occupied beds are traded; moving into a vacant room is not
  supported.
- The lottery is seeded from the clock and cannot be replayed.
- The allocator assumes an even number of registered students.
- Closing the exchange window is not implemented.
- A request that is still open when the exchange program is exited is not
  shown again after a restart, because of how an empty field is saved and
  reloaded. Finish open requests within one run, or restart from the
  allocation.
