#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <math.h>
#include <time.h>

#define MAX_PROCESSES 100
#define MAX_GANTT_CHART_SIZE 1000
#define MAX_FILENAME_LENGTH 256
#define DEFAULT_NICE_VALUE 0
#define MIN_NICE_VALUE -20
#define MAX_NICE_VALUE 19
#define DEFAULT_TIMESLICE 1
#define MIN_VRUNTIME_THRESHOLD 0.01

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
    int first_execution_time;
    int deadline;
    int criticality;
    int period;
    int nice;
    double vruntime;
    double weight;
    bool executed;
    bool completed;
} Process;

typedef struct
{
    double min_granularity;
    double latency;
    double target_latency;
    int total_weight;
} CFSParams;

typedef struct
{
    int process_id;
    int start_time;
    int end_time;
} GanttChartItem;

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

typedef struct RBNode
{
    Process *process;
    struct RBNode *left;
    struct RBNode *right;
    struct RBNode *parent;
    int color;
} RBNode;

Process processes[MAX_PROCESSES];
GanttChartItem gantt_chart[MAX_GANTT_CHART_SIZE];
int gantt_chart_size = 0;
Metrics metrics;
RBNode *root = NULL;

int readProcessesFromFile(Process *processes, const char *filename);
void writeDefaultInputFile(const char *filename);
void calculateWeight(Process *process);
RBNode *createNode(Process *process);
RBNode *insert(RBNode *root, Process *process);
Process *extractMinVruntime(RBNode **root);
void runCFS(Process *processes, int n, CFSParams *cfs);
void calculateMetrics(Process *processes, int n, int total_time);
void displayGanttChart();
void displayProcessDetails(Process *processes, int n);
void displayMetrics();
void addToGanttChart(int process_id, int start_time, int end_time);

RBNode *insert(RBNode *root, Process *process)
{
    if (root == NULL)
    {
        return createNode(process);
    }

    if (process->vruntime < root->process->vruntime)
    {
        root->left = insert(root->left, process);
        root->left->parent = root;
    }
    else
    {
        root->right = insert(root->right, process);
        root->right->parent = root;
    }

    return root;
}

RBNode *createNode(Process *process)
{
    RBNode *node = (RBNode *)malloc(sizeof(RBNode));
    node->process = process;
    node->left = NULL;
    node->right = NULL;
    node->parent = NULL;
    node->color = 1;
    return node;
}

Process *extractMinVruntime(RBNode **root)
{
    if (*root == NULL)
    {
        return NULL;
    }

    RBNode *current = *root;
    RBNode *parent = NULL;

    while (current->left != NULL)
    {
        parent = current;
        current = current->left;
    }

    Process *process = current->process;

    if (parent == NULL)
    {
        *root = current->right;
        if (*root != NULL)
        {
            (*root)->parent = NULL;
        }
    }
    else
    {
        parent->left = current->right;
        if (current->right != NULL)
        {
            current->right->parent = parent;
        }
    }

    free(current);
    return process;
}

void calculateWeight(Process *process)
{
    process->nice = MAX_NICE_VALUE - (process->criticality * 3);
    if (process->nice < MIN_NICE_VALUE)
        process->nice = MIN_NICE_VALUE;
    if (process->nice > MAX_NICE_VALUE)
        process->nice = MAX_NICE_VALUE;

    process->weight = 1024.0 / (0.8 * process->nice + 1024);
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
                   &processes[i].nice) != 7)
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
        processes[i].vruntime = 0;
        calculateWeight(&processes[i]);
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

void runCFS(Process *processes, int n, CFSParams *cfs)
{
    int current_time = 0;
    int completed_processes = 0;
    int idle_time = 0;
    root = NULL;

    double total_weight = 0;

    for (int i = 0; i < n; i++)
    {
        total_weight += processes[i].weight;
    }

    cfs->total_weight = total_weight;

    while (completed_processes < n)
    {
        for (int i = 0; i < n; i++)
        {
            if (processes[i].arrival_time == current_time && !processes[i].completed)
            {
                if (!processes[i].executed)
                {
                    processes[i].vruntime = 0;
                }
                root = insert(root, &processes[i]);
            }
        }

        if (root == NULL)
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

        Process *current_process = extractMinVruntime(&root);

        double active_processes = n - completed_processes;
        cfs->target_latency = fmax(cfs->min_granularity * active_processes, cfs->latency);

        double timeslice = (current_process->weight / cfs->total_weight) * cfs->target_latency;
        if (timeslice < 1)
            timeslice = 1;

        int execution_time = (int)fmin(timeslice, current_process->remaining_burst);

        if (!current_process->executed)
        {
            current_process->first_execution_time = current_time;
            current_process->executed = true;
        }

        addToGanttChart(current_process->id, current_time, current_time + execution_time);

        current_process->remaining_burst -= execution_time;

        current_process->vruntime += execution_time / current_process->weight;

        current_time += execution_time;

        if (current_process->remaining_burst <= 0)
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
            root = insert(root, current_process);
        }

        for (int i = 0; i < n; i++)
        {
            if (!processes[i].executed && !processes[i].completed &&
                processes[i].arrival_time > current_time - execution_time &&
                processes[i].arrival_time <= current_time)
            {

                processes[i].vruntime = 0;
                root = insert(root, &processes[i]);
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
    printf("ProcessID,ArrivalTime,BurstTime,CompletionTime,TurnaroundTime,WaitingTime,ResponseTime,Deadline,Criticality,Period,Nice,Weight\n");
    for (int i = 0; i < n; i++)
    {
        printf("%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%.2f\n",
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
               processes[i].nice,
               processes[i].weight);
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
    CFSParams cfs;
    char filename[MAX_FILENAME_LENGTH];

    cfs.min_granularity = 1.0;
    cfs.latency = 20.0;
    cfs.target_latency = 20.0;

    if (argc > 1)
    {
        strncpy(filename, argv[1], MAX_FILENAME_LENGTH - 1);
        filename[MAX_FILENAME_LENGTH - 1] = '\0';
    }
    else
    {
        strcpy(filename, "input.txt");
        printf("No input file specified. Using default: input.txt\n");
    }

    n = readProcessesFromFile(processes, filename);

    runCFS(processes, n, &cfs);

    displayMetrics();

    return 0;
}
