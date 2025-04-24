#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <math.h>
#include <time.h>

#define MAX_PROCESSES 100
#define MAX_QUEUE_SIZE 100
#define MAX_GANTT_CHART_SIZE 1000
#define MAX_FILENAME_LENGTH 256

// Process structure
typedef struct
{
    int id;
    int arrival_time;
    int burst_time;
    int remaining_burst;
    int completion_time;
    int waiting_time;
    int turnaround_time;
    int response_time;
    int first_execution_time; // For response time calculation
    int deadline;             // For real-time processes
    int criticality;          // Higher for safety-critical tasks (1-10)
    int period;               // For periodic tasks
    int system_priority;      // Manual override or industry standard
    bool executed;            // Flag to check if process has started execution
    bool completed;           // Flag to check if process has completed
} Process;

// Dynamic Time Quantum structure
typedef struct
{
    double base;               // Base time quantum
    double current;            // Current time quantum after adjustment
    double load_factor;        // CPU load factor (0.0 to 1.0)
    double criticality_weight; // Weight for criticality (Wc)
    double deadline_weight;    // Weight for deadline (Wf)
    double aging_weight;       // Weight for aging (Wa)
    double priority_weight;    // Weight for system priority (Ws)
} DynamicQuantum;

// Ready Queue structure
typedef struct
{
    Process *processes[MAX_QUEUE_SIZE];
    int front;
    int rear;
    int size;
} ReadyQueue;

// Gantt Chart structure
typedef struct
{
    int process_id;
    int start_time;
    int end_time;
} GanttChartItem;

// Benchmarking metrics
typedef struct
{
    double avg_turnaround_time;
    double avg_waiting_time;
    double avg_response_time;
    double throughput;
    double fairness_index;
    int starvation_count;
    double load_balancing_efficiency;
} Metrics;

Process processes[MAX_PROCESSES];
GanttChartItem gantt_chart[MAX_GANTT_CHART_SIZE];
int gantt_chart_size = 0;
Metrics metrics;

void initializeQueue(ReadyQueue *queue);
bool isQueueEmpty(ReadyQueue *queue);
bool isQueueFull(ReadyQueue *queue);
void enqueue(ReadyQueue *queue, Process *process);
Process *dequeue(ReadyQueue *queue);
void calculateDynamicPriority(Process *process, int current_time, DynamicQuantum *dtq);
double calculateAgingFactor(Process *process, int current_time);
void sortQueueByPriority(ReadyQueue *queue, int current_time, DynamicQuantum *dtq);
void runDPS_DTQ(Process *processes, int n, DynamicQuantum *dtq);
void calculateMetrics(Process *processes, int n, int total_time);
void displayGanttChart();
void displayProcessDetails(Process *processes, int n);
void displayMetrics();
void addToGanttChart(int process_id, int start_time, int end_time);
int readProcessesFromFile(Process *processes, const char *filename);
void writeDefaultInputFile(const char *filename);

void initializeQueue(ReadyQueue *queue)
{
    queue->front = 0;
    queue->rear = -1;
    queue->size = 0;
}

bool isQueueEmpty(ReadyQueue *queue)
{
    return queue->size == 0;
}

bool isQueueFull(ReadyQueue *queue)
{
    return queue->size == MAX_QUEUE_SIZE;
}

void enqueue(ReadyQueue *queue, Process *process)
{
    if (isQueueFull(queue))
    {
        printf("Queue is full! Cannot add more processes.\n");
        return;
    }

    queue->rear = (queue->rear + 1) % MAX_QUEUE_SIZE;
    queue->processes[queue->rear] = process;
    queue->size++;
}

Process *dequeue(ReadyQueue *queue)
{
    if (isQueueEmpty(queue))
    {
        return NULL;
    }

    Process *process = queue->processes[queue->front];
    queue->front = (queue->front + 1) % MAX_QUEUE_SIZE;
    queue->size--;

    return process;
}

double calculateAgingFactor(Process *process, int current_time)
{
    int waiting_time = current_time - process->arrival_time -
                       (process->burst_time - process->remaining_burst);

    double aging_factor = waiting_time > 0 ? (double)waiting_time / 10.0 : 0.0;
    if (aging_factor > 1.0)
        aging_factor = 1.0;

    return aging_factor;
}

