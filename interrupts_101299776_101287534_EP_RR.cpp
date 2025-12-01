/**
 * @file interrupts.cpp
 * @author Sasisekhar Govind
 * @brief template main.cpp file for Assignment 3 Part 1 of SYSC4001
 * 
 */

#include "interrupts_101299776_101287534.hpp"


// Function to log memory status
std::string log_memory_status(unsigned int current_time, bool cpu_idle, const PCB& running, 
                             const std::vector<PCB>& ready_queue, const std::vector<PCB>& wait_queue) {
    std::string memory_status;
    
    if (!cpu_idle || !ready_queue.empty() || !wait_queue.empty()) {
        memory_status += "Time: " + std::to_string(current_time) + " - ";
        memory_status += "Running: " + (cpu_idle ? "IDLE" : "PID " + std::to_string(running.PID));
        memory_status += ", Ready: " + std::to_string(ready_queue.size());
        memory_status += ", Waiting: " + std::to_string(wait_queue.size()) + "\n";
        
        // Calculate memory usage
        int total_used = 0;
        int total_free = 0;
        int usable_memory = 0;
        
        // First pass: calculate total free and used
        for (int i = 0; i < 6; i++) {
            if (memory_paritions[i].occupied == -1) {
                total_free += memory_paritions[i].size;
            } else {
                total_used += memory_paritions[i].size;
            }
        }

        // Calculate usable memory based on processes waiting for memory
        // Check all processes in NEW state that haven't been assigned memory yet
        int smallest_unassigned_process = -1;
        for (const auto& process : ready_queue) {
            if (process.state == READY && process.partition_number == -1) {
                if (smallest_unassigned_process == -1 || process.size < smallest_unassigned_process) {
                    smallest_unassigned_process = process.size;
                }
            }
        }
        
        // If no processes waiting for memory, usable = total_free
        if (smallest_unassigned_process == -1) {
            usable_memory = total_free;
        } else {
            // Calculate how much memory can actually fit the smallest waiting process, CHECKING FOR EXTERNAL FRAGMENTATION?
            usable_memory = 0;
            for (int i = 0; i < 6; i++) {
                if (memory_paritions[i].occupied == -1 && memory_paritions[i].size >= smallest_unassigned_process) {
                    usable_memory += memory_paritions[i].size;
                }
            }
        }
        
        
        memory_status += "  Memory - Used: " + std::to_string(total_used) + "MB, ";
        memory_status += "Free: " + std::to_string(total_free) + "MB, ";
        memory_status += "Usable: " + std::to_string(usable_memory) + "MB\n";
        
        // Show partition status
        memory_status += "  Partitions:\n";
        for (int i = 0; i < 6; i++) {
            memory_status += "    [" + std::to_string(i) + "] Size: " + std::to_string(memory_paritions[i].size) 
                             + "MB, ";
            if (memory_paritions[i].occupied == -1) {
                memory_status += "Free\n";
            } else {
                memory_status += "Occupied by PID " + std::to_string(memory_paritions[i].occupied) + "\n";
            }
        }
    }
    
    return memory_status;
}

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

// Scheduling function for External Priorities with RR Preemption
void schedule_process(std::vector<PCB> &ready_queue, std::vector<PCB> &wait_queue, PCB &running, bool &cpu_idle, bool &need_reschedule,
                     std::vector<PCB> &job_list, unsigned int current_time,
                     std::string &execution_status, std::string &memory_status){

    if ((!ready_queue.empty() && cpu_idle) || need_reschedule) {
        ExternalPriorities(ready_queue);
        // REVISIT AGAIN TO SEE IF PROPERLY DONEEEE
        running = ready_queue.front();
        ready_queue.erase(ready_queue.begin());
        running.state = RUNNING;
        running.start_time = current_time;  // This now means "last run start time"
        running.time_quantum_used = 0;  // Reset time quantum counter to so we can now  start counting to 100ms

        sync_queue(job_list, running);
        cpu_idle = false;
        execution_status += print_exec_status(current_time, running.PID, READY, RUNNING);
        memory_status += log_memory_status(current_time, cpu_idle, running, ready_queue, wait_queue); // Process state transition indicates memory log

        need_reschedule = false;
        
    }
}


