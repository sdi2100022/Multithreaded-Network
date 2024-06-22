/* @author : Athanasios Giavridis -- sdi2100022 */

#include "../lib/jobProject.h"

/* Insert a new job in the queued jobBuffer of the server and check if there is available space so it can be executed */
char *issueJob(char *job,int socket);

/* Change the concurrency of the server */
char *setConcurrency(int N);

/* Remove/Terminate the job with job_XX jobID from the server*/
char *stop(char *jobID);

/* Get all the tuples that are jobBuffer */
char *poll();

/* Terminate the server */
char *server_exit();
