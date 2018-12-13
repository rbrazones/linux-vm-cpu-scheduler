/**********************************************************************************
 *  Ryan Brazones - rbazones@gmail.com
 *
 *  File: math_func.h
 *
 *  Description: Contains function prototypes with descriptions for various math 
 *  functions used by the VCPU scheduler project
 *
 * ********************************************************************************/

#ifndef MATH_FUNC
#define MATH_FUNC

/*
 *  Calculate mean of inputs and return (NOT used)
 */

float calculate_mean(float *inputs, int size);

/*
 * Calculate standard deviation of inputs (NOT used)
 */

float calculate_stddev(float *inputs, float mean, int size);

/*
 *  Check if we are within criteria based on standard deviation (NOT used)
 */

float check_criteria(float *inputs, int size, float mean, float crit);

/*
 *  Return max value from input
 *  Used to determine max CPU usage of physical CPUs
 */

float return_max(float *inputs, int size);

/*
 *  Determine if criteria is met (no CPU should be more than twice as busy as any other)
 */

int check_max_criteria(float *inputs, int size, float max);

/*
 *  Sort domains based on total usage across all CPUs
 */

int sort_domain_usage(float *domain_total_usage, int *domain_number, int size);

/*
 *  Want to make sure each VCPU only has 1 PCPU
 */

int only_one_bit_set(unsigned char *x);

#endif
