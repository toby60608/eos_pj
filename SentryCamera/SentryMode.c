#include <errno.h>
#include "SentryMode.h"

#define PRINT_MESSAGE_ENABLE (1)
#define SYSTEM_COMMANL_ENABLE (1)

#define COMMAND_STRING_SIZE (512)

typedef enum _ENUM_CAMERA_STATE_T
{
	ENUM_CAMERA_STATE_SUSPENDED,
	ENUM_CAMERA_STATE_ACTIVE,
	ENUM_CAMERA_STATE_MONITOR,
	ENUM_CAMERA_STATE_TAKE_A_VIDEO,
	ENUM_CAMERA_STATE_ALARM,
}ENUM_CAMERA_STATE_T;

typedef struct _STRUCT_SENTRY_MODE_FLAG_T
{
	bool StopProcess:1;
	bool AcyiveSentry:1;
	bool Alarm:1;
}STRUCT_SENTRY_MODE_FLAG_T;

STRUCT_SENTRY_MODE_FLAG_T Flag = {0};
STRCUT_SENTRY_MODE_SHARED_MEMORY_T *pSentryModeSharedMemory;

int SharedMemory;
key_t SharedMemoryKey = DEFAULT_SHARED_MEMORY_KEY_NUMBER;

int Semaphore;
key_t SemaphoreKey;

int MessageQueueId;
key_t MessageQueuekey = DEFAULT_MESSAGE_QUEUE_KEY_NUMBER;

pthread_t pThread_SentryMode;
pthread_t pThread_CommandParser;

uint64_t u64PictureCount = 0;
uint64_t u64VideoCount = 0;
uint64_t TimerCount = 0;

/* P () - returns 0 if OK; -1 if there was a problem */
int P (int s, unsigned short int SemaphoreIndex)
{
	struct sembuf sop =
	{/* the operation parameters */
		.sem_num =  SemaphoreIndex,  /* access the (and only) sem in the array */
		.sem_op  = -1,     /* wait..*/
		.sem_flg =  0,     /* no special options needed */
	};

	if (semop (s, &sop, 1) < 0)
	{
#if (PRINT_MESSAGE_ENABLE != 0)
		printf("P(): semop failed\n");
#endif
		return -1;
	}
	else
	{
		return 0;
	}
}
 
/* V() - returns 0 if OK; -1 if there was a problem */
int V(int s, unsigned short int SemaphoreIndex)
{
	struct sembuf sop =
	{/* the operation parameters */
		.sem_num =  SemaphoreIndex,  /* the (and only) sem in the array */
		.sem_op  =  1,  /* signal */
		.sem_flg =  0,  /* no special options needed */
	};

	if (semop (s, &sop, 1) < 0)
	{
#if (PRINT_MESSAGE_ENABLE != 0)
		printf("V(): semop failed\n");
#endif
		return -1;
	}
	else
	{
		return 0;
	}
}

/* handler of SIGALRM */ 
void SIGALRM_handler (int signum) 
{ 
  TimerCount++;
#if (PRINT_MESSAGE_ENABLE != 0)
	printf("TimerCount: %lu\n", TimerCount);
#endif
}

void TakePicture(char* Name, uint64_t PictureNumber)
{
#if (PRINT_MESSAGE_ENABLE != 0)
	printf("[%s]\n", __func__);
#endif
	char CommandString[COMMAND_STRING_SIZE];
	char DefaultName[] = "Picture";
	if (strlen(Name) == 0)
	{
		Name = DefaultName;
	}
	snprintf(CommandString, COMMAND_STRING_SIZE, "libcamera-still -t 5 -n --width 640 --height 480 -e png -o %s%lu.png", Name, PictureNumber);
#if (PRINT_MESSAGE_ENABLE != 0)
	printf("System(%s)\n", CommandString);
#endif
#if (SYSTEM_COMMANL_ENABLE != 0)
	P(Semaphore, ENUM_SEMAPHORE_ID_CAMERA);
	system(CommandString);
	V(Semaphore, ENUM_SEMAPHORE_ID_CAMERA);
#endif
}

