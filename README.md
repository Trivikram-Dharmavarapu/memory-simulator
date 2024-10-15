# Memory Hierarchy Simulation

This project simulates a memory hierarchy system consisting of TLB, Page Table, Data Cache, and L2 Cache. The goal is to simulate memory accesses (reads and writes), implement the data structures for different cache levels, and output the performance statistics.

## Features

- **TLB (Translation Lookaside Buffer):** Caches the most recent translations from virtual page numbers to physical page numbers.
- **Page Table:** Maps virtual pages to physical pages.
- **Data Cache:** Implements cache lines with a configurable write policy (write-through/no write-allocate).
- **L2 Cache:** Simulates an optional second-level cache.
- **Memory Access Simulation:** Simulates memory reads and writes using the trace file as input, calculates cache hits and misses, and provides simulation statistics.
  
## Configuration Files

The simulation uses two input files:
1. **`trace.config`** – Configuration file that defines TLB, Cache, Page Table, and Memory settings.
2. **`trace.dat`** – Trace file with memory access patterns (reads and writes) in hexadecimal format.

## How it Works

### 1. Read Configuration
The configuration file contains all the parameters needed to set up the memory hierarchy, such as:
- TLB settings (number of sets, set size)
- Page Table settings (virtual pages, physical pages, page size)
- Cache settings (number of sets, set size, line size, and write policy)
- Whether to use virtual addresses, TLB, or L2 Cache.

### 2. Read and Process Trace Data
Each entry in the trace file represents a memory access operation (either `R` for read or `W` for write) followed by a hexadecimal address. The simulation processes these entries one by one:
- **TLB Lookup:** Checks if the virtual address is cached in the TLB.
- **Page Table Lookup:** Translates the virtual address to a physical address if the TLB misses.
- **Data Cache Access:** Simulates cache hit/miss and manages write policies.
- **L2 Cache Access:** (If enabled) Simulates access to the L2 Cache if the Data Cache misses.

### 3. LRU Replacement Policy
The Least Recently Used (LRU) policy is applied in both the TLB and cache sets to evict the least recently used entry when needed.

### 4. Output Statistics
After processing all trace entries, the simulation outputs detailed statistics including:
- TLB hit/miss ratio
- Page Table hit/miss ratio
- Data Cache hit/miss ratio
- L2 Cache hit/miss ratio
- Memory read/write ratio

## Compilation and Execution

To compile and run the program:
```bash
g++ -o memory_sim memory_sim.cpp
./memory_sim
```

Make sure the `trace.config` and `trace.dat` files are in the same directory as the compiled program.

## File Structure

- **`memory_sim.cpp`** – Main source file containing the logic for simulating memory hierarchy and cache behavior.
- **`trace.config`** – Configuration file defining memory hierarchy settings.
- **`trace.dat`** – Trace file with memory access patterns.

## Example Configuration File (`trace.config`)

```plaintext
Data TLB configuration:
Number of sets: 16
Set size: 4

Page Table configuration:
Number of virtual pages: 64
Number of physical pages: 32
Page size: 4096

Data Cache configuration:
Number of sets: 32
Set size: 4
Line size: 64
Write through/no write allocate: y

L2 Cache configuration:
Number of sets: 64
Set size: 8
Line size: 128

Virtual addresses: y
TLB: y
L2 cache: y
```

## Example Trace File (`trace.dat`)

```plaintext
R 0x1A2B
W 0x1A2C
R 0x3C4D
```

## License

This project is open-source and available under the MIT License.

---
