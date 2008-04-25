#include <stdio.h>
#include <sbthread.h>
#include <err.h>

int
worker_main(sbthread_t *th)
{
	sbmsgq_t *msgq;
	sbmsg_t *msg;

	msgq = sbthread_msgq(th);

	while ((msg = sbmsgq_wait(msgq, -1, -1)) != NULL) {
		size_t len;
		void *data;

		data = sbmsg_unpack(msg, &len);
		E_INFO("Got message: %ld\n", sbmsg_data_to_int(data));
		if (data == (void *)512)
			break;
	}

	return 0;
}

int
main(int argc, char *argv[])
{
	sbthread_t *worker;
	int i;
	
	worker = sbthread_start(NULL, worker_main, NULL);
	for (i = 0; i <= 512; ++i) {
		E_INFO("Sending message: %d\n", i);
		sbthread_send(worker, 0, sbmsg_int_to_data(i));
	}
	sbthread_free(worker);

	return 0;
}
