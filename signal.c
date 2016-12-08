#include <signal.h>
#include <stdio.h>
#include <errno.h>
#include <sys/time.h>
#include <string.h>
#include <sched.h>

//reference from 

int time_tick;

int timer_handler(siginfo_t * siginfo, void * data)
{
	printf("Timer alarmed!: %d\n", time_tick++);
}

int main(int argc, char * argv[])
{
	struct sigaction new_sa, old_sa;
	struct itimerval new_itimer;

	// signal handler (SIGALRM) setup
	new_sa.sa_sigaction = &timer_handler;
	sigaction(SIGALRM, &new_sa, &old_sa);

	// timer setup for every 100ms
	memset(&new_itimer, 0, sizeof(new_itimer));
	new_itimer.it_interval.tv_usec = 100000;
	new_itimer.it_value.tv_usec = 100000;
	setitimer(ITIMER_REAL, &new_itimer, NULL);

	// finish cond.
	while (1) {
		if (time_tick == 100) break;
		sched_yield();
	}

	sigaction(SIGALRM, &old_sa, &new_sa);
}

