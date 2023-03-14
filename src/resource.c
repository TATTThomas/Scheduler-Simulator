#include "../include/resource.h"
#include "../include/task.h"

void get_resources(int count, int *resources)
{
    //printf("a\n");
    get_re(count, resources);
}

void release_resources(int count, int *resources)
{
    //printf("b\n");
    release_re(count, resources);
}
