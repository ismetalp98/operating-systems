#include <stdlib.h>
#include <mqueue.h>
#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <time.h>

#include "message.h"

static mqd_t mqctos; //mq client to server
static mqd_t mqstoc; //mq server to client
static int totalCount; //total count

//find max
int max(int num1, int num2)
{
    return (num1 > num2 ) ? num1 : num2;
}

//makes calculations
// calcType == 0 --> count
// calcType == 1 --> avg
// calcType == 2 --> max
int calculateChild(char fileName[], int start, int end, int calcType) {
	int sum = 0;
	int count = 0;
	int maxNum = 0;
	int i;
	FILE *fp;
	fp = fopen(fileName, "r");
	fscanf (fp, "%i", &i);
	while (!feof (fp)) {  
		count++;
		sum += i;
		maxNum = max(maxNum,i);
		if(start != -1 && (i < start || i > end)) { 
			sum -= i; 
			count--;
		}
		fscanf (fp, "%i", &i);   
	}
	fclose (fp);   
	return calcType == 2 ? maxNum : ( calcType ? sum : count);
}

// creates childs and pipes
// calcType == 0 --> count
// calcType == 1 --> avg
// calcType == 2 --> max
int calculateCount(char* fileNames[], int start, int end, int fileCount, int calcType){

	int sum = 0;
	int countRes = 0;
	int maxNum = 0;

	// create pipes
	int fd[fileCount][2];
	for(int i = 0; i < fileCount; i++){
		if (pipe(fd[i]) < 0) {
	    		printf ("could not create pipe\n"); 
	    		exit (1); 
		}
	}
	
	//create childs and get the results
	pid_t  n;
	for(int i = 0; i < fileCount; i++){
		n = fork();
		if(n == 0){
			int res = 0;
			int count = 0;
			close(fd[i][0]);  // close the read end
			if(calcType == 1){ // count
				res = calculateChild(fileNames[i], start, end, 1);
				count = calculateChild(fileNames[i], start, end, 0);
				printf("Result from child %d sum is: %d , count is: %d\n",getpid(), res, count);
				write (fd[i][1], &count, sizeof(int));
			} else{ //avg
				res = calculateChild(fileNames[i], start, end, calcType);
				printf("Result from child %d is= %d\n",getpid(), res);
			}
			write (fd[i][1], &res, sizeof(int));
			close(fd[i][1]); 
			exit(0);
		}
	}
	
	//if calc avarage
	if(calcType == 1) {
		int sumItem = 0;
		int countItem = 0;
		for(int i = 0; i < fileCount; i++){
			close(fd[i][1]);
			read(fd[i][0], &countItem, sizeof(int)); //get item count
			countRes += countItem;
			read(fd[i][0], &sumItem, sizeof(int)); // get item sum
			sum += sumItem;
			close(fd[i][0]);
		}
		printf("total sum is: %d total count is: %d\n", sum, countRes);
		return  sum / (countRes ? countRes : 1);
	} else {
		for(int i = 0; i < fileCount; i++){
			close(fd[i][1]);
			int item;
			read(fd[i][0], &item, sizeof(int));
			countRes += item;
			maxNum = max(maxNum,item);
			close(fd[i][0]);
		}
		int ant = calcType == 2 ? maxNum : countRes;
		return ant;
	}
	fflush (stdout); 
	return -1;
}

int compare (const void * a, const void * b)
{
 	return ( *(int*)b - *(int*)a );
}

int* calculateRangeChild(char fileName[], int start, int end, int k, int *a) {

	int i;
	int loc = 0;
	
	FILE *fp;
	fp = fopen(fileName, "r");
	fscanf (fp, "%i", &i);
	while (!feof (fp)) {  
		if(i <= end && i >= start){
			a[loc++] = i;
		}
		fscanf (fp, "%i", &i);      
	}
	fclose (fp);
	
	return a;
}

int* calculateRange(char* fileNames[], int start, int end, int fileCount, int k,	int *a){
	int size = 0;
	int fd[fileCount][2];
	for(int i = 0; i < fileCount; i++){
		if (pipe(fd[i]) < 0) {
	    		printf ("could not create pipe\n"); 
	    		exit (1); 
		}
	}
	
	//create childs and get the results
	pid_t  n;
	for(int i = 0; i < fileCount; i++){
		n = fork();
		if(n == 0){
			int *n;  
  			int thisSize;
  			close(fd[i][0]);  // close the read end
			thisSize = calculateChild(fileNames[i], start, end, 0);
			write (fd[i][1], &thisSize, sizeof(int));
			if(thisSize != 0){
				int a[thisSize];
				n = calculateRangeChild(fileNames[i], start, end, k, a);
				for(int j = 0 ; j < thisSize; j++){
					write (fd[i][1], &n[j], sizeof(int));
				}
			}				
			close(fd[i][1]); 
			exit(0);
		}
	}
  	
	int loc = 0; 
	for(int i = 0; i < fileCount; i++){
		close(fd[i][1]);
		int count;
		read(fd[i][0], &count, sizeof(int));
		size += count;
            		
	}
	int arr[size];
	for(int i = 0; i < fileCount; i++){
		int count;
		while (read(fd[i][0], &count, sizeof(int)) > 0) {
            		arr[loc++] = count;
	    	}	
		close(fd[i][0]);
	}

	qsort (arr, size, sizeof(int), compare);
	loc = 0;
	for(int i = 0; i < size && loc < k; i++){
		a[loc++] = arr[i];
	}
	totalCount = loc;
	fflush (stdout); 
	return a;
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

// to send result to the client
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
	
		//split request into its components
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
			else { //count with avarage
				result = calculateCount(fileNames, atoi(requestItems[1]), atoi(requestItems[2]), atoi(argv[1]),0);
			}
		} else if (strcmp(requestItems[0],"avg") == 0) { // if request is avg
			if(length == 1) { //avg 
				result = calculateCount(fileNames, -1, -1, atoi(argv[1]),1);
			} 
			else { //avg with range
				result = calculateCount(fileNames, atoi(requestItems[1]), atoi(requestItems[2]), atoi(argv[1]),1);
			}
		} else if (strcmp(requestItems[0],"max") == 0) { // if request is max
			result = calculateCount(fileNames, -1, -1, atoi(argv[1]),2);
		}else if(strcmp(requestItems[0],"range") == 0) { 
			int n; 
			int *arr ;
			int size = atoi(requestItems[3]);
	  		int a[size];
			arr = calculateRange(fileNames, atoi(requestItems[1]), atoi(requestItems[2]), atoi(argv[1]),size, a);
			size = totalCount < atoi(requestItems[3]) ? totalCount : atoi(requestItems[3]);
			n = mq_send(mqstoc, (char *) &size, sizeof(int), 0);
			if (n == -1) {
				perror("mq_send failed\n");
				exit(1);
			}
			for(int i = 0; i < size ; i++){
				n = mq_send(mqstoc, (char *) &arr[i], sizeof(int), 0);
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
