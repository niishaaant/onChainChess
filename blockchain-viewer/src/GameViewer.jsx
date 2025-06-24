import React, { useState, useEffect, useRef } from 'react';
import { ChevronDown, ChevronUp, FolderOpen, X, RefreshCw, Clock, ChevronRight, ArrowRight, Activity } from 'lucide-react';

const GameViewer = ({ directoryHandler }) => {
    const [nodeData, setNodeData] = useState([]);
    const [selectedNode, setSelectedNode] = useState(null);
    const [selectedChain, setSelectedChain] = useState(null);
    const [selectedBlock, setSelectedBlock] = useState(null);
    const [loading, setLoading] = useState(false);
    const [error, setError] = useState(null);
    const [directoryHandle, setDirectoryHandle] = useState(null);
    const [lastRefresh, setLastRefresh] = useState(null);
    const [autoRefresh, setAutoRefresh] = useState(true);

    // Refs to store the interval ID and directory handle
    const refreshIntervalRef = useRef(null);
    const directoryHandleRef = useRef(null);

    // Function to extract nodeId from filename
    const extractNodeId = (filename) => {
        const match = filename.match(/^(-?\d+)_|^([^_]+)_/);
        return match ? (match[1] || match[2]) : null;
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
            setSelectedNode(null);
            setSelectedChain(null);
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

        try {
            setLoading(true);
            setError(null);
            setSelectedNode(null);
            setSelectedChain(null);
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
            const blockchainFiles = files.filter(file => file.name.includes('blockchain.json'));
            const mempoolFiles = files.filter(file => file.name.includes('mempool.json'));
            const completeGamesFiles = files.filter(file => file.name.includes('completeGames.json'));

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

            // Process complete games files
            for (const file of completeGamesFiles) {
                const nodeId = extractNodeId(file.name);
                if (nodeId) {
                    if (!nodeDataMap.has(nodeId)) {
                        nodeDataMap.set(nodeId, { nodeId });
                    }
                    nodeDataMap.get(nodeId).completeGames = file.content;
                }
            }

            // Convert map to array and filter for only nodes with at least one file
            const processedNodeData = Array.from(nodeDataMap.values())
                .filter(node => node.blockchain || node.mempool || node.completeGames);

            if (processedNodeData.length === 0) {
                setError('No valid blockchain, mempool, or completeGames files found in the selected directory.');
                setLoading(false);
                return;
            }

            // Update the selected node if it still exists
            if (selectedNode) {
                const nodeStillExists = processedNodeData.some(node => node.nodeId === selectedNode.nodeId);
                if (!nodeStillExists) {
                    setSelectedNode(null);
                    setSelectedChain(null);
                    setSelectedBlock(null);
                } else {
                    const updatedNode = processedNodeData.find(node => node.nodeId === selectedNode.nodeId);
                    setSelectedNode(updatedNode);

                    // Update selected chain if it still exists
                    if (selectedChain) {
                        // For blockchain and mempool
                        if (selectedChain.type === 'blockchain' && updatedNode.blockchain) {
                            setSelectedChain({
                                type: 'blockchain',
                                data: updatedNode.blockchain
                            });
                        } else if (selectedChain.type === 'mempool' && updatedNode.mempool) {
                            setSelectedChain({
                                type: 'mempool',
                                data: updatedNode.mempool
                            });
                        }
                        // For complete games - need to find the matching game
                        else if (selectedChain.type === 'completeGame' && updatedNode.completeGames) {
                            const gameIndex = selectedChain.index;
                            if (gameIndex < updatedNode.completeGames.length) {
                                setSelectedChain({
                                    type: 'completeGame',
                                    index: gameIndex,
                                    data: updatedNode.completeGames[gameIndex]
                                });
                            } else {
                                setSelectedChain(null);
                            }
                        } else {
                            setSelectedChain(null);
                        }
                    }

                    // Clear selected block if chain was cleared
                    if (!selectedChain) {
                        setSelectedBlock(null);
                    }
                    // Update selected block if it still exists
                    else if (selectedBlock) {
                        if (selectedChain.type === 'blockchain' || selectedChain.type === 'completeGame') {
                            const blockStillExists = selectedChain.data.some(block =>
                                block.index === selectedBlock.index
                            );

                            if (!blockStillExists) {
                                setSelectedBlock(null);
                            } else {
                                const updatedBlock = selectedChain.data.find(block =>
                                    block.index === selectedBlock.index
                                );
                                setSelectedBlock(updatedBlock);
                            }
                        } else {
                            // For mempool, we don't have blocks
                            setSelectedBlock(null);
                        }
                    }
                }
            }

            setNodeData(processedNodeData);
            setLastRefresh(new Date());
            setLoading(false);
        } catch (err) {
            console.error('Error processing directory:', err);
            setError(`Error processing directory: ${err.message}`);
            setLoading(false);
        }
    };

    // Handler for selecting a node
    const handleNodeSelect = (node) => {
        setSelectedNode(node);
        setSelectedChain(null);
        setSelectedBlock(null);
    };

    // Handler for selecting a chain (blockchain, mempool, or complete game)
    const handleChainSelect = (type, data, index = null) => {
        setSelectedChain({
            type,
            data,
            index
        });
        setSelectedBlock(null);
    };

    // Handler for selecting a block
    const handleBlockClick = (block) => {
        setSelectedBlock(block);
    };

    // Close popups
    const closeChainDetail = () => {
        setSelectedChain(null);
        setSelectedBlock(null);
    };

    const closeBlockDetail = () => {
        setSelectedBlock(null);
    };

    // Function to truncate a string
    const truncateString = (str, maxLength = 20) => {
        if (!str) return '';
        if (str.length <= maxLength) return str;
        return str.substring(0, maxLength) + '...';
    };

    // Function to truncate a public key
    const truncatePublicKey = (publicKey) => {
        if (!publicKey) return '';

        // Extract first and last part of the key for display
        const firstPart = publicKey.substring(0, 15);
        const lastPart = publicKey.substring(publicKey.length - 15);

        return `${firstPart}...${lastPart}`;
    };

    // Function to format timestamp
    const formatTimestamp = (timestamp) => {
        if (!timestamp) return '';
        return new Date(timestamp * 1000).toLocaleString();
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
                    <span className="text-xl">Loading game data...</span>
                </div>
            </div>
        );
    }

    // Render file picker if no data loaded yet
    if (nodeData.length === 0) {
        return (
            <div className="flex flex-col items-center justify-center min-h-screen bg-gray-900 text-white p-6">
                <h1 className="text-3xl font-bold text-purple-400 mb-6">Chess Game Viewer</h1>

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
                    <span>Select Game Directory</span>
                </button>

                <p className="mt-4 text-gray-400 max-w-md text-center">
                    Select a directory containing game files in the format {"{nodeId}_blockchain.json"}, {"{nodeId}_mempool.json"}, and {"{nodeId}_completeGames.json"}.
                </p>
            </div>
        );
    }

    // Main view with game data
    return (
        <div className="min-h-screen bg-gray-900 text-white p-4 flex flex-col">
            <header className="mb-6">
                <div className="flex justify-between items-center mb-4">
                    <h1 className="text-2xl md:text-3xl font-bold text-purple-400">Chess Game Viewer</h1>

                    <div className="flex fixed items-center gap-2 top-6 right-65">
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

            <div className="grid grid-cols-1 md:grid-cols-2 lg:grid-cols-3 xl:grid-cols-4 gap-4 mb-6">
                {nodeData.map(node => (
                    <div
                        key={node.nodeId}
                        className={`bg-gray-800 rounded-lg overflow-hidden border-2 transition-colors cursor-pointer ${selectedNode && selectedNode.nodeId === node.nodeId
                            ? 'border-purple-500'
                            : 'border-gray-700 hover:border-gray-500'
                            }`}
                        onClick={() => handleNodeSelect(node)}
                    >
                        <div className="p-3 bg-gray-700 flex justify-between items-center">
                            <h2 className="font-bold text-base text-purple-300">Node {truncateString(node.nodeId, 12)}</h2>
                            <div className="flex items-center gap-2">
                                {node.blockchain && (
                                    <span className="bg-blue-900 text-blue-200 text-xs px-2 py-0.5 rounded-full flex items-center">
                                        <Activity size={10} className="mr-1" />
                                        {node.blockchain.length}
                                    </span>
                                )}
                                {node.completeGames && (
                                    <span className="bg-green-900 text-green-200 text-xs px-2 py-0.5 rounded-full">
                                        {node.completeGames.length}
                                    </span>
                                )}
                            </div>
                        </div>

                        <div className="p-3 text-sm">
                            <div className="flex flex-col gap-2">
                                {node.blockchain && (
                                    <div
                                        className="flex justify-between items-center p-2 bg-gray-700 rounded hover:bg-gray-600"
                                        onClick={(e) => {
                                            e.stopPropagation();
                                            handleChainSelect('blockchain', node.blockchain);
                                        }}
                                    >
                                        <span className="text-blue-300">Blockchain</span>
                                        <ArrowRight size={14} className="text-gray-400" />
                                    </div>
                                )}

                                {node.mempool && (
                                    <div
                                        className="flex justify-between items-center p-2 bg-gray-700 rounded hover:bg-gray-600"
                                        onClick={(e) => {
                                            e.stopPropagation();
                                            handleChainSelect('mempool', node.mempool);
                                        }}
                                    >
                                        <span className="text-yellow-300">Mempool</span>
                                        <ArrowRight size={14} className="text-gray-400" />
                                    </div>
                                )}

                                {node.completeGames && node.completeGames.map((game, index) => (
                                    <div
                                        key={index}
                                        className="flex justify-between items-center p-2 bg-gray-700 rounded hover:bg-gray-600"
                                        onClick={(e) => {
                                            e.stopPropagation();
                                            handleChainSelect('completeGame', game, index);
                                        }}
                                    >
                                        <span className="text-green-300">Complete Game {index + 1}</span>
                                        <ArrowRight size={14} className="text-gray-400" />
                                    </div>
                                ))}
                            </div>
                        </div>
                    </div>
                ))}
            </div>

            {/* Chain detail popup */}
            {selectedChain && (
                <div className="fixed inset-0 bg-black bg-opacity-75 flex items-center justify-center z-40 p-4">
                    <div className="bg-gray-800 rounded-lg shadow-2xl w-full max-w-4xl h-3/4 flex flex-col relative animate-fadeIn">
                        <button
                            onClick={closeChainDetail}
                            className="absolute top-4 right-4 text-gray-400 hover:text-white"
                        >
                            <X size={24} />
                        </button>

                        <div className="p-6 bg-gray-700 rounded-t-lg">
                            <div className="flex justify-between items-center">
                                <div>
                                    <div className="text-sm text-gray-400">Node {selectedNode.nodeId}</div>
                                    <h2 className="text-2xl font-bold text-purple-300">
                                        {selectedChain.type === 'blockchain' && 'Blockchain'}
                                        {selectedChain.type === 'mempool' && 'Mempool'}
                                        {selectedChain.type === 'completeGame' && `Complete Game ${selectedChain.index + 1}`}
                                    </h2>
                                </div>
                                <div className="text-sm text-gray-300">
                                    {selectedChain.type === 'blockchain' && `${selectedChain.data.length} blocks`}
                                    {selectedChain.type === 'mempool' && `${selectedChain.data.length} pending moves`}
                                    {selectedChain.type === 'completeGame' && `${selectedChain.data.length} blocks`}
                                </div>
                            </div>
                        </div>

                        <div className="p-6 flex-1 overflow-auto">
                            {/* Blockchain or Complete Game view */}
                            {(selectedChain.type === 'blockchain' || selectedChain.type === 'completeGame') && (
                                <div className="space-y-6">
                                    {selectedChain.data.map((block, blockIndex) => (
                                        <div
                                            key={blockIndex}
                                            className="relative"
                                        >
                                            {/* Line connecting blocks */}
                                            {blockIndex < selectedChain.data.length - 1 && (
                                                <div className="absolute w-1 bg-purple-500 left-1/2 transform -translate-x-1/2 h-10 top-full z-0"></div>
                                            )}

                                            <div
                                                className="bg-gradient-to-r from-gray-700 to-gray-800 rounded-lg p-4 cursor-pointer hover:from-gray-600 hover:to-gray-700 transform transition-all duration-200 hover:scale-102 hover:shadow-lg relative z-10"
                                                onClick={() => handleBlockClick(block)}
                                            >
                                                <div className="flex justify-between items-center mb-2">
                                                    <div className="font-bold text-purple-300">Block #{block.index}</div>
                                                    {block.moves && (
                                                        <div className="text-xs text-gray-400">{block.moves.length} moves</div>
                                                    )}
                                                </div>

                                                <div className="grid grid-cols-1 sm:grid-cols-2 gap-2 text-xs">
                                                    <div>
                                                        <div className="text-gray-400">Hash:</div>
                                                        <div className="font-mono text-green-400 truncate">{truncateString(block.hash, 30)}</div>
                                                    </div>
                                                    <div>
                                                        <div className="text-gray-400">Previous Hash:</div>
                                                        <div className="font-mono text-blue-400 truncate">{truncateString(block.previousHash, 30)}</div>
                                                    </div>
                                                    <div>
                                                        <div className="text-gray-400">Timestamp:</div>
                                                        <div>{formatTimestamp(block.timestamp)}</div>
                                                    </div>
                                                    <div>
                                                        <div className="text-gray-400">Nonce:</div>
                                                        <div>{block.nonce && block.nonce.toLocaleString()}</div>
                                                    </div>
                                                </div>
                                            </div>
                                        </div>
                                    ))}
                                </div>
                            )}

                            {/* Mempool view */}
                            {selectedChain.type === 'mempool' && (
                                <div className="grid grid-cols-1 md:grid-cols-2 gap-4">
                                    {selectedChain.data.map((mempoolItem, index) => (
                                        <div
                                            key={index}
                                            className="bg-gray-700 rounded-lg p-4 hover:bg-gray-600 transition-colors"
                                        >
                                            <div className="flex justify-between items-center mb-3">
                                                <div className="text-yellow-300 font-medium">Move #{index + 1}</div>
                                                <div className="bg-yellow-800 text-yellow-200 px-2 py-1 rounded text-xs font-mono">
                                                    {mempoolItem.data || mempoolItem.amount}
                                                </div>
                                            </div>

                                            <div className="space-y-2 text-xs">
                                                <div>
                                                    <div className="text-gray-400 mb-1">ID:</div>
                                                    <div className="font-mono">{mempoolItem.id}</div>
                                                </div>
                                                <div>
                                                    <div className="text-gray-400 mb-1">Sender:</div>
                                                    <div className="font-mono text-blue-300 truncate">{truncatePublicKey(mempoolItem.sender)}</div>
                                                </div>
                                                <div>
                                                    <div className="text-gray-400 mb-1">Receiver:</div>
                                                    <div className="font-mono text-green-300 truncate">{truncatePublicKey(mempoolItem.receiver)}</div>
                                                </div>
                                            </div>
                                        </div>
                                    ))}

                                    {selectedChain.data.length === 0 && (
                                        <div className="col-span-2 text-center text-gray-400 py-8">
                                            No pending moves in the mempool
                                        </div>
                                    )}
                                </div>
                            )}
                        </div>
                    </div>
                </div>
            )}

            {/* Block detail popup */}
            {selectedBlock && (
                <div className="fixed inset-0 bg-black bg-opacity-80 flex items-center justify-center z-50 p-4">
                    <div className="bg-gray-800 rounded-lg shadow-2xl w-full max-w-2xl max-h-3/4 flex flex-col relative animate-fadeIn">
                        <button
                            onClick={closeBlockDetail}
                            className="absolute top-4 right-4 text-gray-400 hover:text-white"
                        >
                            <X size={24} />
                        </button>

                        <div className="p-4 bg-gray-700 rounded-t-lg">
                            <div className="flex justify-between items-center">
                                <div>
                                    <div className="text-sm text-gray-400">
                                        {selectedChain.type === 'blockchain' ? 'Blockchain' : 'Complete Game'} Block
                                    </div>
                                    <h2 className="text-xl font-bold text-purple-300">Block #{selectedBlock.index}</h2>
                                </div>
                            </div>
                        </div>

                        <div className="p-4 flex-1 overflow-auto">
                            <div className="grid grid-cols-1 sm:grid-cols-2 gap-3 mb-4 text-sm">
                                <div>
                                    <div className="text-gray-400">Hash:</div>
                                    <div className="font-mono text-green-400 break-all text-xs">{selectedBlock.hash}</div>
                                </div>
                                <div>
                                    <div className="text-gray-400">Previous Hash:</div>
                                    <div className="font-mono text-blue-400 break-all text-xs">{selectedBlock.previousHash}</div>
                                </div>
                                <div>
                                    <div className="text-gray-400">Timestamp:</div>
                                    <div>{formatTimestamp(selectedBlock.timestamp)}</div>
                                </div>
                                <div>
                                    <div className="text-gray-400">Nonce:</div>
                                    <div>{selectedBlock.nonce && selectedBlock.nonce.toLocaleString()}</div>
                                </div>
                                {selectedBlock.gameId && (
                                    <div>
                                        <div className="text-gray-400">Game ID:</div>
                                        <div>{selectedBlock.gameId}</div>
                                    </div>
                                )}
                            </div>

                            {/* Moves section */}
                            {selectedBlock.moves && selectedBlock.moves.length > 0 && (
                                <div>
                                    <h3 className="text-lg font-bold text-purple-300 mb-3">Moves</h3>
                                    <div className="grid grid-cols-1 sm:grid-cols-2 gap-3">
                                        {selectedBlock.moves.map((move, moveIndex) => (
                                            <div
                                                key={moveIndex}
                                                className="bg-gray-700 rounded-lg p-3 hover:bg-gray-600 transition-colors"
                                            >
                                                <div className="flex justify-between items-center mb-2">
                                                    <span className="text-purple-300 font-medium text-sm">Move {moveIndex + 1}</span>
                                                    <span className="bg-yellow-900 text-yellow-300 font-mono px-2 py-1 rounded text-xs">
                                                        {move.data || move.amount}
                                                    </span>
                                                </div>
                                                <div className="space-y-1 text-xs">
                                                    <div className="flex">
                                                        <span className="text-gray-400 w-16">ID:</span>
                                                        <span className="font-mono truncate">{move.id}</span>
                                                    </div>
                                                    <div className="flex">
                                                        <span className="text-gray-400 w-16">Sender:</span>
                                                        <span className="text-blue-300 font-mono truncate">{truncatePublicKey(move.sender)}</span>
                                                    </div>
                                                    <div className="flex">
                                                        <span className="text-gray-400 w-16">Receiver:</span>
                                                        <span className="text-green-300 font-mono truncate">{truncatePublicKey(move.receiver)}</span>
                                                    </div>
                                                </div>
                                            </div>
                                        ))}
                                    </div>
                                </div>
                            )}

                            {(!selectedBlock.moves || selectedBlock.moves.length === 0) && (
                                <div className="text-center text-gray-400 py-6">
                                    No moves in this block
                                </div>
                            )}
                        </div>
                    </div>
                </div>
            )}

            <footer className="mt-auto text-center text-gray-500 text-sm py-2">
                <p>Chess Game Viewer - {nodeData.length} nodes loaded</p>
            </footer>
        </div>
    );
};

export default GameViewer;