import json
import pandas as pd
import matplotlib.pyplot as plt
import seaborn as sns
import numpy as np
from datetime import datetime
from collections import defaultdict

# Load the log data
def load_log_data(file_path="logs.json"):
    with open(file_path, 'r') as f:
        logs = json.load(f)
    return logs

# Function to determine if a node is a player or main node
def is_player_node(node_id):
    return "-----END PUBLIC KEY-----" in node_id

# Convert logs to DataFrame
def logs_to_dataframe(logs):
    df = pd.DataFrame(logs)
    # Drop rows with null messages
    df = df.dropna(subset=['message'])
    df['datetime'] = pd.to_datetime(df['timestamp'], unit='s')
    return df

# Extract node IDs from the logs
def extract_node_ids(df):
    # Extract node IDs from all messages
    node_ids = set()
    
    # Look for patterns like "Node X" or "Node X-Y-Z"
    for msg in df['message'].dropna():
        if 'Node ' in msg:
            parts = msg.split('Node ')[1:]
            for part in parts:
                # Get the node ID part
                node_id = part.split(' ')[0].split('\n')[0]
                # Remove trailing characters like colon, comma, etc.
                node_id = node_id.rstrip(':,.')
                
                # Only add if it looks like a valid node ID
                if node_id and node_id not in ['already', 'with', 'started', 'and']:
                    node_ids.add(node_id)
    
    # Separate into player nodes and main nodes
    player_nodes = [node for node in node_ids if is_player_node(node)]
    main_nodes = [node for node in node_ids if not is_player_node(node)]
    
    return list(main_nodes), list(player_nodes)

# Analyze transaction queue size
def analyze_transaction_queue(df, main_nodes, player_nodes):
    # Track queue size over time for each node type
    main_queue_sizes = []
    player_queue_sizes = []
    main_queue_times = []
    player_queue_times = []
    
    # Sort by timestamp
    df = df.sort_values('timestamp')
    
    # Extract queue info from messages
    queue_messages = df[df['message'].str.contains('Transaction added to Node|Transaction already exists in queue for Node', na=False)]
    
    # Process each message
    for _, row in queue_messages.iterrows():
        msg = row['message']
        timestamp = row['timestamp']
        
        # Extract node ID
        if 'Transaction added to Node' in msg:
            parts = msg.split('Transaction added to Node ')[1].split(':')
            node_id = parts[0]
            if is_player_node(node_id):
                player_queue_sizes.append(1)  # Increment for player
                player_queue_times.append(timestamp)
            else:
                main_queue_sizes.append(1)  # Increment for main
                main_queue_times.append(timestamp)
        
        # Track already existing transactions (indicates queue is filling up)
        elif 'Transaction already exists in queue for Node' in msg:
            node_id = msg.split('Transaction already exists in queue for Node ')[1]
            if is_player_node(node_id):
                player_queue_sizes.append(1)  # Indicate queue activity
                player_queue_times.append(timestamp)
            else:
                main_queue_sizes.append(1)  # Indicate queue activity
                main_queue_times.append(timestamp)
    
    # Create cumulative sums to visualize queue growth over time
    main_cumulative = np.cumsum(main_queue_sizes)
    player_cumulative = np.cumsum(player_queue_sizes)
    
    return main_queue_times, main_cumulative, player_queue_times, player_cumulative

# Identify peak processing periods
def identify_peak_periods(df):
    # Resample data to count events per second
    timeline_df = df.copy()
    timeline_df['timestamp'] = pd.to_datetime(timeline_df['timestamp'], unit='s')
    timeline_df.set_index('timestamp', inplace=True)
    
    # Count different types of events for both player and main nodes
    main_events = timeline_df[~timeline_df['message'].str.contains('-----END PUBLIC KEY-----', na=False)]
    player_events = timeline_df[timeline_df['message'].str.contains('-----END PUBLIC KEY-----', na=False)]
    
    # Resample into time bins
    main_event_counts = main_events.resample('1S').size()
    player_event_counts = player_events.resample('1S').size()
    all_event_counts = timeline_df.resample('1S').size()
    
    # Find peak periods (times with activity > mean + std)
    mean_all_events = all_event_counts.mean()
    std_all_events = all_event_counts.std()
    peak_threshold = mean_all_events + std_all_events
    
    peak_periods = all_event_counts[all_event_counts > peak_threshold]
    
    return main_event_counts, player_event_counts, all_event_counts, peak_periods, peak_threshold

