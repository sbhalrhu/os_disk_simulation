#include <errno.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <time.h>


/*
Disk model goes from 0-5000
Command line has one mandatory one optional argument.
Usage:
./diskSim INITIAL_POSITION [ACCESS_SEQUENCE_FILE]
Initial position is between -4999 and 4999, with sign indicating direction.
ACCESS_SEQUENCE_FILE is a text file with one integer per line, each being between 0 and 4999.
    If access_sequence_file is not provided, generate 100 random numbers.
Output:
    FCFS 445
    SSTF 389
    SCAN 377
    C-SCAN 397
    LOOK 425
    C-LOOK 399
[Algorithm] [tracks_traversed]


Create a struct for each request that has track number and whether it went through each algo.
After FCFS, sort the array so that the other algoritms are easier.
*/

struct track_request {
    int track_number;
    bool fcfs_done;
    bool sstf_done;
    bool scan_done;
    bool cscan_done;
    bool look_done;
    bool clook_done;
};

void parse_reference_file(char *filename, struct track_request **traversal_requests, int *num_requests);
int fcfs(struct track_request *requests, int num_requests, int initial_position);
int sstf(struct track_request *requests, int num_requests, int initial_position);
int scan(struct track_request *sorted_requests, int num_track_requests, int initial_position);
int cscan(struct track_request *sorted_requests, int num_track_requests, int initial_position);
int look(struct track_request *sorted_requests, int num_track_requests, int initial_position);
int clook(struct track_request *sorted_requests, int num_track_requests, int initial_position);

int main(int argc, char *argv[]) {
    if (argc < 2 || argc > 3) {
        fprintf(stderr, "Usage: ./diskSim INITIAL_POSITION [ACCESS_SEQUENCE_FILE]\n");
        return EXIT_FAILURE;
    }

    char *end = NULL;
    errno = 0;
    long parsed_position = strtol(argv[1], &end, 10);
    if (errno == ERANGE || end == argv[1] || *end != '\0' ||
        parsed_position < -4999 || parsed_position > 4999) {
        fprintf(stderr, "Initial position must be between -4999 and 4999.\n");
        return EXIT_FAILURE;
    }
    int initial_position = (int)parsed_position;

    struct track_request *disk_location_requests = NULL;
    int num_track_requests = 0;
    if (argc == 3) {
        parse_reference_file(argv[2], &disk_location_requests, &num_track_requests);
    } else {
        num_track_requests = 100;
        disk_location_requests = malloc((size_t)num_track_requests * sizeof(*disk_location_requests));
        if (disk_location_requests == NULL) {
            perror("malloc");
            return EXIT_FAILURE;
        }

        srand((unsigned int)time(NULL));
        for (int i = 0; i < num_track_requests; i++) {
            disk_location_requests[i] = (struct track_request){
                rand() % 5000, false, false, false, false, false, false
            };
        }
    }

    //create an array with the same requests, but sorted.
    struct track_request *sorted_requests = malloc((size_t)num_track_requests * sizeof(struct track_request));
    if (sorted_requests == NULL) {
        perror("malloc");
        free(disk_location_requests);
        exit(EXIT_FAILURE);
    }
    for (int i = 0; i < num_track_requests; i++) {
        sorted_requests[i] = disk_location_requests[i];
    }
    for (int i = 0; i < num_track_requests - 1; i++) {
        for (int j = i + 1; j < num_track_requests; j++) {
            if (sorted_requests[j].track_number < sorted_requests[i].track_number) {
                struct track_request temp = sorted_requests[i];
                sorted_requests[i] = sorted_requests[j];
                sorted_requests[j] = temp;
            }
        }
    }




    int fcfs_movement = fcfs(disk_location_requests, num_track_requests, initial_position);
    int sstf_movement = sstf(sorted_requests, num_track_requests, initial_position);
    int scan_movement = scan(sorted_requests, num_track_requests, initial_position);
    int cscan_movement = cscan(sorted_requests, num_track_requests, initial_position);
    int look_movement = look(sorted_requests, num_track_requests, initial_position);
    int clook_movement = clook(sorted_requests, num_track_requests, initial_position);


    printf("FCFS %d\n", fcfs_movement);
    printf("SSTF %d\n", sstf_movement);
    printf("SCAN %d\n", scan_movement);
    printf("C-SCAN %d\n", cscan_movement);
    printf("LOOK %d\n", look_movement);
    printf("C-LOOK %d\n", clook_movement);

    free(sorted_requests);
    free(disk_location_requests);
    return EXIT_SUCCESS;
}


