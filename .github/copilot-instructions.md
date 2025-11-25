# QuarkOS (Limine C Template) AI Instructions

## Project Overview
This is an x86-64 OS kernel project ("QuarkOS") based on the Limine bootloader protocol. It is a freestanding C environment with custom implementations for standard library functions.

## Architecture & Structure
- **Kernel Space** (`kernel/`): Core OS logic.
  - `src/main.c`: Kernel entry point (`kmain`).
  - `src/arch/x86_64/`: Architecture-specific code (IDT, GDT, ISR, syscalls).
  - `src/hardware/`: Drivers (APIC, PIT, ACPI, Memory).
  - `src/system/`: System services (Scheduler, Terminal, Exec).
  - `include/`: Kernel headers.
- **User Space** (`user/`): User-mode applications compiled as ELF binaries.
- **Modules** (`modules/`): Kernel modules.
- **Bootloader** (`limine/`): Limine bootloader binaries and configuration (`limine.conf`).

## Critical Developer Workflows

### Building & Running
- **Full Build & Run:** `make run` (Builds kernel, user, modules, ISO, and launches QEMU).
- **Build Only:** `make all` (Generates `template.iso`).
- **Kernel Only:** `make kernel` (Useful for quick kernel iteration).
- **Clean:** `make clean` or `make distclean` (removes deps).
- **Dependencies:** If headers are missing, run `./kernel/get-deps`.

### Debugging
- **QEMU Monitor:** `make run` launches QEMU. Use QEMU monitor for state inspection if configured.
- **Logging:** Use `printf()` which is hooked to the `flanterm` terminal emulator for screen output.
- **Panic:** `hcf()` (Halt Catch Fire) is used for unrecoverable errors.

## Coding Conventions & Patterns

### Freestanding C Environment
- **No Standard Libc:** Do NOT include `<stdio.h>` or `<stdlib.h>` from the host system.
- **Internal Lib:** Use headers in `kernel/include/lib/` (e.g., `stdio.h`, `stdlib.h`, `string.h`) which map to implementations in `kernel/src/lib/`.
- **Types:** Use `<stdint.h>`, `<stddef.h>`, `<stdbool.h>` from `freestnd-c-hdrs`.

### Limine Protocol Integration
- **Requests:** Use `volatile` structs in specific sections to request features from the bootloader.
  ```c
  __attribute__((used, section(".limine_requests")))
  static volatile struct limine_module_request module_request = { ... };
  ```
- **Responses:** Check `request.response` for NULL before accessing.

### Memory Management
- **PMM:** Physical Memory Manager (`hardware/memory/pmm.h`).
- **VMM:** Virtual Memory Manager (`hardware/memory/paging.h`).
- **Heap:** Kernel heap (`hardware/memory/heap.h`) - `kmalloc`, `kfree`.

### Interrupts & Syscalls
- **IDT:** Defined in `arch/x86_64/idt.c`.
- **Syscalls:** Handled in `arch/x86_64/syscalls.c`.
- **Context Switching:** `arch/x86_64/context_switch.asm`.

## Common Tasks
- **Adding a Driver:** Place header in `kernel/include/drivers/` and source in `kernel/src/drivers/`. Initialize in `kmain`.
- **New Syscall:** Add handler in `syscalls.c`, update `syscalls.h`, and register in `syscall_init`.
- **User Program:** Add source to `user/src/`, build with `make user`, and it will be bundled as a module.

## Important Files
- `kernel/src/main.c`: Kernel initialization sequence.
- `kernel/GNUmakefile`: Kernel build configuration (flags, includes).
- `limine.conf`: Boot menu configuration.
- `kernel/include/kernel.h`: Global kernel definitions.