# Analyze resource utilization
def analyze_resource_utilization(df, main_nodes, player_nodes):
    # Count mining and block events per node type over time
    main_activity = []
    player_activity = []
    main_activity_times = []
    player_activity_times = []
    
    # Mining and block activities
    mining_msgs = df[df['message'].str.contains('Mining|mined by Node|Block', na=False)]
    
    # Count by time bins
    timeline_df = mining_msgs.copy()
    timeline_df['timestamp'] = pd.to_datetime(timeline_df['timestamp'], unit='s')
    timeline_df.set_index('timestamp', inplace=True)
    
    # Separate player and main node activity
    main_mining = timeline_df[~timeline_df['message'].str.contains('-----END PUBLIC KEY-----', na=False)]
    player_mining = timeline_df[timeline_df['message'].str.contains('-----END PUBLIC KEY-----', na=False)]
    
    # Resample to count events per second
    main_activity_counts = main_mining.resample('1S').size()
    player_activity_counts = player_mining.resample('1S').size()
    
    return main_activity_counts, player_activity_counts

# Analyze game statistics
def analyze_game_stats(df):
    # Find game-related messages
    game_msgs = df[df['message'].str.contains('Game', na=False)]
    
    # Extract game IDs and states
    game_data = []
    
    for _, row in game_msgs.iterrows():
        msg = row['message']
        timestamp = row['timestamp']
        
        if 'Game ID:' in msg and 'Players:' in msg:
            try:
                game_id = msg.split('Game ID:')[1].split('\n')[0].strip()
                players = msg.split('Players:')[1].split('\n')[0].strip()
                winner = 'None' if 'Winner ID: None' in msg else msg.split('Winner ID:')[1].split('\n')[0].strip()
                complete = 'Yes' if 'Game Complete: Yes' in msg else 'No'
                chain_size = msg.split('Chain Size:')[1].split('\n')[0].strip()
                
                # Count moves
                move_count = msg.count('Move:')
                
                game_data.append({
                    'game_id': game_id,
                    'players': players,
                    'winner': winner,
                    'complete': complete,
                    'chain_size': int(chain_size),
                    'move_count': move_count,
                    'timestamp': timestamp
                })
            except:
                # Skip malformed messages
                continue
    
    # Convert to DataFrame for analysis
    game_df = pd.DataFrame(game_data)
    
    return game_df

