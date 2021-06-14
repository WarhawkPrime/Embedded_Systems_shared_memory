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
#include <new>

#include <chrono>
#include <ctime>

using namespace std;

#define SHM_NAME        "/estSHM"
#define QUEUE_SIZE      1
#define NUM_MESSAGES    10

struct PackedData {
	Motion_t motion;
	UInt64 time;
};
typedef struct PackedData PackedData_t;

/* ========== own ========== */
#define BUF_SIZE 1024
void *addr;
Int8 *que;

CBinarySemaphore *binary_semaphore;
CCommQueue *queue;


void init_shared_memory(){
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
}


PackedData_t create_Sensordata(){
	SensorTag st;
	st.initRead();
	st.writeMovementConfig();
	Motion_t motion = st.getMotion();

	auto now = std::chrono::system_clock::now();
	time_t tt = std::chrono::system_clock::to_time_t (now);
	tt = static_cast<UInt64> (time(NULL));

	PackedData_t pck;
	pck.motion = motion;
	pck.time = tt;
	return pck;
}

const CMessage create_Message(){
	PackedData_t pck = create_Sensordata();

	CMessage msg;
}

/* ========== ========== ========== */



int main()
{
		shm_unlink(SHM_NAME);
		int fd = shm_open(SHM_NAME, O_CREAT | O_EXCL | O_RDWR, S_IRUSR | S_IWUSR);
		if (fd == -1){
			perror("shm_open");
		}

		int size = CCommQueue::getNumOfBytesNeeded(NUM_MESSAGES) + sizeof(CBinarySemaphore);

		if (int res = ftruncate(fd, size)) {
			perror("ftruncate");
		}

		addr = mmap(NULL, BUF_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
		if(addr == MAP_FAILED) {
			perror("mmap");
		}

		binary_semaphore = new(addr) CBinarySemaphore();
		queue = new(addr + sizeof(CBinarySemaphore)) CCommQueue(QUEUE_SIZE, *binary_semaphore);


    cout << "Creating a child process ..." << endl;
    pid_t pid = fork();

		/* ========== CHILD ========== */
    if (0 == pid)
    {
        // Child process - Reads all Messages from the Queue and outputs them with auxiliary data.
				/* ========== ========== ========== */

				//sleep(1);

				//binary_semaphore = new(addr) CBinarySemaphore();
				//CCommQueue *queues = reinterpret_cast<CCommQueue *> (addr + sizeof(CBinarySemaphore));

				//binary_semaphore->take();

				CMessage msg;
				std::cout << queue->getNumOfMessages() << std::endl;

				for(int i = 0; i < NUM_MESSAGES; i++) {

					if(queue->getMessage(msg)){
						std::cout << "a message exists" << std::endl;
					}

				}


				/* ========== ========== ========== */
				printf("from child: pid=%d, parent_pid=%d\n",(int)getpid(), (int)getppid());
				exit(42);
    }
		/* ========== PARENT ========== */
    else if (pid > 0)
    {
				//binary_semaphore->take();
        // Parent process - Writes all Messages into the Queue
				/* ========== ========== ========== */
				//PackedData_t pck = create_Sensordata();

				for(int i = 0; i < NUM_MESSAGES; i++) {

					const CMessage c_msg;
					queue->add(c_msg);
					std::cout << queue->getNumOfMessages() << std::endl;

				}

				//binary_semaphore->give();

				//sleep(1);

				printf("from parent: pid=%d child_pid=%d\n",(int)getpid(), (int)pid);
				/* ========== ========== ========== */
				int status;
			  pid_t waited_pid = waitpid(pid, &status, 0);

				if (waited_pid < 0) {
			    perror("waitpid() failed");
			    exit(EXIT_FAILURE);
				}
				else if (waited_pid == pid) {
					if (WIFEXITED(status)) {
				    /* WIFEXITED(status) returns true if the child has terminated
				     * normally. In this case WEXITSTATUS(status) returns child's
				     * exit code.
				     */
				  	printf("from parent: child exited with code %d\n",WEXITSTATUS(status));
				  }
				}
    }
    else
    {
        // Error
				perror("fork() failed");
			  exit(EXIT_FAILURE);
    }


		shm_unlink(SHM_NAME);
    return 0;
}
