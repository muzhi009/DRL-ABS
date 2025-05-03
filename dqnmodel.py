import torch
from torch import nn
from torch.nn import functional as F
import numpy as np
import collections
import random
import csv

# ------------------------------------- #
# Experience replay pool
# ------------------------------------- #

class ReplayBuffer:
    def __init__(self, capacity):  # Maximum capacity of experience pool
        # Create a queue, first in first out
        self.buffer = collections.deque(maxlen=capacity)
    # Add data to the queue
    def add(self, state, action, reward, next_state, done):
        # Save in list type
        self.buffer.append((state, action, reward, next_state, done))
    # Randomly sample batch_size sets of data from the queue
    def sample(self, batch_size):
        # Randomly select batch_size elements
        transitions = random.sample(self.buffer, batch_size)
        state, action, reward, next_state, done = zip(*transitions)
        return np.array(state), action, reward, np.array(next_state), done
    # Measure the length of the queue at the current moment
    def size(self):
        return len(self.buffer)

# ------------------------------------- #
# Dueling DQN
# ------------------------------------- #

class DuelingDQNNet(nn.Module):
    def __init__(self, n_states, n_hiddens1, n_hiddens2, n_actions):
        super(DuelingDQNNet, self).__init__()
        self.fc1 = nn.Linear(n_states, n_hiddens1)
        self.fc2 = nn.Linear(n_hiddens1, n_hiddens2)
        
        self.advantage = nn.Linear(n_hiddens2, n_actions)
        self.value = nn.Linear(n_hiddens2, 1)
    
    def forward(self, x):
        x = F.relu(self.fc1(x))
        x = F.relu(self.fc2(x))
        
        advantage = self.advantage(x)
        value = self.value(x)
        
        q_values = value + (advantage - advantage.mean(dim=1, keepdim=True))
        
        return q_values

# ------------------------------------- #
# Algorithm main body
# ------------------------------------- #

class DuelingDQN:
    def __init__(self, n_states, n_hiddens1, n_hiddens2, n_actions, dqn_lr, gamma, device, updatePeriod):
        # Value network - training
        self.dueling_dqn = DuelingDQNNet(n_states, n_hiddens1, n_hiddens2, n_actions).to(device)
        # Value network - target
        self.target_dueling_dqn = DuelingDQNNet(n_states, n_hiddens1, n_hiddens2, n_actions).to(device)
        
        # Initialize the parameters of the value network, the parameters of the two value networks are the same
        self.target_dueling_dqn.load_state_dict(self.dueling_dqn.state_dict())
        
        # Optimizer for the value network
        self.dueling_dqn_optimizer = torch.optim.Adam(self.dueling_dqn.parameters(), lr=dqn_lr)
        
        # Attribute assignment
        self.gamma = gamma  # Discount factor
        self.n_actions = n_actions
        self.device = device
        self.updatePeriod = updatePeriod
        self.total_loss = 0  # Used to count the total loss of each round to calculate the average loss of each round of training
        self.total_updates = 0 # Used for training round statistics
    
    # Action selection
    def take_action(self, state, epsilon):
        if random.random() < epsilon:
            # Choose a random action
            action = random.randint(0, self.n_actions - 1)
        else:
            # Choose action with highest Q-value
            state = torch.tensor(state, dtype=torch.float).view(1, -1).to(self.device)
            q_values = self.dueling_dqn(state)
            action = q_values.argmax(dim=1).item()
        
        return action
    
    # Direct update
    def update(self, transition_dict):
        states = torch.tensor(transition_dict['states'], dtype=torch.float).to(self.device)
        action = torch.tensor(transition_dict['actions']).view(-1, 1).to(self.device)
        rewards = torch.tensor(transition_dict['rewards'], dtype=torch.float).view(-1, 1).to(self.device)
        next_states = torch.tensor(transition_dict['next_states'], dtype=torch.float).to(self.device)
        dones = torch.tensor(transition_dict['dones'], dtype=torch.float).view(-1, 1).to(self.device)
        
        # The value target network obtains the value of each action at the next moment
        next_q_values = self.target_dueling_dqn(next_states)
        
        # The target value of the action value at the current moment
        q_targets = rewards + self.gamma * next_q_values.max(dim=1, keepdim=True)[0] * (1 - dones)
        q_targets = q_targets.view(-1, 1)  # Adjust the shape of the target value to [batch_size, 1]
        
        # The predicted value of the action value at the current moment
        q_values = self.dueling_dqn(states).gather(1, action)
        
        # Calculate the loss
        lambda_l2 = 0.001 # L2 regularization coefficient
        l2_norm = sum(p.pow(2.0).sum() for p in self.dueling_dqn.parameters())
        dueling_dqn_loss = torch.mean(F.mse_loss(q_values, q_targets)) + lambda_l2 * l2_norm

        # Accumulate loss and update times
        self.total_loss += dueling_dqn_loss.item()
        

        # Gradient update of the value network
        self.dueling_dqn_optimizer.zero_grad()
        dueling_dqn_loss.backward()
        self.dueling_dqn_optimizer.step()
        
        # Update the target model every updatePeriod times of training
        if self.total_updates % self.updatePeriod == 0:
            self.target_dueling_dqn.load_state_dict(self.dueling_dqn.state_dict())
        self.total_updates += 1
    def end_of_epoch(self):
        # Calculate the average loss for this round
        average_loss = self.total_loss / self.total_updates if self.total_updates > 0 else 0
        print("Average Dueling DQN Loss for this epoch:", average_loss)
        # Specify the path of the CSV file to be written
        csv_file = 'average_loss_data.csv'
        # Open the CSV file in write mode
        with open(csv_file, 'a', newline='') as file:
            # Create a CSV writer object
            writer = csv.writer(file)
            # Write the header (if any)
            # Check if the file is empty, if so, write the header
            if file.tell() == 0:
                writer.writerow(['average_loss'])  # Write the header
            writer.writerow([average_loss])        
        print("Data has been written to the CSV file:", csv_file)
        # Reset the loss and update counter
        self.total_loss = 0
        self.total_updates = 0
    
    def save_models(self, dueling_dqn_path, target_dueling_dqn_path):
        torch.save(self.dueling_dqn.state_dict(), dueling_dqn_path)  # Save the parameters of the value network
        torch.save(self.target_dueling_dqn.state_dict(), target_dueling_dqn_path) # Save the parameters of the target value network