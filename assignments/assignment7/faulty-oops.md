To test the faulty driver, I first loaded it inside my QEMU Buildroot image using insmod /lib/modules/$(uname -r)/extra/faulty.ko.
After confirming that /proc/devices listed a new entry named faulty, I created a device node for it:

# cat /proc/devices | grep faulty
247 faulty
# mknod /dev/faulty c 247 0
# chmod 666 /dev/faulty


Then I triggered the fault by writing to the device:
# echo hello_world > /dev/faulty


As I expected, the kernel printed an oops message on the console.

Here is the important part of that log:

Unable to handle kernel NULL pointer dereference at virtual address 0000000000000000
Internal error: Oops: 0000000096000045 [#1] SMP
Modules linked in: faulty(O) hello(O) scull(O)
CPU: 0 PID: 115 Comm: sh
pc : faulty_write+0x10/0x20 [faulty]
Call trace:

faulty_write+0x10/0x20 [faulty]
ksys_write+0x74/0x110
__arm64_sys_write+0x1c/0x30
el0_svc+0x2c/0x90


This oops tells us that the crash happened inside the faulty_write() function in the faulty module.
The instruction pointer (pc) shows faulty_write+0x10, which matches the line in the source code where a null pointer is dereferenced.

In the file misc-modules/faulty.c, the faulty_write() function contains this line:

*(int *)0 = 0;


That line writes to address 0x0, which is not valid in kernel space.
When the CPU tries to store a value there, it triggers a data abort (a kind of memory access fault) and the kernel reports it as an “Oops.”

This behavior is known (intentional). The driver is designed to demonstrate how a kernel bug looks when it occurs, and how you can use the oops information to find the exact faulty instruction.

In this case, the pc value and the function name in the trace directly point to the offending code.