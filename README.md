# Multi-Threaded Request Processing System

## Overview

This project implements a multi-threaded request processing system using C++. It simulates a service-oriented architecture where different service types are managed by worker threads. These threads process incoming requests based on priority and resource availability, dynamically allocating requests to appropriate threads while handling cases such as blocking, waiting, or rejecting requests due to resource constraints.

## Features

- **Multi-threaded Processing**: Utilizes worker threads to handle multiple requests simultaneously, improving system efficiency and reducing processing time.
- **Priority-Based Scheduling**: Requests are scheduled and processed according to the priority levels assigned to each worker thread, ensuring that higher-priority requests are addressed first.
- **Resource Management**: Each worker thread monitors and manages its own resources, processing requests only if the required resources are available, thus preventing overutilization.
- **Request Handling**: The system can process requests immediately, block them, place them in a waiting queue, or reject them based on the current availability of resources and thread status.
- **Performance Metrics**: The system tracks key metrics such as waiting time, turnaround time, and the number of requests rejected, providing insights into the efficiency and performance of the request processing mechanism.

## Architecture

- **Services**: The system supports multiple service types, each represented by a collection of worker threads.
- **Worker Threads**: Each thread is configured with a priority level and a maximum resource capacity. Threads within a service are sorted and managed based on their priority to handle incoming requests effectively.
- **Request Queue**: A queue is maintained for each service, storing incoming requests that are yet to be processed.
- **Execution Queue**: Each worker thread has an execution queue that manages the actual processing of requests.

## Design Approach

- **Thread Synchronization**: The program uses mutex locks to synchronize access to shared resources, ensuring thread safety when handling requests and updating resource availability.
- **Dynamic Request Allocation**: Requests are dynamically assigned to worker threads based on the availability of resources and the priority of the threads, optimizing the utilization of system resources.
- **Resource Reclamation**: Once a request is processed, the allocated resources are released and made available for future requests, maintaining a balance in resource distribution.

## Key Components

1. **Request Struct**: Represents a request with attributes such as ID, type, required resources, arrival time, start time, and completion time.
2. **Worker Struct**: Represents a worker thread with attributes like priority level, resource limits, and a mutex for thread synchronization.
3. **Service Struct**: Represents a service type containing multiple worker threads and a queue for managing incoming requests.
4. **Request Handling Functions**: Includes functions for checking queue status, fetching requests, executing requests, and managing worker threads.

## License

This project is licensed under the MIT License. You are free to use, modify, and distribute this software. See the `LICENSE` file for more details.

## Contact

For any questions or issues, please feel free to contact the author at [your-email@example.com].