void Take2sVideo(char* Name, uint64_t VideoNumber)
{
#if (PRINT_MESSAGE_ENABLE != 0)
	printf("[%s]\n", __func__);
#endif
	char CommandString[COMMAND_STRING_SIZE];
	char DefaultName[] = "2sVideo";
	if (strlen(Name) == 0)
	{
		Name = DefaultName;
	}
	snprintf(CommandString, COMMAND_STRING_SIZE, "libcamera-vid -t 2000 -n --width 640 --height 480 -o %s%lu.h264", Name, VideoNumber);
#if (PRINT_MESSAGE_ENABLE != 0)
	printf("System(%s)\n", CommandString);
#endif
#if (SYSTEM_COMMANL_ENABLE != 0)
	P(Semaphore, ENUM_SEMAPHORE_ID_CAMERA);
	system(CommandString);
	V(Semaphore, ENUM_SEMAPHORE_ID_CAMERA);
#endif
}

void Take6sVideo(char* Name, uint64_t VideoNumber)
{
#if (PRINT_MESSAGE_ENABLE != 0)
	printf("[%s]\n", __func__);
#endif
	char CommandString[COMMAND_STRING_SIZE];
	char DefaultName[] = "6sVideo";
	if (strlen(Name) == 0)
	{
		Name = DefaultName;
	}
	snprintf(CommandString, COMMAND_STRING_SIZE, "libcamera-vid -t 6000 -n --width 640 --height 480 -o %s%lu.h264", Name, VideoNumber);
#if (PRINT_MESSAGE_ENABLE != 0)
	printf("System(%s)\n", CommandString);
#endif
#if (SYSTEM_COMMANL_ENABLE != 0)
	P(Semaphore, ENUM_SEMAPHORE_ID_CAMERA);
	system(CommandString);
	V(Semaphore, ENUM_SEMAPHORE_ID_CAMERA);
#endif
}

void signal_handler(int signum)
{
#if (PRINT_MESSAGE_ENABLE != 0)
	printf("signal_handler: caught signal %d\n", signum);
#endif
	switch(signum)
	{
		case SIGINT:
		{
#if (PRINT_MESSAGE_ENABLE != 0)
			printf("SIGINT\n");
#endif
			for (uint32_t SemaphoreIndex = 0; SemaphoreIndex < ENUM_SEMAPHORE_ID_TOTAL; SemaphoreIndex++)
			{
				semctl (Semaphore, SemaphoreIndex, IPC_RMID, 0);
			}
			shmctl (SharedMemory, IPC_RMID, 0);
			msgctl (MessageQueueId, IPC_RMID, NULL);
			exit(0);
			break;
		}
		case SIGUSR1:
		{
#if (PRINT_MESSAGE_ENABLE != 0)
			printf("SIGUSR1\n");
#endif
			P(Semaphore, ENUM_SEMAPHORE_ID_SUSPEND_SIGNAL_FLAG);
			P(Semaphore, ENUM_SEMAPHORE_ID_ALARM_SIGNAL_FLAG);
			if (Flag.AcyiveSentry == false)
			{
				Flag.AcyiveSentry = true;
			}
			else if (Flag.Alarm == false)
			{
				Flag.AcyiveSentry = false;
			}
			V(Semaphore, ENUM_SEMAPHORE_ID_ALARM_SIGNAL_FLAG);
			V(Semaphore, ENUM_SEMAPHORE_ID_SUSPEND_SIGNAL_FLAG);
			break;
		}
		case SIGUSR2:
		{
#if (PRINT_MESSAGE_ENABLE != 0)
			printf("SIGUSR2\n");
#endif
			//Catch Alarm signal
			P(Semaphore, ENUM_SEMAPHORE_ID_ALARM_SIGNAL_FLAG);
			Flag.Alarm = !Flag.Alarm;
			P(Semaphore, ENUM_SEMAPHORE_ID_ALARM_SIGNAL_FLAG);
			break;
		}
		default:
		{
			break;
		}
	}
}

