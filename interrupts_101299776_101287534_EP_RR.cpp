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
        // Check all processes in NEW/READY state that haven't been assigned memory yet
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
            // Calculate how much memory can actually fit the smallest waiting process
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
        running = ready_queue.front();
        ready_queue.erase(ready_queue.begin());
        running.state = RUNNING;
        running.start_time = current_time;  // "last run start time"
        running.time_quantum_used = 0;      // reset quantum counter

        sync_queue(job_list, running);
        cpu_idle = false;
        execution_status += print_exec_status(current_time, running.PID, READY, RUNNING);
        memory_status += log_memory_status(current_time, cpu_idle, running, ready_queue, wait_queue);

        need_reschedule = false;
    }
}


/**
 * Checks if the process should request I/O anytime after running a set of
 * time in the CPU, based on TOTAL CPU time used so far.
 */
bool should_request_io(const PCB &process) {
    if (process.io_freq <= 0) return false;

    unsigned int cpu_used = process.processing_time - process.remaining_time;
    return (cpu_used > 0 && cpu_used % process.io_freq == 0);
}



std::tuple<std::string> run_simulation(std::vector<PCB> list_processes) {
    std::string execution_status;
    std::string memory_status; // For bonus mark - memory analysis

    std::vector<PCB> ready_queue;   // ready queue
    std::vector<PCB> wait_queue;    // wait (I/O) queue
    std::vector<PCB> job_list;      // all processes

    unsigned int current_time = 0;
    PCB running;
    bool cpu_idle = true;
    bool need_reschedule = false; // indicates if we need rescheduling due to preemption 

    //Initialize an empty running process
    idle_CPU(running);

    //Print out the starting prompt (EPRR has its own format)
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
        // Temporary storage for transitions that happened in THIS time unit.
        std::vector<std::tuple<int, states, states>> transitions;

        //  === 1. ADMIT NEW PROCESSES THAT HAVE ARRIVED ===
        for (auto &process : list_processes) {
            if (process.arrival_time == current_time) {
                assign_memory(process);

                process.state = READY;
                ready_queue.push_back(process);
                job_list.push_back(process);

                execution_status += print_exec_status(current_time, process.PID, NEW, READY);
                memory_status += log_memory_status(current_time, cpu_idle, running, ready_queue, wait_queue);
            
                // Preempt if new process has higher priority (smaller PID) AND CPU is busy
                if (!cpu_idle && process.PID < running.PID) {
                    execution_status += print_exec_status(current_time, running.PID, RUNNING, READY);
                    ready_queue.push_back(running);
                    sync_queue(job_list, running);
                    idle_CPU(running);
                    cpu_idle = true;
                    need_reschedule = true;
                }
            }
        }

        // === 2. SCHEDULE A PROCESS FROM READY QUEUE (EP with RR) ===
        if (cpu_idle || need_reschedule) {
            schedule_process(ready_queue, wait_queue, running, cpu_idle, need_reschedule, 
                            job_list, current_time, execution_status, memory_status);
        }
        need_reschedule = false;

        // == 3. UPDATE WAIT QUEUE ==
        for (auto it = wait_queue.begin(); it != wait_queue.end(); ) {
            if (it->io_remaining_time > 0) {
                it->io_remaining_time--;
                it++;
            } else if (it->io_remaining_time == 0) {
                PCB ready_process = *it;
                ready_process.state = READY;
                ready_queue.push_back(ready_process);
                transitions.push_back({it->PID, WAITING, READY});
                it = wait_queue.erase(it); 
            } else {
                it++;
            }
        }

        // == 4. EXECUTE RUNNING PROCESS ==
        if (!cpu_idle) {
            // 1ms of CPU time
            running.remaining_time--;
            running.time_quantum_used++;

            sync_queue(job_list, running);

            if (running.remaining_time <= 0) {
                // Process completes
                running.state = TERMINATED;
                free_memory(running);
                sync_queue(job_list, running);
                transitions.push_back({running.PID, RUNNING, TERMINATED});

                idle_CPU(running);
                cpu_idle = true;

            } else if (running.time_quantum_used >= TIME_QUANTUM) {
                // Time quantum expires → preempt
                running.state = READY;
                ready_queue.push_back(running);
                sync_queue(job_list, running);
                transitions.push_back({running.PID, RUNNING, READY});

                idle_CPU(running);
                cpu_idle = true;

            } else if (should_request_io(running)) {
                // I/O request
                running.state = WAITING;
                running.io_remaining_time = running.io_duration;
                wait_queue.push_back(running);
                sync_queue(job_list, running);
                transitions.push_back({running.PID, RUNNING, WAITING});

                idle_CPU(running);
                cpu_idle = true;
            }
        }

        // === 5. INCREMENT TIME & LOG TRANSITIONS ===
        current_time++;  // advance simulation by 1ms

        for (const auto& [pid, old_state, new_state] : transitions) {
            execution_status += print_exec_status(current_time, pid, old_state, new_state);
        }

        // Log memory state at this time if anything is active
        memory_status += log_memory_status(current_time, cpu_idle, running, ready_queue, wait_queue);
    }

    // === 6. APPEND MEMORY ANALYSIS (BONUS) SECTION ===
    execution_status += "\n=== MEMORY ANALYSIS (BONUS) ===\n";
    execution_status += memory_status;

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
