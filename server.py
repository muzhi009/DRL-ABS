import socket
import threading
import time
import json
import torch
from agent import RLAgent

def handle_client(connection, address):
    try:
        print("Connected to:", address)

        while True:
            recv_str = connection.recv(1024)
            if recv_str== b'\x00':
                break
            elif recv_str==b'CLOSE_CONNECT\x00':
                print('rl agent train over')
                rl_agent.show_reward_pic()
                break
            json_data = recv_str.decode("utf-8")
            
            # Parse the received JSON data
            data = json.loads(json_data)
            
            state = [data["a"], data["b"], data["c"], data["d"]]
            reward =data["reward"]
            # print(f'state is {state[0]} {state[1]} {state[2]} {state[3]}, reward is {reward}')
            done = data["done"]
            if not done:
                action = rl_agent.get_action_by_one_step(state, reward, done)
                
                send_str = str(action)
                connection.send(bytes(send_str + '\0', "ascii"))  

            else:
                rl_agent.done_print()
                break

    except Exception as e:
        print("Exception occurred:", e)
    finally:
        print("close to:", address)
        connection.close()

def DRLServer():
    server = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    server.bind(("localhost", 8888))
    server.listen(5)
    
    try:
        while True:
            connection, address = server.accept()
            client_thread = threading.Thread(target=handle_client, args=(connection, address))
            client_thread.start()
    except Exception as e:
        print("Exception occurred:", e)
    finally:
        server.close()
        print("Server exit!")

if __name__ == '__main__':
    print(torch.__version__)
    env_name = "DuelingDQN-NS3-v0"  # env name
    rl_agent = RLAgent(env_name, 4, 3)  # Create RLAgent instance
    DRLServer()