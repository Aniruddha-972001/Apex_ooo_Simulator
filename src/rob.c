#include "rob.h"
#include "cpu_settings.h"
#include "macros.h"

bool rob_get_completed(Rob *rob, IQE *iqe) {
	IQE head = rob->queue[0];

	if (!head.completed || rob->len == 0) return false;

	*iqe = head;

	for (int i = 1; i < rob->len; i++) {
		rob->queue[i - 1] = rob->queue[i];
	}

	rob->len -= 1;

	return true;
}

IQE *rob_push_iqe(Rob *rob, IQE iqe) {
	if (rob->len >= ROB_CAPACITY) {
		DBG("WARN", "Trying to push instruction to full ROB. %c", ' ');
		return 0;
	}

	rob->queue[rob->len] = iqe;
	rob->len += 1;

	return &rob->queue[rob->len - 1];
}
