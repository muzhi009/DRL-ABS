# Parameter definition
import argparse  # Parameter setting

# Create an argument parser
parser = argparse.ArgumentParser()

# Parameter definition
parser.add_argument('--dqn_lr', type=float, default=1e-3, help='Learning rate of DQN')
parser.add_argument('--n_hiddens1', type=int, default=64, help='Number of neurons in the hidden layer')
parser.add_argument('--n_hiddens2', type=int, default=64, help='Number of neurons in the hidden layer')
parser.add_argument('--gamma', type=float, default=0.99, help='Discount factor')
parser.add_argument('--buffer_size', type=int, default=5000, help='Capacity of the experience replay buffer')
parser.add_argument('--min_size', type=int, default=200, help='Start training when the experience replay buffer size exceeds 200')
parser.add_argument('--batch_size', type=int, default=64, help='Number of samples per training batch')
parser.add_argument('--update_period', type=int, default=100, help='Interval for model updates')

# Parse the arguments
args = parser.parse_args()