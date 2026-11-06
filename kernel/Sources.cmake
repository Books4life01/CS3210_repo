# Source files built along with the kernel

set(kernel_SOURCES
  # ASM Files
  src/asm/entry.S
  src/asm/swtch.S
  src/asm/trapasm.S

  # Generated
  gen/vectors.S

  # C Files
  src/bio.c
	src/console.c
	src/exec.c
	src/file.c
	src/fs.c
	src/ide.c
	src/ioapic.c
	src/kalloc.c
	src/kbd.c
	src/lapic.c
	src/log.c
	src/main.c
	src/mp.c
	src/picirq.c
	src/pipe.c
	src/proc.c
	src/sleeplock.c
	src/spinlock.c
	src/string.c
	src/syscall.c
	src/sysfile.c
	src/sysproc.c
	src/syspci.c
	src/trap.c
	src/uart.c
	src/vm.c
  )

set(kernel_LAB4_NET_AG
  # Switch this between lab4_net_ag.c and lab4_net_ag_noprint.c to enable/disable
  #   report printing

  src/lab4_net_ag.c
#   src/lab4_net_ag_noprint.c
  )