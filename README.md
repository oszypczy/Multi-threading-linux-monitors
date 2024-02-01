# Communication Buffer Implementation in C++ (monitors)

## Overview

This C program implements a communication buffer data structure to facilitate communication between processes in a Linux environment. The communication buffer allows for the storage of elements in a First-In-First-Out (FIFO) or Last-In-First-Out (LIFO) order. The implementation ensures synchronization to prevent issues such as reading from an empty buffer, writing to a full buffer, and interference between processes reading and writing.

## Problem Description

In a multi-process environment, communication buffers serve as a means for processes to exchange data. The challenge lies in coordinating access to these buffers to avoid race conditions, ensuring orderly reading and writing operations.

## Communication Buffer

A communication buffer is a structured data container capable of holding up to M elements of the same type. Processes can extract elements from the buffer in FIFO or LIFO order, providing flexibility in data retrieval.

## Synchronization Mechanisms

### 1. Preventing Reading from an Empty Buffer

To address the issue of reading from an empty buffer, the implementation includes a mechanism to block processes attempting to read when the buffer is empty. This ensures that a process can only read when there is data available.

### 2. Avoiding Writing to a Full Buffer

To prevent writing to a full buffer, the program includes a mechanism to block processes attempting to write when the buffer is at its capacity. This ensures that a process can only write when there is available space in the buffer.

### 3. Avoiding Interference

To prevent interference between processes reading and writing, the implementation uses monitors. Monitors encapsulate the communication buffer and provide mutual exclusion, allowing only one process to access the buffer at a time. This ensures the integrity of the buffer and prevents data corruption.
