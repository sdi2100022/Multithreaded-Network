/* @author : Athanasios Giavridis -- sdi2100022 */

#include "../lib/jobCommands.h"

/* Function for the implementation of the controller thread,
 * gets the socket of the requester(jobCommander) through arg,
 * reads its message and handles the commands,
 * all commands return a message to be written to the jobCommander 
 * and closes the socket(except for issueJob).
*/
void *controller_thread(void *arg);

/* Function for the implementation of the worker thread, 
 * checks the global condition variables,
 * removes the first job in the jobBuffer.
 * runs its command,
 * creates a file and puts the output in it,
 * turns the file into a string and writes it to the jobCommander,
 * when it finishes it signals some condition variables so other threads can wake up.
 */
void *worker_thread(void *arg);

