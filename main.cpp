/* ========== ========== ========== */
/*
Shared Memory muss im jedem Prozess einzeln im Speicher vereinbart werden


*/
/* ========== ========== ========== */

#include <iostream>
#include <stdlib.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/time.h>

#include "CCommQueue.h"
#include "SensorTag.h"

#include <pthread.h>
#include <semaphore.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>


using namespace std;

#define SHM_NAME        "/estSHM"
#define QUEUE_SIZE      1
#define NUM_MESSAGES    10

//===== own
#define BUF_SIZE 1024

struct PackedData {
	Motion_t motion;
	UInt64 time;
};
typedef struct PackedData PackedData_t;

void *addr;
CBinarySemaphore *binary_semaphore;
CCommQueue *queue;

int *glob_var;

int main()
{

		shm_unlink(SHM_NAME);
		// Create shared memory object and set its size to the size of our structure.

		int fd = shm_open(SHM_NAME, O_CREAT | O_EXCL | O_RDWR, S_IRUSR | S_IWUSR);

		if (fd == -1){
			perror("shm_open");
		}

		if (int res = ftruncate(fd, BUF_SIZE)) {
			perror("ftruncate");
		}

		// Map the object into the caller's address space.
		addr = mmap(NULL, BUF_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);

		if(addr == MAP_FAILED) {
			perror("mmap");
		}


	//binary_semaphore = new(addr) CBinarySemaphore();
	//queue = new(addr + sizeof(binary_semaphore)) CCommQueue(QUEUE_SIZE, *binary_semaphore);
	//binary_semaphore = new CBinarySemaphore();
	//CMessage msg;
	//queue->getMessage(msg);
	//glob_var = (int) mmap(NULL, sizeof *glob_var, PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);
	//*glob_var = 1;



	binary_semaphore = new(addr) CBinarySemaphore;
	queue = new(addr + sizeof(binary_semaphore)) CCommQueue(QUEUE_SIZE, *binary_semaphore);
	//glob_var = new(addr) int;


	//===== 1. fork =====
	pid_t child_pid = fork();

	if (child_pid < 0) {
		perror("fork() failed");
		exit(EXIT_FAILURE);
	} else if (child_pid == 0) {

		printf("from child: pid=%d, parent_pid=%d\n",(int)getpid(), (int)getppid());

		/* ========== ========== ========== */

		//*glob_var = 5;
		std::cout << queue->getNumOfMessages() << std::endl;

		/* ========== ========== ========== */

		//exit child
		exit(42);
	} else if (child_pid > 0) {
		// Print message from parent process.
		printf("from parent: pid=%d child_pid=%d\n",(int)getpid(), (int)child_pid);

		/* ========== ========== ========== */


		//binary_semaphore = new CBinarySemaphore();
		//binary_semaphore->takeWithTimeOut(1000);
		sleep(1);
		std::cout << "var: " << *glob_var << std::endl;
		munmap(glob_var, sizeof *glob_var);



		MostMessage m_msg;
		unsigned char data [19] = {42};
		memcpy(m_msg.data.bytes, data, 19);
		const CMessage *c_msg = new CMessage(m_msg);

		if(!queue->add(*c_msg)){
			perror("adding to queue failed");
		}


		//CMessage *sm(msg);
		//sm->setSenderID( 1 );
		//const CMessage *c_sm = sm;
		//queue->add(*c_sm);

		//std::cout << queue->getNumOfMessages() << std::endl;


		/* ========== ========== ========== */

		// Wait until child process exits or terminates.
		int status;
		pid_t waited_pid = waitpid(child_pid, &status, 0);

		if (waited_pid < 0) {
			perror("waitpid() failed");
			exit(EXIT_FAILURE);
		} else if (waited_pid == child_pid) {
			if (WIFEXITED(status)) {

			/* WIFEXITED(status) returns true if the child has terminated
			 * normally. In this case WEXITSTATUS(status) returns child's
			 * exit code.
			 */

			munmap(glob_var, sizeof *glob_var);
			printf("from parent: child exited with code %d\n",WEXITSTATUS(status));
			}
		}
	 }

	// Unlink shared Memory

	//binary_semaphore->~CBinarySemaphore();
	//queue->~CCommQueue();

	//fd = shm_unlink(SHM_NAME);
	shm_unlink(SHM_NAME);
	return 0;
}
