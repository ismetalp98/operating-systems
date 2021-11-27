#include <stdlib.h>
#include <mqueue.h>
#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>

#include "message_th.h"

static mqd_t mqctos;
static mqd_t mqstoc;
static int totalCount;
static int resCount[10];
static int resSum[10];
static int resAvg[10];
static int resMaxNum[10];
static int resRange[1000];

int max(int num1, int num2)
{
    return (num1 > num2 ) ? num1 : num2;
}

void* calcChild(void *arg_ptr)
{
	int sum = 0;
	int count = 0;
	int maxNum = -1;
	struct arg *args;
	args = (struct arg *) arg_ptr;
	
	int i;
	FILE *fp;
	fp = fopen(args->fileName, "r");
	fscanf (fp, "%i", &i);
	while (!feof (fp)) {  
		count++;
		sum += i;
		maxNum = max(maxNum,i);
		if(args->start != -1 && (i < args->start || i > args->end)) { 
			sum -= i; 
			count--;
		}
		fscanf (fp, "%i", &i);   
	}
	fclose (fp);
	resSum[args->t_index] = sum;
	resCount[args->t_index] = count;
	resAvg[args->t_index] = sum / ( count ? count : count + 1 );
	resMaxNum[args->t_index] = maxNum;
	pthread_exit(NULL);
}

// creates childs and pipes
// calcType == 0 --> count
// calcType == 1 --> avg
// calcType == 2 --> max
int calculateCount(char* fileNames[], int start, int end, int fileCount, int calcType){
   
	pthread_t tids[fileCount + 1];	/*thread ids*/
	struct arg t_args[fileCount];	/*thread function arguments*/

	for(int i = 0; i < fileCount; i++){
		int ret;
		t_args[i].start = start;
		t_args[i].end = end;
		t_args[i].t_index = i;
		t_args[i].calcType = calcType;
		t_args[i].fileName = fileNames[i];
		
		if(calcType == 1){ 
			ret = pthread_create(&(tids[i]),NULL, calcChild, (void *) &(t_args[i]));
			struct arg argsCount;
			argsCount.start = start;
			argsCount.end = end;
			argsCount.t_index = fileCount;
			argsCount.calcType = 0;
			argsCount.fileName = fileNames[i];
			ret = pthread_create(&(tids[fileCount]),NULL, calcChild, (void *) &(argsCount));
		} else{ 
			ret = pthread_create(&(tids[i]),NULL, calcChild, (void *) &(t_args[i]));
		}
		
		if (ret != 0) {
			printf("thread create failed \n");
			exit(1);
		}
		printf("thread %i with tid %u created\n", i,
		       (unsigned int) tids[i]);
	}
	
	printf("main: waiting all threads to terminate\n");
	for (int i = 0; i < fileCount; ++i) {
		int ret;
		ret = pthread_join(tids[i], NULL);
		if (ret != 0) {
			printf("thread join failed \n");
			exit(0);
		}
	}
	
	int sum = 0;
	int count = 0;
	int maxNum = 0;
	for (int i = 0; i < fileCount; ++i) {
		if(calcType == 0){
			count += resCount[i];
		} else if(calcType == 1){
			count += resCount[i];
			sum += resSum[i];
		} else {
			maxNum = max(maxNum, resMaxNum[i]);
		}
	}
	totalCount = count;
	return calcType == 2 ? maxNum : ( calcType ? (sum / count) : count);
}

int compare (const void * a, const void * b)
{
 	return ( *(int*)b - *(int*)a );
}

void *calculateRangeChild(void *arg_ptr) {

	struct arg *args;
	args = (struct arg *) arg_ptr;
	int arr[resCount[args->t_index]];
	int i;
	int loc = 0;
	
	FILE *fp;
	fp = fopen(args->fileName, "r");
	fscanf (fp, "%i", &i);
	while (!feof (fp)) {  
		if(i >= args->start && i <= args->end) { 
			arr[loc++] = i;
		}
		fscanf (fp, "%i", &i);   
	}
	fclose (fp);
	for(int i = 0; i < resCount[args->t_index]; i++){
		args->arr[i] = arr[i];
	}
	pthread_exit(NULL);
}

