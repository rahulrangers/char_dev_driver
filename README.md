
## Custom Linux Character Device Driver

This project is a simple Linux kernel module that implements a character device driver for a virtual device. It includes basic read and write operations, simulated data storage, and a structure for handling custom ioctl commands. The project demonstrates how to interact with a Linux device from user space using a device file.

## Table of Contents
- [Overview](#overview)
- [Features](#features)
- [Building and Running](#building-and-running)
- [Using the Device](#using-the-device)
- [Code Structure](#code-structure)

## Overview

This module registers a virtual character device in the Linux kernel and allows interaction through basic read and write operations. It provides a simple simulated data storage using a buffer and supports custom commands through ioctl (Input/Output Control).

## Features

- **Device Registration**: Registers a character device with a dynamically allocated major number.
- **Device File Creation**: Creates a device file at `/dev/mycdev-0` for user-space access.
- **Read/Write Operations**: Supports read and write operations for transferring data between user and kernel space.
- **Custom ioctl Commands**: Provides an ioctl interface for future expansion to handle custom commands.
- **Memory Management**: Efficiently manages memory allocation and deallocation for data storage.

## Building and Running

### Prerequisites

- Linux kernel development environment
- GCC compiler
- Root or sudo access to load and unload the kernel module

### Build the Module

Compile the kernel module with `make`:
```sh
cd kernel
make all
```

### Load the Module

Use `insmod` to insert the module into the kernel:
```sh
sudo insmod chrdev_kernel.ko
```

Verify the device registration:
```sh
lsmod | grep -i "chrdev_kernel"
ls -lrta /dev/mycdev-0
```

### Compile test file
To remove the module from the kernel, use:
```sh
cd user
gcc -o chrdev_user chrdev_user.c
```

### Give Permissions
```sh
sudo cp udev/rules.d/99-chrdev.rules /etc/udev/rules.d/
```
## Using the Device

### Write Data

To write data to the device:
```sh
./chrdev_user -d /dev/mycdev-0 -w "Hello World"
```

### Read Data

To read data from the device:
```sh
./chrdev_user -d /dev/mycdev-0 -r"
```

### Testing with ioctl

To reset the data in the device:
```sh
./chrdev_user -d /dev/mycdev-0 -i"
```
### Unload the Module

To remove the module from the kernel, use:
```sh
sudo rmmod mycdev
```
## Code Structure

- **Header Files**: Includes necessary kernel headers for module development.
- **Initialization (`init_module`)**: Sets up the device, registers it, and allocates memory for data storage.
- **Cleanup (`cleanup_module`)**: Frees allocated memory, destroys the device file, and unregisters the device.
- **File Operations**:
    - `cdev_open`: Called when the device is opened.
    - `cdev_read`: Reads data from the device.
    - `cdev_write`: Writes data to the device.
    - `cdev_ioctl`: Resets the data in the device.







































































































