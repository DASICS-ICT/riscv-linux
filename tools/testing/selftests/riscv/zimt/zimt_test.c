// SPDX-License-Identifier: GPL-2.0-only

#include <errno.h>
#include <linux/prctl.h>
#include <signal.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/prctl.h>
#include <sys/ptrace.h>
#include <sys/syscall.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#include "kselftest.h"

#ifndef __NR_riscv_hwprobe
#define __NR_riscv_hwprobe 258
#endif

#ifndef PROT_ZIMT
#define PROT_ZIMT 0x20
#endif

#ifndef PTRACE_GETTAG
#define PTRACE_GETTAG 34
#endif

#ifndef PTRACE_SETTAG
#define PTRACE_SETTAG 35
#endif

struct riscv_hwprobe {
	int64_t key;
	uint64_t value;
};

#define RISCV_HWPROBE_KEY_BASE_BEHAVIOR 3

static long riscv_hwprobe_call(struct riscv_hwprobe *pairs, size_t pair_count,
			       size_t cpusetsize, unsigned long *cpus,
			       unsigned int flags)
{
	return syscall(__NR_riscv_hwprobe, pairs, pair_count, cpusetsize,
		       cpus, flags);
}

static void test_hwprobe_syscall(void)
{
	struct riscv_hwprobe pair = { .key = RISCV_HWPROBE_KEY_BASE_BEHAVIOR };
	long rc;

	rc = riscv_hwprobe_call(&pair, 1, 0, NULL, 0);
	ksft_test_result(rc == 0, "hwprobe syscall basic query\n");
}

static void test_prctl_tagged_addr_ctrl(void)
{
	long ctrl = prctl(PR_GET_TAGGED_ADDR_CTRL, 0, 0, 0, 0);

	if (ctrl < 0) {
		ksft_test_result_skip("PR_GET_TAGGED_ADDR_CTRL unsupported\n");
		return;
	}

	ksft_test_result(prctl(PR_SET_TAGGED_ADDR_CTRL, ctrl, 0, 0, 0) == 0,
			 "PR_SET_TAGGED_ADDR_CTRL round-trip\n");
}

static void test_mmap_prot_zimt(void)
{
	void *p;

	p = mmap(NULL, 4096, PROT_READ | PROT_WRITE | PROT_ZIMT,
		 MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
	if (p == MAP_FAILED) {
		ksft_test_result(errno == EINVAL || errno == EOPNOTSUPP,
				 "mmap(PROT_ZIMT) rejected when unavailable\n");
		return;
	}

	ksft_test_result(munmap(p, 4096) == 0,
			 "mmap(PROT_ZIMT) accepted and unmapped\n");
}

static bool wait_stopped(pid_t pid)
{
	int status;

	if (waitpid(pid, &status, 0) < 0)
		return false;

	return WIFSTOPPED(status);
}

static void test_ptrace_gettag(void)
{
	pid_t pid;
	int rc;
	uint8_t tag = 0;

	pid = fork();
	if (pid < 0)
	{
		ksft_test_result_fail("fork for PTRACE_GETTAG\n");
		return;
	}

	if (pid == 0) {
		for (;;)
			pause();
	}

	if (ptrace(PTRACE_ATTACH, pid, NULL, NULL) < 0) {
		kill(pid, SIGKILL);
		waitpid(pid, NULL, 0);
		ksft_test_result_fail("ptrace attach for PTRACE_GETTAG\n");
		return;
	}

	if (!wait_stopped(pid)) {
		kill(pid, SIGKILL);
		waitpid(pid, NULL, 0);
		ksft_test_result_fail("wait child stop for PTRACE_GETTAG\n");
		return;
	}

	rc = ptrace(PTRACE_GETTAG, pid, 0, &tag);
	ksft_test_result(rc == -1 &&
			 (errno == EOPNOTSUPP || errno == ENODEV || errno == EIO),
			 "PTRACE_GETTAG syscall path\n");

	ptrace(PTRACE_DETACH, pid, NULL, NULL);
	kill(pid, SIGKILL);
	waitpid(pid, NULL, 0);
}

static void test_ptrace_settag(void)
{
	pid_t pid;
	int rc;
	uint8_t tag = 1;

	pid = fork();
	if (pid < 0)
	{
		ksft_test_result_fail("fork for PTRACE_SETTAG\n");
		return;
	}

	if (pid == 0) {
		for (;;)
			pause();
	}

	if (ptrace(PTRACE_ATTACH, pid, NULL, NULL) < 0) {
		kill(pid, SIGKILL);
		waitpid(pid, NULL, 0);
		ksft_test_result_fail("ptrace attach for PTRACE_SETTAG\n");
		return;
	}

	if (!wait_stopped(pid)) {
		kill(pid, SIGKILL);
		waitpid(pid, NULL, 0);
		ksft_test_result_fail("wait child stop for PTRACE_SETTAG\n");
		return;
	}

	rc = ptrace(PTRACE_SETTAG, pid, 0, &tag);
	ksft_test_result(rc == -1 &&
			 (errno == EOPNOTSUPP || errno == ENODEV || errno == EIO),
			 "PTRACE_SETTAG syscall path\n");

	ptrace(PTRACE_DETACH, pid, NULL, NULL);
	kill(pid, SIGKILL);
	waitpid(pid, NULL, 0);
}

int main(void)
{
	ksft_print_header();
	ksft_set_plan(5);

	test_hwprobe_syscall();
	test_prctl_tagged_addr_ctrl();
	test_mmap_prot_zimt();
	test_ptrace_gettag();
	test_ptrace_settag();

	ksft_finished();
}
