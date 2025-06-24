import React, { useState, useRef } from 'react';
import BlockchainViewer from './BlockchianViewer.jsx';

import GameViewer from './GameViewer.jsx';
import { FolderOpen } from 'lucide-react';

function App() {
    const [directoryHandle, setDirectoryHandle] = useState(null);
    const [activeView, setActiveView] = useState('blockchain'); // 'blockchain' or 'game'
    const [loading, setLoading] = useState(false);
    const [error, setError] = useState(null);

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

            // Show directory picker
            const dirHandle = await window.showDirectoryPicker();
            setDirectoryHandle(dirHandle);
            setLoading(false);
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

    // Toggle between blockchain and game view
    const toggleView = () => {
        setActiveView(activeView === 'blockchain' ? 'game' : 'blockchain');
    };

    // Render directory picker if no directory is selected
    if (!directoryHandle) {
        return (
            <div className="flex flex-col items-center justify-center min-h-screen bg-gray-900 text-white p-6">
                <h1 className="text-3xl font-bold text-purple-400 mb-6">Blockchain & Game Viewer</h1>

                {error && (
                    <div className="mb-6 text-red-500 p-4 bg-red-900 bg-opacity-30 rounded-lg max-w-md text-center">
                        {error}
                    </div>
                )}

                <button
                    onClick={openDirectoryPicker}
                    className="px-6 py-3 bg-purple-600 rounded-lg flex items-center gap-2 hover:bg-purple-700 transition-colors"
                    disabled={!isFileSystemAccessSupported() || loading}
                >
                    {loading ? (
                        <div className="animate-spin h-5 w-5 border-2 border-white rounded-full border-t-transparent"></div>
                    ) : (
                        <FolderOpen size={20} />
                    )}
                    <span>Select Directory</span>
                </button>

                <p className="mt-4 text-gray-400 max-w-md text-center">
                    Select a directory containing blockchain and game files to view and analyze.
                </p>
            </div>
        );
    }

    // Render the active view with toggle button
    return (
        <div className="min-h-screen bg-gray-900 text-white">
            <div className="fixed top-4 right-4 z-50">
                <button
                    onClick={toggleView}
                    className="px-4 py-2 bg-purple-600 rounded-lg font-medium hover:bg-purple-700 transition-colors"
                >
                    Switch to {activeView === 'blockchain' ? 'Game' : 'Blockchain'} View
                </button>
            </div>

            {activeView === 'blockchain' ? (
                <BlockchainViewer directoryHandler={directoryHandle} />
            ) : (
                <GameViewer directoryHandler={directoryHandle} />
            )}
        </div>
    );
}

export default App;