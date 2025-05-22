# Linux Memory Management

## Overview

The Linux kernel manages memory in two primary ways:

- **Virtual Memory-based approach**: Memory is virtualized, which is the usual case.
- **Physical Memory organization**: How the kernel organizes actual RAM pages.

---

## Why Do We Need Virtual Memory?

- The Linux kernel uses a **flat memory model**, meaning programs assume they execute in the **same linear address space**.
- This works fine for a **single process**, but fails when **multiple processes** are running simultaneously — each cannot share the same physical memory space.
- **Virtual memory** solves this by giving **each process the illusion of having the same virtual address space**, while in reality, each maps to **different physical memory**.

### How is This Achieved?

- Through the use of a **page table** for each process.
- Each process has its **own page table**, which maps **virtual addresses** to **physical addresses**.
- Two different processes can use the **same virtual address**, but these will map to **different physical memory**.

### Role of the MMU

- The **Memory Management Unit (MMU)** handles the **translation from virtual memory to physical memory** using the process's page table.
- This translation ensures process isolation and memory protection, enabling multitasking safely and efficiently.

---

## Virtual Address Space (VAS)

- Each process in Linux has its own **User Virtual Space (UVS)**, but shares a common **Kernel Virtual Space (KVS)**.
- Therefore, **any changes in KVS are visible to all processes**.

### 32-bit Address Space Layout

- On 32-bit Linux systems (4 GB virtual address space), the address space is typically split into:

| User Space | Kernel Space | Split Ratio |
|------------|--------------|-------------|
| 3 GB       | 1 GB         | 3:1         |
| 2 GB       | 2 GB         | 2:2 (less common) |

### Kernel Virtual Segment (KVS) Breakdown

- The **Kernel Virtual Address Space** includes:
  - **Kernel Image**
  - **vmalloc area**
  - **Loadable Kernel Modules (LKMs)**
  - **Lowmem Region**
  - **Highmem Region**

#### Lowmem Region

- **Directly mapped** into physical RAM.
- Known as **kernel logical addresses**.
- Efficient access without intermediate translation.
- Typically starts at `PAGE_OFFSET` (e.g., `0xc0000000` on 32-bit IA-32 with 3:1 split).
- Used by `kmalloc()`:
  - Allocates **physically contiguous memory** from lowmem.
  - Fast and suitable for small/medium allocations.

#### Highmem Region

- Not directly mapped.
- Needs **temporary mappings** to access.
- Used when RAM exceeds the size of the lowmem region.
- Kernel uses highmem for large memory systems on 32-bit architectures.
- Requires special handling to translate addresses.

#### vmalloc Region

- Allocates memory that is **virtually contiguous** but not necessarily **physically contiguous**.
- Located in a distinct part of kernel virtual address space.
- Used by:
  - Kernel components
  - Device drivers
  - `vmalloc()` API

---

## Understanding `/proc/PID/maps`

- Each line of the `/proc/PID/maps` output corresponds to a **VMA (Virtual Memory Area)** data structure.
- The **VMA** abstracts memory segments or mappings.
- **Note**: Only user-space segments are represented by VMAs. Kernel segments do **not** have VMAs.

---

## Memory Regions in VAS

### 1. User Mode VAS (UVS)

- Takes **3 GB** of VAS: from `0x00000000` to `0xbfffffff`.
- Unique for each process.
- Changes in UVS are not visible to other processes.

### 2. Kernel Mode VAS (KVS)

- Takes **1 GB** of VAS: from `0xc0000000` to `0xffffffff`.
- Shared among all processes.
- Includes:
  - Kernel code/data
  - vmalloc space
  - Lowmem and Highmem regions
  - Kernel modules

---

## High Memory on 32-bit Systems

- Example: On a 32-bit system with 3:1 VM split and 512 MB RAM:
    - Entire 512 MB can be direct-mapped into kernel at `PAGE_OFFSET`.
- Issue with large RAM (e.g., 2 GB):
    - Only part (e.g., 768 MB) is **direct-mapped** into **lowmem**.
    - Remaining RAM goes into **ZONE_HIGHMEM**:
        - Cannot be directly mapped.
        - Temporarily mapped when needed.
        - Known as the **high-memory region**.

---

## Physical Memory Organization in Linux

The Linux kernel organizes physical RAM into a **hierarchical structure** to allow optimal memory usage and system performance.

---

### 🔰 Memory Hierarchy: Nodes → Zones → Page Frames

