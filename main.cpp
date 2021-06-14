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
	UInt16 id;
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


PackedData_t create_Sensordata(int id){
	SensorTag st;
	st.initRead();
	st.writeMovementConfig();
	Motion_t motion = st.getMotion();

	auto now = std::chrono::system_clock::now();
	//time_t rt = std::chrono::system_clock::to_time_t (updated);
	auto now_ms = std::chrono::time_point_cast<std::chrono::nanoseconds>(now);
	auto value = now_ms.time_since_epoch();
	long duration = value.count();

	PackedData_t pck;
	pck.id = id;
	pck.motion = motion;
	pck.time = duration;
	return pck;
}


const CMessage create_Message(int id){

	PackedData_t pck = create_Sensordata(id);

	//CMessage msg;
	MostMessage msg;
	msg.data.PackedData.id = pck.id;
	msg.data.PackedData.motion = pck.motion;
	msg.data.PackedData.time = pck.time;
	//msg.data.bytes[0] = mt.gyro.x;
	//std::cout << msg.data.bytes[0] << std::endl;

	const CMessage c_msg(msg);
	return c_msg;
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
				long mean_time = 0;


				for(int i = 0; i < NUM_MESSAGES; i++) {
					CMessage msg;
					binary_semaphore->take();
					if(queue->getMessage(msg)) {

						//new now time
						auto now = std::chrono::system_clock::now();
						//time_t rt = std::chrono::system_clock::to_time_t (updated);
						auto now_ms = std::chrono::time_point_cast<std::chrono::nanoseconds>(now);
						auto value = now_ms.time_since_epoch();
    				long received_time = value.count();

						const MostMessage* mmsg = msg.getMostMessage();

						UInt64 send_time = mmsg->data.PackedData.time;

						long duration = received_time - send_time;

						mean_time += duration;

						//Ausgaben:
						//std::cout << std::endl;
						//std::cout << "a message exists" << std::endl;
						//std::cout << "Message Number: " << mmsg->data.PackedData.id << std::endl;
						//std::cout << "Message send: " << send_time  << " ns"<< std::endl;
						//std::cout << "Message received: " << received_time << " ns"<< std::endl;
						//std::cout << "Send time: " << duration << " ns"<< std::endl;
						//std::cout << "GyroX: " << mmsg->data.PackedData.motion.gyro.x << std::endl;
						//std::cout << "GyroX: " << mmsg->data.PackedData.motion.gyro.y << std::endl;
						//std::cout << "GyroX: " << mmsg->data.PackedData.motion.gyro.z << std::endl;
						//std::cout << "GyroX: " << mmsg->data.PackedData.motion.acc.x << std::endl;
						//std::cout << "GyroX: " << mmsg->data.PackedData.motion.acc.y << std::endl;
						//std::cout << "GyroX: " << mmsg->data.PackedData.motion.acc.z << std::endl;
					}
				}

				//std::cout << std::endl;
				mean_time = mean_time / NUM_MESSAGES;
				std::cout << "Mean Time: " << mean_time << " ns" << std::endl;

				/* ========== ========== ========== */
				printf("from child: pid=%d, parent_pid=%d\n",(int)getpid(), (int)getppid());
				exit(42);
    }
		/* ========== PARENT ========== */
    else if (pid > 0)
    {

				for(int i = 0; i < NUM_MESSAGES; i++) {

					const CMessage c_msg = create_Message(i);
					queue->add(c_msg);
					binary_semaphore->give();
				}


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