/*
Parse the reference file, populate an array of requests, count the number of tracks traversed.

*/
void parse_reference_file(char *filename, struct track_request **traversal_requests, int *num_requests) {
    FILE *file = fopen(filename, "r");
    if (file == NULL) {
        perror(filename);
        exit(EXIT_FAILURE);
    }

    *traversal_requests = NULL;
    *num_requests = 0;
    int capacity = 0;
    int track;
    int result;

    /*
    read track numbers as integers line by line.
    */
    while ((result = fscanf(file, "%d", &track)) == 1) {
        if (track < 0 || track > 4999) {
            fprintf(stderr, "Track number out of range: %d\n", track);
            free(*traversal_requests);
            fclose(file);
            exit(EXIT_FAILURE);
        }

        /*
        have a buffer for storing these requests.
        resize the buffer every time it's filled.
        8 chosen as initial size.
        */
        if (*num_requests == capacity) {
            int new_capacity = capacity == 0 ? 8 : capacity * 2;
            struct track_request *new_requests = realloc(*traversal_requests, (size_t)new_capacity * sizeof(struct track_request));

            /*if realloc fails, throw error*/
            if (new_requests == NULL) {
                perror("realloc");
                free(*traversal_requests);
                fclose(file);
                exit(EXIT_FAILURE);
            }

            *traversal_requests = new_requests;
            capacity = new_capacity;
        }

        /* store track number in request array */
        (*traversal_requests)[(*num_requests)++] = (struct track_request){track,
        false, false, false, false, false, false};
    }

    if (ferror(file)) {
        fprintf(stderr, "Error reading reference file\n");
        free(*traversal_requests);
        *traversal_requests = NULL;
        *num_requests = 0;
        fclose(file);
        exit(EXIT_FAILURE);
    }

    /*if we stop reading before reaching end of file, throw error.*/
    if (result != EOF) {
        fprintf(stderr, "Invalid data in reference file\n");
        free(*traversal_requests);
        *traversal_requests = NULL;
        *num_requests = 0;
        fclose(file);
        exit(EXIT_FAILURE);
    }

    if (*num_requests == 0) {
        fprintf(stderr, "No track requests found in reference file\n");
        fclose(file);
        exit(EXIT_FAILURE);
    }

    fclose(file);
}

int fcfs(struct track_request *requests, int num_requests, int initial_position) {
    int total_movement = 0;
    int current_position = abs(initial_position);

    /*
    iterate through list of requests.
    take difference between current and next position in array.
    */
    for (int i = 0; i < num_requests; ++i) {
        total_movement += abs(requests[i].track_number - current_position);
        current_position = requests[i].track_number;
        requests[i].fcfs_done = true;
    }

    return total_movement;
}

int sstf(struct track_request *requests, int num_requests, int initial_position) {
    int total_movement = 0;
    int current_position = abs(initial_position);
    
    int remaining = num_requests;
    /*
    iterate through list of all requests.
    look for shortest seek time, travel there.
    mark that request as done.
    repeat until all requests are done.
    */
    while (remaining > 0) {
        int best_index = -1;
        int best_distance = INT_MAX;

        for (int i = 0; i < num_requests; ++i) {
            if (requests[i].sstf_done) {
                continue;
            }
            int distance = abs(requests[i].track_number - current_position);
            if (distance < best_distance) {
                best_distance = distance;
                best_index = i;
            }
        }

        if (best_index < 0) {
            break;
        }

        total_movement += best_distance;
        current_position = requests[best_index].track_number;
        requests[best_index].sstf_done = true;
        --remaining;
    }

    return total_movement;
}

//assume the list of requests is sorted in ascending order. 
int scan(struct track_request *sorted_requests, int num_track_requests, int initial_position) {
    int head = abs(initial_position);
    int tracks_traversed = 0;
    int i;

    if (initial_position >= 0) {
        /* Find the first request at or above the head. */
        i = 0;
        while (i < num_track_requests && sorted_requests[i].track_number < head) {
            i++;
        }
        int initial_index = i;

        /* keep moving up in the farther requests. */
        for (; i < num_track_requests; i++) {
            tracks_traversed += abs(sorted_requests[i].track_number - head);
            head = sorted_requests[i].track_number;
            sorted_requests[i].scan_done = true;
        }

        /* if lower requests exist, go to 4999 and move downward. */
        if (initial_index > 0) {
            tracks_traversed += 4999 - head;
            head = 4999;

            for (i = initial_index - 1; i >= 0; i--) {
                tracks_traversed += abs(sorted_requests[i].track_number - head);
                head = sorted_requests[i].track_number;
                sorted_requests[i].scan_done = true;
            }
        }

    } else {
        /* Find the last request at or below the head. */
        i = num_track_requests - 1;
        while (i >= 0 && sorted_requests[i].track_number > head) {
            i--;
        }
        int initial_index = i;
        /* Move downward through all lower requests. */
        

        
        for (; i >= 0; i--) {
            tracks_traversed += abs(sorted_requests[i].track_number - head);
            head = sorted_requests[i].track_number;
            sorted_requests[i].scan_done = true;
        }

        /* If higher requests exist, go to 0 and move upward. */
        if (initial_index < num_track_requests - 1) {
            tracks_traversed += head;
            head = 0;

            for (i = initial_index + 1; i < num_track_requests; i++) {
                tracks_traversed += abs(sorted_requests[i].track_number - head);
                head = sorted_requests[i].track_number;
                sorted_requests[i].scan_done = true;
            }
        }
        
    }

    return tracks_traversed;
}

