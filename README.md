# DRL-ABS

## Introduction

This project is deep reinforcement learning-based adaptive buffer sizing. It uses NS3 for network simulation and PyTorch to implement the Dueling DQN algorithm in reinforcement learning. It enables a combined simulation through socket communication. 

### Dueling_DQN Folder

This folder contains the PyTorch  related code for implementing the Dueling DQN algorithm.

### ns3socket Folder

It holds the NS3 - specific code related to socket communication. The code here is responsible for setting up the network simulation environment in NS3, establishing socket connections for data exchange with the PyTorch part, and handling the simulation events related to RL state and action.

### fifo-duelingDQN-queue-disc.cc

This file implements all methods of the class

### fifo-duelingDQN-queue-disc.h

 This header file defines the queue discipline about Dueling DQN under traffic-control.  

## Usage

Install NS3 and pytorch, including nlohmann json.

1. ns-3: https://www.nsnam.org/
2. nlohmann json:  [nlohmann/json: JSON for Modern C++](https://github.com/nlohmann/json) 
3. pytorch: [pytorch/pytorch: Tensors and Dynamic neural networks in Python with strong GPU acceleration](https://github.com/pytorch/pytorch) 

- Go to the DQN folder and run Python
- Create a new ns3 module named ns3socket and replace the contents of the model folder
- Place the cc and h files in the ns3 src/traffic control folder and modify the configuration files
- Write a script and run it
- Regarding how to run/execute the program, please refer to the tutorial documentation provided by ns-3.
