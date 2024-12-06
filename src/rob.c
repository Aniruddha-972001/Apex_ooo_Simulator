#include "rob.h"

bool rob_get_completed(Rob *rob, IQE *iqe) {
	IQE head = rob->queue[0];

	if (!head.completed) return false;

	*iqe = head;

	for (int i = 1; i < rob->len; i++) {
		rob->queue[i - 1] = rob->queue[i];
	}

	rob->len -= 1;

	return true;
}