void* SentryMode_Handler(void* data)
{
#if (PRINT_MESSAGE_ENABLE != 0)
	printf("[%s] Start\n", __func__);
#endif
	ENUM_CAMERA_STATE_T state = ENUM_CAMERA_STATE_SUSPENDED;
	uint64_t u64LocalTimerCount = 0;

	struct sigaction SA_SIGALRM;
	struct itimerval timer;
	
	memset (&SA_SIGALRM, 0, sizeof (SA_SIGALRM));
	SA_SIGALRM.sa_handler = &SIGALRM_handler;
	sigaction (SIGALRM, &SA_SIGALRM, NULL);
	/* Configure the timer to expire after 1000 msec */
  timer.it_value.tv_sec = 0;
  timer.it_value.tv_usec = 1000;

  /* Reset the timer back to 2 sec after expired */
  timer.it_interval.tv_sec = 2;
  timer.it_interval.tv_usec = 0;
	/* Start timer */
  setitimer(ITIMER_REAL, &timer, NULL);

	while(Flag.StopProcess == false)
	{
		switch (state)
		{
			case ENUM_CAMERA_STATE_SUSPENDED:
			{
				P(Semaphore, ENUM_SEMAPHORE_ID_SUSPEND_SIGNAL_FLAG);
				if (Flag.AcyiveSentry == true)
				{
					state = ENUM_CAMERA_STATE_ACTIVE;
				}
				V(Semaphore, ENUM_SEMAPHORE_ID_SUSPEND_SIGNAL_FLAG);
				break;
			}
			case ENUM_CAMERA_STATE_ACTIVE:
			{
				//Take pictures
				if (TimerCount != u64LocalTimerCount)
				{
					TakePicture("Picture", TimerCount);
					u64LocalTimerCount = TimerCount;
				}
				P(Semaphore, ENUM_SEMAPHORE_ID_SUSPEND_SIGNAL_FLAG);
				if (Flag.AcyiveSentry == false)
				{
					state = ENUM_CAMERA_STATE_SUSPENDED;
				}
				V(Semaphore, ENUM_SEMAPHORE_ID_SUSPEND_SIGNAL_FLAG);
				break;
			}
			case ENUM_CAMERA_STATE_MONITOR:
			{
				if (TimerCount != u64LocalTimerCount)
				{
					//Take 2-second video
					Take2sVideo("2sVideo", TimerCount);
					//Get a picture and compare them
					
					u64LocalTimerCount = TimerCount;
				}
				P(Semaphore, ENUM_SEMAPHORE_ID_ALARM_SIGNAL_FLAG);
				if (Flag.Alarm == true)
				{
					state = ENUM_CAMERA_STATE_TAKE_A_VIDEO;
				}
				V(Semaphore, ENUM_SEMAPHORE_ID_ALARM_SIGNAL_FLAG);
				P(Semaphore, ENUM_SEMAPHORE_ID_SUSPEND_SIGNAL_FLAG);
				if (Flag.AcyiveSentry == false)
				{
					state = ENUM_CAMERA_STATE_SUSPENDED;
				}
				V(Semaphore, ENUM_SEMAPHORE_ID_SUSPEND_SIGNAL_FLAG);
				break;
			}
			case ENUM_CAMERA_STATE_TAKE_A_VIDEO:
			{
				//Take 6-second video
				Take2sVideo("6sVideo", u64LocalTimerCount);
				
				state = ENUM_CAMERA_STATE_ALARM;
				break;
			}
			case ENUM_CAMERA_STATE_ALARM:
			{
				P(Semaphore, ENUM_SEMAPHORE_ID_ALARM_SIGNAL_FLAG);
				if (Flag.Alarm == false)
				{
					state = ENUM_CAMERA_STATE_MONITOR;
				}
				V(Semaphore, ENUM_SEMAPHORE_ID_ALARM_SIGNAL_FLAG);
				break;
			}
			default:
			{
				break;
			}
		}
		usleep(100);
	}
#if (PRINT_MESSAGE_ENABLE != 0)
	printf("[%s] Exit\n", __func__);
#endif
	pthread_exit(NULL);
}