void calculateDynamicPriority(Process *process, int current_time, DynamicQuantum *dtq)
{
    double priority = 0.0;

    double criticality_component = process->criticality / 10.0;
    double deadline_component = 0.0;
    if (process->deadline > 0)
    {
        int time_to_deadline = process->deadline - current_time;
        if (time_to_deadline <= 0)
        {
            deadline_component = 1.0;
        }
        else
        {
            deadline_component = 1.0 / (1.0 + time_to_deadline);
        }
    }

    double period_component = 0.0;
    if (process->period > 0)
    {
        period_component = 1.0 / process->period;
    }

    double aging_component = calculateAgingFactor(process, current_time);

    double system_priority_component = process->system_priority / 10.0;
    priority = (dtq->criticality_weight * criticality_component) +
               (dtq->deadline_weight * deadline_component) +
               (dtq->aging_weight * aging_component) +
               (dtq->priority_weight * system_priority_component);

    dtq->current = dtq->base * (1.0 + priority) * (1.0 - 0.5 * dtq->load_factor);

    process->system_priority = (int)(priority * 100);
}

void sortQueueByPriority(ReadyQueue *queue, int current_time, DynamicQuantum *dtq)
{
    for (int i = 0; i < queue->size; i++)
    {
        int idx = (queue->front + i) % MAX_QUEUE_SIZE;
        calculateDynamicPriority(queue->processes[idx], current_time, dtq);
    }

    for (int i = 0; i < queue->size - 1; i++)
    {
        for (int j = 0; j < queue->size - i - 1; j++)
        {
            int idx1 = (queue->front + j) % MAX_QUEUE_SIZE;
            int idx2 = (queue->front + j + 1) % MAX_QUEUE_SIZE;

            if (queue->processes[idx1]->system_priority < queue->processes[idx2]->system_priority)
            {
                Process *temp = queue->processes[idx1];
                queue->processes[idx1] = queue->processes[idx2];
                queue->processes[idx2] = temp;
            }
        }
    }
}

void addToGanttChart(int process_id, int start_time, int end_time)
{
    if (gantt_chart_size < MAX_GANTT_CHART_SIZE)
    {
        gantt_chart[gantt_chart_size].process_id = process_id;
        gantt_chart[gantt_chart_size].start_time = start_time;
        gantt_chart[gantt_chart_size].end_time = end_time;
        gantt_chart_size++;
    }
    else
    {
        printf("Gantt chart is full!\n");
    }
}

int readProcessesFromFile(Process *processes, const char *filename)
{
    FILE *file = fopen(filename, "r");
    if (file == NULL)
    {
        printf("Error opening file %s. Creating a default input file...\n", filename);
        writeDefaultInputFile(filename);

        file = fopen(filename, "r");
        if (file == NULL)
        {
            printf("Failed to create default input file. Exiting...\n");
            exit(1);
        }
        printf("Default input file created successfully.\n");
    }

    int n;
    if (fscanf(file, "%d", &n) != 1)
    {
        printf("Error reading number of processes from file.\n");
        fclose(file);
        exit(1);
    }

    if (n <= 0 || n > MAX_PROCESSES)
    {
        printf("Invalid number of processes: %d (must be between 1 and %d)\n", n, MAX_PROCESSES);
        fclose(file);
        exit(1);
    }

    for (int i = 0; i < n; i++)
    {
        if (fscanf(file, "%d %d %d %d %d %d %d",
                   &processes[i].id,
                   &processes[i].arrival_time,
                   &processes[i].burst_time,
                   &processes[i].deadline,
                   &processes[i].criticality,
                   &processes[i].period,
                   &processes[i].system_priority) != 7)
        {
            printf("Error reading data for process %d\n", i + 1);
            fclose(file);
            exit(1);
        }

        processes[i].remaining_burst = processes[i].burst_time;
        processes[i].completion_time = 0;
        processes[i].waiting_time = 0;
        processes[i].turnaround_time = 0;
        processes[i].response_time = 0;
        processes[i].first_execution_time = -1;
        processes[i].executed = false;
        processes[i].completed = false;
    }

    fclose(file);
    return n;
}

