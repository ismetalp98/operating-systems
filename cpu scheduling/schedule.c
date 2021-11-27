#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
#include <assert.h>

struct data
{
    int arrivalTime;
    int burstTime;
    int burstId;
    int checked;
    int remaining;
    int turnTime;
};

struct data dataArr[1000];
int size;
int queue[10000];
int front = -1;
int rear = -1;

void insert(int n)
{
    if (front == -1)
        front = 0;
    rear = rear + 1;
    queue[rear] = n;
}

int delete ()
{
    int n;
    n = queue[front];
    front = front + 1;
    return n;
}

int max(int num1, int num2)
{
    return (num1 > num2) ? num1 : num2;
}

void FCFS()
{
    int lastFinish = 0;
    int avarageTime = 0;
    for (int i = 0; i < size; i++)
    {
        if (dataArr[i].arrivalTime > lastFinish)
        { //if cpu is idle
            avarageTime += dataArr[i].arrivalTime - lastFinish;
            lastFinish = dataArr[i].arrivalTime;
        }
        lastFinish += dataArr[i].burstTime;
        avarageTime += lastFinish - dataArr[i].arrivalTime;
    }
    int avg = avarageTime / size;
    if (avarageTime % size > (size / 2))
    {
        avg++;
    }
    printf("FCFS = %d\n", avg);
}

void SJF()
{
    int lastFinish = 0;
    int avarageTime = 0;

    for (int i = 0; i < size; i++)
    {
        int min = INT_MAX;
        int loc = 0;
        for (int j = 0; j < size; j++)
        {
            if (dataArr[j].arrivalTime <= lastFinish && dataArr[j].burstTime <= min && dataArr[j].checked == 0)
            {
                loc = j;
                min = dataArr[j].burstTime;
            }
        }
        if (min == INT_MAX)
        { //if cpu is idle
            loc = i;
            lastFinish = dataArr[i].arrivalTime;
        }

        lastFinish += dataArr[loc].burstTime;
        dataArr[loc].checked = 1;
        avarageTime += lastFinish - dataArr[loc].arrivalTime;
    }

    int avg = avarageTime / size;
    if (avarageTime % size > (size / 2))
    {
        avg++;
    }
    printf("SJF = %d\n", avg);
}

void SRTF()
{
    int reminingProcess = size;
    int timePassed = 0;
    int min = INT_MAX;
    int shorttestLoc = 0;
    int check = 0;
    int turnTime = 0;

    while (reminingProcess != 0)
    {
        for (int j = 0; j < size; j++)
        {
            if ((dataArr[j].arrivalTime <= timePassed) && (dataArr[j].remaining < min) && dataArr[j].remaining != 0)
            {
                min = dataArr[j].remaining;
                shorttestLoc = j;
                check = 1;
            }
        }
        if (check == 0)
        {
            timePassed++;
            continue;
        }

        dataArr[shorttestLoc].remaining--;
        min = dataArr[shorttestLoc].remaining;
        timePassed++;

        if (dataArr[shorttestLoc].remaining == 0)
        {
            reminingProcess--;
            check = 0;
            int waitingTime = timePassed - dataArr[shorttestLoc].burstTime - dataArr[shorttestLoc].arrivalTime;
            turnTime += (waitingTime < 0 ? 0 : waitingTime) + dataArr[shorttestLoc].burstTime;
            min = INT_MAX;
        }
    }
    int avg = turnTime / size;
    if (turnTime % size > (size / 2))
    {
        avg++;
    }
    printf("SRTF = %d\n", avg);
}

void RR(int quantum)
{
    int curProc;
    int exist[10] = {0};
    int lastFinish = 0;
    int avgTurn = 0;
    insert(0);
    exist[0] = 1;
    int lastAdded = 0;

    while (front <= rear)
    {
        curProc = delete ();
        lastAdded = max(lastAdded, curProc);
    
        for (int i = 0; i < quantum && dataArr[curProc].remaining > 0; i++)
        {
            dataArr[curProc].remaining -= 1;
            lastFinish++;
            for (int j = 0; j < size; j++)
            {
                if (exist[j] == 0 && dataArr[j].arrivalTime < lastFinish)
                {
                    insert(j);
                    exist[j] = 1;
                }
            }
        }   

        if (dataArr[curProc].remaining != 0)
        {
            insert(curProc);
        }

        for (int i = 0; i < size; i++)
        {
            if (exist[i] == 0 && dataArr[i].arrivalTime <= lastFinish)
            {
                insert(i);
                exist[i] = 1;
            }
        }

        if (dataArr[curProc].remaining == 0)
        {
            dataArr[curProc].turnTime = lastFinish - dataArr[curProc].arrivalTime;
            avgTurn += dataArr[curProc].turnTime;
        }

        if (rear < front && lastAdded != size - 1)
        {
            insert(lastAdded + 1);
            exist[lastAdded + 1] = 1;
            avgTurn += dataArr[lastAdded + 1].arrivalTime - lastFinish;
            lastFinish = dataArr[lastAdded + 1].arrivalTime;
        }
    }

    int avg = avgTurn / size;
    if (avgTurn % size > (size / 2))
    {
        avg++;
    }
    printf("RR = %d\n", avg);
}

int main(int argc, char *argv[])
{
    int qua = atoi(argv[2]);
    int burstLength;
    int arrivalTime;
    int burstID;
    char *filename = argv[1];

    int i = 0;

    FILE *file;
    file = fopen(filename, "r");
    if (file == NULL)
    {
        perror("Error in file opening\n");
        exit(1);
    }

    while (!feof(file))
    {
        fscanf(file, "%d\n%d\n%d\n", &burstID, &arrivalTime, &burstLength);
        dataArr[i].burstId = burstID;
        dataArr[i].arrivalTime = arrivalTime;
        dataArr[i].burstTime = burstLength;
        dataArr[i].remaining = burstLength;
        i++;
    }
    fclose(file);
    size = i;
    FCFS();
    SJF();
    SRTF();
    for (int i = 0; i < size; i++)
    {
        dataArr[i].remaining = dataArr[i].burstTime;
    }
    RR(qua);
    return 0;
}
