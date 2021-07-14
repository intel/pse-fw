/*
 * Copyright (c) 2021 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 */

/**
 * @file
 * @brief Sample application for sub task creation in user Mode.
 * This example demonstrates how to create two sub tasks in user
 * Mode which also need access to kernel objects.
 * @{
 */

/**
 * @brief Hardware setup.
 * There is no specific external hardware setup required for the
 * User Mode sample application.
 */

/* Local Includes */
#include <kernel_structs.h>
#include <string.h>
#include <user_app_framework/user_app_framework.h>
#include <zephyr.h>

/** Declares kernel thread structures for the two sub tasks */
__kernel static struct k_thread app_sub_task1_handle;
__kernel static struct k_thread app_sub_task2_handle;

/** Declares thread stacks for the two sub tasks */
static K_THREAD_STACK_DEFINE(app_sub_task1_stack, 4096);
static K_THREAD_STACK_DEFINE(app_sub_task2_stack, 4096);

/** Semaphore with maximum count of 12 */
static K_SEM_DEFINE(sem, 0, 12);

/** Declare a global varible which only this App(7) can access. */
APP_GLOBAL_VAR(7) static int cnt;

/** Add the requried kernel object pointer which needs to be accessed in user
 * mode into the list array.
 */
static const void *obj_list[] = { &sem, &app_sub_task1_handle,
				  &app_sub_task2_handle, &app_sub_task1_stack,
				  &app_sub_task2_stack };

/* @brief app_sub_task functions
 * Sub Tasks which can access resource allocated to this App(7)
 * app_sub_task1 and app_sub_task2 demonstrates use of semaphore
 * to synchronize the tasks.
 */
static void app_sub_task1(void *q1, void *q2, void *q3)
{
	int i;

	/* Use ARG_UNUSED([arg]) to avoid compiler warnings about
	 * unused arguments.
	 */
	ARG_UNUSED(q1);
	ARG_UNUSED(q2);
	ARG_UNUSED(q3);

	printf("Main APP 7 Task 1 started\n");

	for (i = 0; i < 6; i++) {
		k_sem_give(&sem);
		printf("Sem given %d\n", ++cnt);
	}

	printf("Main APP 7 Task 1 done\n");
}

static void app_sub_task2(void *q1, void *q2, void *q3)
{
	int i;

	/* Use ARG_UNUSED([arg]) to avoid compiler warnings about
	 * unused arguments.
	 */
	ARG_UNUSED(q1);
	ARG_UNUSED(q2);
	ARG_UNUSED(q3);

	printf("Main APP 7 Task 2 started\n");

	for (i = 0; i < 6; i++) {
		k_sem_take(&sem, K_FOREVER);
		printf("Sem taken %d\n", cnt--);
	}

	printf("Main APP 7 Task 2 done\n");
}

/* @brief app_main function
 * App Main Entry. This function depicts how to create
 * sub tasks for a sample app in userspace and access
 * kernel objects based on the permissions inherited.
 */
static void app_main(void *q1, void *q2, void *q3)
{
	k_tid_t thread_a, thread_b;

	/* Use ARG_UNUSED([arg]) to avoid compiler warnings about
	 * unused arguments.
	 */
	ARG_UNUSED(q1);
	ARG_UNUSED(q2);
	ARG_UNUSED(q3);

	printf("App 7 Main\n");

	/** Create two sub task which inherit permission given to
	 * this App(7).
	 */
	thread_a = k_thread_create(&app_sub_task1_handle, app_sub_task1_stack,
				   1024, app_sub_task1, NULL, NULL, NULL, 10,
				   APP_USER_MODE_TASK | K_INHERIT_PERMS, K_FOREVER);

	thread_b = k_thread_create(&app_sub_task2_handle, app_sub_task2_stack,
				   1024, app_sub_task2, NULL, NULL, NULL, 12,
				   APP_USER_MODE_TASK | K_INHERIT_PERMS, K_FOREVER);

	k_thread_start(&app_sub_task1_handle);
	k_thread_start(&app_sub_task2_handle);

	printf("App 7 Main return\n");
}

/** Define a more complex App which need to access some kernel objects and have
 * multiple sub task.
 */
DEFINE_USER_MODE_APP(7, USER_MODE_APP | K_PART_GLOBAL, app_main, 1024,
		     (void **)obj_list, 5, NULL);

/**
 * @}
 */
