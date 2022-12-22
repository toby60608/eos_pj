#include <sys/sem.h>
#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include "SentryMode.h"

int CommandNumber;
int Semaphore;
STRCUT_SENTRY_MODE_SHARED_MEMORY_T *pSentryModeSharedMemory;
key_t SharedMemoryKey = DEFAULT_SHARED_MEMORY_KEY_NUMBER;
int SharedMemory;

int MessageQueueId;

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

void signal_handler(int signum)
{
	printf("signal_handler: caught signal %d\n", signum);
	if (signum == SIGINT)
	{
		printf("SIGINT\n");
		msgctl (pSentryModeSharedMemory->MessageQueueKey, IPC_RMID, NULL);
		shmctl (pSentryModeSharedMemory->SharedMemoryKey, IPC_RMID, 0);
		exit(0);
	}
	else if (signum == SIGUSR1)
	{
		printf("SIGUSR1\n");
	}
}

int main(int argc, char *argv[])
{
	int ret;
	struct sigaction sa;
	sa.sa_handler = signal_handler;
	sigemptyset(&sa.sa_mask);
	sa.sa_flags = 0;
	
	if (sigaction(SIGINT, &sa, NULL) == -1)
	{
		printf("[%s] Failed to caught signal\n", __func__);
		exit(-1);
	}
	/* Create the segment */
	else if ((SharedMemory = shmget(SharedMemoryKey, sizeof(STRCUT_SENTRY_MODE_SHARED_MEMORY_T), IPC_CREAT | 0666)) < 0)
	{
		printf("[%s] shmget failed\n", __func__);
		exit(1);
	}
	else if ((pSentryModeSharedMemory = shmat(SharedMemory, NULL, 0)) == ((void *) -1))
	{
		printf("[%s] shmat failed\n", __func__);
		shmctl (SharedMemory, IPC_RMID, 0);
		exit(1);
	}
	else if ((MessageQueueId = msgget(pSentryModeSharedMemory->MessageQueueKey, 0)) == -1)
	{
#if (PRINT_MESSAGE_ENABLE != 0)
		printf("[%s] msgget failed\n", __func__);
#endif
		exit(-1);
	}
	/* Init complete */
	if (argc > 1)
	{
		CommandNumber = atoi(argv[1]);
		printf("[%s] CommandNumber: %d\n", __func__, CommandNumber);
		switch(CommandNumber)
		{
			case 0:
			{
				printf("[%s] PID: %d\n", __func__, getpid());
				break;
			}
			case 1:
			{
				printf("[%s] SharedMemoryKey: %d\n", __func__, SharedMemoryKey);
				break;
			}
			case 2:
			{
				printf("[%s] SharedMemory->Name: %s\n", __func__, pSentryModeSharedMemory->Name);
				break;
			}
			case 3:
			{
				printf("[%s] SharedMemory->Pid: %d\n", __func__, pSentryModeSharedMemory->SentryModePid);
				break;
			}
			case 4:
			{
				printf("[%s] SharedMemoryKey: %d\n", __func__, pSentryModeSharedMemory->SharedMemoryKey);
				printf("[%s] SemaphoreKey: %d\n", __func__, pSentryModeSharedMemory->SemaphoreKey);
				printf("[%s] MessageQueueKey: %d\n", __func__, pSentryModeSharedMemory->MessageQueueKey);
				break;
			}
			case 5:
			{
				printf("[%s] kill(pSentryModeSharedMemory->SentryModePid, SIGINT)\n", __func__);
				kill(pSentryModeSharedMemory->SentryModePid, SIGINT);
				break;
			}
			case 6:
			{
				printf("[%s] kill(pSentryModeSharedMemory->SentryModePid, SIGUSR1)\n", __func__);
				kill(pSentryModeSharedMemory->SentryModePid, SIGUSR1);
				break;
			}
			case 7:
			{
				printf("[%s] kill(pSentryModeSharedMemory->SentryModePid, SIGUSR2)\n", __func__);
				kill(pSentryModeSharedMemory->SentryModePid, SIGUSR2);
				break;
			}
			case 8:
			{
				printf("[%s] Send loopBack\n", __func__);
				STRUCT_COMMAND_PACKET_T msg =
				{
					.MessageType = SENTRY_MODE_COMMAND_MESSAGE_TYPE,
					.Id = ENUM_SENTRY_MODE_COMMAND_ID_LOOPBACK,
					.Length = 16,
				};
				printf("Send Loopback - id: 0x%02X, length: %d", msg.Id, msg.Length);
				for (int index = 0; index < msg.Length; index++)
				{
					if ((index & 0xF) == 0)
					{
						printf("\n");
					}
					msg.Payload[index] = index;
					printf("0x%02X ", msg.Payload[index]);
				}
				printf("\n");
				ret = msgsnd(MessageQueueId, &msg, sizeof(STRUCT_COMMAND_PACKET_T) - 4, 0);

				memset (&msg, 0, sizeof(STRUCT_COMMAND_PACKET_T));
				ret = msgrcv(MessageQueueId, &msg, sizeof(STRUCT_COMMAND_PACKET_T) - 4, SENTRY_MODE_COMMAND_MESSAGE_TYPE, 0);
				printf("Read Loopback - Id: 0x%02X, Length: %d", msg.Id, msg.Length);
				for (int index = 0; index < msg.Length; index++)
				{
					if ((index & 0xF) == 0)
					{
						printf("\n");
					}
					printf("0x%02X ", msg.Payload[index]);
				}
				printf("\n");
				break;
			}
			case 9:
			{
				printf("[%s] Send CHECK is Sentry mode active command\n", __func__);
				STRUCT_COMMAND_PACKET_T msg =
				{
					.MessageType = SENTRY_MODE_COMMAND_MESSAGE_TYPE,
					.Id = ENUM_SENTRY_MODE_COMMAND_ID_CHECK_IS_SENTRY_MODE_ACTIVE,
					.Length = 0,
				};
				printf("Send Loopback - id: 0x%02X, length: %d", msg.Id, msg.Length);
				for (int index = 0; index < msg.Length; index++)
				{
					if ((index & 0xF) == 0)
					{
						printf("\n");
					}
					msg.Payload[index] = index;
					printf("0x%02X ", msg.Payload[index]);
				}
				printf("\n");
				ret = msgsnd(MessageQueueId, &msg, sizeof(STRUCT_COMMAND_PACKET_T) - 4, 0);

				memset (&msg, 0, sizeof(STRUCT_COMMAND_PACKET_T));
				ret = msgrcv(MessageQueueId, &msg, sizeof(STRUCT_COMMAND_PACKET_T) - 4, SENTRY_MODE_COMMAND_MESSAGE_TYPE, 0);
				printf("Read Loopback - Id: 0x%02X, Length: %d", msg.Id, msg.Length);
				for (int index = 0; index < msg.Length; index++)
				{
					if ((index & 0xF) == 0)
					{
						printf("\n");
					}
					printf("0x%02X ", msg.Payload[index]);
				}
				printf("\n");
				if (msg.Id != (ENUM_SENTRY_MODE_COMMAND_ID_CHECK_IS_SENTRY_MODE_ACTIVE | 0x80))
				{
					printf("Receive Error response ID\n");
				}
				if (msg.Length < 1)
				{
					printf("Receive Error response Length\n");
				}
				else
				{
					printf("Is Active: %d\n", msg.Payload[0]);
				}
				break;
			}
			case 10:
			{
				printf("[%s] Send SUSPEND Sentry mode command\n", __func__);
				STRUCT_COMMAND_PACKET_T msg =
				{
					.MessageType = SENTRY_MODE_COMMAND_MESSAGE_TYPE,
					.Id = ENUM_SENTRY_MODE_COMMAND_ID_SUSPEND_SENTRY_MODE,
					.Length = 0,
				};
				printf("Send Loopback - id: 0x%02X, length: %d", msg.Id, msg.Length);
				for (int index = 0; index < msg.Length; index++)
				{
					if ((index & 0xF) == 0)
					{
						printf("\n");
					}
					msg.Payload[index] = index;
					printf("0x%02X ", msg.Payload[index]);
				}
				printf("\n");
				ret = msgsnd(MessageQueueId, &msg, sizeof(STRUCT_COMMAND_PACKET_T) - 4, 0);

				memset (&msg, 0, sizeof(STRUCT_COMMAND_PACKET_T));
				ret = msgrcv(MessageQueueId, &msg, sizeof(STRUCT_COMMAND_PACKET_T) - 4, SENTRY_MODE_COMMAND_MESSAGE_TYPE, 0);
				printf("Read Loopback - Id: 0x%02X, Length: %d", msg.Id, msg.Length);
				for (int index = 0; index < msg.Length; index++)
				{
					if ((index & 0xF) == 0)
					{
						printf("\n");
					}
					printf("0x%02X ", msg.Payload[index]);
				}
				printf("\n");
				if (msg.Id != (ENUM_SENTRY_MODE_COMMAND_ID_SUSPEND_SENTRY_MODE | 0x80))
				{
					printf("Receive Error response ID\n");
				}
				if (msg.Length < 1)
				{
					printf("Receive Error response Length\n");
				}
				else if (msg.Payload[0] != 0)
				{
					printf("Receive NACK: %d\n", msg.Payload[0]);
				}
				else
				{
					printf("Successful\n");
				}
				break;
			}
			case 11:
			{
				printf("[%s] Send ACTIVE Sentry mode command\n", __func__);
				STRUCT_COMMAND_PACKET_T msg =
				{
					.MessageType = SENTRY_MODE_COMMAND_MESSAGE_TYPE,
					.Id = ENUM_SENTRY_MODE_COMMAND_ID_RESPONSE_ACTIVE_SENTRY_MODE,
					.Length = 0,
				};
				printf("Send Loopback - id: 0x%02X, length: %d", msg.Id, msg.Length);
				for (int index = 0; index < msg.Length; index++)
				{
					if ((index & 0xF) == 0)
					{
						printf("\n");
					}
					msg.Payload[index] = index;
					printf("0x%02X ", msg.Payload[index]);
				}
				printf("\n");
				ret = msgsnd(MessageQueueId, &msg, sizeof(STRUCT_COMMAND_PACKET_T) - 4, 0);

				memset (&msg, 0, sizeof(STRUCT_COMMAND_PACKET_T));
				ret = msgrcv(MessageQueueId, &msg, sizeof(STRUCT_COMMAND_PACKET_T) - 4, SENTRY_MODE_COMMAND_MESSAGE_TYPE, 0);
				printf("Read Loopback - Id: 0x%02X, Length: %d", msg.Id, msg.Length);
				for (int index = 0; index < msg.Length; index++)
				{
					if ((index & 0xF) == 0)
					{
						printf("\n");
					}
					printf("0x%02X ", msg.Payload[index]);
				}
				printf("\n");
				if (msg.Id != (ENUM_SENTRY_MODE_COMMAND_ID_RESPONSE_ACTIVE_SENTRY_MODE | 0x80))
				{
					printf("Receive Error response ID\n");
				}
				if (msg.Length < 1)
				{
					printf("Receive Error response Length\n");
				}
				else if (msg.Payload[0] != 0)
				{
					printf("Receive NACK: %d\n", msg.Payload[0]);
				}
				else
				{
					printf("Successful\n");
				}
				break;
			}
			case 12:
			{
				printf("[%s] Send TAKE PICTURE command\n", __func__);
				STRUCT_COMMAND_PACKET_T msg =
				{
					.MessageType = SENTRY_MODE_COMMAND_MESSAGE_TYPE,
					.Id = ENUM_SENTRY_MODE_COMMAND_ID_TAKE_PICTURES,
					.Length = 0,
				};
				printf("Send - id: 0x%02X, length: %d", msg.Id, msg.Length);
				ret = msgsnd(MessageQueueId, &msg, sizeof(STRUCT_COMMAND_PACKET_T) - 4, 0);

				memset (&msg, 0, sizeof(STRUCT_COMMAND_PACKET_T));
				ret = msgrcv(MessageQueueId, &msg, sizeof(STRUCT_COMMAND_PACKET_T) - 4, SENTRY_MODE_COMMAND_MESSAGE_TYPE, 0);
				printf("Read - Id: 0x%02X, Length: %d", msg.Id, msg.Length);
				for (int index = 0; index < msg.Length; index++)
				{
					if ((index & 0xF) == 0)
					{
						printf("\n");
					}
					printf("0x%02X ", msg.Payload[index]);
				}
				printf("\n");
				if (msg.Id != (ENUM_SENTRY_MODE_COMMAND_ID_RESPONSE_TAKE_PICTURES | 0x80))
				{
					printf("Receive Error response ID\n");
				}
				if (msg.Length < 1)
				{
					printf("Receive Error response Length\n");
				}
				else if (msg.Payload[0] != 0)
				{
					printf("Receive NACK: %d\n", msg.Payload[0]);
				}
				else
				{
					printf("Successful\n");
				}
				break;
			}
			case 13:
			{
				printf("[%s] Send TAKE 2S VIDEO command\n", __func__);
				STRUCT_COMMAND_PACKET_T msg =
				{
					.MessageType = SENTRY_MODE_COMMAND_MESSAGE_TYPE,
					.Id = ENUM_SENTRY_MODE_COMMAND_ID_TAKE_2S_VIDEO,
					.Length = 0,
				};
				printf("Send - id: 0x%02X, length: %d", msg.Id, msg.Length);
				ret = msgsnd(MessageQueueId, &msg, sizeof(STRUCT_COMMAND_PACKET_T) - 4, 0);

				memset (&msg, 0, sizeof(STRUCT_COMMAND_PACKET_T));
				ret = msgrcv(MessageQueueId, &msg, sizeof(STRUCT_COMMAND_PACKET_T) - 4, SENTRY_MODE_COMMAND_MESSAGE_TYPE, 0);
				printf("Read - Id: 0x%02X, Length: %d", msg.Id, msg.Length);
				for (int index = 0; index < msg.Length; index++)
				{
					if ((index & 0xF) == 0)
					{
						printf("\n");
					}
					printf("0x%02X ", msg.Payload[index]);
				}
				printf("\n");
				if (msg.Id != (ENUM_SENTRY_MODE_COMMAND_ID_TAKE_2S_VIDEO | 0x80))
				{
					printf("Receive Error response ID\n");
				}
				if (msg.Length < 1)
				{
					printf("Receive Error response Length\n");
				}
				else if (msg.Payload[0] != 0)
				{
					printf("Receive NACK: %d\n", msg.Payload[0]);
				}
				else
				{
					printf("Successful\n");
				}
				break;
			}
			case 14:
			{
				printf("[%s] Send TAKE 6S VIDEO command\n", __func__);
				STRUCT_COMMAND_PACKET_T msg =
				{
					.MessageType = SENTRY_MODE_COMMAND_MESSAGE_TYPE,
					.Id = ENUM_SENTRY_MODE_COMMAND_ID_TAKE_6S_VIDEO,
					.Length = 0,
				};
				printf("Send - id: 0x%02X, length: %d", msg.Id, msg.Length);
				ret = msgsnd(MessageQueueId, &msg, sizeof(STRUCT_COMMAND_PACKET_T) - 4, 0);

				memset (&msg, 0, sizeof(STRUCT_COMMAND_PACKET_T));
				ret = msgrcv(MessageQueueId, &msg, sizeof(STRUCT_COMMAND_PACKET_T) - 4, SENTRY_MODE_COMMAND_MESSAGE_TYPE, 0);
				printf("Read - Id: 0x%02X, Length: %d", msg.Id, msg.Length);
				for (int index = 0; index < msg.Length; index++)
				{
					if ((index & 0xF) == 0)
					{
						printf("\n");
					}
					printf("0x%02X ", msg.Payload[index]);
				}
				printf("\n");
				if (msg.Id != (ENUM_SENTRY_MODE_COMMAND_ID_TAKE_6S_VIDEO | 0x80))
				{
					printf("Receive Error response ID\n");
				}
				if (msg.Length < 1)
				{
					printf("Receive Error response Length\n");
				}
				else if (msg.Payload[0] != 0)
				{
					printf("Receive NACK: %d\n", msg.Payload[0]);
				}
				else
				{
					printf("Successful\n");
				}
				break;
			}
			default:
			{
				printf("[%s] No Function\n", __func__);
				break;
			}
		}
	}
	else
	{
		printf("%s <Command Number>\n", argv[0]);
	}
	return 0;
}
