/* @author : Athanasios Giavridis -- sdi2100022 */

#include "../lib/jobUtil.h"

/* Server Structure contains the basic components of our Server */
typedef struct __jobServer{
    jobQueue *jobBuffer; // The buffer(or Queue) of the server for the jobs waiting to be run by the worker threads
    int bufferSize; // The max size of the buffer 
    int concurrency; // Concurrency of the worker threads, set to 1 as default
    int activeWorkers; // Count of the active worker threads
    bool EXIT; // Boolean flag to know if the exit command has been called, to empty the worker threads
}Server;




