/**********************************************************************************
 *  Ryan Brazones - rbazones@gmail.com
 *
 *  File: vcpu_scheduler.c
 *
 *  Description: Main file for VCPU scheduler project. The VCPU scheduler will take
 *  in a command line parameter telling it how ofter to trigger (in seconds). Upon
 *  triggering, the VCPU scheduler will then collect host and domain information to 
 *  determine CPU usage information for each domain. If the scheduler determines that 
 *  the CPU load of domains is not balanced on the system, it will determine a balanced
 *  VCPU to PCPU mapping for each domain.
 *
 * ********************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>
#include <string.h>
#include <getopt.h>
#include <signal.h>

#include "vcpu_scheduler.h"
#include "math_func.h"

// Global 
hypervisor *h1;

// prototypes
int init_hypervisor(hypervisor *h);
int list_domains(hypervisor *h);
int snapshot(virDomainPtr domain, domain_info *dom, hypervisor *h, int k);
int init_domains(virDomainPtr domain, domain_info *dom, hypervisor *h);
int get_statistics(hypervisor *h);
int get_index(domain_info *dom);
float get_elapsed(domain_info *dom);
int get_percent(float elapsed, domain_info *dom, hypervisor *h);
int print_usage(hypervisor *h);
int reschedule(hypervisor *h);

/*
 *  Cleanup signal functions
 */

static void _sig_handler(int signaled){
    if (signaled == SIGINT || signaled == SIGTERM){
        printf("\nIn the Cleanup function!\n");
        
        // Free all memory
        int i = 0;

        for (i = 0; i < h1->active_domains; i++){
            
            virTypedParamsClear(h1->dom[i].p1, h1->dom[i].nparams * h1->dom[i].max_vcpus);
            free(h1->dom[i].info);
            free(h1->dom[i].mapping);
            free(h1->dom[i].desired);
            free(h1->dom[i].percent);
        }

        free(h1->cpu_usage);
        free(h1->info);
        free(h1->domains);
        free(h1->dom);

        // Cleanup hypervisor
        virConnectClose(h1->conn);   
        free(h1);
        
        exit(signaled);
    }
}

/*
 *  Main Function
 */

int main(int argc, char **argv){

    int i = 0;
   
    /*
     *  Parse command line arguments
     */

    int sleep_time = 0;

    if (argc != 2){
        fprintf(stderr, "ERROR: invalid command line paramters\n");
        exit(1);
    
    } else if (argc == 2){
        const char* sleepy= argv[1];
        sleep_time = atoi(sleepy);

        if (sleep_time <= 0){
            fprintf(stderr, "Invalid time interval specfied\n");
            exit(1);
        }
    }

    printf("Time interval specified = %d\n", sleep_time);

    /*
     *  Setup signal handlers for cleanup
     */

    if (signal(SIGINT, _sig_handler) == SIG_ERR){
        fprintf(stderr, "Failed to catch SIGINT\n");
        exit(-1);
    }

    if (signal(SIGTERM, _sig_handler) == SIG_ERR){
        fprintf(stderr, "Failed to catche SIGTERM\n");
        exit(-1);
    }

    /*
     *  Hypervisor initialization
     */
    
    h1 = malloc(sizeof(hypervisor));

    if (init_hypervisor(h1) != 0){
        fprintf(stderr, "ERROR: init_hypervisor\n");
        exit(1);
    }

    /*
     *  List all active domains
     */

    if (list_domains(h1) != 0){
        fprintf(stderr, "ERROR: list_domains\n");
        exit(1);
    }

    /*
     *  Domain initialization and data
     */

    for (i = 0; i < h1->active_domains; i++){

        if (init_domains(h1->domains[i], &h1->dom[i], h1) != 0){
            fprintf(stderr, "ERROR: init_domains() on domain %d\n", i);
            exit(1);
        }
    } 

    /*
     *  Main loop
     */

    for(;;){
    
        /*
         *  Initial timestamp/data capture
         */
        
        for (i = 0; i < h1->active_domains; i++){

            if (snapshot(h1->domains[i], &h1->dom[i], h1, 0) != 0){
                fprintf(stderr, "ERROR: snapshot()\n");
                exit(1);
            } 
        }

        /*
         *  Sleep for the specified amount of time
         */

        sleep(sleep_time);

        /*
         *  Secondary timestamp/data capture
         */
        
        for (i = 0; i < h1->active_domains; i++){

            if (snapshot(h1->domains[i], &h1->dom[i], h1, 1) != 0){
                fprintf(stderr, "ERROR: snapshot()\n");
                exit(1);
            } 
        }

        /*
         *  get and print usage information
         *  then make a scheduling decision
         */
        
        if (get_statistics(h1) != 0){
            fprintf(stderr, "ERROR: get_statistics()\n");
            exit(1);
        }

        if (print_usage(h1) != 0){
            fprintf(stderr, "ERROR: print_usage\n");
            exit(1);
        }
    }

    return 0;
}