void writeDefaultInputFile(const char *filename)
{
    FILE *file = fopen(filename, "w");
    if (file == NULL)
    {
        printf("Error creating default input file.\n");
        return;
    }

    int n = 10;
    fprintf(file, "%d\n", n);

    fprintf(file, "1 0 8 20 7 0 5\n");
    fprintf(file, "2 2 4 15 9 0 8\n");
    fprintf(file, "3 4 2 10 6 10 3\n");
    fprintf(file, "4 6 6 25 3 0 4\n");
    fprintf(file, "5 8 5 0 5 12 6\n");
    fprintf(file, "6 10 3 18 8 0 7\n");
    fprintf(file, "7 12 7 30 4 15 5\n");
    fprintf(file, "8 14 1 17 10 0 9\n");
    fprintf(file, "9 16 9 0 2 20 2\n");
    fprintf(file, "10 18 4 25 7 0 6\n");

    fclose(file);
}

void runDPS_DTQ(Process *processes, int n, DynamicQuantum *dtq)
{
    ReadyQueue ready_queue;
    initializeQueue(&ready_queue);

    int current_time = 0;
    int completed_processes = 0;
    int idle_time = 0;

    while (completed_processes < n)
    {
        for (int i = 0; i < n; i++)
        {
            if (processes[i].arrival_time == current_time)
            {
                enqueue(&ready_queue, &processes[i]);
            }
        }

        if (isQueueEmpty(&ready_queue))
        {
            current_time++;
            idle_time++;
            if (idle_time == 1)
            {
                addToGanttChart(-1, current_time - 1, current_time);
            }
            else
            {
                gantt_chart[gantt_chart_size - 1].end_time = current_time;
            }
            continue;
        }
        else
        {
            idle_time = 0;
        }

        dtq->load_factor = (double)ready_queue.size / n;

        sortQueueByPriority(&ready_queue, current_time, dtq);

        Process *current_process = dequeue(&ready_queue);

        if (!current_process->executed)
        {
            current_process->first_execution_time = current_time;
            current_process->executed = true;
        }

        calculateDynamicPriority(current_process, current_time, dtq);
        int time_quantum = (int)dtq->current;
        if (time_quantum < 1)
            time_quantum = 1;
        int execution_time = (current_process->remaining_burst < time_quantum) ? current_process->remaining_burst : time_quantum;

        addToGanttChart(current_process->id, current_time, current_time + execution_time);

        current_process->remaining_burst -= execution_time;
        current_time += execution_time;

        if (current_process->remaining_burst == 0)
        {
            current_process->completed = true;
            current_process->completion_time = current_time;
            current_process->turnaround_time = current_process->completion_time - current_process->arrival_time;
            current_process->waiting_time = current_process->turnaround_time - current_process->burst_time;
            current_process->response_time = current_process->first_execution_time - current_process->arrival_time;
            completed_processes++;
        }
        else
        {
            enqueue(&ready_queue, current_process);
        }

        for (int i = 0; i < n; i++)
        {
            if (!processes[i].executed &&
                processes[i].arrival_time > current_time - execution_time &&
                processes[i].arrival_time <= current_time)
            {
                enqueue(&ready_queue, &processes[i]);
            }
        }
    }

    calculateMetrics(processes, n, current_time);
}

void calculateMetrics(Process *processes, int n, int total_time)
{
    double total_turnaround_time = 0.0;
    double total_waiting_time = 0.0;
    double total_response_time = 0.0;
    double sum_of_squares = 0.0;
    double sum = 0.0;
    int starvation_threshold = 20;
    int starved_count = 0;

    for (int i = 0; i < n; i++)
    {
        total_turnaround_time += processes[i].turnaround_time;
        total_waiting_time += processes[i].waiting_time;
        total_response_time += processes[i].response_time;

        sum += processes[i].turnaround_time;
        sum_of_squares += (double)processes[i].turnaround_time * processes[i].turnaround_time;

        if (processes[i].waiting_time > starvation_threshold)
        {
            starved_count++;
        }
    }

    metrics.avg_turnaround_time = total_turnaround_time / n;
    metrics.avg_waiting_time = total_waiting_time / n;
    metrics.avg_response_time = total_response_time / n;

    metrics.throughput = (double)n / total_time;

    metrics.fairness_index = (sum * sum) / (n * sum_of_squares);

    metrics.starvation_count = starved_count;

    double mean_waiting_time = total_waiting_time / n;
    double variance = 0.0;

    for (int i = 0; i < n; i++)
    {
        variance += pow(processes[i].waiting_time - mean_waiting_time, 2);
    }

    double std_dev = sqrt(variance / n);
    double coefficient_of_variation = std_dev / mean_waiting_time;

    metrics.load_balancing_efficiency = 1.0 / (1.0 + coefficient_of_variation);
}

