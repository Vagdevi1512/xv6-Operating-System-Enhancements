
// extern struct proc *mlfq2[NMLFQ][NPROC];      // Queues for each level of MLFQ
// int queue_size[NMLFQ];                // Number of processes in each queue
// int timeslice[NMLFQ] = {1, 4, 8, 16}; // Time slices for each level

// // Enqueue process p into queue q
// void enqueue(int q, struct proc *p) {
//   if (queue_size[q] < NPROC) {
//     mlfq2[q][queue_size[q]++] = p;
//     p->queue = q; // Update process queue number
//     p->check = 1; // Mark process as being in the queue
//   }
// }

// // Dequeue the process from queue q
// struct proc *dequeue(int q) {
//   if (queue_size[q] > 0) {
//     struct proc *p = mlfq2[q][0];
//     // Shift all other elements forward
//     for (int i = 1; i < queue_size[q]; i++) {
//       mlfq2[q][i - 1] = mlfq2[q][i];
//     }
//     queue_size[q]--;
//     p->check = 0; // Mark process as no longer in the queue
//     return p;
//   }
//   return 0;
// }

// // Remove a specific process from queue q
// void remove_from_queue(int q, struct proc *p) {
//   for (int i = 0; i < queue_size[q]; i++) {
//     if (mlfq2[q][i] == p) {
//       // Shift all processes forward to fill the gap
//       for (int j = i + 1; j < queue_size[q]; j++) {
//         mlfq2[q][j - 1] = mlfq2[q][j];
//       }
//       queue_size[q]--;
//       p->check = 0; // Mark process as no longer in the queue
//       break;
//     }
//   }
// }

// // Promote a process to a higher-priority queue
// void promote(struct proc *p) {
//   int current_queue = p->queue;
//   if (current_queue > 0) {
//     remove_from_queue(current_queue, p); // Remove from the current queue
//     p->queue--;                          // Move to higher-priority queue
//     enqueue(p->queue, p);                // Enqueue in the new queue
//   }
// }

// // Demote a process to a lower-priority queue
// void demote(struct proc *p) {
//   int current_queue = p->queue;
//   if (current_queue < NMLFQ - 1) {
//     remove_from_queue(current_queue, p); // Remove from the current queue
//     p->queue++;                          // Move to lower-priority queue
//     enqueue(p->queue, p);                // Enqueue in the new queue
//   }
// }