void* CommandParser_Handler(void* data)
{
#if (PRINT_MESSAGE_ENABLE != 0)
	printf("[%s] Start\n", __func__);
#endif
	int ret;
	STRUCT_COMMAND_PACKET_T RxCommand;
	while(Flag.StopProcess == false)
	{
		ret = msgrcv(MessageQueueId, &RxCommand, sizeof(STRUCT_COMMAND_PACKET_T) - 4, SENTRY_MODE_COMMAND_MESSAGE_TYPE, 0);
#if (PRINT_MESSAGE_ENABLE != 0)
		printf("[%s] ret: %d, Command Id: 0x%02X, Length: %d\n", __func__, ret, RxCommand.Id, RxCommand.Length);
#endif
		switch (RxCommand.Id & 0x7Fu)
		{
			case ENUM_SENTRY_MODE_COMMAND_ID_LOOPBACK:
			{
				RxCommand.Id |= 0x80;
				/* Response */
				ret = msgsnd(MessageQueueId, &RxCommand, sizeof(STRUCT_COMMAND_PACKET_T) - 4, 0);
#if (PRINT_MESSAGE_ENABLE != 0)
				printf("Send Loopback - id: 0x%02X, length: %d", RxCommand.Id, RxCommand.Length);
				for (int index = 0; index < RxCommand.Length; index++)
				{
					if ((index & 0xF) == 0)
					{
						printf("\n");
					}
					printf("0x%02X ", RxCommand.Payload[index]);
				}
				printf("\n");
#endif
				break;
			}
			case ENUM_SENTRY_MODE_COMMAND_ID_CHECK_IS_SENTRY_MODE_ACTIVE:
			{
				/* Response */
				RxCommand.Id |= 0x80;
				RxCommand.Length = 1;
				memset(RxCommand.Payload, 0, 256);
				P(Semaphore, ENUM_SEMAPHORE_ID_SUSPEND_SIGNAL_FLAG);
				RxCommand.Payload[0] = Flag.AcyiveSentry;
				V(Semaphore, ENUM_SEMAPHORE_ID_SUSPEND_SIGNAL_FLAG);
				ret = msgsnd(MessageQueueId, &RxCommand, sizeof(STRUCT_COMMAND_PACKET_T) - 4, 0);
#if (PRINT_MESSAGE_ENABLE != 0)
				printf("Check suspended - id: 0x%02X, length: %d", RxCommand.Id, RxCommand.Length);
				for (int index = 0; index < RxCommand.Length; index++)
				{
					if ((index & 0xF) == 0)
					{
						printf("\n");
					}
					printf("0x%02X ", RxCommand.Payload[index]);
				}
				printf("\n");
#endif
				break;
			}
			case ENUM_SENTRY_MODE_COMMAND_ID_SUSPEND_SENTRY_MODE:
			{
				P(Semaphore, ENUM_SEMAPHORE_ID_SUSPEND_SIGNAL_FLAG);
				Flag.AcyiveSentry = false;
				V(Semaphore, ENUM_SEMAPHORE_ID_SUSPEND_SIGNAL_FLAG);
				/* Response */
				RxCommand.Id |= 0x80;
				RxCommand.Length = 1;
				memset(RxCommand.Payload, 0, 256);
				ret = msgsnd(MessageQueueId, &RxCommand, sizeof(STRUCT_COMMAND_PACKET_T) - 4, 0);
#if (PRINT_MESSAGE_ENABLE != 0)
				printf("Suspend sentry mode - id: 0x%02X, length: %d", RxCommand.Id, RxCommand.Length);
				for (int index = 0; index < RxCommand.Length; index++)
				{
					if ((index & 0xF) == 0)
					{
						printf("\n");
					}
					printf("0x%02X ", RxCommand.Payload[index]);
				}
				printf("\n");
#endif
				break;
			}
			case ENUM_SENTRY_MODE_COMMAND_ID_ACTIVE_SENTRY_MODE:
			{
				P(Semaphore, ENUM_SEMAPHORE_ID_SUSPEND_SIGNAL_FLAG);
				Flag.AcyiveSentry = true;
				V(Semaphore, ENUM_SEMAPHORE_ID_SUSPEND_SIGNAL_FLAG);
				/* Response */
				RxCommand.Id |= 0x80;
				RxCommand.Length = 1;
				memset(RxCommand.Payload, 0, 256);
				ret = msgsnd(MessageQueueId, &RxCommand, sizeof(STRUCT_COMMAND_PACKET_T) - 4, 0);
#if (PRINT_MESSAGE_ENABLE != 0)
				printf("Active sentry mode - id: 0x%02X, length: %d", RxCommand.Id, RxCommand.Length);
				for (int index = 0; index < RxCommand.Length; index++)
				{
					if ((index & 0xF) == 0)
					{
						printf("\n");
					}
					printf("0x%02X ", RxCommand.Payload[index]);
				}
				printf("\n");
#endif
				break;
			}
			case ENUM_SENTRY_MODE_COMMAND_ID_CHECK_IS_SENTRY_MODE_ALARM:
			{
				/* Response */
				RxCommand.Id |= 0x80;
				RxCommand.Length = 1;
				memset(RxCommand.Payload, 0, 256);
				P(Semaphore, ENUM_SEMAPHORE_ID_ALARM_SIGNAL_FLAG);
				RxCommand.Payload[0] = Flag.Alarm;
				V(Semaphore, ENUM_SEMAPHORE_ID_ALARM_SIGNAL_FLAG);
				ret = msgsnd(MessageQueueId, &RxCommand, sizeof(STRUCT_COMMAND_PACKET_T) - 4, 0);
#if (PRINT_MESSAGE_ENABLE != 0)
				printf("Check alarm - id: 0x%02X, length: %d", RxCommand.Id, RxCommand.Length);
				for (int index = 0; index < RxCommand.Length; index++)
				{
					if ((index & 0xF) == 0)
					{
						printf("\n");
					}
					printf("0x%02X ", RxCommand.Payload[index]);
				}
				printf("\n");
#endif
				break;
			}
			case ENUM_SENTRY_MODE_COMMAND_ID_CLOSE_ALARM:
			{
				P(Semaphore, ENUM_SEMAPHORE_ID_ALARM_SIGNAL_FLAG);
				Flag.Alarm = false;
				V(Semaphore, ENUM_SEMAPHORE_ID_ALARM_SIGNAL_FLAG);
				/* Response */
				RxCommand.Id |= 0x80;
				RxCommand.Length = 1;
				memset(RxCommand.Payload, 0, 256);
				ret = msgsnd(MessageQueueId, &RxCommand, sizeof(STRUCT_COMMAND_PACKET_T) - 4, 0);
#if (PRINT_MESSAGE_ENABLE != 0)
				printf("Close alarm - id: 0x%02X, length: %d", RxCommand.Id, RxCommand.Length);
				for (int index = 0; index < RxCommand.Length; index++)
				{
					if ((index & 0xF) == 0)
					{
						printf("\n");
					}
					printf("0x%02X ", RxCommand.Payload[index]);
				}
				printf("\n");
#endif
				break;
			}
			case ENUM_SENTRY_MODE_COMMAND_ID_TRIGGER_ALARM:
			{
				P(Semaphore, ENUM_SEMAPHORE_ID_ALARM_SIGNAL_FLAG);
				Flag.Alarm = true;
				V(Semaphore, ENUM_SEMAPHORE_ID_ALARM_SIGNAL_FLAG);
				/* Response */
				RxCommand.Id |= 0x80;
				RxCommand.Length = 1;
				memset(RxCommand.Payload, 0, 256);
				ret = msgsnd(MessageQueueId, &RxCommand, sizeof(STRUCT_COMMAND_PACKET_T) - 4, 0);
#if (PRINT_MESSAGE_ENABLE != 0)
				printf("Close alarm - id: 0x%02X, length: %d", RxCommand.Id, RxCommand.Length);
				for (int index = 0; index < RxCommand.Length; index++)
				{
					if ((index & 0xF) == 0)
					{
						printf("\n");
					}
					printf("0x%02X ", RxCommand.Payload[index]);
				}
				printf("\n");
#endif
				break;
			}
			case ENUM_SENTRY_MODE_COMMAND_ID_TAKE_PICTURES:
			{
				TakePicture("TestPicture", TimerCount);
				RxCommand.Id |= 0x80;
				RxCommand.Length = 1;
				memset(RxCommand.Payload, 0, 256);
				ret = msgsnd(MessageQueueId, &RxCommand, sizeof(STRUCT_COMMAND_PACKET_T) - 4, 0);
#if (PRINT_MESSAGE_ENABLE != 0)
				printf("Take picture - id: 0x%02X, length: %d", RxCommand.Id, RxCommand.Length);
				for (int index = 0; index < RxCommand.Length; index++)
				{
					if ((index & 0xF) == 0)
					{
						printf("\n");
					}
					printf("0x%02X ", RxCommand.Payload[index]);
				}
				printf("\n");
#endif
				break;
			}
			case ENUM_SENTRY_MODE_COMMAND_ID_TAKE_2S_VIDEO:
			{
				Take2sVideo("Test2sVideo", TimerCount);
				RxCommand.Id |= 0x80;
				RxCommand.Length = 1;
				memset(RxCommand.Payload, 0, 256);
				ret = msgsnd(MessageQueueId, &RxCommand, sizeof(STRUCT_COMMAND_PACKET_T) - 4, 0);
#if (PRINT_MESSAGE_ENABLE != 0)
				printf("Take picture - id: 0x%02X, length: %d", RxCommand.Id, RxCommand.Length);
				for (int index = 0; index < RxCommand.Length; index++)
				{
					if ((index & 0xF) == 0)
					{
						printf("\n");
					}
					printf("0x%02X ", RxCommand.Payload[index]);
				}
				printf("\n");
#endif
				break;
			}
			case ENUM_SENTRY_MODE_COMMAND_ID_TAKE_6S_VIDEO:
			{
				Take6sVideo("Test6sVideo", TimerCount);
				RxCommand.Id |= 0x80;
				RxCommand.Length = 1;
				memset(RxCommand.Payload, 0, 256);
				ret = msgsnd(MessageQueueId, &RxCommand, sizeof(STRUCT_COMMAND_PACKET_T) - 4, 0);
#if (PRINT_MESSAGE_ENABLE != 0)
				printf("Take picture - id: 0x%02X, length: %d", RxCommand.Id, RxCommand.Length);
				for (int index = 0; index < RxCommand.Length; index++)
				{
					if ((index & 0xF) == 0)
					{
						printf("\n");
					}
					printf("0x%02X ", RxCommand.Payload[index]);
				}
				printf("\n");
#endif
				break;
			}
			default:
			{
				break;
			}
		}
	}
#if (PRINT_MESSAGE_ENABLE != 0)
	printf("[%s] Exit\n", __func__);
#endif
	pthread_exit(NULL);
}