- **Node**: Abstracts a **physical RAM bank** (hardware), often associated with one or more CPU cores.
- **Zone**: Linux's way of handling hardware memory quirks (e.g., `ZONE_DMA`, `ZONE_NORMAL`, `ZONE_HIGHMEM`).
- **Page Frame**: Smallest manageable unit of RAM; typically **4 KB** in size.

---

### 🧠 NUMA vs UMA Architectures

- **NUMA (Non-Uniform Memory Access)**:
  - Each CPU core accesses its **nearest memory node** faster.
  - Systems have multiple nodes and are typical in servers and large machines.
  - Enhances performance by improving memory locality.
  - Uses the `pg_data_t` structure to represent each node.

- **UMA (Uniform Memory Access)**:
  - All memory access latency is uniform regardless of CPU core.
  - Common in desktops, laptops, and embedded systems.

---

### 🗂️ Zones in Linux

| Zone         | Description                                                               |
|--------------|---------------------------------------------------------------------------|
| `ZONE_DMA`   | For devices requiring memory below 16 MB                                  |
| `ZONE_DMA32` | For 32-bit DMA-capable devices (below 4 GB)                               |
| `ZONE_NORMAL`| Standard low memory zone, directly mapped by kernel                       |
| `ZONE_HIGHMEM`| High memory on 32-bit systems; temporarily mapped by the kernel         |
| `ZONE_MOVABLE`| For memory migration/hotplug operations                                  |

Each **zone** contains a range of **Page Frame Numbers (PFNs)**.

---

## 🧩 Linux Page Allocator (Buddy System Allocator - BSA)

The kernel’s **primary memory allocator**, known as the **Buddy System Allocator**, uses a power-of-two allocation strategy.

### 📦 Freelist Organization

- The **buddy system freelist** is an **array of pointers** to **doubly linked circular lists**.
- Each index in the array represents an **order**:
  - `order = n` implies chunk size of `2^n` pages.
  - On x86 and ARM, `MAX_ORDER = 11` → `order 0` to `order 10`.

| Order | Pages | Memory Size (4 KB pages) |
|-------|-------|---------------------------|
| 0     | 1     | 4 KB                      |
| 1     | 2     | 8 KB                      |
| 2     | 4     | 16 KB                     |
| 3     | 8     | 32 KB                     |
| ...   | ...   | ...                       |
| 10    | 1024  | 4 MB                      |

---

### ⚙️ Page Allocation Flow (Example)

- Request: A device driver asks for **128 KB**.
1. 128 KB = **32 pages** → `order = 5` (as 2^5 = 32)
2. Kernel checks **order 5** list:
   - If available → allocate
   - If not, check **order 6**, **order 7**, etc.
3. Suppose order 6 (64 pages = 256 KB) is available:
   - **Split** the 256 KB into two 128 KB buddies.
   - Allocate one, return the other to order 5 list.

---

### 🔄 Deallocation (Freeing Pages)

- On freeing a 128 KB chunk:
  - Kernel checks if its **buddy** (adjacent block of same size) is also free.
  - If yes, **merge** the buddies into a larger chunk (256 KB).
  - Enqueue the merged chunk in **order 6 list**.
  - This merging continues recursively, reducing fragmentation.

---

### ⚠️ Internal Fragmentation: The Downside

- Suppose a request is made for **132 KB** (not a power of two).
  - Nearest available chunk: **256 KB** (order 6)
  - Driver uses only 132 KB → **124 KB wasted**!
- This waste is called **internal fragmentation**, the primary downside of the buddy system.

---

### ✅ Pros of Buddy System

- Avoids external fragmentation
- Allocates **physically contiguous memory**
- **CPU cache line aligned** memory
- **Time complexity**: O(log n), making it reasonably fast

### ❌ Cons

- Can result in significant **internal fragmentation**
- Allocates memory in powers-of-two → inefficient for uneven sizes

---

### 🔁 Interaction with Slab Allocator

- The **slab allocator** (e.g., SLUB) sits **on top of the page allocator**.
- Used for small, frequent allocations (e.g., kernel objects, inode cache).
- Ultimately, all allocations still come from the **page allocator**, and thus from **lowmem**.

---

### 🔒 Kernel vs User Memory

- **Kernel memory**:
  - Allocated via `kmalloc()`, `vmalloc()`, `alloc_pages()`, etc.
  - **Non-swappable** for performance and safety.
- **User memory**:
  - Allocated via `malloc()`, `mmap()`
  - **Swappable** by default (can be locked using `mlock()`/`mlockall()`).

---


