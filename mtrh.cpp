#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <queue>
#include <thread>
#include <mutex>
#include <pthread.h>
#include <chrono>
#include <algorithm>
#include <vector>

using namespace std;
using namespace chrono;

// Structure representing a request in the system
struct Request
{
    int id;
    int type;
    int required_resources;
    high_resolution_clock::time_point arrival_time;
    high_resolution_clock::time_point start_time;
    high_resolution_clock::time_point finish_time;
    milliseconds waiting_duration;
    milliseconds turnaround_duration;
};

// Structure representing a worker thread
struct Worker
{
    int priority_level;
    int max_resources;
    int current_resources;
    pthread_mutex_t lock;
    bool completed_requests;
};

// Structure representing a service type
struct Service
{
    int id;
    int total_threads;
    queue<Request *> request_queue;
    pthread_mutex_t queue_lock;
    vector<Worker> workers;
    bool all_requests_completed;
};

vector<Service> service_list;
int rejected_requests = 0;
int forced_wait_requests = 0;
int blocked_requests = 0;
pthread_mutex_t reject_lock;
pthread_mutex_t wait_lock;
pthread_mutex_t block_lock;
vector<int> executed_requests;

// Function to check if the service queue is empty
bool is_queue_empty(int service_index)
{
    pthread_mutex_lock(&service_list[service_index].queue_lock);
    bool empty = service_list[service_index].request_queue.empty();
    pthread_mutex_unlock(&service_list[service_index].queue_lock);
    return empty;
}

// Get the front request from the service queue
Request *get_front_request(int service_index)
{
    pthread_mutex_lock(&service_list[service_index].queue_lock);
    Request *front_request = service_list[service_index].request_queue.front();
    service_list[service_index].request_queue.pop();
    pthread_mutex_unlock(&service_list[service_index].queue_lock);
    return front_request;
}

// Add a request to the service queue
void add_request_to_queue(int service_index, Request *request)
{
    pthread_mutex_lock(&service_list[service_index].queue_lock);
    service_list[service_index].request_queue.push(request);
    pthread_mutex_unlock(&service_list[service_index].queue_lock);
}

// Check if execution queue is empty
bool is_exec_queue_empty(int service_index, int thread_index, queue<Request *> &exec_queue)
{
    pthread_mutex_lock(&service_list[service_index].workers[thread_index].lock);
    bool empty = exec_queue.empty();
    pthread_mutex_unlock(&service_list[service_index].workers[thread_index].lock);
    return empty;
}

// Get the front request from the execution queue
Request *get_exec_queue_front(int service_index, int thread_index, queue<Request *> &exec_queue)
{
    pthread_mutex_lock(&service_list[service_index].workers[thread_index].lock);
    Request *request = exec_queue.front();
    exec_queue.pop();
    pthread_mutex_unlock(&service_list[service_index].workers[thread_index].lock);
    return request;
}

// Main function to process a request
void process_request(int service_index, int thread_index, Request *request)
{
    request->start_time = high_resolution_clock::now(); // Request processing start time
    cout << "Service: " << service_index << ", Thread: " << thread_index << ", Request: " << request->id << " started processing." << endl;

    executed_requests.push_back(request->id);
    sleep(5); // Simulate processing time

    cout << "Service: " << service_index << ", Thread: " << thread_index << ", Request: " << request->id << " finished processing." << endl;

    request->finish_time = high_resolution_clock::now(); // Request processing end time
    pthread_mutex_lock(&service_list[service_index].workers[thread_index].lock);
    service_list[service_index].workers[thread_index].current_resources += request->required_resources; // Update available resources
    pthread_mutex_unlock(&service_list[service_index].workers[thread_index].lock);
    request->turnaround_duration = duration_cast<milliseconds>(request->finish_time - request->arrival_time); // Turnaround time calculation
    request->waiting_duration = duration_cast<milliseconds>(request->start_time - request->arrival_time);     // Waiting time calculation
}

// Function representing a worker thread that processes requests
void worker_function(int service_index, int thread_index, queue<Request *> &exec_queue)
{
    vector<thread> active_threads;
    while (true)
    {
        if (!is_exec_queue_empty(service_index, thread_index, exec_queue))
        {
            Request *request = get_exec_queue_front(service_index, thread_index, exec_queue);
            active_threads.emplace_back(process_request, service_index, thread_index, request);
        }
        else if (service_list[service_index].workers[thread_index].completed_requests)
        {
            break;
        }
        usleep(10); // Short sleep to avoid busy-waiting
    }

    for (thread &thr : active_threads)
    {
        thr.join();
    }
}

// Custom comparator for sorting workers by priority
bool compare_priority(Worker w1, Worker w2)
{
    return w1.priority_level < w2.priority_level;
}