void calculateRange(char* fileNames[], int start, int end, int fileCount, int k){
	int arr[totalCount];
	pthread_t tids[fileCount + 1];	/*thread ids*/
	struct arg t_args[fileCount];	/*thread function arguments*/

	for(int i = 0; i < fileCount; i++){
		int ret;
		t_args[i].start = start;
		t_args[i].end = end;
		t_args[i].t_index = i;
		t_args[i].calcType = -1;
		t_args[i].fileName = fileNames[i];
		t_args[i].k = k;
		
		ret = pthread_create(&(tids[i]),NULL, calculateRangeChild, (void *) &(t_args[i]));
		
		if (ret != 0) {
			printf("thread create failed \n");
			exit(1);
		}
		printf("thread %i with tid %u created\n", i,
		       (unsigned int) tids[i]);
	}
	
	printf("main: waiting all threads to terminate\n");
	for (int i = 0; i < fileCount; ++i) {
		int ret;
		ret = pthread_join(tids[i], NULL);
		if (ret != 0) {
			printf("thread join failed \n");
			exit(0);
		}
	}
	
	int loc = 0;
	for (int i = 0; i < fileCount; i++) {
		int index = 0;
		while(t_args[i].arr[index] != 0){
			arr[loc++] =  t_args[i].arr[index++];
		}
	}
	qsort (arr, totalCount, sizeof(int), compare);
	for (int i = 0; i < loc; i++) {
		resRange[i] = arr[i];
	}
}

// get request from client
struct request getRequest(){
	struct request *requestPtr;
	struct mq_attr mq_attr;
	char *requestBuffer;
	int n;

	mq_getattr(mqctos, &mq_attr);
	
	requestBuffer = (char *) malloc(mq_attr.mq_msgsize);
	n = mq_receive(mqctos, (char *) requestBuffer, mq_attr.mq_msgsize, NULL);
	
	if (n == -1) {
		perror("mq_receive failed\n");
		exit(1);
	}

	requestPtr = (struct request *) requestBuffer;
	free(requestBuffer);
	printf("\nClient request is: %s\n", requestPtr->usersRequest);
	return *requestPtr;
}

void giveResponse(int result) {
	struct response response;
	int n;
	
	response.response = result;
	n = mq_send(mqstoc, (char *) &response, sizeof(struct response), 0);
		
	if (n == -1) {
		perror("mq_send failed\n");
		exit(1);
	}
}

int main(int argc, char *argv[]) {
	
	struct request request;
	int result;

	//open message queues 
	mqctos = mq_open(MQCTOS, O_RDWR);
	mqstoc = mq_open(MQSTOC, O_RDWR);
	if (mqctos == -1 || mqstoc == -1) {
		perror("can not open msg queue\n");
		exit(1);
	}
	printf("mq client to server opened, mq id = %d\n", (int) mqctos);
	printf("mq server to client opened, mq id = %d\n", (int) mqstoc);
	
	request = getRequest(); // get request
	

	while(strcmp(request.usersRequest,"quit") != 0){
		char *str = request.usersRequest;
		int length = 0;
		char *ptr = strtok(str, " ");
		char *requestItems[4];
		while(ptr != NULL) {
			requestItems[length++] = ptr;
			ptr = strtok(NULL, " ");
		}

		// get file names
		char *fileNames[atoi(argv[1])];
		for(int i = 0; i < argc - 2; i++){
			fileNames[i] = argv[i + 2];
		}	
		
		if (strcmp(requestItems[0],"count") == 0) {
			if(length == 1) { // count
				result = calculateCount(fileNames, -1, -1, atoi(argv[1]),0);
			} 
			else {
				result = calculateCount(fileNames, atoi(requestItems[1]), atoi(requestItems[2]), atoi(argv[1]),0);
			}
		} else if (strcmp(requestItems[0],"avg") == 0) { // if request is avg
			if(length == 1) { //avg
				result = calculateCount(fileNames, -1, -1, atoi(argv[1]),1);
			} 
			else {
				result = calculateCount(fileNames, atoi(requestItems[1]), atoi(requestItems[2]), atoi(argv[1]),1);
			}
		} else if (strcmp(requestItems[0],"max") == 0) { // if request is max
			result = calculateCount(fileNames, -1, -1, atoi(argv[1]),2);
		} else if(strcmp(requestItems[0],"range") == 0) { 
			int n; 
			
			calculateCount(fileNames, atoi(requestItems[1]), atoi(requestItems[2]), atoi(argv[1]),0);
			calculateRange(fileNames, atoi(requestItems[1]), atoi(requestItems[2]), atoi(argv[1]),atoi(requestItems[3]));
			int size = totalCount < atoi(requestItems[3]) ? totalCount : atoi(requestItems[3]);
			n = mq_send(mqstoc, (char *) &size, sizeof(int), 0);
			if (n == -1) {
				perror("mq_send failed\n");
				exit(1);
			}
			for(int i = 0; i < size ; i++){
				n = mq_send(mqstoc, (char *) &resRange[i], sizeof(int), 0);
				if (n == -1) {
					perror("mq_send failed\n");
					exit(1);
				}
			}
			request = getRequest(); // get request
			continue;
		}
		giveResponse(result); // send response
		request = getRequest(); // get request
	}


	// close message queus
	mq_close(mqctos);
	mq_close(mqstoc);
	return 0;
}
