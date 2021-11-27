struct request {
	char usersRequest[64];
};

struct response {
	int response;
};

struct arg {
	char *fileName;
	int start;
	int end;
	int calcType;
	int t_index;		/* the index of the created thread */
	int arr[1000];
	int k;
};

#define MQCTOS "/mqctosTh"
#define MQSTOC "/mqstocTh"