/*
 *  Initialize the hypervisor connection and get node info
 */

int init_hypervisor(hypervisor *h){
    
    /*
     * Initialize connection to hypervisor
     */

    h->conn = virConnectOpen("qemu:///system");
    if (h->conn == NULL){
        fprintf(stderr, "ERROR: vicConnectOpen\n");
        return -1;
    }

    /*
     *  Node information
     */

    h->info = malloc(sizeof(virNodeInfo));

    if (0 != virNodeGetInfo(h->conn, h-> info)){
        fprintf(stderr, "ERROR: virNodeGetInfo\n");
        return -1;
    }

    h->cpu_usage = calloc(h->info->cpus, sizeof(float));
    h->balance = 0;
    h->override = 0;

    return 0;
}

/*
 *  List all active domains
 */

int list_domains(hypervisor *h){

    unsigned int flags = VIR_CONNECT_LIST_DOMAINS_ACTIVE;

    h->active_domains = virConnectListAllDomains(h->conn, &(h->domains), flags);

    if (h->active_domains < 0){
        fprintf(stderr, "ERROR: virConnectListAllDomains\n");
        return -1;
    }

    h->dom = malloc(h->active_domains * sizeof(domain_info));

    return 0;
}

/*
 *  Domain initialization
 */

int init_domains(virDomainPtr domain, domain_info *dom, hypervisor *h){

    dom->max_vcpus = 0;
    dom->nparams = 0;
    dom->p1 = NULL;
    dom->p2 = NULL;
    dom->np1 = 0;
    dom->np2 = 0;
    dom->maplen = VIR_CPU_MAPLEN(h->info->cpus);
    dom->ncpumaps = h->info->cpus; 
    dom->num_ret = 0;
    dom->index = -1;
    dom->percent = calloc(h->info->cpus, sizeof(float));

    /*
     *  How many VCPUs can infor be collected for
     */

    dom->max_vcpus = virDomainGetCPUStats(domain, NULL, 0, 0, 0, 0);

    if (dom->max_vcpus < 0){
        fprintf(stderr, "ERROR: virDomainGetCPUStats\n");
        return -1;
    }

    /*
     *  How many stats are collected per VCPU
     */

    dom->nparams = virDomainGetCPUStats(domain, NULL, 0, 0, 1, 0);

    if (dom->nparams < 0){
        fprintf(stderr, "ERROR: virDomainGetCPUStats\n");
        return -1;
    }

    /*
     *  Allocate memory for storing 
     */

    dom->p1 = calloc(dom->nparams * dom->max_vcpus, sizeof(virTypedParameter));
    dom->p2 = calloc(dom->nparams * dom->max_vcpus, sizeof(virTypedParameter));

    dom->info = calloc(dom->ncpumaps, sizeof(virVcpuInfo));
    
    dom->mapping = calloc(dom->ncpumaps, dom->maplen);
    dom->desired = calloc(dom->ncpumaps, dom->maplen);

    return 0;
}

/*
 *  Snapshot time/cputime for CPU utilization
 */

int snapshot(virDomainPtr domain, domain_info *dom, hypervisor *h, int k){

    if(k == 0){
        clock_gettime(CLOCK_MONOTONIC, &(dom->t1));
        dom->np1 = virDomainGetCPUStats(domain, dom->p1, dom->nparams, 0, dom->max_vcpus, 0);
    }else if(k == 1){
        clock_gettime(CLOCK_MONOTONIC, &(dom->t2));
        dom->np2 = virDomainGetCPUStats(domain, dom->p2, dom->nparams, 0, dom->max_vcpus, 0);
        dom->num_ret = virDomainGetVcpus(domain, dom->info, h->info->cpus, dom->mapping, dom->maplen);

        if (dom->num_ret > 1){
            printf("We were only expecting 1 VCPU per domain . . .\n");
        }

        /*
         *  Override if 1 VCPU has more than 1 PCPU check
         */

        if (only_one_bit_set(dom->mapping) != 1){
            h->override = 1;
        }

    }else{
        fprintf(stderr, "ERROR: snapshot()\n");
        return -1;
    }

    return 0;
}

/*
 *  Retrieve CPU usage information
 */

int get_statistics(hypervisor *h){

    int j = 0;

    float elapsed = 0;

    for (j = 0; j < h->info->cpus; j++){
        h->cpu_usage[j] = 0;
    }

    for (j = 0; j < h->active_domains; j++){
    
        if (h->dom[j].index == -1){
            h->dom[j].index = get_index(&h->dom[j]);            
        }

        elapsed = get_elapsed(&h->dom[j]);

        if (get_percent(elapsed, &h->dom[j], h) != 0){
            fprintf(stderr, "Failed to get percent usage\n");
            return -1;
        }
    }

    return 0;
}

/*
 *  Determine which index for finding cputime information per domain
 */

