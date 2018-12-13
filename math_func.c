/**********************************************************************************
 *  Ryan Brazones - rbazones@gmail.com
 *
 *  File: math_func.c
 *
 *  Description: Contains implementations of functions in math_func.h. These funcitons
 *  are used for various things such as determining CPU usage based on criteria, 
 *  sorting input lists, and determining if any VCPU has more than one mapped PCPU.
 *
 * ********************************************************************************/

#include <math.h>
#include <stdlib.h>
#include <stdio.h>

float calculate_mean(float *inputs, int size){

    float sum = 0;
    float num = (float)size;

    int i = 0;

    for (i = 0; i < size; i++){

        sum += inputs[i];
    }

    return (sum / num);
}

float calculate_stddev(float *inputs, float mean, int size){

    float v_sum = 0;
    float num = (float)size;
    float variance = 0;    

    int i = 0;

    for (i = 0; i < size; i++){

        v_sum = v_sum + pow((inputs[i] - mean),2);
    }

    variance = v_sum / num;

    return (sqrt(variance));
}

float check_criteria(float *inputs, int size, float mean, float crit){

    int i = 0;

    for (i = 0; i < size; i++){
        
        if (abs(inputs[i] - mean) > crit)
            return 0;
    } 

    return 1;
}

float return_max(float *inputs, int size){
    
    int i;
    float max = 0;

    for (i = 0; i < size; i++){
        
        if (inputs[i] > max)
            max = inputs[i];
    }

    return max;
}

int check_max_criteria(float *inputs, int size, float max){

    int i = 0;

    for (i = 0; i < size; i++){
        
        if ((max / 2.0) > inputs[i])
           return 0; 
    }
    
    return 1;
}

int sort_domain_usage(float *domain_total_usage, int *domain_number, int size){

    int i = 0;
    int j = 0;

    float temp_usage = 0;
    int temp_domain = 0;

    for (i = 0; i < size; ++i){

        for (j = i + 1; j < size; ++j){

            if (domain_total_usage[i] < domain_total_usage[j]){

                temp_usage = domain_total_usage[i];
                temp_domain = domain_number[i];
                domain_total_usage[i] = domain_total_usage[j];
                domain_number[i] = domain_number[j];
                domain_total_usage[j] = temp_usage;
                domain_number[j] = temp_domain;
            }
        }
    }

    return 0;
}

int only_one_bit_set(unsigned char *x){
    return *x && !(*x & (*x-1));
}

