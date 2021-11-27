#include <stdlib.h>
#include <mqueue.h>
#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <time.h>

#include "message.h"

static mqd_t mqctos;
static mqd_t mqstoc;

void createMq()
{
	mqd_t mqctos;
	mqctos = mq_open(MQSTOC, O_RDWR | O_CREAT, 0666, NULL);
	
	if (mqctos == -1) {
		perror("can not create msg queue\n");
		exit(1);
	}
	printf("client to server mq created, mq id = %d\n", (int) mqctos);	
	
	mqd_t mqstoc;
	mqstoc = mq_open(MQCTOS, O_RDWR | O_CREAT, 0666, NULL);
	if (mqstoc == -1){
		perror("can not create msg queue\n");
		exit(1);
	}
	printf("server to client mq created, mq id = %d\n", (int) mqstoc);
	mq_close(mqstoc);
	mq_close(mqctos);
}

int compare (const void * a, const void * b)
{
 	return ( *(int*)a - *(int*)b );
}

void getResponse(char *request){

	int n; // check
	
	//response variables
	struct mq_attr mq_attr;
	char *responseBuffer;
	struct response *response;
	int *responseArr;
	
	//get response
	mq_getattr(mqstoc, &mq_attr);
	
	char *ptr = strtok(request, " ");
	
	if(strcmp(ptr,"range") != 0){
		responseBuffer = (char *) malloc(mq_attr.mq_msgsize);
		n = mq_receive(mqstoc, (char *) responseBuffer, mq_attr.mq_msgsize, NULL);
		if (n == -1) {
			perror("mq_receive failed\n");
			exit(1);
		}
		response = (struct response *) responseBuffer;
		printf("Servers response is %d\n\n", response->response);
		free(responseBuffer);
	} else {
		int *sizePtr;
		int size;
		responseBuffer = (char *) malloc(mq_attr.mq_msgsize);
		n = mq_receive(mqstoc, (char *) responseBuffer, mq_attr.mq_msgsize, NULL);
		sizePtr = (int*) responseBuffer;
		size = *sizePtr;
		int arr[size];
		printf("Servers response is \n\n");
		free(responseBuffer);
		for(int i = 0 ; i < size; i++){
			responseBuffer = (char *) malloc(mq_attr.mq_msgsize);
			n = mq_receive(mqstoc, (char *) responseBuffer, mq_attr.mq_msgsize, NULL);
			responseArr = (int*) responseBuffer;
			arr[i] = *responseArr;
			free(responseBuffer);
		}
		qsort (arr, size, sizeof(int), compare);
		for(int i = 0 ; i < size; i++){
			printf("item %d\n", arr[i]);
		}
	}
}

int main()
{	
	int n; // check
	createMq();
	//request variables
	struct request request;
	char input[64];

	//open message queus
	mqctos = mq_open(MQCTOS, O_RDWR);
	mqstoc = mq_open(MQSTOC, O_RDWR);
	
	//check message queus is open
	if (mqctos == -1 || mqstoc == -1) {
		perror("can not open msg queue\n");
		exit(1);
	}
	printf("mq client to server opened, mq id = %d\n", (int) mqctos);
	printf("mq server to client opened, mq id = %d\n", (int) mqstoc);
	
	//get request form user
	printf("Enter request: ");
   	scanf("%[^\n]%*c", input);
   	
	while (strcmp("quit",input) != 0) {
		printf("Client request is: %s\n", input); //print clients request
		strcpy(request.usersRequest, input); //copy request to struct
		n = mq_send(mqctos, (char *) &request, sizeof(struct request), 0); // send request
		
		//check error
		if (n == -1) {
			perror("mq_send failed\n");
			exit(1);
		}
		getResponse(input);
		
		printf("Enter request: ");
		scanf("%[^\n]%*c", input);
	}
	
	printf("Client request is: %s\n", input); //print clients request
	strcpy(request.usersRequest, input); //copy request to struct
	n = mq_send(mqctos, (char *) &request, sizeof(struct request), 0); // send request
		
	//check error
	if (n == -1) {
		perror("mq_send failed\n");
		exit(1);
	}

	mq_close(mqctos);
	mq_close(mqstoc);
	return 0;
}
