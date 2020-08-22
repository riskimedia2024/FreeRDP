#include <pthread.h>
#include <stdio.h>

void *ent(void *arg)
{
    printf("This is new thread\n");
    return NULL;
}

int main(int argc, char const *argv[])
{
    pthread_t p;
    pthread_create(&p, NULL, ent, NULL);
    pthread_join(p, NULL);
    return 0;
}