int main(int argc, char *argv[])
{
	struct sigaction sa;
	sa.sa_handler = signal_handler;
	sigemptyset(&sa.sa_mask);
	sa.sa_flags = 0;
	if(argc > 1)
	{
		// ./SentryMode <SharedMemoryKey>
		SharedMemoryKey = atoi(argv[1]);
	}
	if (sigaction(SIGINT, &sa, NULL) == -1)
	{
#if (PRINT_MESSAGE_ENABLE != 0)
		printf("Failed to sigaction SIGINT\n");
#endif
		exit(-1);
	}
	else if (sigaction(SIGUSR1, &sa, NULL) == -1)
	{
#if (PRINT_MESSAGE_ENABLE != 0)
		printf("Failed to sigaction SIGUSR1\n");
#endif
		exit(-1);
	}
	else if (sigaction(SIGUSR2, &sa, NULL) == -1)
	{
#if (PRINT_MESSAGE_ENABLE != 0)
		printf("Failed to sigaction SIGUSR2\n");
#endif
		exit(-1);
	}
	/* Create the segment */
	else if ((SharedMemory = shmget(SharedMemoryKey, sizeof(STRCUT_SENTRY_MODE_SHARED_MEMORY_T), IPC_CREAT | 0666)) < 0)
	{
#if (PRINT_MESSAGE_ENABLE != 0)
		printf("[%s] shmget failed\n", __func__);
#endif
		exit(1);
	}
	else if ((pSentryModeSharedMemory = shmat(SharedMemory, NULL, 0)) == ((void *) -1))
	{
#if (PRINT_MESSAGE_ENABLE != 0)
		printf("[%s] shmat failed\n", __func__);
#endif
		shmctl (SharedMemory, IPC_RMID, 0);
		exit(1);
	}
	/* Create Message Queue */
	else if ((MessageQueueId = msgget(MessageQueuekey, IPC_CREAT | MSQ_MODE)) == -1)
	{
#if (PRINT_MESSAGE_ENABLE != 0)
		printf("[%s] msgget failed\n", __func__);
#endif
		exit(-1);
	}
	/* Create Semaphore */
	else if ((Semaphore = semget(SemaphoreKey, ENUM_SEMAPHORE_ID_TOTAL, IPC_CREAT | IPC_EXCL | SEM_MODE)) < 0)  
	{
		shmctl (SharedMemory, IPC_RMID, 0);
#if (PRINT_MESSAGE_ENABLE != 0)
		printf("[%s] creation of semaphore %d failed: %s\n", __func__, SemaphoreKey, strerror(errno)); 
#endif
		exit(-1); 
	}
	
	for (int index = 0; index < ENUM_SEMAPHORE_ID_TOTAL; index++)
	{
		/* set semaphore (s[0]) value to initial value (val) */ 
		if (semctl(Semaphore, index, SETVAL, 1) < 0 )
		{
			for (uint32_t SemaphoreIndex = 0; SemaphoreIndex < ENUM_SEMAPHORE_ID_TOTAL; SemaphoreIndex++)
			{
				semctl (Semaphore, SemaphoreIndex, IPC_RMID, 0);
			}
			shmctl (SharedMemory, IPC_RMID, 0);
#if (PRINT_MESSAGE_ENABLE != 0)
			printf("[%s] Unable to initialize semaphore: %s\n", __func__, strerror(errno));
#endif
			exit(-1);
		}
	}
	if (pthread_create(&pThread_SentryMode, NULL, SentryMode_Handler, NULL) != 0)
	{
		Flag.StopProcess = true;
		for (uint32_t SemaphoreIndex = 0; SemaphoreIndex < ENUM_SEMAPHORE_ID_TOTAL; SemaphoreIndex++)
		{
			semctl (Semaphore, SemaphoreIndex, IPC_RMID, 0);
		}
		shmctl (SharedMemory, IPC_RMID, 0);
#if (PRINT_MESSAGE_ENABLE != 0)
		printf("[%s] pthread_create failed\n", __func__);
#endif
		exit(-1);
	}
	else if (pthread_create(&pThread_CommandParser, NULL, CommandParser_Handler, NULL) != 0)
	{
		Flag.StopProcess = true;
		for (uint32_t SemaphoreIndex = 0; SemaphoreIndex < ENUM_SEMAPHORE_ID_TOTAL; SemaphoreIndex++)
		{
			semctl (Semaphore, SemaphoreIndex, IPC_RMID, 0);
		}
		shmctl (SharedMemory, IPC_RMID, 0);
#if (PRINT_MESSAGE_ENABLE != 0)
		printf("[%s] pthread_create failed\n", __func__);
#endif
		exit(-1);
	}
	else
	{
		memset(pSentryModeSharedMemory, 0, sizeof(STRCUT_SENTRY_MODE_SHARED_MEMORY_T));
		char* pName = argv[0];
		char* pTempAddress = pName;
		int NameStringLength = strlen(argv[0]);
		while(NameStringLength > 0)
		{
			if (*pTempAddress == '/')
			{
				pName = (pTempAddress + 1);
			}
			NameStringLength--;
			pTempAddress++;
		}
		strcat(pSentryModeSharedMemory->Name, pName);
		pSentryModeSharedMemory->SentryModePid = getpid();
		pSentryModeSharedMemory->SharedMemoryKey = SharedMemoryKey;
		pSentryModeSharedMemory->SemaphoreKey = SemaphoreKey;
		pSentryModeSharedMemory->MessageQueueKey = MessageQueuekey;
		printf("[%s] Name: %s\n", __func__, pSentryModeSharedMemory->Name);
		printf("[%s] PID: %d\n", __func__, pSentryModeSharedMemory->SentryModePid);
		printf("[%s] SharedMemoryKey: %d\n", __func__, pSentryModeSharedMemory->SharedMemoryKey);
		printf("[%s] SemaphoreKey: %d\n", __func__, pSentryModeSharedMemory->SemaphoreKey);
		printf("[%s] MessageQueueKey: %d\n", __func__, pSentryModeSharedMemory->MessageQueueKey);
	}
	
	while (Flag.StopProcess == false);
	{
		//infinity loop
	}
	for (uint32_t SemaphoreIndex = 0; SemaphoreIndex < ENUM_SEMAPHORE_ID_TOTAL; SemaphoreIndex++)
	{
		semctl (Semaphore, SemaphoreIndex, IPC_RMID, 0);
	}
	shmctl (SharedMemory, IPC_RMID, 0);
	printf("[%s] Close process\n", __func__);
	return 0;
}
