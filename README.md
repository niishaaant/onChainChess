# 🧠 ChessChain: A Blockchain-Based Chess Game Simulator

## 📌 Project Overview

**ChessChain** simulates a decentralized chess platform where each move made by players is treated as a transaction on a *temporary game-specific blockchain*. Once a game concludes, the entire sequence of moves (the temporary chain) is transmitted as a single transaction to a **main blockchain network**.

This design mimics a modular, off-chain transaction structure — suitable for scalability and minimizing on-chain overhead — similar to Layer-2 scaling in blockchain systems.

---

## 🎯 Key Features

- 🔐 **RSA-based Identity**: Each player generates a public-private key pair using OpenSSL.
- ♟️ **Game-as-Blockchain**: Every chess game operates on its own mini blockchain (GameChain).
- 🔄 **Move-as-Transaction**: Each chess move is cryptographically signed and treated as a transaction.
- 🧱 **Mining**: Players mine blocks with moves; MainNodes mine blocks of completed games.
- 🌐 **Decentralized Network**: Multiple nodes are simulated; all support P2P syncing.
- 📤 **Finalization**: At the end of a game, the game chain is sent to the MainNode for verification and inclusion in the main chain.

---

## 🚀 How It Works

### 1. **Initialization**
- All data files are reset and necessary directories (`./data/`) are created.
- MainNodes and Players are initialized.
- Nodes are interconnected and mining threads are launched.

### 2. **Starting a Game**
- Two players create a temporary `Game` blockchain.
- Each player's move is signed and stored in a shared mempool.

### 3. **Mining by Players**
- After 5 valid moves are collected, a block is mined and added to the temporary chain.
- The game ends when a preset number of blocks (e.g., 3) are mined.

### 4. **Game Finalization**
- The complete game chain is validated.
- A `Player` sends the game as a transaction to a `MainNode`.

### 5. **Main Chain Inclusion**
- `MainNode` verifies the game, adds it to its transaction queue.
- When enough games are collected, a `MainBlock` is mined and added to the main chain.

---

## ⚙️ Dependencies

- **C++17**
- **OpenSSL** (for cryptographic key management)
- **nlohmann/json** (for JSON handling)
- **POSIX/Linux system** (for `system("rm -rf")` and other shell operations)
- Threads and mutexes from `std::thread`, `std::mutex`

---

## 🔧 Compilation & Execution

### 1. **Install Dependencies**

```bash
sudo apt update
sudo apt install libssl-dev
```

### 2. **Build the Project**

```bash
g++ -std=c++17 -o main main.cpp BlockGame.cpp Player.cpp Game.cpp Move.cpp MainBlock.cpp MainNode.cpp MainChain.cpp  -pthread -lssl -lcrypto
```

### 3. **Run It**

```bash
./main
```