# Create system load analysis visualization
def visualize_system_load(logs):
    df = logs_to_dataframe(logs)
    main_nodes, player_nodes = extract_node_ids(df)
    
    main_queue_times, main_cumulative, player_queue_times, player_cumulative = analyze_transaction_queue(df, main_nodes, player_nodes)
    main_event_counts, player_event_counts, all_event_counts, peak_periods, peak_threshold = identify_peak_periods(df)
    main_activity_counts, player_activity_counts = analyze_resource_utilization(df, main_nodes, player_nodes)
    game_df = analyze_game_stats(df)
    
    # Create a figure with multiple subplots
    fig = plt.figure(figsize=(20, 16))
    
    # 1. Transaction Queue Size Over Time
    ax1 = plt.subplot2grid((3, 2), (0, 0))
    
    # Convert timestamps to datetime for better plotting
    main_datetime = [datetime.fromtimestamp(ts) for ts in main_queue_times]
    player_datetime = [datetime.fromtimestamp(ts) for ts in player_queue_times]
    
    if main_queue_times:  # Only plot if we have data
        ax1.plot(main_datetime, main_cumulative, label="Main Nodes Queue", 
                marker='o', markersize=3, alpha=0.7, color='blue')
    
    if player_queue_times:  # Only plot if we have data
        ax1.plot(player_datetime, player_cumulative, label="Player Nodes Queue", 
                marker='o', markersize=3, alpha=0.7, color='green')
    
    ax1.set_title('Transaction Queue Growth Over Time by Node Type', fontsize=14)
    ax1.set_xlabel('Time')
    ax1.set_ylabel('Cumulative Transactions')
    ax1.legend()
    ax1.grid(True, linestyle='--', alpha=0.7)
    
    # 2. Peak Processing Periods
    ax2 = plt.subplot2grid((3, 2), (0, 1))
    
    # Plot all events
    ax2.plot(all_event_counts.index, all_event_counts.values, label='Total Activity', 
            color='purple', alpha=0.5)
    ax2.plot(main_event_counts.index, main_event_counts.values, label='Main Node Activity', 
            color='blue', alpha=0.5)
    ax2.plot(player_event_counts.index, player_event_counts.values, label='Player Node Activity', 
            color='green', alpha=0.5)
    
    # Highlight peak periods
    if not peak_periods.empty:
        ax2.scatter(peak_periods.index, peak_periods.values, color='red', 
                   label='Peak Activity', s=50)
    
    # Add threshold line
    ax2.axhline(y=peak_threshold, color='red', linestyle='--', 
               label=f'Peak Threshold ({peak_threshold:.2f})')
    
    ax2.set_title('System Activity and Peak Processing Periods', fontsize=14)
    ax2.set_xlabel('Time')
    ax2.set_ylabel('Events per Second')
    ax2.legend()
    ax2.grid(True, linestyle='--', alpha=0.7)
    
    # 3. Resource Utilization Over Time
    ax3 = plt.subplot2grid((3, 2), (1, 0), colspan=2)
    
    # Plot activity levels
    ax3.plot(main_activity_counts.index, main_activity_counts.values, 
            label='Main Node Mining Activity', color='blue')
    ax3.plot(player_activity_counts.index, player_activity_counts.values, 
            label='Player Node Mining Activity', color='green')
    
    ax3.set_title('Mining and Block Activity by Node Type', fontsize=14)
    ax3.set_xlabel('Time')
    ax3.set_ylabel('Mining Events per Second')
    ax3.legend()
    ax3.grid(True, linestyle='--', alpha=0.7)
    
    # 4. Game Statistics
    ax4 = plt.subplot2grid((3, 2), (2, 0))
    
    if not game_df.empty:
        # Analyze chain size growth over time
        game_df_filtered = game_df.sort_values('timestamp').drop_duplicates(subset=['game_id'], keep='last')
        
        # Bar chart of chain sizes by game
        sns.barplot(x='game_id', y='chain_size', data=game_df_filtered, ax=ax4)
        ax4.set_title('Final Chain Size by Game', fontsize=14)
        ax4.set_xlabel('Game ID')
        ax4.set_ylabel('Chain Size')
        ax4.set_xticklabels(ax4.get_xticklabels(), rotation=45, ha='right')
    else:
        ax4.text(0.5, 0.5, "No game data available", ha='center', va='center', fontsize=14)
    
    # 5. Move Count Analysis
    ax5 = plt.subplot2grid((3, 2), (2, 1))
    
    if not game_df.empty:
        # Track move counts over time
        game_df_sorted = game_df.sort_values('timestamp')
        game_grouped = game_df_sorted.groupby('game_id')
        
        for game_id, group in game_grouped:
            # Convert timestamps to datetime
            times = [datetime.fromtimestamp(ts) for ts in group['timestamp']]
            ax5.plot(times, group['move_count'], label=f'Game {game_id}', marker='o')
        
        ax5.set_title('Move Count Progress by Game', fontsize=14)
        ax5.set_xlabel('Time')
        ax5.set_ylabel('Number of Moves')
        ax5.legend(loc='upper left', bbox_to_anchor=(1, 1))
        ax5.grid(True, linestyle='--', alpha=0.7)
    else:
        ax5.text(0.5, 0.5, "No game data available", ha='center', va='center', fontsize=14)
    
    # Create additional figure for the game move distribution
    fig2 = plt.figure(figsize=(12, 8))
    ax6 = fig2.add_subplot(111)
    
    if not game_df.empty and 'move_count' in game_df.columns:
        # Extract all moves from all games
        all_moves = []
        chess_notation_pattern = r'Move: ([KQRBNP]?[a-h][1-8])'
        
        for _, row in df.iterrows():
            msg = str(row['message'])
            if 'Move:' in msg:
                import re
                moves = re.findall(chess_notation_pattern, msg)
                all_moves.extend(moves)
        
        if all_moves:
            # Count frequency of each move
            move_counts = pd.Series(all_moves).value_counts().reset_index()
            move_counts.columns = ['move', 'count']
            
            # Plot
            sns.barplot(x='move', y='count', data=move_counts.head(20), ax=ax6)
            ax6.set_title('Top 20 Chess Moves Distribution', fontsize=14)
            ax6.set_xlabel('Chess Move')
            ax6.set_ylabel('Frequency')
            ax6.set_xticklabels(ax6.get_xticklabels(), rotation=45, ha='right')
        else:
            ax6.text(0.5, 0.5, "No move data found", ha='center', va='center', fontsize=14)
    else:
        ax6.text(0.5, 0.5, "No game data available", ha='center', va='center', fontsize=14)
    
    # Save the figures
    plt.figure(fig.number)
    plt.tight_layout()
    plt.savefig('system_load_analysis.png', dpi=300)
    
    plt.figure(fig2.number)
    plt.tight_layout()
    plt.savefig('chess_moves_distribution.png', dpi=300)
    
    plt.show()
    
    return fig, fig2

if __name__ == "__main__":
    logs = load_log_data()
    visualize_system_load(logs)