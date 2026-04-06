#include <iostream>
using namespace std;


// global variables and constraints

const int TIME_QUANTUM = 4;
const int SIGNAL_INTERVAL = 3;
const int MAX_PROCESSES = 100;

int q1[MAX_PROCESSES], front1 = 0, rear1 = 0; // priority 1
int q2[MAX_PROCESSES], front2 = 0, rear2 = 0; // priority 2
int q3[MAX_PROCESSES], front3 = 0, rear3 = 0; // priority 3
  

// structures
struct GanttEntry {
    int pid;
    int startTime;
    int endTime;
};

struct Process {
    int pid;
    char type;
    int priority;
    int totalBurst;
    int remainingBurst;
    int executed;
    int quantumUsed;
    int signalCount;
    bool finished;
};

int getPriority(char type) {
    if (type == 'A') return 2;
    if (type == 'B') return 3;
    return 1; // else (type == 'C')
}

int getBurst(char type) {
    if (type == 'A') return 10;
    if (type == 'B') return 7;
    return 5;  // else (type == 'C')
}

char getChildType(char type) {
    if (type == 'A') return 'B';
    if (type == 'B') return 'C';
    return '-';  // C cannot fork
}

bool canFork(char type) {
    return (type == 'A' || type == 'B');
}

Process makeProcess(int pid, char type) {
    Process p;
    p.pid = pid;
    p.type = type;
    p.priority = getPriority(type);
    p.totalBurst = getBurst(type);
    p.remainingBurst = getBurst(type);
    p.executed = 0;
    p.quantumUsed = 0;
    p.signalCount = 0;
    p.finished = false;
    return p;
}


// scheduling logic functions

void runOneTick(Process& p) {
    if (p.finished) return;

    p.remainingBurst--;
    p.executed++;
    p.quantumUsed++;

    if (p.remainingBurst == 0) {
        p.finished = true;
    }
}


void enqueueByPriority(Process processes[], int processIndex) {
    int priority = processes[processIndex].priority;

    if (priority == 1) {
        q1[rear1++] = processIndex;
    }
    else if (priority == 2) {
        q2[rear2++] = processIndex;
    }
    else {
        q3[rear3++] = processIndex;
    }
}

void checkAndCreateFork(Process processes[], int& processCount, int& nextPid, Process& runningProcess) {
    if (!canFork(runningProcess.type)) {
        return;
    }

    if (runningProcess.executed > 0 && runningProcess.executed % 3 == 0) {
        char childType = getChildType(runningProcess.type);
        processes[processCount] = makeProcess(nextPid, childType);
        enqueueByPriority(processes,processCount);

        processCount++;
        nextPid++;
    }
}

// helper functions for scheduling logic

bool hasReadyProcess() {
    return (front1 < rear1) || (front2 < rear2) || (front3 < rear3);
}


int dequeueHighestPriority() {
    if (front1 < rear1) return q1[front1++];
    if (front2 < rear2) return q2[front2++];
    if (front3 < rear3) return q3[front3++];
    return -1;
}

void enqueueFrontQ2(int processIndex) {
    if (front2 > 0) {
        front2--;
        q2[front2] = processIndex;
    }
}

void enqueueFrontQ3(int processIndex) {
    if (front3 > 0) {
        front3--;
        q3[front3] = processIndex;
    }
}

bool higherPriorityReady(int priority) {
    if (priority == 2) {
        return (front1 < rear1); // only Q1 is higher than Q2
    }
    if (priority == 3) {
        return (front1 < rear1) || (front2 < rear2); // Q1 or Q2 higher than Q3
    }
    return false; // nothing higher than Q1
}

void requeueAfterQuantum(Process processes[], int processIndex) {
    int priority = processes[processIndex].priority;

    if (higherPriorityReady(priority)) {
        if (priority == 2) {
            enqueueFrontQ2(processIndex);
        } else if (priority == 3) {
            enqueueFrontQ3(processIndex);
        }
    } else {
        enqueueByPriority(processes, processIndex);
    }
}

void handleSchedulingBoundary(Process processes[], int& runningIndex,GanttEntry gantt[], int& ganttCount, int& sliceStartTime, int currentTime) {
    if (runningIndex == -1) return;

    // Case 1: process finished
    if (processes[runningIndex].finished) {

        // record the gantt slice
        gantt[ganttCount].pid = processes[runningIndex].pid;
        gantt[ganttCount].startTime = sliceStartTime;
        gantt[ganttCount].endTime = currentTime;
        ganttCount++;

        processes[runningIndex].quantumUsed = 0;
        runningIndex = dequeueHighestPriority();

        if (runningIndex != -1) {
            sliceStartTime = currentTime;
        }

        return;
    }

    // Case 2: quantum expired
    if (processes[runningIndex].quantumUsed == TIME_QUANTUM) {

        // record the gantt slice
        gantt[ganttCount].pid = processes[runningIndex].pid;
        gantt[ganttCount].startTime = sliceStartTime;
        gantt[ganttCount].endTime = currentTime;
        ganttCount++;

        processes[runningIndex].quantumUsed = 0;

        requeueAfterQuantum(processes, runningIndex);

        runningIndex = dequeueHighestPriority();

        if (runningIndex != -1) {
            sliceStartTime = currentTime;
        }
    }
}


void handleSignal(Process processes[], int runningIndex, int currentTime) {
    if (runningIndex == -1) return;

    if (currentTime % SIGNAL_INTERVAL == 0) {
        processes[runningIndex].signalCount++;

    }
}


int main() {
    Process processes[MAX_PROCESSES];
    int processCount = 0;
    int nextPid = 1;

    processes[processCount++] = makeProcess(nextPid++, 'A'); // P1
    processes[processCount++] = makeProcess(nextPid++, 'B'); // P2

    enqueueByPriority(processes, 0); // P1
    enqueueByPriority(processes, 1); // P2

    int currentTime = 0;
    int runningIndex = dequeueHighestPriority();

    GanttEntry gantt[MAX_PROCESSES * 20];
    int ganttCount = 0;

    int sliceStartTime = 0;

    while (runningIndex != -1 || hasReadyProcess()) {
        // pick the next process here
        if (runningIndex == -1) {
            runningIndex = dequeueHighestPriority();
        }

        if (runningIndex == -1) {
            break;
        }

        runOneTick(processes[runningIndex]);
        currentTime++;

        checkAndCreateFork(processes, processCount, nextPid, processes[runningIndex]);

        handleSchedulingBoundary(processes, runningIndex, gantt, ganttCount, sliceStartTime, currentTime);

        handleSignal(processes, runningIndex, currentTime);
    }

    // printing Gantt by iterating over the recorded slices

    cout << "\nGantt Chart:\n";
    for (int i = 0; i < ganttCount; i++) {
        cout << "[" << gantt[i].startTime << "-" << gantt[i].endTime << "] : "
            << "P" << gantt[i].pid << '\n';
    }

    // printing signal counts for each process
    cout << "\nSignal Counts:\n";
    for (int i = 0; i < processCount; i++) {
        cout << "P" << processes[i].pid << " : "
             << processes[i].signalCount << '\n';
    }

    return 0;
}