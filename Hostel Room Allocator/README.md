The allocator has 4 stages -

1) Stage 1 ( requires stage1.c and roomInventory.c ) - The student registration window in which students can select their roomates ( another student or random allocation ) alongside with their hostel choices. The output of this stage is the registered students list (along with roomate choice (LOCKED or RANDOM)) and a non registered students list.

2) Stage 2 ( requires stage2_pairing.c stage2_contract.c stage2_demand.c stage2_main.c roomInventory ) - It pairs the random roomates based on compatibility score calculated using the choices of each student. The output gives us the final pairing student pairing list ( with status as LOCKED for all ) alongside the demand report for each hostel floor.

3) Stage 3 ( requires stage3_pairedListMergerLottery.c roomInventory.c ) - The output file of stage 2 has 2 lines per student ( one of the student himself/herself and the other of the student as a roomate ). Stage 3 merges these lines into one line and implements the Fisher Yates shuffle algorithm to shuffle the list  of candidates.

4) Stage 4 ( requires gcc stage4_allocate.c roomInventory.c ) - This allocates room according the order in the stage 3 output file and allocates the first available room for each respective student choice. If all the student's choices are taken, then they are added in the pool for phase 2 allocation which allocates the nearest available hostel floor or other hostel to the student pair. The final output is the allocation list which contains the room alloted to each pair of roomates

The common files -
1) Common has the repeatedly used structures and functions so that each stage can be synced.
2) roomInventory.c has the room inventory checking functions like floorExists(), freeRoomsOnFloor() etc.
3) vacant_rooms.txt has the available hostel rooms available for allocation.
4) EligibleStudentFile.txt has the list of eligible student roll numbers who can register for hostel allocation.

Assumptions made for the allocation process -
1) There are even number of students
2) The number of rooms is atleast half of the number of students
3) No solo allocation   
