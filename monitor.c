// Simple command-line kernel monitor useful for
// controlling the kernel and exploring the system interactively.

#include <inc/stdio.h>
#include <inc/string.h>
#include <inc/memlayout.h>
#include <inc/assert.h>
#include <inc/x86.h>

#include <kern/console.h>
#include <kern/monitor.h>
#include <kern/kdebug.h>

#define CMDBUF_SIZE	80	// enough for one VGA text line


struct Command {
	const char *name;
	const char *desc;
	// return -1 to force monitor to exit
	int (*func)(int argc, char** argv, struct Trapframe* tf);
};

// LAB 1: add your command to here...

static struct Command commands[] = {
	{ "help", "Display this list of commands", mon_help },
	{ "kerninfo", "Display information about the kernel", mon_kerninfo },
    { "backtrace", "Display backtrace of current kernel stack", mon_backtrace},
    { "mydisplay", "Shows a display created", mydisplay }
};

/***** Implementations of basic kernel monitor commands *****/

int
mon_help(int argc, char **argv, struct Trapframe *tf)
{
	int i;

	for (i = 0; i < ARRAY_SIZE(commands); i++)
		cprintf("%s - %s\n", commands[i].name, commands[i].desc);
	return 0;
}

int
mon_kerninfo(int argc, char **argv, struct Trapframe *tf)
{
	extern char _start[], entry[], etext[], edata[], end[];

	cprintf("Special kernel symbols:\n");
	cprintf("  _start                  %08x (phys)\n", _start);
	cprintf("  entry  %08x (virt)  %08x (phys)\n", entry, entry - KERNBASE);
	cprintf("  etext  %08x (virt)  %08x (phys)\n", etext, etext - KERNBASE);
	cprintf("  edata  %08x (virt)  %08x (phys)\n", edata, edata - KERNBASE);
	cprintf("  end    %08x (virt)  %08x (phys)\n", end, end - KERNBASE);
	cprintf("Kernel executable memory footprint: %dKB\n",
		ROUNDUP(end - entry, 1024) / 1024);
	return 0;
}

int
mon_backtrace(int argc, char **argv, struct Trapframe *tf)
{
	// LAB 1: Your code here.
    // HINT 1: use read_ebp().
    // HINT 2: print the current ebp on the first line (not current_ebp[0])
	uint32_t* ebp, eip, args[10]; // fixed array size from 9 to 10
	struct Eipdebuginfo info;

	cprintf("Stack backtrace:\n");

	ebp = (uint32_t*)read_ebp(); // read current ebp value
	while (ebp != 0) { // while there is a valid ebp value
		eip = ebp[1]; // read the eip value from the stack
		args[0] = ebp[2]; // read 1 argument from the stack
		args[1] = ebp[3]; // read 2 argument from the stack
		args[2] = ebp[4]; // read 3 argument from the stack
		args[3] = ebp[5]; // read 4 argument from the stack
		args[4] = ebp[6]; // read 5 argument from the stack
		args[5] = ebp[7]; // read 6 argument from the stack
		args[6] = ebp[8]; // read 7 argument from the stack
		args[7] = ebp[9]; // read 8 argument from the stack
		args[8] = ebp[10]; // read 9 argument from the stack

		cprintf("  ebp %08x  eip %08x  args", ebp, eip);
		for (int i = 0; i < 10; i++) // loop through the array of arguments
			cprintf(" %08x", args[i]); // print the argument value
		cprintf("\n");

		debuginfo_eip(eip, &info); // get the debug info for the eip value
		cprintf("         %s:%d: %.*s+%d\n",
			info.eip_file, info.eip_line,
			info.eip_fn_namelen, info.eip_fn_name,
			eip - info.eip_fn_addr); // print the debug info for the eip value

		ebp = (uint32_t*)*ebp; // read the next ebp value from the stack
	}
	return 0;
}

int
mydisplay()
{
	cprintf("\e[0;30m                 \n");
	cprintf("\e[0;30m    @@@    _  ,-.\n");
	cprintf("\e[0;31m   @@@@@  (,-/)  )\n");
	cprintf("\e[0;31m    @@@  {        }\n");
	cprintf("\e[0;35m     | o-' 9       ;\n");
	cprintf("\e[0;35m     |  \\         /     BOOM\n");
	cprintf("\e[0;34m     |   `-.     (        TEDDY\n");
	cprintf("\e[0;33m     |  ,'/  ,--.;          BOOM\n");
	cprintf("\e[0;33m ,-. _,','  /   ||\n");
	cprintf("\e[0;32m |  (  / _,'    /|\n");
	cprintf("\e[0;39m >-. `( (    _,' |\n");
	cprintf("\e[0;37m |  \\_.--`~~' `.  )\n");
	cprintf("\e[0;37m |             ;-'\n");
	cprintf("\e[0;36m `.__,.      ,'\n");
	cprintf("\e[0;36m      `----'\n\e[0;37m");

	return 0;
}



/***** Kernel monitor command interpreter *****/

#define WHITESPACE "\t\r\n "
#define MAXARGS 16

static int
runcmd(char *buf, struct Trapframe *tf)
{
	int argc;
	char *argv[MAXARGS];
	int i;

	// Parse the command buffer into whitespace-separated arguments
	argc = 0;
	argv[argc] = 0;
	while (1) {
		// gobble whitespace
		while (*buf && strchr(WHITESPACE, *buf))
			*buf++ = 0;
		if (*buf == 0)
			break;

		// save and scan past next arg
		if (argc == MAXARGS-1) {
			cprintf("Too many arguments (max %d)\n", MAXARGS);
			return 0;
		}
		argv[argc++] = buf;
		while (*buf && !strchr(WHITESPACE, *buf))
			buf++;
	}
	argv[argc] = 0;

	// Lookup and invoke the command
	if (argc == 0)
		return 0;
	for (i = 0; i < ARRAY_SIZE(commands); i++) {
		if (strcmp(argv[0], commands[i].name) == 0)
			return commands[i].func(argc, argv, tf);
	}
	cprintf("Unknown command '%s'\n", argv[0]);
	return 0;
}

void
monitor(struct Trapframe *tf)
{
	char *buf;

	cprintf("Welcome to the JOS kernel monitor!\n");
	cprintf("Type 'help' for a list of commands.\n");


	while (1) {
		buf = readline("K> ");
		if (buf != NULL)
			if (runcmd(buf, tf) < 0)
				break;
	}
}