/* Assume requests are sorted by track_number in ascending order. */
int cscan(struct track_request *sorted_requests, int num_track_requests, int initial_position) {
    int head = abs(initial_position);
    int tracks_traversed = 0;
    int i;

    if (initial_position >= 0) {
        i = 0;
        while (i < num_track_requests && sorted_requests[i].track_number < head) {
            i++;
        }
        int initial_index = i;

        for (; i < num_track_requests; i++) {
            tracks_traversed += abs(sorted_requests[i].track_number - head);
            head = sorted_requests[i].track_number;
            sorted_requests[i].cscan_done = true;
        }

        if (initial_index > 0) {
            tracks_traversed += 4999 - head;
            head = 4999;
            tracks_traversed += head;
            head = 0;

            for (i = 0; i < initial_index; i++) {
                tracks_traversed += abs(sorted_requests[i].track_number - head);
                head = sorted_requests[i].track_number;
                sorted_requests[i].cscan_done = true;
            }
        }
    } else {
        i = num_track_requests - 1;
        while (i >= 0 && sorted_requests[i].track_number > head) {
            i--;
        }
        int initial_index = i;

        for (; i >= 0; i--) {
            tracks_traversed += abs(sorted_requests[i].track_number - head);
            head = sorted_requests[i].track_number;
            sorted_requests[i].cscan_done = true;
        }

        if (initial_index < num_track_requests - 1) {
            tracks_traversed += head;
            head = 0;
            tracks_traversed += 4999 - head;
            head = 4999;

            for (i = num_track_requests - 1; i > initial_index; i--) {
                tracks_traversed += abs(sorted_requests[i].track_number - head);
                head = sorted_requests[i].track_number;
                sorted_requests[i].cscan_done = true;
            }
        }
    }

    return tracks_traversed;
}

/* Assume requests are sorted by track_number in ascending order. */
int look(struct track_request *sorted_requests, int num_track_requests, int initial_position) {
    int head = abs(initial_position);
    int tracks_traversed = 0;
    int i;

    if (initial_position >= 0) {
        i = 0;
        while (i < num_track_requests && sorted_requests[i].track_number < head) {
            i++;
        }
        int initial_index = i;

        for (; i < num_track_requests; i++) {
            tracks_traversed += abs(sorted_requests[i].track_number - head);
            head = sorted_requests[i].track_number;
            sorted_requests[i].look_done = true;
        }

        for (i = initial_index - 1; i >= 0; i--) {
            tracks_traversed += abs(sorted_requests[i].track_number - head);
            head = sorted_requests[i].track_number;
            sorted_requests[i].look_done = true;
        }
    } else {
        i = num_track_requests - 1;
        while (i >= 0 && sorted_requests[i].track_number > head) {
            i--;
        }
        int initial_index = i;

        for (; i >= 0; i--) {
            tracks_traversed += abs(sorted_requests[i].track_number - head);
            head = sorted_requests[i].track_number;
            sorted_requests[i].look_done = true;
        }

        for (i = initial_index + 1; i < num_track_requests; i++) {
            tracks_traversed += abs(sorted_requests[i].track_number - head);
            head = sorted_requests[i].track_number;
            sorted_requests[i].look_done = true;
        }
    }

    return tracks_traversed;
}

/* Assume requests are sorted by track_number in ascending order. */
int clook(struct track_request *sorted_requests, int num_track_requests, int initial_position) {
    int head = abs(initial_position);
    int tracks_traversed = 0;
    int i;

    if (initial_position >= 0) {
        i = 0;
        while (i < num_track_requests && sorted_requests[i].track_number < head) {
            i++;
        }
        int initial_index = i;

        for (; i < num_track_requests; i++) {
            tracks_traversed += abs(sorted_requests[i].track_number - head);
            head = sorted_requests[i].track_number;
            sorted_requests[i].clook_done = true;
        }

        if (initial_index > 0) {
            tracks_traversed += abs(sorted_requests[0].track_number - head);
            head = sorted_requests[0].track_number;
            sorted_requests[0].clook_done = true;

            for (i = 1; i < initial_index; i++) {
                tracks_traversed += abs(sorted_requests[i].track_number - head);
                head = sorted_requests[i].track_number;
                sorted_requests[i].clook_done = true;
            }
        }
    } else {
        i = num_track_requests - 1;
        while (i >= 0 && sorted_requests[i].track_number > head) {
            i--;
        }
        int initial_index = i;

        for (; i >= 0; i--) {
            tracks_traversed += abs(sorted_requests[i].track_number - head);
            head = sorted_requests[i].track_number;
            sorted_requests[i].clook_done = true;
        }

        if (initial_index < num_track_requests - 1) {
            tracks_traversed += abs(sorted_requests[num_track_requests - 1].track_number - head);
            head = sorted_requests[num_track_requests - 1].track_number;
            sorted_requests[num_track_requests - 1].clook_done = true;

            for (i = num_track_requests - 2; i > initial_index; i--) {
                tracks_traversed += abs(sorted_requests[i].track_number - head);
                head = sorted_requests[i].track_number;
                sorted_requests[i].clook_done = true;
            }
        }
    }

    return tracks_traversed;
}
