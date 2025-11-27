/**
 * @file interrupts.cpp
 * @author Sasisekhar Govind
 * @brief template main.cpp file for Assignment 3 Part 1 of SYSC4001
 * 
 */

#include "interrupts_student1_student2.hpp"
#include <queue>
#include <algorithm>


// External Priorities scheduler - processes are prioritized by PID (lower PID = higher priority)
void ExternalPriorities(std::vector<PCB> &ready_queue) {
    std::sort( 
        ready_queue.begin(),
        ready_queue.end(),
        [](const PCB &first, const PCB &second) {
            return first.PID < second.PID; // Lower PID = higher priority
        } 
    );
}

// Check if a process should request I/O based on its frequency
bool should_request_io(const PCB &process, unsigned int time_ran) {
    // If the duration ran from the current time and when the total CPU time its ran
    // (no I/O in between) is equal to its set i/o frequency, request  I/O for that process
    return (process.io_freq > 0 && time_ran > 0 && time_ran % process.io_freq == 0);
}

std::tuple<std::string> run_simulation(std::vector<PCB> list_processes) {
    std::string execution_status;
    std::string memory_status; // For bonus mark - memory analysis

    std::vector<PCB> ready_queue;   //The ready queue of processes
    std::vector<PCB> wait_queue;    //The wait queue of processes
    std::vector<PCB> job_list;      //A list to keep track of all the processes.

    unsigned int current_time = 0;
    PCB running;
    bool cpu_idle = true;

    //Initialize an empty running process
    idle_CPU(running);

    // Create output table header
    execution_status = print_exec_header();

    // DEBUG: Check if list of processes were properly loaded
    std::cout << "\n=== DEBUG: PROCESSES LOADED ===" << std::endl;
    std::cout << "Total processes loaded: " << list_processes.size() << std::endl;
    
    if (list_processes.empty()) {
        std::cout << "WARNING: No processes were loaded!" << std::endl;
    } else {
        std::cout << "PID | Size | Arrival | CPU Time | I/O Freq | I/O Dur | Priority | State" << std::endl;
        std::cout << "----|------|---------|----------|----------|---------|----------|-------" << std::endl;
        
        for (const auto& process : list_processes) {
            std::cout << std::setw(3) << process.PID << " | "
                      << std::setw(4) << process.size << " | "
                      << std::setw(7) << process.arrival_time << " | "
                      << std::setw(8) << process.processing_time << " | "
                      << std::setw(8) << process.io_freq << " | "
                      << std::setw(7) << process.io_duration << " | "
                      << std::setw(8) << process.priority << " | "
                      << process.state << std::endl;
        }
    }
    std::cout << "=== END DEBUG ===\n" << std::endl;



    // Main simulation loop
    while (!all_process_terminated(job_list) || !ready_queue.empty() || !wait_queue.empty()) {
        std::cout << "=== HELLO ===\n" << std::endl;
        //  == 1. Admit new processes that have arrived ==

        //Population of ready queue is given to you as an example.
        //Go through the list of proceeses
        for(auto &process : list_processes) {
            if(process.arrival_time == current_time) {//check if the AT = current time
                //if so, assign memory and put the process into the ready queue
                assign_memory(process);

                process.state = READY;  //Set the process state to READY
                ready_queue.push_back(process); //Add the process to the ready queue
                job_list.push_back(process); //Add it to the list of processes

                execution_status += print_exec_status(current_time, process.PID, NEW, READY);
            }
        }

        
        //  == 2. Update wait queue (handle I/O completion) to see if we need to push it back to ready ==
        
        // Update I/O duration timers and move completed processes to ready queue
        // Utilize an interator for the wait queue
        for (auto it = wait_queue.begin(); it != wait_queue.end(); ) {
            if (it->io_remaining_time > 0) {
                // Decrement each process' duration timer by 1 time unit
                it->io_remaining_time--;

                // If I/O has completed, move to the ready queue
                if (it->io_remaining_time == 0) {
                    PCB ready_process = *it;
                    ready_process.state = READY;
                    ready_queue.push_back(ready_process);
                    execution_status += print_exec_status(current_time, it->PID, WAITING, READY);
                    
                    // Remove from wait queue
                    it = wait_queue.erase(it);
                } else {
                    it++;
                }
            } else {
                // Almost never happens since it would be out of waiting list but added for failsafe/debugging
                it++;
            }
        }
        
        
        // === 3. Schedule a process from ready queue to run (External Priorities - No Preemption) ===

        // Only find a new process to run if there are any processes in the read queue and CPU is idle
        if (!ready_queue.empty() && cpu_idle) {
            // Sort by external priorities (higher priority number = higher priority)
            ExternalPriorities(ready_queue);
            
            // Get the highest priority process
            running = ready_queue.front();
            ready_queue.erase(ready_queue.begin());
            
            running.state = RUNNING;
            sync_queue(job_list, running);
            cpu_idle = false;  // CPU is not idle
            
            execution_status += print_exec_status(current_time, running.PID, READY, RUNNING);
            
            // If this is first time running, set start time
            if (running.start_time == -1) {
                running.start_time = current_time;
            }
            
            std::cout << "Scheduled process " << running.PID << " (lowest PID = highest priority)" << std::endl;
        }



        // === 4. Execute the running process for this time unit ===

        // Dont start any running process simulating if the CPU isn't even working on a process
        if (!cpu_idle) {
            running.remaining_time--;
            
            // Check if process completed
            if (running.remaining_time <= 0) {
                running.state = TERMINATED;
                free_memory(running);
                sync_queue(job_list, running);
                execution_status += print_exec_status(current_time, running.PID, RUNNING, TERMINATED);
                
                // Free CPU
                idle_CPU(running);   // Make the running PCB set to an idle CPU state 
                cpu_idle = true;
            }
            
            // Check if process should request I/O
            unsigned int time_run = current_time - running.start_time + 1; // +1 because we're about to complete this time unit
            if (should_request_io(running, time_run)) {
                running.state = WAITING;
                running.io_remaining_time = running.io_duration; // Set the I/O duration timer
                wait_queue.push_back(running);
                sync_queue(job_list, running);
                execution_status += print_exec_status(current_time, running.PID, RUNNING, WAITING);
                
                // Free CPU
                idle_CPU(running);
                cpu_idle = true;
            }
            
            // Process continues running
            sync_queue(job_list, running);
        }
        
        
        //execute_running_process(execution_status);
        
        // 5. Log memory status (for bonus mark)    COME BACK TO
        if (!cpu_idle || !ready_queue.empty() || !wait_queue.empty()) {
            memory_status += "Time: " + std::to_string(current_time) + " - ";
            memory_status += "Running: " + (cpu_idle ? "IDLE" : "PID " + std::to_string(running.PID));
            memory_status += ", Ready: " + std::to_string(ready_queue.size());
            memory_status += ", Waiting: " + std::to_string(wait_queue.size()) + "\n";
            
            // Calculate memory usage
            int total_used = 0;
            int total_free = 0;
            int usable_memory = 0;
            
            for (int i = 0; i < 6; i++) {
                if (memory_paritions[i].occupied == -1) {
                    total_free += memory_paritions[i].size;
                    usable_memory += memory_paritions[i].size;
                } else {
                    total_used += memory_paritions[i].size;
                }
            }
            
            memory_status += "  Memory - Used: " + std::to_string(total_used) + "MB, ";
            memory_status += "Free: " + std::to_string(total_free) + "MB, ";
            memory_status += "Usable: " + std::to_string(usable_memory) + "MB\n";
            
            // Show partition status
            memory_status += "  Partitions: ";
            for (int i = 0; i < 6; i++) {
                memory_status += "P" + std::to_string(memory_paritions[i].partition_number) + ":";
                memory_status += (memory_paritions[i].occupied == -1 ? "free" : "PID" + std::to_string(memory_paritions[i].occupied));
                memory_status += (i < 5 ? ", " : "");
            }
            memory_status += "\n\n";
        }
        
        current_time++;  // Every iteration of loop indicates 1 time unit passing
    }

    // Close the output table
    execution_status += print_exec_footer();
    
    // Add memory analysis to execution file for bonus mark
    execution_status += "\n\n\n=== MEMORY ANALYSIS (BONUS) ===\n";
    execution_status += memory_status;

    return std::make_tuple(execution_status);
}

int main(int argc, char** argv) {
    // Get the input file from the user
    if (argc != 2) {
        std::cout << "ERROR!\nExpected 1 argument, received " << argc - 1 << std::endl;
        std::cout << "To run the program, do: ./interrupts <your_input_file.txt>" << std::endl;
        return -1;
    }

    // Open the input file
    auto file_name = argv[1];
    std::ifstream input_file;
    input_file.open(file_name);

    // Ensure that the file actually opens
    if (!input_file.is_open()) {
        std::cerr << "Error: Unable to open file: " << file_name << std::endl;
        return -1;
    }

    // Parse the entire input file and populate a vector of PCBs
    std::string line;
    std::vector<PCB> list_process;
    while (std::getline(input_file, line)) {
        auto input_tokens = split_delim(line, ", ");
        auto new_process = add_process(input_tokens);
        list_process.push_back(new_process);
    }
    input_file.close();

    // With the list of processes, run the simulation
    auto [exec] = run_simulation(list_process);

    write_output(exec, "execution.txt");

    return 0;
}
