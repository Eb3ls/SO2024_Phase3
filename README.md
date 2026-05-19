# MicroPandOS — Phase 3: Virtual Memory & User Processes

Kernel implementation for the uMPS3 MIPS simulator, developed as part of the Operating Systems course at the University of Bologna (2024).

This is Phase 3, which adds demand paging and isolated user-space processes on top of the base kernel from Phases 1 and 2.

## What this implements

- **Demand paging** — TLB miss handling and page fault resolution
- **Swap pool** — flash device as backing store for page swap in/out
- **Round-robin page replacement** — with dirty-bit-aware eviction
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

- [uMPS3](https://github.com/virtualsquare/umps3) simulator
- `mipsel-linux-gnu` cross-compiler toolchain

## Build & Run

```bash
make
```

Open uMPS3, create a new machine, load `kernel.core.umps`, and configure the device paths to match the `testers/` directory.

## Authors

Davide Sarti, Francesco Tramontana, Leonardo Berselli, Francesco Tomba — Bachelor's in Computer Science, University of Bologna (2024)
