#define _GNU_SOURCE
#include <stdio.h>
#include <pthread.h>
#include <dlfcn.h>
#include <stdlib.h>

#include <signal.h>
#include <ucontext.h>

void sigsegv(int sig, siginfo_t *siginfo, void *context)
{
	ucontext_t *ucontext = context;
	printf("SIGSEGV: %p\n", ucontext->uc_mcontext.gregs[REG_CR2]);
	exit(1);
}

void set_sigsegv()
{
	struct sigaction act = {
		.sa_sigaction = sigsegv,
		.sa_flags = SA_SIGINFO,
	};

	if (sigaction(SIGSEGV, &act, NULL) < 0)
	{
		perror("sigaction");
		exit(1);
	}
}

static void *libHandle = NULL;
static int (*ptr)(pthread_t*, const pthread_attr_t*, void*(*)(void*), void*);
struct fake_entry_arg
{
    void *real_arg;
    void *(*real_entry)(void *);
};

void *fake_entry(void *__arg)
{
    struct fake_entry_arg *arg = (struct fake_entry_arg *) __arg;
    set_sigsegv();
    void *ret = arg->real_entry(arg->real_arg);
    free(arg);

    return ret;
}

int pthread_create(pthread_t *thread, const pthread_attr_t *attr,
                    void *(*startRoutine)(void *), void *arg)
{
    if (!libHandle)
    {
        libHandle = dlopen("/usr/lib/libpthread.so.0", RTLD_LAZY | RTLD_LOCAL);
        ptr = dlsym(libHandle, "pthread_create");
    }

    struct fake_entry_arg *fake_arg = (struct fake_entry_arg*)malloc(sizeof(struct fake_entry_arg));
    fake_arg->real_entry = startRoutine;
    fake_arg->real_arg = arg;

    return ptr(thread, attr, fake_entry, fake_arg);
}
