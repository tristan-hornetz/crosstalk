# CrossTalk (SRBDS) Proof-of-Concept

This repository contains Proof-of-Concept exploits for vulnerabilities described in [**CrossTalk: Speculative Data Leaks Across Cores Are Real**](https://download.vusec.net/papers/crosstalk_sp21.pdf) by Ragab, Milburn, Razavi, Bos and Giuffrida.

All demos utilise primitives from our [RIDL PoC](https://github.com/tristan-hornetz/ridl).
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

If this is not the case, the other demos are also unlikely to work. Please note
that on some CPUs, you need to pass the _--taa_ parameter for this demo to function correctly.

## Demo #2: Observing RDRAND instructions

One instruction that interferes with the global staging buffer is _rdrand_, which invokes the hardware-based
random number generator. In this demo, a victim thread invokes _rdrand_ every few seconds. 
An attacker thread on another physical core then attempts to detect the instruction and forwards the result.

The demo can be started with 
```shell
./demo_rdrand
 ```

Note: The attacker will occasionally pick up noise instead of an _rdrand_ value. This can be mitigated by quitting all 
background applications. During testing, we noticed that even running a graphical session manager can
mess with the leaked values.

## Demo #3: Covert channel across cores

This demo aims to transmit data across cores. To do so, we utilise the same principles as with the _rdrand_ demo.
_rdrand_ is invoked repeatedly until the eight least significant bits of the random value match the char
that we want to transmit. We can then observe this value from our attacker thread. 

The demo can be started with 
```shell
./demo_string
 ```

## Warnings

* We provide this code as-is, without any warranty of any kind. It may cause undesirable behaviour on your system, 
  or not work at all. You are fully responsible for any consequences that might arise from running this code. Please refer to the license for further information.
  
  
* If you find that your system is susceptible to SRBDS, you should avoid using it as a multi-user system. SGX enclaves and VMs offer no protection. Make sure to install the latest microcode updates (either in your BIOS or your OS).


* This code is meant for testing purposes and should not be used on production systems. _Do not run this code on any machine that is not yours, or could be used by another person._

## Acknowledgements

Parts of this project are based on the [Meltdown Proof-of-Concept](https://github.com/IAIK/meltdown).  

[A PoC for several RIDL-class vulnerabilities](https://github.com/vusec/ridl) was published by the authors of the 
original RIDL-paper. Their implementation (especially for TAA) was used for troubleshooting and served as an additional source
for information on RIDL.