void displayGanttChart()
{
    printf("\n\nGantt Chart:\n");

    printf(" ");
    for (int i = 0; i < gantt_chart_size; i++)
    {
        int duration = gantt_chart[i].end_time - gantt_chart[i].start_time;
        for (int j = 0; j < duration; j++)
        {
            printf("--");
        }
        printf(" ");
    }
    printf("\n|");

    for (int i = 0; i < gantt_chart_size; i++)
    {
        int duration = gantt_chart[i].end_time - gantt_chart[i].start_time;
        for (int j = 0; j < duration; j++)
        {
            if (gantt_chart[i].process_id == -1)
            {
                printf("I ");
            }
            else
            {
                printf("P%d", gantt_chart[i].process_id);
            }
            if (j < duration - 1)
            {
                printf(" ");
            }
        }
        printf("|");
    }

    printf("\n ");
    for (int i = 0; i < gantt_chart_size; i++)
    {
        int duration = gantt_chart[i].end_time - gantt_chart[i].start_time;
        for (int j = 0; j < duration; j++)
        {
            printf("--");
        }
        printf(" ");
    }

    printf("\n");
    for (int i = 0; i < gantt_chart_size; i++)
    {
        printf("%2d", gantt_chart[i].start_time);
        int duration = gantt_chart[i].end_time - gantt_chart[i].start_time;
        for (int j = 0; j < duration * 2 - 1; j++)
        {
            printf(" ");
        }
    }
    printf("%2d\n", gantt_chart[gantt_chart_size - 1].end_time);
}

void displayProcessDetails(Process *processes, int n)
{
    printf("ProcessID,ArrivalTime,BurstTime,CompletionTime,TurnaroundTime,WaitingTime,ResponseTime,Deadline,Criticality,Period,SystemPriority\n");

    for (int i = 0; i < n; i++)
    {
        printf("%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d\n",
               processes[i].id,
               processes[i].arrival_time,
               processes[i].burst_time,
               processes[i].completion_time,
               processes[i].turnaround_time,
               processes[i].waiting_time,
               processes[i].response_time,
               processes[i].deadline,
               processes[i].criticality,
               processes[i].period,
               processes[i].system_priority);
    }
}

void displayMetrics()
{
    printf("Metric,Value\n");
    printf("Average Turnaround Time,%.2f\n", metrics.avg_turnaround_time);
    printf("Average Waiting Time,%.2f\n", metrics.avg_waiting_time);
    printf("Average Response Time,%.2f\n", metrics.avg_response_time);
    printf("Throughput,%.2f\n", metrics.throughput);
    printf("Fairness Index,%.2f\n", metrics.fairness_index);
    printf("Starvation Count,%d\n", metrics.starvation_count);
    printf("Load Balancing Efficiency,%.2f\n", metrics.load_balancing_efficiency);
}

int main(int argc, char *argv[])
{
    int n;
    DynamicQuantum dtq;
    char filename[MAX_FILENAME_LENGTH];

    dtq.base = 4.0;
    dtq.current = dtq.base;
    dtq.load_factor = 0.0;
    dtq.criticality_weight = 0.35;
    dtq.deadline_weight = 0.30;
    dtq.aging_weight = 0.25;
    dtq.priority_weight = 0.10;

    if (argc > 1)
    {
        strncpy(filename, argv[1], MAX_FILENAME_LENGTH - 1);
        filename[MAX_FILENAME_LENGTH - 1] = '\0';
    }
    else
    {
        strcpy(filename, "input.txt");
        printf("No input file specified. Using default: %s\n", filename);
    }

    n = readProcessesFromFile(processes, filename);

    runDPS_DTQ(processes, n, &dtq);

    displayMetrics();

    return 0;
}