int get_index(domain_info *dom){

    int i = 0;

    for (i = 0; i < dom->nparams; i++){
        if (strcmp(dom->p1[i].field, VIR_DOMAIN_CPU_STATS_CPUTIME) == 0){
            break;
        }
    }

    if (i == dom->nparams){
        fprintf(stderr, "ERROR: CPUTIME statisic not found. Killing program\n");
        exit(-1);
    }

    return i;
}

/*
 *  Calculate elapsed CPU time in nanoseconds
 */

float get_elapsed(domain_info *dom){
    
    float temp;

    temp = (float)(
                (dom->t2.tv_sec - dom->t1.tv_sec) * 1000000000L +
                (dom->t2.tv_nsec - dom->t1.tv_nsec)
                  );

    return temp;
}

/*
 *  Calculate percent usage for each domain on each pCPU
 */

int get_percent(float elapsed, domain_info *dom, hypervisor *h){

    int i;
    int j = 0;
    float cputime;
    dom->total = 0;

    for (i = 0; i < h->info->cpus; i++){

        cputime = 0;
        cputime = (float)(dom->p2[dom->index+j].value.ul - dom->p1[dom->index+j].value.ul);
        dom->percent[i] = (cputime / elapsed) * 100;
        dom->total += dom->percent[i];
        j += dom->nparams;
 
        h->cpu_usage[i] += dom->percent[i];
    }

    return 0;
}

/*
 *  Print out CPU usage information for entire system and all domains
 */

int print_usage(hypervisor *h){

    int i = 0;
    int j = 0;

    /*
     *  Print out Table Headers
     */

    printf("\n-------------------------------------\n");

    printf("\n%12s%12s", "Domain", "  CPU Mask");        
    for(i = 0; i < h->info->cpus; i++){
        printf("%12s%d", "CPU", i); 
    }

    printf("%12s", "Total");

    /*
     *  Populate table with information
     */
    
    for(j = 0; j < h->active_domains; j++){
        
        char mask[10];
        
        memset(mask, 0x0, sizeof(mask));

        sprintf(mask, "0x%x", *h->dom[j].mapping);
        printf("\n");
        printf("%12s", virDomainGetName(h->domains[j]));
        printf("%12s", mask);

        for (i = 0; i < h->info->cpus; i++){
            
            printf("%12.2f%%", h->dom[j].percent[i]);
        }       

        printf("%12.2f%%", h->dom[j].total);
    }
    printf("\n");
    printf("%12s%12s", "CPU Total", "");

    for (j = 0; j < h->info->cpus; j++){
        printf("%12.2f%%", h->cpu_usage[j]);
    }

    printf("\n");

   
    float max = 0;

    max = return_max(h->cpu_usage, h->info->cpus);

    printf("\n============================\n");
    printf("== Scheduler Statistics:  ==\n");
    printf("============================\n");
    
    printf("==  Max = %.2f\n", max);

    if (h->override == 1){

        h->override = 0;

        printf("== Rescheduling!\n== (each VCPU only gets 1 PCPU)\n");

        if (reschedule(h) != 0){
            fprintf(stderr, "ERROR rescheduling\n");
            exit(1);
        }

    }else if (check_max_criteria(h->cpu_usage, h->info->cpus, max) == 1){
        printf("==  Load Balanced: YES\n");
        h->balance = 0;
    }else if(max > 2.0){
        printf("==  Load Balanced: NO \n");
        h->balance++;

        if (h->balance >= 3){
            //do scheduling action
            h->balance = 0;

            printf("==  Rescheduling VCPU mappings . . .\n");

            if (reschedule(h) != 0){
                fprintf(stderr, "ERROR: reschedule\n");
                exit(1);
            }

        }
    }else{
        printf("==  Load Balanced: N/A\n");
        h->balance = 0;
    }
    printf("============================\n");

    return 0;
}

/*
 *  Re-balance the workloads of vCPUs
 */

int reschedule(hypervisor *h){

    float *domain_total_usage;
    int *domain_number;
    int i = 0;

    domain_total_usage = calloc(h->active_domains, sizeof(float));
    domain_number = calloc(h->active_domains, sizeof(int));

    for (i = 0; i < h->active_domains; i++){
        
        domain_number[i] = i;
        domain_total_usage[i] = h->dom[i].total;
    }

    /*
     *  Sort domain total usage from highest to lowest
     */
    
    if(sort_domain_usage(domain_total_usage, domain_number, h->active_domains) != 0){
        fprintf(stderr, "ERROR: sort_domain_usage\n");
        exit(1);
    }

    for (i = 0; i < h->active_domains; i++){
        
        *h->dom[domain_number[i]].desired = 1 << (i % h->info->cpus);

        if(virDomainPinVcpuFlags(
                                    h->domains[domain_number[i]],
                                    0, // 1 vcpu per domain assumption
                                    h->dom[domain_number[i]].desired,
                                    h->dom[domain_number[i]].maplen,
                                    0
                    ) != 0){
            fprintf(stderr, "ERROR: remapping failed\n");
            exit(1);
        }
    } 

    free(domain_total_usage);
    free(domain_number);

    return 0;
}

