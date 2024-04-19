#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <omp.h>

#define MAX_INT 2147483647

int x;                                          // number of cities
int matrix[12][12];                             // the distance matrix, given x <= 12
int best_path[12];                              // return path
int distance = MAX_INT;                         // return distance

// the recursion thread function for dfs, this is sequential but we'll call it concurrently
// we need all these argument because of the recursive nature that requires to keep passing on variables
void thread_dfs(int root,                       // the dfs root
                int num_visited,                // use num_visited to check if all nodes are traversed
                int current_path[],             // the array of the visited nodes
                bool visited[],                 // use visited[i] to check if node i is valid to traverse next
                int thread_best_path[],         // local best path
                int* thread_distance) {         // local shortest distance (have to use a pointer to ensure correct update and communication with the main thread)
    if (num_visited == x) {                     // stop recursion when all nodes are visited
        int current_distance = 0;
        for (int i = 0; i < x - 1; i++) {       // loop through the current path and sum up the length
            current_distance += matrix[current_path[i]][current_path[i + 1]];
        }
        if (current_distance < *thread_distance) {
            for (int i = 0; i < x; i++) {
                thread_best_path[i] = current_path[i]; // update the local best path
            }
            *thread_distance = current_distance; // update the local shortest distance
        }
        return;                                 // return from the function
    }

    for (int i = 1; i < x; i++) {               // loop through the remaining cities
        if (visited[i] == false) {              // check if the city is not visited
            visited[i] = true;                  // mark the city as visited
            current_path[num_visited] = i;      // add the city to the current path
            thread_dfs(i, num_visited + 1, current_path, visited, thread_best_path, thread_distance); // recursively call thread_dfs
            visited[i] = false;                 // mark the city as unvisited for backtracking
        }
    }
}

int main(int argc, char* argv[]) {
    if (argc != 4) {
        printf("Please use the format: ./ptsm [number of cities] [number of threads] [filename]\n");
        exit(1);
    }

    x = atoi(argv[1]);                          // the number of cities
    int t = atoi(argv[2]);                      // the number of threads
    char* filename = argv[3];

    FILE* file = fopen(filename, "r");
    if (file == NULL) {
        printf("Error opening file.\n");
        exit(1);
    }

    for (int i = 0; i < x; i++) {
        for (int j = 0; j < x; j++) {
            fscanf(file, "%d", &matrix[i][j]);  // read the integers into the matrix using fscanf
        }
    }
    fclose(file);

    int thread_distances[t];                    // array to store the local shortest distances for each thread
    int thread_best_paths[t][12];               // arrays to store the local best paths for each thread, easier than pointers

    #pragma omp parallel num_threads(t)         // start a parallel region with t threads
    {
        int thread_id = omp_get_thread_num();   // get the thread ID
        bool visited[12] = {false};             // initialize visited array to false
        int thread_distance = MAX_INT;          // initialize thread_distance to MAX_INT
        int current_path[12];                   // array to store the current path

    #pragma omp for 
        for (int i = 1; i < x; i++) {           // use the method in the instruction, spliting the possible second nodes to threads
            for (int j = 0; j < x; j++) visited[j] = false;  // Reinitialize visited for each root
            visited[0] = true;                  // mark the starting city as visited
            visited[i] = true;                  // mark the current city as visited
            current_path[0] = 0;                // set the starting city in the current path
            current_path[1] = i;                // set the current city in the current path
            thread_dfs(i, 2, current_path, visited, thread_best_paths[thread_id], &thread_distance); // call thread function
        }
        thread_distances[thread_id] = thread_distance; // store the local shortest distance for the thread
    }

    for (int i = 0; i < t; i++) {               // in main thread, atomically loop through the threads
        if (thread_distances[i] < distance) {
            for (int j = 0; j < x; j++) {
                best_path[j] = thread_best_paths[i][j]; // update the global best path
            }
            distance = thread_distances[i];     // update the global shortest distance
        }
    }

    printf("Best path:");                       // print the messages as required
    for (int i = 0; i < x; i++) {
        printf(" %d", best_path[i]);
    }
    printf("\n");
    printf("Distance: %d\n", distance);

    return 0;
}