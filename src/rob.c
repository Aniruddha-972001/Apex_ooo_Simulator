#include "rob.h"
#include "cpu_settings.h"
#include "macros.h"
#include "rs.h"
#include <stdlib.h>

bool rob_get_completed(Rob *rob, IQE *iqe) {
	if (rob->head == NULL) return false;

	if (rob->head->iqe.completed) {
		*iqe = rob->head->iqe;

		RobNode *temp = rob->head;
		rob->head = temp->next;
		free(temp);

		rob->len -= 1;

		return true;
	}

	return false;
}

IQE *rob_push_iqe(Rob *rob, IQE iqe) {
	if (rob->len >= ROB_CAPACITY) {
		DBG("WARN", "Trying to push instruction to full ROB. %c", ' ');
		return 0;
	}

	RobNode *node = (RobNode *)malloc(sizeof(RobNode));
	if (node == NULL) {
		DBG("WARN", "Failed to malloc \n %c", ' ');
		exit(1);
	}
	node->iqe = iqe;
	node->next = NULL;

	rob->len += 1;

	if (rob->head == NULL) {
		rob->head = node;

		printf("INFO: ROB Added -> ");
		print_iqe(&rob->head->iqe);

		return &rob->head->iqe;
	} else {
		RobNode *head = rob->head;
		while (head->next != NULL) head = head->next;
		head->next = node;

		return &head->next->iqe;
	}

	// return &node->iqe;
}
