# Lab 4: Networking

In this lab, you'll implement a device driver for a network interface card (NIC)
and a simple Ethernet protocol to allow multiple instances of xv6 to
communicate.

You've learned that operating systems manage hardware devices and provide
abstractions for the user to build upon. We'll explore how an operating system
discovers, initializes, and communicates with devices on a computer. One useful
protocol is Peripheral Component Interconnect, or PCI, which we'll use to
initialize and use a NIC: the Intel e1000.

In this lab, you can earn a bonus of up to 1% towards your overall course grade.
To qualify, you must pass Tests 1-9; failing any of Tests 1-9 yields a 0% bonus.
If qualified, your exact bonus is calculated strictly as (autograder points
earned / maximum possible autograder points) * 1%. Manual grading scores are
entirely excluded from this calculation.

**Important**: if you submit without completing all the parts, make sure to
implement all syscall stubs mentioned in this lab to prevent `make failure`. We
cannot give submissions back due to failed compilations.

## Resources
In this lab, you will likely be referring to these resources:
* The xv6 manual
* The [Intel Manual](https://d2pgu9s4sfmw1s.cloudfront.net/UAM/Prod/Done/a062E00001d9gh8QAA/ff7854ec-ee97-4f95-8879-eae3000993f3?Expires=1648211526&Key-Pair-Id=APKAJKRNIMMSNYXST6UA&Signature=Q5vLcCzTLkgRyPJNK96-1Uc1yQMN7W~PWfVvSTHWVkcioECb9ekgoUgT0nd-sosC3LwSdH75fTG9K7zd5dtDdmPEG5uma20mCHXSaEAtpXfQHMhZls1GKv7ZbSfU9OQLpvWz9hqsAjSI56mrIuJWRjuuUjKlWBHtR8f3weFJEyZRm~xGExnE-HPZCDOl9H3~zuMDZO0U4wDGLMyHUVKzevSoPduV3Z-JUCR5Yuo37KLsBUlwYcu7cIQT9xfGQH9UEoLs~948noenzb5Tz4TyM04YncLqj-WrQXQBeze46JPFZZLXRYu6IrhSvLV~7NCxhmPRF1zDKzlTSSoY30~eEw__)
* The [e1000 Manual](https://pdos.csail.mit.edu/6.828/2018/readings/hardware/8254x_GBe_SDM.pdf)
* The [OSDev wiki](https://wiki.osdev.org/PCI) on PCI

## Part 1: Inline x86 Assembly

Recall CS 2110, CS 2200, or any equivalent class you have taken. To communicate
with devices, we could use I/O ports with `IN` and `OUT` instructions or use
reserved memory addresses.

### Requirements
Write wrapper functions for the `inl` and `outl` instructions in
`include/asm/x86.h` to facilitate port-mapped I/O. These functions should write
to I/O ports with 32-bit widths.

Hint: xv6 communicates with a UART device using byte-width `inb` and `outb`
instructions to write characters to a screen.

## Part 2: PCI Device Discovery

Now that you can read and write to port-mapped devices, we can _discover_
devices connected to xv6.

In a Linux shell, try executing the `lspci` command. The output will describe
all the devices connected via a PCI port to your processor. In this part, you'll
implement a simpler version of `lspci` for xv6.

### Requirements
Implement the following system call:

```c
int lspci(void);

Return Value:
0 in all cases

Description:
Prints the vendor ID and device ID, both in hexadecimal, of all valid PCI
devices in the format:
Vendor <vendor_id1> Device <device_id1>
Vendor <vendor_id2> Device <device_id2>
...
```

**For grading purposes,** the output of the devices must be ordered by bus
number, then by device number, then by function number -- all of which are in
ascending order. Do **not** include the `0x` prefix before any hexadecimal
numbers.

For this part, you will determine the device and vendor ID for each valid
device attached. Read the [OSDev wiki on PCI](https://wiki.osdev.org/PCI) up to 
and including the _"Brute Force" scan_ section for documentation on how to do
this.

Use the `inl` and `outl` instructions from Part 1 to interact with the PCI host
controller using two port-mapped registers, `CONFIG_ADDRESS = 0xCF8` and
`CONFIG_DATA = 0xCFC`. You can use these device registers by setting up the
`CONFIG_ADDRESS` register appropriately and then reading from the `CONFIG_DATA`
register.

If you were writing firmware and wanted to discover all devices, you would need
to configure all 'PCI bridges', which are a special type of PCI device, to
forward subordinate devices to the host PCI controller. For our purposes, you
may assume that the QEMU firmware already configures the PCI bridges to do this.

### Testing

QEMU has a monitor you can invoke by running `C-a c` inside QEMU. Run `info pci`
in this monitor. This should give you a detailed listing of the PCI devices
attached to the QEMU instance.

If you examine `scripts/xv6-qemu`, you can see how we specify devices to attach
to QEMU. Modify the script to specify additional PCI devices to test your
implementation.

## Part 3: MAC Address Querying

Now that you can list every connected device, the next step is to find the
device we are interested in: an Intel e1000 NIC. This will help us determine its
MAC address, which is required to send data to another computer over a network.

PCI devices lay out their information and control registers in regions of I/O
ports and memory addresses. The beginning and extent of these regions can be
derived from the Base Address Registers of a given device. With the correct
base address and offset, you can communicate in any documented way with the
device and do things like query its MAC address.

### Requirements

Implement the following system call:

```c
int whatsmymac(unsigned char *mac);

Return Value:
-1 on invalid args, 0 on success

Description:
Returns the MAC address of the NIC

Usage:
unsigned char mac[6];
if (whatsmymac(mac) < 0) {
  // failure
}
```

In Part 2, you iterated through each PCI device to inspect some of its header
fields, like Device ID and Vendor ID. You will now find the PCI device
corresponding to our NIC and use its headers to determine its Base Address
Register. Read the following sections from the OSDev wiki:
- [Command Register](https://wiki.osdev.org/PCI#Command_Register)
- [PCI Header Type 0x0](https://wiki.osdev.org/PCI#Header_Type_0x0)
- [Base Address Registers](https://wiki.osdev.org/PCI#Base_Address_Registers)

Then, to find the correct offset from the Base Address Register, read sections
13.5.2 and 13.5.3 of the [Intel e1000 manual](https://pdos.csail.mit.edu/6.828/2018/readings/hardware/8254x_GBe_SDM.pdf).

For this part and going forward, you may find a subset of the constants
and macros defined in [QEMU's own list](https://github.com/qemu/qemu/blob/master/hw/net/e1000_regs.h)
helpful. You should create your own header file with a list of those you find
relevant to your implementation.

**Look it up:** what is the vendor and device ID of the Intel e1000?

**Hint**: set the `Bus Master` bit of the NIC's PCI command register to enable
the NIC to perform DMA accesses using PCI buses!

### Testing

`./scripts/xv6-qemu` has a `--address` option which lets you specify the MAC
address of the NIC:
```bash
/xv6/build $ ./xv6-qemu --address=05:AB:1E:F0:00:0D
```

Use this option to test your implementation.

## Part 4: NIC Driver & Ethernet Protocol

Now that you know how to use the e1000, you'll implement a device driver for it.

NICs serve the function of copying large amounts of data from your computer to
the network and vice versa. Because of this, it's inefficient for a processor
to copy data from or to a NIC.

So, many I/O devices use a technique called Direct Memory Access (DMA), which
allows the device to directly access the computer's physical memory. Software
tells the device the physical address ranges it can access, and the device will
automatically write to or read from those ranges.

On initialization, you'll have to allocate and assign ranges of kernel memory
for DMA usage. Ranges where the device can read outgoing transmissions are
called the *transmit ring*. Ranges where the device can write incoming
transmissions are called the *receive ring*.

### Requirements

Implement the following functions prototyped in `kernel/include/e1000.h` in
`kernel/src/e1000.c`:
- `e1000_init`: Initialization of the device
- `e1000_transmit`: Perform packet transmission
- `e1000_receive`: Perform packet reception

First, implement `e1000_init` (transmission initialization only) by reading
Section 14.5 of the [Intel e1000 manual](https://pdos.csail.mit.edu/6.828/2018/readings/hardware/8254x_GBe_SDM.pdf).
Then, implement `e1000_transmit`, which should place the data in an empty buffer
on the transmit ring and mark the buffer for transmission. You will find
sections 3.3.1 - 3.3.3 and 3.4 of the [Intel e1000 manual](https://pdos.csail.mit.edu/6.828/2018/readings/hardware/8254x_GBe_SDM.pdf) helpful. It should **not** overwrite any buffers in the
transmit ring if it is full. Pay close attention to address alignment
requirements.

Note that `e1000_transmit` is non-blocking (it doesn't wait for the packet to
be sent). We'll now expose a blocking version of `e1000_transmit` to userspace
through a syscall `eth_send`. This will enable the user to send Ethernet frames
over the network. **Implement this syscall in `kernel/src/eth.c`**.
```c
int eth_send(char *data, uint size);

Return Value:
-1 on invalid args, 0 otherwise

Description:
Sends `size` bytes of `data`, encapsulated by an Ethernet frame, using the NIC.
Blocks until the frame is sent.
```

Consult `kernel/include/eth.h` for the `struct ethframe` definition. The fields
of the `ethframe` are follows:
- `dest_mac`: Source MAC address
- `src_mac`: Destination MAC address
- `length`: Length of the payload. Note that in the literature, this is called
the `ethr_type` field. `ethr_type` is an overloaded field and can mean different
things based on its value. For our purposes, we are using it as a length
specifier
- `payload`: Buffer of size 256 bytes. The maximum size of an Ethernet frame is
actually 1512 bytes. To avoid colliding with other `ethr_type` values we
restrict the size of the payload to 256 bytes

As the client, we don't (yet) know the MAC address of the machine we want to
communicate with. In such cases, setting the `dest_mac` to `0xFFFFFFFFFFFF` will
broadcast the packet over the network.

Next, you'll implement reception. Implement `e1000_init` (receive
initialization) by following Section 14.4 of the [Intel e1000 manual](https://pdos.csail.mit.edu/6.828/2018/readings/hardware/8254x_GBe_SDM.pdf). Then, implement `e1000_receive`, which should move
the received data out of the buffer and free the buffer to be reused by the NIC.
You will find sections 3.2.2, 3.2.3, and 3.2.6 of the [Intel e1000 manual](https://pdos.csail.mit.edu/6.828/2018/readings/hardware/8254x_GBe_SDM.pdf) helpful. It should **not** free any buffers
in the receive ring if it is full. Pay close attention to address alignment
requirements.

Analogous to `e1000_send`, `e1000_receive` is also non-blocking. However, we
will expose a blocking version to userspace via the `eth_receive` syscall.
**Implement this syscall in `kernel/src/eth.c`**.
```c
int eth_receive(char *data, uint size);

Return Value:
-1 on invalid args, 0 otherwise

Description:
Receives `size` bytes of data, encapsulated by an Ethernet frame, using the NIC
into `data`. Blocks until a frame is received.
```

**Important**: make sure that your implementation is in `kernel/src/e1000.c` and
`kernel/src/eth.c`. Furthermore, upon sending and receiving data through the NIC, you must
report the Ethernet frame sent or received to the autograder. You can achieve
this by passing the frame and its size into `lab4_net_report_send` and
`lab4_net_report_recv`. The interface is defined in
`kernel/include/lab4_net_ag.h`.

### Testing

If you examine `./scripts/xv6-qemu`, you'll find that it gives you the option to
run one of two xv6 instances: a client and a server. These are interchangeable,
but for semantic purposes, you'll generally try to send data from the client to
the server.

When you run `./xv6-qemu`, it runs the client instance by default. You can
choose to run the server instance by using the `--server` option.
```bash
/xv6/build $ ./xv6-qemu --server
```

When QEMU simulates a network device, it provides the option to record data sent
over the device in a packet capture (.pcap) file. When running the client
instance, data is recorded in `client.pcap`. When running the server, data is
recorded in `server.pcap`.

These files can be read with utilities like `tcpdump` or Wireshark. Ensure you
have access to at least one of these. This will help you debug your transmission
and reception. For example, you can use `tcpdump` as follows:
```
/xv6/build $ tcpdump -t -XXnr client.pcap
```

If you inspect the `.pcap` files, you should see that it describes your packet
as an Ethernet frame.

Write user-space client and server programs that use the system calls you
implemented above. Run them on separate client and server xv6 instances. They
should be able to communicate if implemented correctly!

Note that you are not guaranteed to receive packets in-order. Ethernet makes no
guarantee of providing an ordered byte-stream. That is the job of higher-level
protocols like TCP and QUIC.

## Part 5: Packet Filters

In lecture, you learned about Extended Berkeley Packet Filters (eBPFs), which
facilitate extensibility in operating system kernels. In this final part, you
will provide a simple mechanism to customize the xv6 kernel by installing packet
filters into the network stack.

This lab's expectations are significantly simpler than those of eBPFs. You'll be
implementing an in-kernel virtual machine (VM) and an interpreter for an
instruction set, but you won't implement verification, JIT-compilation, nor
maps. The only supported hook is within the NIC driver. Do not go overboard with
edge cases; malicious, unsafe, or incorrect BPF programs are undefined behavior
for this lab since you will not be implementing a verifier. You will only filter
packet reception, not transmission.

xv6 BPF VM Description:
- Harvard architecture
- 8 byte-width registers
- 512 bytes of memory
- Packets are loaded into memory address 0x0
- Instruction store capacity of 128 instructions
- One BPF program loaded at a time
- One instruction pointer (`ip`), which is an index into the instruction store
pointing to the next instruction to execute (e.g. `ip=0` refers to the first
instruction, `ip=1` refers to the second, etc.)
- BPF programs must end in a `ret_pass` or `ret_drop` instruction

xv6 BPF VM ISA:
```
ld <src_addr>, <dst_reg>
Pseudocode:
dst_reg = mem[src_addr];

ldi <imm>, <dst_reg>
Pseudocode:
dst_reg = imm;

je <reg0>, <reg1>, <offset>
Pseudocode:
if (reg0 == reg1) ip += offset;

jgt <reg0>, <reg1>, <offset>
Pseudocode:
if (reg0 > reg1) ip += offset;

jge <reg0>, <reg1>, <offset>
Pseudocode:
if (reg0 >= reg1) ip += offset;

ret_pass
Pass the packet, exits the filter program.

ret_drop
Drop the packet, exits the filter program.
```

### Requirements

Implement the following functions prototyped in `kernel/include/bpf.h`:
```c
void bpf_init(void);
Description:
Initializes the BPF VM.

int bpf_load(struct bpf_insn *program, uint num_insns);
Description:
Loads a BPF program into the VM.

Return Value:
-1 on invalid args, 0 otherwise

int bpf_run(char *pkt, uint pktsize);
Description:
Runs the BPF program.

Return Value:
0 to pass the packet, -1 to drop the packet
```

Place these functions appropriately in the kernel. Then, implement the following
system call:
```c
int bpf_load(struct bpf_insn *program, uint num_insns);
Description:
Loads a xv6 BPF program into the kernel.

Return Value:
-1 on invalid args, 0 otherwise
```

### Testing
- Try to write some simple packet filters, like filtering by address, payload 
lengths, etc.

## General Advice and Hints
- Be careful with how you implement circular buffers, especially head and tail
pointers
- Use `sleeplocks` instead of `spinlocks` to protect I/O operations
- Use the `volatile` C qualifier when accessing memory-mapped I/O addresses
- Consider when to use host byte order vs. network byte order
- Consider how memory-mapped I/O / DMA addresses must be aligned

## Submission

Run `scripts/submit.sh` to generate and submit `submission.zip` to Gradescope.

## Grading 

### Autograder

This lab is partially autograded. The test cases are categorized as:
* Part 2: PCI Device Discovery
    * Tests 1-2
* Part 3: MAC Address Querying
    * Tests 3-4
* Part 4: NIC Driver & Ethernet Protocol
    * Tests 5-9
* Part 5: Packet Filters
    * Tests 10-14

### Manual Grading

This lab is partially manually graded. We will check for the following:
- Mutual exclusion in newly created kernel data structures
- Violations of the student honor code
- Attempts to subvert the autograder

This list is not exhaustive.