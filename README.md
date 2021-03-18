# CrossTalk (SRBDS) Proof-of-Concept

This repository contains Proof-of-Concept exploits for vulnerabilities described in [**CrossTalk: Speculative Data Leaks Across Cores Are Real**](https://download.vusec.net/papers/crosstalk_sp21.pdf) by Ragab, Milburn, Razavi, Bos and Giuffrida.

The demos in this repository utilise primitives from our [RIDL PoC](https://github.com/tristan-hornetz/ridl).

Testing was done on an Intel Core i7-7700K CPU with Debian and Linux Kernel 5.10.0.

## Setup

To successfully run these demos, an x86_64 Intel CPU and a fairly modern Linux-based OS are required.
The demos will not run on Windows or any other OS directly. We also recommend using a CPU with Intel TSX, but this is not strictly required.  


**Dependencies**

You need _cmake_ to build the demos.
On Debian and Ubuntu, it can be installed like this:
<!-- prettier-ignore -->
```shell
 sudo apt-get install cmake
 ```
**Build**

Building is as simple a cloning the repository and running
```shell
 make
 ```
If your CPU supports Intel TSX, you can manually disable TSX support with
```shell
 make notsx
 ```

**Run**

Before running any of the demos, you should allocate some huge pages with
```shell
 echo 16 | sudo tee /proc/sys/vm/nr_hugepages
 ```

If your CPU supports Intel TSX, you can pass the _--taa_ parameter to any of the demos to
utilise _TSX Asynchronous Abort_ instead of basic RIDL for leaking the LFB contents. 
This may work on systems that are resistant against other RIDL-type attacks.


## Demo #1: Reading the CPU Brand String

To check if any data is forwarded from a global staging buffer,
this demo utilizes the _cpuid_ instruction. It should fill the staging buffer with the CPU Brand
String, which can then be leaked from the Line Fill Buffer of any
other CPU core.

The demo can be started with 
```shell
./demo_cpuid
 ```
The output should be similar to this:

```
Intel(R) Core(TM) i7-7700K CPU @ 4.20GHz************************
 ```

If the output is even remotely similar to your CPU's Brand String, 
your system is vulnerable.

## Acknowledgements

Parts of this project are based on the [Meltdown Proof-of-Concept](https://github.com/IAIK/meltdown).  

[A PoC for several RIDL-class vulnerabilities](https://github.com/vusec/ridl) was published by the authors of the 
original RIDL-paper. Their implementation (especially for TAA) was used for troubleshooting and served as an additional source
for information on RIDL.
