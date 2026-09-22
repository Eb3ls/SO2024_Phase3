# MicroPandOS — Phase 3: Virtual Memory & User Processes

Kernel implementation for the uMPS3 MIPS simulator, developed as part of the Operating Systems course at the University of Bologna (2024).

This is Phase 3, which adds demand paging and isolated user-space processes on top of the base kernel from Phases 1 and 2.

## What this implements

- **Demand paging** — TLB miss handling and page fault resolution
- **Swap pool** — flash device as backing store for page swap in/out
- **Round-robin page replacement** — occupied writable pages are written back before reuse
- **Virtual address spaces** — 8 isolated user processes with distinct ASIDs
- **System Service Thread (SST)** — per-process kernel service wrapper (I/O, TOD, termination)
- **Swap mutex** — dedicated process for mutual exclusion on the swap pool
- **Support-level exception handler** — user-space SYSCALL, program trap, TLB refill

## Architecture

| Phase | Description |
|-------|-------------|
| Phase 1 | PCB and message queue (kernel data structures) |
| Phase 2 | Base kernel: scheduler, SSI, interrupt handler, syscall |
| **Phase 3** | Virtual memory and user processes (this repo) |

## Requirements

- [uMPS3](https://github.com/virtualsquare/umps3) **3.0.5**, including its headers, startup code, ROMs, `umps3-elf2umps`, and `umps3-mkdev`
- `mipsel-linux-gnu` GCC and binutils, plus GNU Make

On Ubuntu 24.04, the dependencies are available as packages:

```bash
sudo apt update
sudo apt install build-essential gcc-mipsel-linux-gnu umps3
```

For other distributions, follow the upstream uMPS3 installation instructions and install a MIPS little-endian cross toolchain.

## Build & Run

```bash
make
```

This builds `kernel.core.umps`, its symbol table, and all eight flash images in `testers/`. To build only the user programs, run `make -C testers`.

The Makefiles detect uMPS3 in `/usr` or fall back to `/usr/local`. If it is installed elsewhere, add its `bin` directory to `PATH` and pass its prefix:

```bash
make UMPS3_DIR_PREFIX=/path/to/umps3
```

The cross compiler prefix can also be overridden with `XT_PRG_PREFIX=/path/to/mipsel-linux-gnu-`. Run `make clean` before changing toolchains or installation prefixes.

From the repository root, open the supplied machine configuration:

```bash
umps3 uMPS3Machine
```

The configuration uses one CPU, 128 RAM frames, a 16-entry TLB, and the following flash images. It already maps the kernel, symbol table, terminals, and printers; there is no need to create a new machine manually.

| Flash device | User program | Expected output |
|-------------|--------------|-----------------|
| 0 | `todTest.umps` | `TOD Test Concluded Successfully` on terminal 0 |
| 1–4 | `terminalTest1.umps`–`terminalTest4.umps` | A successful print and numbered test message on the corresponding terminal |
| 5 | `fibEight.umps` | `Recursion Concluded Successfully` on terminal 5 |
| 6 | `fibEleven.umps` | `Recursion Concluded Successfully` on terminal 6 |
| 7 | `printerTest.umps` | `printTest is ok` on terminal 7 and printer 7 |

The machine file's bootstrap and execution ROM paths point to `/usr/share/umps3`. For a different installation prefix, update **both** paths in `uMPS3Machine` (or in the simulator's machine settings). Keep the relative kernel and device paths anchored to the repository directory.

Start execution in the simulator. All eight user programs should finish, then the kernel should reach `HALT`. Inspect every terminal and printer 7: reaching `HALT` alone does not establish that every test passed. Flash images are writable backing stores; use `make clean && make` to restore fresh images before repeating a run.

## Validation

```bash
make test
```

The host regression tests require **x86-64 Linux**, a native C compiler, and the uMPS3 3.0.5 headers. Older uMPS3 headers use a nonstandard `NULL` sentinel that conflicts with the native C library in these tests. They use Linux memory mappings to exercise the kernel's 32-bit pointer interface. The tests check process completion, bounded printer/terminal writes, and blocked-process accounting using the repository's C functions with hardware calls stubbed. `HOST_CC` selects the native compiler; `UMPS3_DIR_PREFIX` selects the uMPS3 installation as above.

GitHub Actions builds the MIPS kernel and all user programs and runs these regression tests. Host tests and a successful cross-compilation do not replace executing the full kernel in uMPS3; the simulator checks above cover that separate step.

## Project documentation

- [Original Phase 3 report](README_PHASE_3.md), in Italian
- [University of Bologna's 2023/24 Phase 3 specification](https://www.cs.unibo.it/~renzo/so/so2324/MicroPandOS/MicroPandOSPhase3Spec.pdf)

This is a university teaching kernel for the uMPS3 architecture. Phases 1 and 2 are included because Phase 3 depends on their process, messaging, scheduling, and interrupt services.

## Authors

Davide Sarti, Francesco Ciofini, Leonardo Berselli, Francesco Tomba — Bachelor's in Computer Science, University of Bologna (2024)
