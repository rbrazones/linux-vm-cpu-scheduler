/**********************************************************************************
 *  Ryan Brazones - rbazones@gmail.com
 *
 *  File: vcpu_shceduler.h
 *
 *  Description: Header file for VCPU scheduler. Contains the struct definitions
 *  for storing domain and hypervisor info in the VCPU scheduler project.
 *
 * ********************************************************************************/

#include <libvirt/libvirt.h>
#include <time.h>

/*
 *  Struct for storing information about each domain
 */

typedef struct {

    struct timespec t1, t2;        // uesd for measuring CPU time
    virTypedParameterPtr p1, p2;   // used for mesauring CPU time
    int max_vcpus;                  
    int nparams;
    int np1, np2;
    int ncpumaps;
    int maplen;
    virVcpuInfoPtr info;
    int num_ret;
    unsigned char *mapping;
    unsigned char *desired;
    float *percent;
    float total;
    int index;

}domain_info;

/*
 *  Struct for storing information about hypervisor
 */

typedef struct {
    
    virConnectPtr conn;     // connection to hypervisor
    virNodeInfoPtr info;    // storing information about host node
    virDomainPtr *domains;  // keeping track of all the domains
    int active_domains;     // how many active domains do we have
    domain_info *dom;
    float *cpu_usage;       // usage per CPU cor
    int balance;
    int override;           // we will balance no matter what if any VCPU has more than 1 PCPU

}hypervisor;