// Function to manage a service and assign requests to worker threads
void manage_service(int service_index)
{
    thread service_threads[service_list[service_index].total_threads];            // Array of worker threads
    queue<Request *> exec_queue[service_list[service_index].total_threads];       // Separate execution queue for each worker thread

    sort(service_list[service_index].workers.begin(), service_list[service_index].workers.end(), compare_priority); // Sort workers by priority

    for (int thread_index = 0; thread_index < service_list[service_index].total_threads; thread_index++)
    {
        pthread_mutex_init(&service_list[service_index].workers[thread_index].lock, NULL);
        cout << "Service: " << service_list[service_index].id << ", Thread: " << thread_index << ", Priority: " << service_list[service_index].workers[thread_index].priority_level << ", Resources: " << service_list[service_index].workers[thread_index].max_resources << endl;

        service_threads[thread_index] = thread(worker_function, service_index, thread_index, ref(exec_queue[thread_index]));
    }

    while (true)
    {
        if (!is_queue_empty(service_index))
        {
            Request *request = get_front_request(service_index); // Fetch a request from the service queue

            bool request_handled = false;
            for (int thread_index = 0; thread_index < service_list[service_index].total_threads; thread_index++)
            {
                pthread_mutex_lock(&service_list[service_index].workers[thread_index].lock);
                if (request->required_resources <= service_list[service_index].workers[thread_index].current_resources)
                {
                    service_list[service_index].workers[thread_index].current_resources -= request->required_resources;
                    exec_queue[thread_index].push(request);
                    request_handled = true;
                    pthread_mutex_unlock(&service_list[service_index].workers[thread_index].lock);
                    break;
                }
                pthread_mutex_unlock(&service_list[service_index].workers[thread_index].lock);
            }

            if (!request_handled)
            {
                add_request_to_queue(service_index, request);
                pthread_mutex_lock(&wait_lock);
                forced_wait_requests++;
                pthread_mutex_unlock(&wait_lock);
            }
        }
        else if (service_list[service_index].all_requests_completed)
        {
            break;
        }
        usleep(100);
    }

    for (int thread_index = 0; thread_index < service_list[service_index].total_threads; thread_index++)
    {
        service_threads[thread_index].join();
        pthread_mutex_destroy(&service_list[service_index].workers[thread_index].lock);
    }
}

int main()
{
    srand(time(NULL));
    int num_services, num_threads;

    cout << "Enter the number of services: ";
    cin >> num_services;

    cout << "Enter the number of threads per service: ";
    cin >> num_threads;

    service_list.resize(num_services);

    thread main_threads[num_services];

    for (int service_index = 0; service_index < num_services; service_index++)
    {
        service_list[service_index].id = service_index;
        service_list[service_index].total_threads = num_threads;
        service_list[service_index].all_requests_completed = false;
        pthread_mutex_init(&service_list[service_index].queue_lock, NULL);
    }

    // Set up services and worker threads
    for (int service_index = 0; service_index < num_services; service_index++)
    {
        for (int thread_index = 0; thread_index < num_threads; thread_index++)
        {
            int priority, resources;
            cout << "Enter priority and resources for Service " << service_index << " Thread " << thread_index << ": ";
            cin >> priority >> resources;

            Worker worker;
            worker.priority_level = priority;
            worker.max_resources = resources;
            worker.current_resources = resources;
            service_list[service_index].workers.push_back(worker);
        }
    }

    int total_requests;
    cout << "Enter the total number of requests: ";
    cin >> total_requests;

    for (int service_index = 0; service_index < num_services; service_index++)
    {
        main_threads[service_index] = thread(manage_service, service_index);
    }

    int request_id = 0;
    vector<Request *> all_requests;

    // Accept requests from user
    cout << "Enter request type and required resources:" << endl;
    for (int i = 0; i < total_requests; i++)
    {
        int request_type, resources;
        cin >> request_type >> resources;

        if (request_type < 0 || request_type >= num_services)
        {
            cout << "Invalid request type: " << request_type << endl;
            continue;
        }

        cout << "Request ID: " << request_id << ", Type: " << request_type << ", Resources: " << resources << " - Request received." << endl;

        Request *new_request = new Request;
        new_request->id = request_id;
        new_request->type = request_type;
        new_request->required_resources = resources;
        new_request->arrival_time = high_resolution_clock::now();
        all_requests.push_back(new_request);
        add_request_to_queue(request_type, new_request);
        request_id++;
    }

    sleep(2);
    for (int service_index = 0; service_index < num_services; service_index++)
    {
        service_list[service_index].all_requests_completed = true;
        for (int thread_index = 0; thread_index < num_threads; thread_index++)
        {
            service_list[service_index].workers[thread_index].completed_requests = true;
        }
    }

    for (int service_index = 0; service_index < num_services; service_index++)
    {
        main_threads[service_index].join();
        pthread_mutex_destroy(&service_list[service_index].queue_lock);
    }

    cout << "Order of requests executed: ";
    for (int request : executed_requests)
    {
        cout << request << " ";
    }
    cout << endl;

    cout << "Number of rejected requests: " << rejected_requests << endl;

    for (Request *req : all_requests)
    {
        cout << "Request ID: " << req->id << ", Waiting Time: " << req->waiting_duration.count() << "ms"
             << ", Turnaround Time: " << req->turnaround_duration.count() << "ms" << endl;
    }

    // Calculate average waiting and turnaround times
    milliseconds total_waiting_time(0);
    milliseconds total_turnaround_time(0);
    int completed_count = 0;

    for (Request *req : all_requests)
    {
        if (req->turnaround_duration.count() != 0)
        {
            total_waiting_time += req->waiting_duration;
            total_turnaround_time += req->turnaround_duration;
            completed_count++;
        }
    }

    if (completed_count > 0)
    {
        total_waiting_time /= completed_count;
        total_turnaround_time /= completed_count;

        cout << "Average Waiting Time: " << total_waiting_time.count() << "ms" << endl;
        cout << "Average Turnaround Time: " << total_turnaround_time.count() << "ms" << endl;
    }

    return 0;
}