/**
 * Checks if the process should request I/O antime after running a set of
 * time in the CPU. 
 * 
 * @returns true if the amount of time in between its start and its total CPU time
 * (no I/O in between) is equal to its set i/o frequency, request  I/O for that process
 */
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
    bool need_reschedule = false; // Control variable that indicates if we need rescheduling due to preemption 


    //Initialize an empty running process
    idle_CPU(running);

    //Print out the starting prompt
    execution_status = "Time    PID   OldState      NewState      Description\n";
    execution_status += "-----------------------------------------------------\n";

    // INITIAL DEBUG PRINT TO CHECK PROCESSES LOADED
    std::cout << "\n=== DEBUG: PROCESSES LOADED ===\n";
    std::cout << "Total processes loaded: " << list_processes.size() << "\n";
    std::cout << "PID | Size | Arrival | CPU Time | I/O Freq | I/O Dur | Priority | State\n";
    std::cout << "----|------|---------|----------|----------|---------|----------|-------\n";
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
    std::cout << "=== END DEBUG ===\n" << std::endl;


    // Main simulation loop
    while (!all_process_terminated(job_list) || job_list.empty()) {
        /**
         *                 --- PREEMPTION NOTE ---
         * 
         */
        
        
        // Temporary storage for transitions that happened in this time unit
        std::vector<std::tuple<int, states, states>> transitions;

        //  === 1. ADMIT NEW PROCESSES THAT HAVE ARRIVED ===

        //Population of ready queue is given to you as an example.
        //Go through the list of proceeses
        for(auto &process : list_processes) {
            if(process.arrival_time == current_time) { //check if the AT = current time
                //if so, assign memory and put the process into the ready queue
                assign_memory(process);

                process.state = READY;  //Set the process state to READY
                ready_queue.push_back(process); //Add the process to the ready queue
                job_list.push_back(process); //Add it to the list of processes

                execution_status += print_exec_status(current_time, process.PID, NEW, READY);
                memory_status += log_memory_status(current_time, cpu_idle, running, ready_queue, wait_queue); // Process state transition indicates memory log
            
                
                // Only preempt if new process has higher priority AND CPU is busy
                if (!cpu_idle && process.PID < running.PID) {
                    execution_status += print_exec_status(current_time, running.PID, RUNNING, READY);
                    //transitions.push_back({running.PID, RUNNING, READY});
                    ready_queue.push_back(running); // Add running back to ready queue
                    sync_queue(job_list, running);
                    idle_CPU(running);
                    cpu_idle = true;
                    need_reschedule = true;
                }
            }
        }

        // === 2. SCHEDULE A PROCESS FROM THE READY QUEUE (EXTERNAL PRIORITIES WITH RR) ===

        // Only find a new process to run if there are any proceeses in the ready queue and CPU is idle
        if (cpu_idle || need_reschedule) {
            schedule_process(ready_queue, wait_queue, running, cpu_idle, need_reschedule, 
                            job_list, current_time, execution_status, memory_status);
        }
        // Reset the flag AFTER scheduling attempt
        need_reschedule = false;


        
        // == 3. UPDATE WAIT QUEUE ==

        // Update I/O duration timers and move completed processes to ready queue
        // Utilize an interator for the wait queue
        for (auto it = wait_queue.begin(); it != wait_queue.end(); ) {
            if (it->io_remaining_time > 0) {
                // For every 1ms in this loop, decrement the remaining I/O time
                it->io_remaining_time--;     

                // Keep the process in wait queue
                it++;
            } else if (it->io_remaining_time == 0) {
                // Check if it has reached the end of the I/O time
                PCB ready_process = *it;
                ready_process.state = READY;
                ready_queue.push_back(ready_process);
                transitions.push_back({it->PID, WAITING, READY});
                //timing calc
                //Remove the process from the wait queue
                it = wait_queue.erase(it); 
            } else {
                // Almost never happens since it would be out of waiting list but added for failsafe/debugging
                it++;
            }
        }



        // == 4. EXECUTE RUNNING PROCESS ==

        // Dont start any running process simulating if the CPU isn't even working on a process
        if (!cpu_idle) {
            // Decrement remaining time by 1ms for utilizing CPU
            running.remaining_time--;
            
            // Increment time quantum to track its usage
            running.time_quantum_used++;  

            sync_queue(job_list, running);

            // Calculate how long this process has been running in the CPU
            unsigned int time_ran_CPU = (current_time  + 1) - running.start_time;
            
            if (running.remaining_time <= 0) {
                // Check if process completed and needs to terminate
                running.state = TERMINATED;
                free_memory(running);
                sync_queue(job_list, running);
                transitions.push_back({running.PID, RUNNING, TERMINATED});
                sync_queue(job_list, running);

                // Free CPU
                idle_CPU(running);   // Make the running PCB set to an idle CPU state 
                cpu_idle = true;     // Will automatically look for a new process to run in next time unit

       
            } else if (running.time_quantum_used >= TIME_QUANTUM) {
                // Time quantum has expired and has sent the 
                running.state = READY;
                ready_queue.push_back(running);  // Move to back of ready queue
                sync_queue(job_list, running);
                transitions.push_back({running.PID, RUNNING, READY});
                
                // Free CPU
                idle_CPU(running);   // Make the running PCB set to an idle CPU state 
                cpu_idle = true;     // Will automatically look for a new process to run in next time unit

            } else if (should_request_io(running, time_ran_CPU)) {
                // Check if process should request I/O based on set I/O frequency
                running.state = WAITING;
                running.io_remaining_time = running.io_duration;
                wait_queue.push_back(running);
                sync_queue(job_list, running);
                transitions.push_back({running.PID, RUNNING, WAITING});

                // Free CPU
                idle_CPU(running);   // Make the running PCB set to an idle CPU state 
                cpu_idle = true;     // Will automatically look for a new process to run in next time unit
            }      
        }

        // === SAFETY CHECK: AVOID NON-TERMINATING STATE ===
        // If the CPU is idle and both the ready and wait queues are empty, then there is no
        // further work that can be done in this simulation. Any processes that are still not
        // marked TERMINATED in job_list are in an inconsistent state (e.g., logically finished
        // but never had their state updated). To avoid an infinite loop, we conservatively
        // mark all remaining non-terminated processes as TERMINATED and exit the loop.
        if (cpu_idle && ready_queue.empty() && wait_queue.empty()) {
            for (auto &p : job_list) {
                if (p.state != TERMINATED) {
                    p.state = TERMINATED;
                    p.remaining_time = 0;
                }
            }
            break;
        }

        // === 5. INCREMENT CURRENT TIMER ===
        current_time++;  // Every iteration of loop indicates 1ms (i.e. assumed time unit) passing

        // === 6. LOG ALL POSTPONED TRANSITION EXECUTION LOGS ===
        for (const auto& [pid, old_state, new_state] : transitions) {
            // The actions that occurred over the course of 1ms will now be logged into the 
            // status page as the current timer has now accurately incremented to show the 
            // passing of 1ms
            execution_status += print_exec_status(current_time, pid, old_state, new_state);
        }
        
    }

    return {execution_status};
}

int main(int argc, char **argv) {
    // Reject empty argument list
    if (argc <= 1) {
        std::cerr << "Error: No input files provided.\n";
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

    // Reads the input file line by line and constructs a list of processes
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
