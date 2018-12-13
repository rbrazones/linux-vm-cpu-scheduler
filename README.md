# Linux VM CPU Scheduler
Tool for allocating CPU resources for virtual machines running in Linux

## About

This project presents a simple method for allocating CPU resources in a multicore system to virtual machines running on that system. That is, it load balances the system by pinning virtual CPUs (VCPUs) to physical CPUs (PCPUs) based on the CPU utilization of the virtual machines.

## Project Contents

This project contains the following items:
* `README.md` (this file)
* `Makefile`
* `vcpu_scheduler` (pre-compiled binary)
* `vcpu_scheduler.c`
* `vcpu_scheduler.h`
* `math_func.c`
* `math_func.h`
* `math_func.o` (pre-compiled)i

## How to Compile

Verify the following packages are installed:

* `qemu-kvm`
* `libvirt-bin`
* `libvirt-dev`
* `virt-manager`
* `qemu-system`
* `uvtool`
* `uvtool-libvirt`

This project also utilizes the math libraries (math.h, linked via -lm)

To compile:  
    `$ make`

To clean:  
    `$ make clean`

To clean and recompile:  
    `$make clean; make`

## How to Run

Prepare VMs

1. Create the desired amount of VMs - 8 are recommended for a system with 4 physical cores
2. Verify by typing "virsh list" to view all VMs
3. To run the scheduler, run the following command: `$ ./vcpu_scheduler <time interval>`

You must specify 1 command line parameter; an interger to tell the VCPU scheduler how long to wait
between triggering (in seconds).

It is recommended to use the shortest time interval (1 second) so that the VCPU scheduler can
react the fastest and assign PCPU mappings quickly.

Once the VCPU scheduler is running, you should see print-outs to console give domain mappings
and CPU usage information.

(Example of console output is shown below)

(A terminal console of width cols=105 is recommended to view this output)
(Wider console is required for more CPUS -- each addition physical CPU adds a column)

```
-------------------------------------
 
      Domain    CPU Mask         CPU0         CPU1         CPU2         CPU3       Total
   thirdtest         0x2        0.00%       41.67%        0.00%        0.00%       41.67%
 seventhtest         0x8        0.00%        0.00%        0.01%       41.62%       41.64%
   sixthtest         0x1       41.24%        0.00%        0.01%        0.00%       41.24%
  eighthtest         0x4        0.00%        0.00%       41.51%        0.00%       41.51%
   fifthtest         0x2        0.00%       41.59%        0.00%        0.01%       41.60%
  secondtest         0x1       41.29%        0.00%        0.00%        0.00%       41.30%
  fourthtest         0x4        0.00%        0.00%       41.57%        0.00%       41.57%
   firsttest         0x8        0.01%        0.00%        0.00%       41.66%       41.67%
   CPU Total                   82.54%       83.26%       83.11%       83.29%
 
============================
== Scheduler Statistics:  ==
============================
==  Max = 83.29
==  Load Balanced: YES
============================
```

## Test System Configuraiton:

This project was developed and testing on the following system configuration:

* Intel (R) Xeon (R) CPU E3-1268L v3 @ 2.30 GHz, 8M LLC (Haswell 2013)
* 2x8 GB of DDR3 @ 1600 MHz (Hynix) 
* OS: Ubuntu 14.04.1 (64-bit) Linux Kernel 3.19.0-25-generic
* BIOS: Hyperthreading disabled, SpeedStep disabled
* Compiler: GCC 4.8.4

## Design Choices and Notes

The VCPU scheduler will determine whether or not the system is balanced based on the following
two factors:

1.  Each VCPU must be pinned to exactly ONE PCPU. If any VCPU is pinned to more than one PCPU, then this will automatically trigger the balancing algorithm regardless of the CPU loads
2.  No PCPU should be more than twice (2x) as busy as any other PCPU. This will be checked each time the VCPU scheduler triggers.

If any VCPU has more than one PCPU in its mappings, the balancing algorithm immediately triggers.

For any CPU balancing-based triggerings (Factor #2), the VCPU scheduler will wait for 3 
consecutive executions before it triggers the rebalancing algorithm. This is to prevent any
false positive from occurring when workloads are first starting.

There is also a check to see if any PCPU is at least 2.00% utilized. If not, it is assumed that
there is either no workload running, or it is so light, that there is no real need to perform
any load balancing at this point. This also prevents the balancing algorithm from running for 
instances where no load is running. For example, a 0.08% and 0.04% would normally trigger the
balancing algorithm.

CPU load itself is calculated as the amount of time a domain executes on that PCPU in a specified
amount of time (this is given as "cputime"). For example, if 2 second time interval is used, if a
domain executes for 1 second on that CPU, then its usage will be 50%

The balancing itself is performed by sorting the domains in order of total CPU usage( across all
PCPUS), and then pinning them to a single PCPU in a round-robin fashion. This method has proven to
be effective for the 5 test cases presented in the assignment.

