# Rainbow

Rainbow screen test is a Silent Data Corruption(SDC) test. The test attempts to
find CPU hardware faults by exercising privilege code in kernel and hypervisor.

The basic operation of the test is as follows:

* Do little work in user space, push the drama up into kernel or hypervisor.
* Provoke with Fork, COW page promotion, mmap and mprotect, Looping back data through pipes
* Data provenance is marked by assigning each process a unique recognizable data pattern.


## Prerequisites:

Designed to run under Unix/Linux OS.

* cmake: https://cmake.org/
* Abseil-cpp: https://github.com/abseil/abseil-cpp


## Building

```
sh$ git clone https://github.com/google/rainbow.git
sh$ cd rainbow
sh$ mkdir build
sh$ cd build
sh$ cmake ..
sh$ cmake --build . --target rainbow
```
Note that rainbow must be built with the C++17 standard.

## Future updates

* Update instruction to run rainbow with gVisor to provoke hypervisor.
