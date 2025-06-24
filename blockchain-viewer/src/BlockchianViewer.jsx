import React, { useState, useEffect, useRef } from 'react';
import { ChevronDown, ChevronUp, FolderOpen, X, RefreshCw, Clock } from 'lucide-react';

const BlockchianViewer = ({ directoryHandler }) => {
  const [nodeData, setNodeData] = useState([]);
  const [selectedBlock, setSelectedBlock] = useState(null);
  const [selectedNodeId, setSelectedNodeId] = useState(null);
  const [loading, setLoading] = useState(false);
  const [error, setError] = useState(null);
  const [directoryHandle, setDirectoryHandle] = useState(null);
  const [lastRefresh, setLastRefresh] = useState(null);
  const [autoRefresh, setAutoRefresh] = useState(true);

  // Ref to store the interval ID
  const refreshIntervalRef = useRef(null);
  // Ref to store the directory handle for use in the interval
  const directoryHandleRef = useRef(null);

  // Function to extract nodeId from filename
  const extractNodeId = (filename) => {
    const match = filename.match(/^(-?\d+)_/);
    return match ? match[1] : null;
  };

  // Function to check if the File System Access API is supported
  const isFileSystemAccessSupported = () => {
    return 'showDirectoryPicker' in window;
  };

  // Function to open a directory picker and get access to the directory
  const openDirectoryPicker = async () => {
    if (!isFileSystemAccessSupported()) {
      setError('Your browser does not support the File System Access API. Please use Chrome, Edge, or another supported browser.');
      return;
    }

    try {
      setLoading(true);
      setError(null);
      setSelectedBlock(null);

      // Show directory picker
      const dirHandle = await window.showDirectoryPicker();
      setDirectoryHandle(dirHandle);
      directoryHandleRef.current = dirHandle;

      // Process all files in the directory
      await processDirectory(dirHandle);

      // Set up auto-refresh interval
      setupAutoRefresh(dirHandle);
    } catch (err) {
      console.error('Error accessing directory:', err);
      if (err.name === 'AbortError') {
        // User cancelled the picker
        setError('Directory selection was cancelled.');
      } else {
        setError(`Error accessing directory: ${err.message}`);
      }
      setLoading(false);
    }
  };

  const openDirectoryPickerr = async () => {
    if (!isFileSystemAccessSupported()) {
      setError('Your browser does not support the File System Access API. Please use Chrome, Edge, or another supported browser.');
      return;
    }

    try {
      setLoading(true);
      setError(null);
      setSelectedBlock(null);

      // Show directory picker
      const dirHandle = directoryHandler;
      setDirectoryHandle(dirHandle);
      directoryHandleRef.current = dirHandle;

      // Process all files in the directory
      await processDirectory(dirHandle);

      // Set up auto-refresh interval
      setupAutoRefresh(dirHandle);
    } catch (err) {
      console.error('Error accessing directory:', err);
      if (err.name === 'AbortError') {
        // User cancelled the picker
        setError('Directory selection was cancelled.');
      } else {
        setError(`Error accessing directory: ${err.message}`);
      }
      setLoading(false);
    }
  };

  // Function to set up auto-refresh interval
  const setupAutoRefresh = (dirHandle) => {
    // Clear any existing interval
    if (refreshIntervalRef.current) {
      clearInterval(refreshIntervalRef.current);
    }

    // Set up new interval
    if (autoRefresh) {
      refreshIntervalRef.current = setInterval(() => {
        processDirectory(dirHandle);
      }, 1000); // Refresh every 1 second
    }
  };

  // Toggle auto-refresh
  const toggleAutoRefresh = () => {
    const newAutoRefresh = !autoRefresh;
    setAutoRefresh(newAutoRefresh);

    if (newAutoRefresh && directoryHandleRef.current) {
      setupAutoRefresh(directoryHandleRef.current);
    } else if (refreshIntervalRef.current) {
      clearInterval(refreshIntervalRef.current);
    }
  };

  // Manually refresh data
  const manualRefresh = () => {
    if (directoryHandleRef.current) {
      processDirectory(directoryHandleRef.current);
    }
  };

  // Function to process the selected directory
  const processDirectory = async (dirHandle) => {
    try {
      setLoading(true);

      // Collect all JSON files in the directory
      const files = [];
      for await (const entry of dirHandle.values()) {
        if (entry.kind === 'file' && entry.name.endsWith('.json')) {
          const file = await entry.getFile();
          const content = await file.text();
          files.push({
            name: entry.name,
            content: JSON.parse(content)
          });
        }
      }

      // Filter and organize files by nodeId
      const blockchainFiles = files.filter(file => file.name.includes('_mainBlockchain.json'));
      const mempoolFiles = files.filter(file => file.name.includes('_mainMempool.json'));

      // Group files by nodeId
      const nodeDataMap = new Map();

      // Process blockchain files
      for (const file of blockchainFiles) {
        const nodeId = extractNodeId(file.name);
        if (nodeId) {
          if (!nodeDataMap.has(nodeId)) {
            nodeDataMap.set(nodeId, { nodeId });
          }
          nodeDataMap.get(nodeId).blockchain = file.content;
        }
      }

      // Process mempool files
      for (const file of mempoolFiles) {
        const nodeId = extractNodeId(file.name);
        if (nodeId && nodeDataMap.has(nodeId)) {
          nodeDataMap.get(nodeId).mempool = file.content;
        }
      }

      // Convert map to array and filter for only complete node data
      const processedNodeData = Array.from(nodeDataMap.values())
        .filter(node => node.blockchain && node.mempool);

      if (processedNodeData.length === 0) {
        setError('No valid blockchain/mempool file pairs found in the selected directory.');
        setLoading(false);
        return;
      }

      // Update the selected block if it exists in the new data
      if (selectedBlock) {
        const nodeStillExists = processedNodeData.some(node => node.nodeId === selectedBlock.nodeId);
        if (!nodeStillExists) {
          setSelectedBlock(null);
        } else {
          const node = processedNodeData.find(node => node.nodeId === selectedBlock.nodeId);
          const blockStillExists = node.blockchain.some(block => block.index === selectedBlock.block.index);
          if (!blockStillExists) {
            setSelectedBlock(null);
          } else {
            const updatedBlock = node.blockchain.find(block => block.index === selectedBlock.block.index);
            setSelectedBlock({
              nodeId: selectedBlock.nodeId,
              block: updatedBlock
            });
          }
        }
      }

      setNodeData(processedNodeData);
      if (!selectedNodeId && processedNodeData.length > 0) {
        setSelectedNodeId(processedNodeData[0].nodeId);
      }

      setLastRefresh(new Date());
      setLoading(false);
    } catch (err) {
      console.error('Error processing directory:', err);
      setError(`Error processing directory: ${err.message}`);
      setLoading(false);
    }
  };

  const handleBlockClick = (node, block) => {
    setSelectedBlock({
      nodeId: node.nodeId,
      block: block
    });
  };

  const closeBlockDetail = () => {
    setSelectedBlock(null);
  };

  // Function to parse game data from a string without relying on newlines
  const parseGameData = (gameString) => {
    // Initialize game data structure
    const gameData = {
      gameId: '',
      players: [],
      winnerId: '',
      gameComplete: false,
      chainSize: '',
      moves: []
    };

    // Extract game ID
    const gameIdMatch = gameString.match(/Game ID:\s*(\d+)/);
    if (gameIdMatch) {
      gameData.gameId = gameIdMatch[1];
    }

    // Extract players - capture everything between "Players:" and "Winner ID:"
    const playersMatch = gameString.match(/Players:\s*(.*?)(?=\s*Winner ID:|$)/s);
    if (playersMatch) {
      // Split by whitespace and filter out empty strings
      gameData.players = playersMatch[1].trim().split(/\s+/).filter(Boolean);
    }

    // Extract winner ID
    const winnerIdMatch = gameString.match(/Winner ID:\s*(-----BEGIN PUBLIC KEY-----[\s\S]+?-----END PUBLIC KEY-----)/);
    if (winnerIdMatch) {
      gameData.winnerId = winnerIdMatch[1].trim();
    }

    // Extract game completion status
    const gameCompleteMatch = gameString.match(/Game Complete:\s*(Yes|No)/);
    if (gameCompleteMatch) {
      gameData.gameComplete = gameCompleteMatch[1] === 'Yes';
    }

    // Extract chain size
    const chainSizeMatch = gameString.match(/Chain Size:\s*(\d+)/);
    if (chainSizeMatch) {
      gameData.chainSize = chainSizeMatch[1];
    }

    // Extract moves section
    const movesStartIndex = gameString.indexOf('Moves:');
    if (movesStartIndex !== -1) {
      const movesSection = gameString.substring(movesStartIndex + 'Moves:'.length);

      // Use regex to extract all moves
      // A move consists of Sender, Receiver, and Move information
      const moveRegex = /\s*Sender:\s*(-----BEGIN PUBLIC KEY-----[\s\S]+?-----END PUBLIC KEY-----)\s*,\s*Receiver:\s*(-----BEGIN PUBLIC KEY-----[\s\S]+?-----END PUBLIC KEY-----)\s*,\s*Move:\s*([^\s]+)/g;

      let moveMatch;
      while ((moveMatch = moveRegex.exec(movesSection)) !== null) {
        gameData.moves.push({
          sender: moveMatch[1].trim(),
          receiver: moveMatch[2].trim(),
          move: moveMatch[3].trim()
        });
      }
    }

    return gameData;
  };

  // Function to display a truncated public key
  const truncatePublicKey = (publicKey) => {
    if (!publicKey) return '';

    // Extract first and last part of the key for display
    const firstPart = publicKey.substring(0, 20);
    const lastPart = publicKey.substring(publicKey.length - 20);

    return `${firstPart}...${lastPart}`;
  };

  // Clean up interval on unmount
  useEffect(() => {
    return () => {
      if (refreshIntervalRef.current) {
        clearInterval(refreshIntervalRef.current);
      }
    };
  }, []);

  // Update interval when autoRefresh changes
  useEffect(() => {
    if (directoryHandleRef.current) {
      setupAutoRefresh(directoryHandleRef.current);
    }
  }, [autoRefresh]);

  // Check for File System Access API on component mount
  useEffect(() => {
    if (!isFileSystemAccessSupported()) {
      setError('Your browser does not support the File System Access API. Please use Chrome, Edge, or another supported browser.');
    }

    openDirectoryPickerr();
  }, []);

  // Render loading state
  if (loading && nodeData.length === 0) {
    return (
      <div className="flex items-center justify-center min-h-screen bg-gray-800 text-white">
        <div className="flex items-center gap-2">
          <RefreshCw size={24} className="animate-spin" />
          <span className="text-xl">Loading blockchain data...</span>
        </div>
      </div>
    );
  }

  // Render file picker if no data loaded yet
  if (nodeData.length === 0) {
    return (
      <div className="flex flex-col items-center justify-center min-h-screen bg-gray-900 text-white p-6">
        <h1 className="text-3xl font-bold text-purple-400 mb-6">Blockchain Game Viewer</h1>

        {error && (
          <div className="mb-6 text-red-500 p-4 bg-red-900 bg-opacity-30 rounded-lg max-w-md text-center">
            {error}
          </div>
        )}

        <button
          onClick={openDirectoryPicker}
          className="px-6 py-3 bg-purple-600 rounded-lg flex items-center gap-2 hover:bg-purple-700 transition-colors"
          disabled={!isFileSystemAccessSupported()}
        >
          <FolderOpen size={20} />
          <span>Select Blockchain Directory</span>
        </button>

        <p className="mt-4 text-gray-400 max-w-md text-center">
          Select a directory containing blockchain and mempool files in the format {"{nodeId}_mainBlockchain.json"} and {"{nodeId}_mainMempool.json"}.
        </p>
      </div>
    );
  }

  // Main view with blockchain data
  return (
    <div className="min-h-screen bg-gray-900 text-white p-4 flex flex-col">
      <header className="mb-6">
        <div className="flex justify-between items-center mb-4">
          <h1 className="text-2xl md:text-3xl font-bold text-purple-400">Blockchain Game Viewer</h1>

          <div className="flex fixed items-center gap-2 top-6 right-55">
            {loading && (
              <div className="animate-pulse text-blue-400 flex items-center text-xs">
                <RefreshCw size={12} className="animate-spin mr-1" />
                Updating...
              </div>
            )}

            <button
              onClick={toggleAutoRefresh}
              className={`px-3 py-1 flex items-center gap-1 rounded text-xs font-medium ${autoRefresh ? 'bg-green-700 text-green-100' : 'bg-gray-700 text-gray-300'
                }`}
            >
              <Clock size={12} />
              {autoRefresh ? 'Auto-Refresh On' : 'Auto-Refresh Off'}
            </button>

            <button
              onClick={manualRefresh}
              disabled={loading}
              className="px-3 py-1 bg-blue-700 rounded flex items-center gap-1 text-xs font-medium disabled:opacity-50"
            >
              <RefreshCw size={12} />
              Refresh Now
            </button>

            <button
              onClick={openDirectoryPicker}
              className="px-3 py-1 bg-gray-700 rounded-lg flex items-center gap-1 hover:bg-gray-600 transition-colors text-xs"
            >
              <FolderOpen size={12} />
              <span>Change Directory</span>
            </button>
          </div>
        </div>

        {lastRefresh && (
          <div className="text-xs text-gray-400 mb-2">
            Last updated: {lastRefresh.toLocaleTimeString()}
          </div>
        )}
      </header>

      <div className="flex-1 grid grid-cols-1 md:grid-cols-2 lg:grid-cols-3 gap-6 overflow-auto pb-6">
        {nodeData.map(node => (
          <div key={node.nodeId} className="flex flex-col bg-gray-800 rounded-lg overflow-hidden">
            <div className="p-4 bg-gray-700 flex justify-between items-center">
              <h2 className="font-bold text-xl text-purple-300">Node {node.nodeId}</h2>
              <div className="text-gray-300 text-sm">{node.blockchain.length} blocks</div>
            </div>

            <div className="flex-1 p-4 overflow-auto max-h-screen">
              <div className="space-y-4">
                {node.blockchain.map((block, index) => (
                  <div
                    key={block.index}
                    className="relative"
                  >
                    {/* Line connecting blocks */}
                    {index < node.blockchain.length - 1 && (
                      <div className="absolute w-1 bg-purple-500 left-1/2 transform -translate-x-1/2 h-10 top-full z-0"></div>
                    )}

                    <div
                      className="bg-gradient-to-r from-gray-700 to-gray-800 rounded-lg p-4 cursor-pointer hover:from-gray-600 hover:to-gray-700 transform transition-all duration-200 hover:scale-102 hover:shadow-lg"
                      onClick={() => handleBlockClick(node, block)}
                    >
                      <div className="flex justify-between items-center mb-2">
                        <div className="font-bold text-purple-300">Block #{block.index}</div>
                        <div className="text-xs text-gray-400">{block.games.length} games</div>
                      </div>

                      <div className="grid grid-cols-1 gap-2 text-xs">
                        <div>
                          <div className="text-gray-400">Hash:</div>
                          <div className="font-mono text-green-400 truncate">{block.hash.substring(0, 20)}...</div>
                        </div>
                        <div>
                          <div className="text-gray-400">Previous:</div>
                          <div className="font-mono text-blue-400 truncate">{block.previousHash.substring(0, 20)}...</div>
                        </div>
                        <div className="flex justify-between">
                          <div>
                            <div className="text-gray-400">Time:</div>
                            <div>{new Date(block.timestamp * 1000).toLocaleTimeString()}</div>
                          </div>
                          <div className="text-right">
                            <div className="text-gray-400">Nonce:</div>
                            <div>{block.nonce.toLocaleString()}</div>
                          </div>
                        </div>
                      </div>
                    </div>
                  </div>
                ))}
              </div>
            </div>
          </div>
        ))}
      </div>

      {/* Block detail popup */}
      {selectedBlock && (
        <div className="fixed inset-0 bg-black bg-opacity-75 flex items-center justify-center z-50 p-4">
          <div className="bg-gray-800 rounded-lg shadow-2xl w-full max-w-4xl h-3/4 flex flex-col relative animate-fadeIn">
            <button
              onClick={closeBlockDetail}
              className="absolute top-4 right-4 text-gray-400 hover:text-white"
            >
              <X size={24} />
            </button>

            <div className="p-6 bg-gray-700 rounded-t-lg">
              <div className="flex flex-col md:flex-row justify-between items-start md:items-center gap-4">
                <div>
                  <div className="text-sm text-gray-400">Node {selectedBlock.nodeId}</div>
                  <h2 className="text-2xl font-bold text-purple-300">Block #{selectedBlock.block.index}</h2>
                </div>
                <div className="grid grid-cols-2 gap-x-6 gap-y-2 text-sm">
                  <div>
                    <div className="text-gray-400">Timestamp:</div>
                    <div>{new Date(selectedBlock.block.timestamp * 1000).toLocaleString()}</div>
                  </div>
                  <div>
                    <div className="text-gray-400">Nonce:</div>
                    <div>{selectedBlock.block.nonce.toLocaleString()}</div>
                  </div>
                  <div>
                    <div className="text-gray-400">Hash:</div>
                    <div className="font-mono text-green-400 truncate">{selectedBlock.block.hash.substring(0, 16)}...</div>
                  </div>
                  <div>
                    <div className="text-gray-400">Previous Hash:</div>
                    <div className="font-mono text-blue-400 truncate">{selectedBlock.block.previousHash.substring(0, 16)}...</div>
                  </div>
                </div>
              </div>
            </div>

            <div className="p-6 flex-1 overflow-auto">
              <h3 className="text-xl font-bold text-purple-300 mb-4">Games ({selectedBlock.block.games.length})</h3>

              {selectedBlock.block.games.length === 0 ? (
                <div className="text-gray-400">No games in this block</div>
              ) : (
                <div className="space-y-6">
                  {selectedBlock.block.games.map((game, gameIndex) => {
                    // Parse the game data using our improved function
                    const gameData = parseGameData(game);

                    return (
                      <div
                        key={gameIndex}
                        className="bg-gray-800 border border-gray-700 rounded-lg overflow-hidden"
                      >
                        <div className="bg-gray-700 p-4">
                          <div className="flex flex-wrap justify-between gap-2 items-center">
                            <div className="font-bold text-lg">Game #{gameData.gameId}</div>
                            <div className={`px-3 py-1 rounded-full text-xs font-medium ${gameData.gameComplete ? 'bg-green-800 text-green-200' : 'bg-yellow-800 text-yellow-200'}`}>
                              {gameData.gameComplete ? 'Complete' : 'In Progress'}
                            </div>
                          </div>
                        </div>

                        <div className="p-4">
                          <div className="grid grid-cols-1 md:grid-cols-2 gap-4 mb-4">
                            <div>
                              <div className="text-gray-400 mb-1 text-sm">Players:</div>
                              <div className="space-y-1">
                                {gameData.players.map((player, playerIndex) => (
                                  <div key={playerIndex} className="font-mono text-xs text-blue-300 truncate py-1 px-2 bg-gray-900 rounded">
                                    {player}
                                  </div>
                                ))}
                              </div>
                            </div>

                            <div>
                              <div className="text-gray-400 mb-1 text-sm">Winner:</div>
                              {gameData.winnerId ? (
                                <div className="font-mono text-xs text-green-300 truncate py-1 px-2 bg-gray-900 rounded">
                                  {truncatePublicKey(gameData.winnerId)}
                                </div>
                              ) : (
                                <div className="text-gray-500 text-sm">No winner yet</div>
                              )}

                              <div className="mt-3">
                                <div className="text-gray-400 text-sm">Chain Size:</div>
                                <div className="text-sm">{gameData.chainSize}</div>
                              </div>
                            </div>
                          </div>

                          <div>
                            <div className="text-gray-400 mb-2 text-sm font-medium">Moves:</div>
                            <div className="grid grid-cols-1 sm:grid-cols-2 gap-2">
                              {gameData.moves.map((move, moveIndex) => (
                                <div key={moveIndex} className="bg-gray-700 rounded p-3 hover:bg-gray-600 transition-colors">
                                  <div className="flex justify-between items-center mb-2">
                                    <span className="text-purple-300 font-medium text-sm">Move {moveIndex + 1}</span>
                                    <span className="bg-yellow-900 text-yellow-300 font-mono px-2 py-1 rounded text-xs">{move.move}</span>
                                  </div>
                                  <div className="space-y-1">
                                    <div className="text-xs flex items-center">
                                      <span className="text-gray-400 w-16">Sender:</span>
                                      <span className="text-blue-300 font-mono truncate text-xs">{truncatePublicKey(move.sender)}</span>
                                    </div>
                                    <div className="text-xs flex items-center">
                                      <span className="text-gray-400 w-16">Receiver:</span>
                                      <span className="text-green-300 font-mono truncate text-xs">{truncatePublicKey(move.receiver)}</span>
                                    </div>
                                  </div>
                                </div>
                              ))}

                              {gameData.moves.length === 0 && (
                                <div className="text-gray-500 text-sm col-span-2">No moves recorded</div>
                              )}
                            </div>
                          </div>
                        </div>
                      </div>
                    );
                  })}
                </div>
              )}
            </div>
          </div>
        </div>
      )}

      <footer className="mt-6 text-center text-gray-500 text-sm py-2">
        <p>Blockchain Game Viewer - {nodeData.length} nodes loaded</p>
      </footer>
    </div>
  );
};

export default BlockchianViewer;