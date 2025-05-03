import numpy as np
import torch
from dqnmodel import ReplayBuffer, DuelingDQN
from parsers import args 
import csv
import time

class RLAgent:
    def __init__(self, env_name, m_states_dim, m_actions_dim):
        self.env_name = env_name
        self.args = args
        self.device = torch.device('cuda') if torch.cuda.is_available() else torch.device('cpu')
        
        # Create environment
        self.n_states = m_states_dim
        self.n_actions = m_actions_dim
        self.last_state = [0,0,0,0]
        self.last_action = 0
        self.sum_reward = 0
        self.sum_reward_list = []  # Record the reward for each episode
        self.episode = 1
        self.isfirst = True
        self.episodeCount  = 0

        # Experience replay buffer
        self.replay_buffer = ReplayBuffer(capacity=self.args.buffer_size)
        
        # RL model
        self.agent = DuelingDQN(n_states=self.n_states,
                          n_hiddens1=self.args.n_hiddens1,
                          n_hiddens2=self.args.n_hiddens2,
                          n_actions=self.n_actions,
                          dqn_lr=self.args.dqn_lr,
                          gamma=self.args.gamma,
                          device=self.device,
                          updatePeriod=self.args.update_period
                          )
    def select_action(self, state):
        epsilon = 1 / (self.episode/5 + 1)
        action = self.agent.take_action(state, epsilon)
        self.last_state = state
        self.last_action = action
        return action
    def agent_update(self, reward, next_state, done):
        self.replay_buffer.add(self.last_state, self.last_action, reward, next_state, done) #Add experience to replay pool
        self.episodeCount+=1
        self.sum_reward += reward
        # If the experience pool exceeds min_size, start training
        if self.replay_buffer.size() > self.args.min_size:
            # Random sampling of batch_size group from the experience pool
            s, a, r, ns, d = self.replay_buffer.sample(self.args.batch_size)
            # Constructing a dataset
            transition_dict = {
                'states': s,
                'actions': a,
                'rewards': r,
                'next_states': ns,
                'dones': d,
            }
            # Model train
            self.agent.update(transition_dict)

    def get_action_by_one_step(self, state, reward, done):
        if not self.isfirst:
            self.agent_update(reward, state, done)
        action = self.select_action(state)
        self.isfirst = False
        return action
    def done_print(self):
        self.episodeCount = 0
        self.sum_reward_list.append(self.sum_reward)
        self.sum_reward = 0
        self.episode +=1
        self.isfirst = True
        self.agent.end_of_epoch()
        self.agent.save_models('dueling_dqn.pth','target_dueling_dqn.pth')
    def show_reward_pic(self):
        csv_file = 'reward_data.csv'
        with open(csv_file, 'w', newline='') as file:
            writer = csv.writer(file)
            writer.writerow(['Reward'])
            # Write reward list to CSV file
            for reward in self.sum_reward_list:
                writer.writerow([reward])
        print("Data has been written to CSV file:", csv_file)


if __name__ == "__main__":
    
    env_name = "DuelingDQN-NS3-v0"  # env name
    rl_agent = RLAgent(env_name, 4, 3)
    
    state = [10, 2.1, 3.2, 0.5]
    reward =0.8
    done = False
    action = rl_agent.get_action_by_one_step(state, reward, done)
    print('current state action is ',action)
