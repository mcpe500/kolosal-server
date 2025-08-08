# Kolosal Server - Downloads API Documentation

## Overview

The Downloads API provides comprehensive model download management capabilities for the Kolosal server. It supports downloading, monitoring, pausing, resuming, and canceling model downloads with real-time progress tracking. The API handles both regular downloads and startup downloads (with automatic engine creation).

**Base Endpoints:** `/downloads` or `/v1/downloads`  
**Supported Operations:** GET, POST, DELETE

`/v1` is a stable alias; both `/downloads` and `/v1/downloads` resolve identically today. For production integrations, prefer the versioned path to guard against future non-breaking additions on the un-versioned alias.

### Quick Method Matrix

| Action | Method + Path | Alternate | Body | Idempotent | Notes |
|--------|---------------|----------|------|------------|-------|
| List active downloads | `GET /v1/downloads` | `GET /downloads` | None | Yes | Includes summary counts |
| Single download status | `GET /v1/downloads/{id}` | `GET /downloads/{id}` | None | Yes | Detailed progress fields |
| Cancel single | `DELETE /v1/downloads/{id}` | `POST /v1/downloads/{id}/cancel` | None | No | Fails if already terminal |
| Pause | `POST /v1/downloads/{id}/pause` | — | None | No | Only if status=downloading |
| Resume | `POST /v1/downloads/{id}/resume` | — | None | No | Only if status=paused |
| Cancel all | `DELETE /v1/downloads` | `POST /v1/downloads/cancel` | Optional | No | Skips already terminal |

Terminal statuses: `completed`, `failed`, `cancelled`. Transitional: `downloading`, `paused`, `creating_engine`.

## API Endpoints

### 1. Get All Downloads Status

**Endpoint:** `GET /downloads` or `GET /v1/downloads`  
**Description:** Retrieve status and progress information for all active downloads

#### Response Format (200 OK)

```json
{
  "active_downloads": [
    {
      "model_id": "string",
      "status": "string",
      "download_type": "string",
      "url": "string",
      "local_path": "string",
      "progress": {
        "downloaded_bytes": "integer",
        "total_bytes": "integer",
        "percentage": "number",
        "download_speed_bps": "number"
      },
      "timing": {
        "start_time": "integer",
        "elapsed_seconds": "integer",
        "estimated_remaining_seconds": "integer",
        "end_time": "integer"
      },
      "error_message": "string",
      "engine_creation": {
        "model_id": "string",
        "load_immediately": "boolean",
        "main_gpu_id": "integer"
      }
    }
  ],
  "summary": {
    "total_active": "integer",
    "startup_downloads": "integer",
    "regular_downloads": "integer",
    "embedding_model_downloads": "integer",
    "llm_model_downloads": "integer"
  },
  "timestamp": "integer"
}
```

### 2. Get Specific Download Status

**Endpoint:** `GET /downloads/{model_id}` or `GET /v1/downloads/{model_id}`  
**Description:** Get detailed progress information for a specific model download

#### Single Download Response (200 OK)

```json
{
  "model_id": "string",
  "status": "string",
  "url": "string",
  "local_path": "string",
  "progress": {
    "downloaded_bytes": "integer",
    "total_bytes": "integer",
    "percentage": "number",
    "download_speed_bps": "number"
  },
  "timing": {
    "start_time": "integer",
    "elapsed_seconds": "integer",
    "estimated_remaining_seconds": "integer",
    "end_time": "integer"
  },
  "error_message": "string",
  "engine_creation": {
    "model_id": "string",
    "model_type": "string",
    "load_immediately": "boolean",
    "main_gpu_id": "integer",
    "inference_engine": "string",
    "embedding_features": {
      "normalization": "boolean",
      "supports_batching": "boolean",
      "optimized_for_retrieval": "boolean"
    },
    "recommended_usage": {
      "document_embedding": "boolean",
      "semantic_search": "boolean",
      "similarity_computation": "boolean"
    },
    "llm_features": {
      "text_generation": "boolean",
      "chat_completion": "boolean",
      "instruction_following": "boolean"
    }
  }
}
```

### 3. Cancel Specific Download

**Endpoint:** `DELETE /downloads/{model_id}` or `POST /downloads/{model_id}/cancel`  
**Description:** Cancel a specific model download that is in progress

#### Cancel Response (200 OK)

```json
{
  "success": true,
  "message": "Download cancelled successfully",
  "model_id": "string",
  "previous_status": "string",
  "download_type": "string",
  "timestamp": "integer",
  "engine_creation": {
    "model_id": "string",
    "load_immediately": "boolean",
    "main_gpu_id": "integer"
  }
}
```

### 4. Pause Download

**Endpoint:** `POST /downloads/{model_id}/pause`  
**Description:** Pause an active model download

#### Pause Response (200 OK)

```json
{
  "success": true,
  "message": "Download paused successfully",
  "model_id": "string"
}
```

### 5. Resume Download

**Endpoint:** `POST /downloads/{model_id}/resume`  
**Description:** Resume a paused model download

#### Resume Response (200 OK)

```json
{
  "success": true,
  "message": "Download resumed successfully",
  "model_id": "string"
}
```

### 6. Cancel All Downloads

**Endpoint:** `DELETE /downloads` or `POST /downloads/cancel`  
**Description:** Cancel all active downloads

#### Bulk Cancel Response (200 OK)

```json
{
  "success": true,
  "message": "Bulk download cancellation completed",
  "summary": {
    "total_downloads": "integer",
    "successful_cancellations": "integer",
    "failed_cancellations": "integer",
    "startup_cancellations": "integer",
    "regular_cancellations": "integer"
  },
  "cancelled_downloads": [
    {
      "model_id": "string",
      "previous_status": "string",
      "download_type": "string",
      "cancelled_at": "integer",
      "engine_creation": {
        "model_id": "string",
        "load_immediately": "boolean",
        "main_gpu_id": "integer"
      }
    }
  ],
  "failed_cancellations": [
    {
      "model_id": "string",
      "status": "string",
      "reason": "string"
    }
  ],
  "timestamp": "integer"
}
```

## Field Descriptions

### Download Status Values

| Status | Description |
|--------|-------------|
| `downloading` | Download is actively in progress |
| `paused` | Download has been paused by user |
| `creating_engine` | Model downloaded, engine creation in progress |
| `completed` | Download and engine creation completed successfully |
| `cancelled` | Download was cancelled by user |
| `failed` | Download failed due to error |

### Download Types

| Type | Description |
|------|-------------|
| `regular` | Standard model download without engine creation |
| `startup` | Download with automatic engine creation after completion |

### Model Types

| Type | Description |
|------|-------------|
| `embedding` | Embedding model for text vectorization |
| `llm` | Large Language Model for text generation |

## JavaScript Examples

### Basic Usage with Fetch API

```javascript
class DownloadManager {
  constructor(baseURL = 'http://localhost:8080', apiKey = null) {
    this.baseURL = baseURL;
    this.headers = {
      'Content-Type': 'application/json',
      ...(apiKey && { 'X-API-Key': apiKey })
    };
  }

  async getAllDownloads() {
    try {
      const response = await fetch(`${this.baseURL}/downloads`, {
        method: 'GET',
        headers: this.headers
      });

      if (!response.ok) {
        const errorData = await response.json();
        throw new Error(`API Error (${response.status}): ${errorData.error?.message || 'Unknown error'}`);
      }

      return await response.json();
    } catch (error) {
      console.error('Failed to get downloads:', error);
      throw error;
    }
  }

  async getDownloadStatus(modelId) {
    try {
      const response = await fetch(`${this.baseURL}/downloads/${encodeURIComponent(modelId)}`, {
        method: 'GET',
        headers: this.headers
      });

      if (!response.ok) {
        const errorData = await response.json();
        throw new Error(`API Error (${response.status}): ${errorData.error?.message || 'Download not found'}`);
      }

      return await response.json();
    } catch (error) {
      console.error(`Failed to get download status for ${modelId}:`, error);
      throw error;
    }
  }

  async cancelDownload(modelId) {
    try {
      const response = await fetch(`${this.baseURL}/downloads/${encodeURIComponent(modelId)}/cancel`, {
        method: 'POST',
        headers: this.headers
      });

      if (!response.ok) {
        const errorData = await response.json();
        throw new Error(`API Error (${response.status}): ${errorData.error?.message || 'Failed to cancel'}`);
      }

      return await response.json();
    } catch (error) {
      console.error(`Failed to cancel download for ${modelId}:`, error);
      throw error;
    }
  }

  async pauseDownload(modelId) {
    try {
      const response = await fetch(`${this.baseURL}/downloads/${encodeURIComponent(modelId)}/pause`, {
        method: 'POST',
        headers: this.headers
      });

      if (!response.ok) {
        const errorData = await response.json();
        throw new Error(`API Error (${response.status}): ${errorData.error?.message || 'Failed to pause'}`);
      }

      return await response.json();
    } catch (error) {
      console.error(`Failed to pause download for ${modelId}:`, error);
      throw error;
    }
  }

  async resumeDownload(modelId) {
    try {
      const response = await fetch(`${this.baseURL}/downloads/${encodeURIComponent(modelId)}/resume`, {
        method: 'POST',
        headers: this.headers
      });

      if (!response.ok) {
        const errorData = await response.json();
        throw new Error(`API Error (${response.status}): ${errorData.error?.message || 'Failed to resume'}`);
      }

      return await response.json();
    } catch (error) {
      console.error(`Failed to resume download for ${modelId}:`, error);
      throw error;
    }
  }

  async cancelAllDownloads() {
    try {
      const response = await fetch(`${this.baseURL}/downloads/cancel`, {
        method: 'POST',
        headers: this.headers
      });

      if (!response.ok) {
        const errorData = await response.json();
        throw new Error(`API Error (${response.status}): ${errorData.error?.message || 'Failed to cancel all'}`);
      }

      return await response.json();
    } catch (error) {
      console.error('Failed to cancel all downloads:', error);
      throw error;
    }
  }

  // Utility methods for monitoring
  formatBytes(bytes) {
    if (bytes === 0) return '0 Bytes';
    const k = 1024;
    const sizes = ['Bytes', 'KB', 'MB', 'GB', 'TB'];
    const i = Math.floor(Math.log(bytes) / Math.log(k));
    return parseFloat((bytes / Math.pow(k, i)).toFixed(2)) + ' ' + sizes[i];
  }

  formatSpeed(bytesPerSecond) {
    return this.formatBytes(bytesPerSecond) + '/s';
  }

  formatTime(seconds) {
    if (seconds < 0) return 'Unknown';
    
    const hours = Math.floor(seconds / 3600);
    const minutes = Math.floor((seconds % 3600) / 60);
    const secs = seconds % 60;

    if (hours > 0) {
      return `${hours}h ${minutes}m ${secs}s`;
    } else if (minutes > 0) {
      return `${minutes}m ${secs}s`;
    } else {
      return `${secs}s`;
    }
  }
}

// Example usage
const downloadManager = new DownloadManager('http://localhost:8080');

async function monitorDownloads() {
  try {
    console.log('📥 Getting all download statuses...\n');
    
    const allDownloads = await downloadManager.getAllDownloads();
    
    console.log(`📊 Download Summary:`);
    console.log(`Total active: ${allDownloads.summary.total_active}`);
    console.log(`Startup downloads: ${allDownloads.summary.startup_downloads}`);
    console.log(`Regular downloads: ${allDownloads.summary.regular_downloads}`);
    console.log(`Embedding models: ${allDownloads.summary.embedding_model_downloads}`);
    console.log(`LLM models: ${allDownloads.summary.llm_model_downloads}\n`);

    if (allDownloads.active_downloads.length === 0) {
      console.log('✅ No active downloads');
      return;
    }

    console.log('📋 Active Downloads:');
    console.log('==================');

    for (const download of allDownloads.active_downloads) {
      console.log(`\n🔄 ${download.model_id}`);
      console.log(`   Status: ${download.status}`);
      console.log(`   Type: ${download.download_type}`);
      console.log(`   Progress: ${download.progress.percentage.toFixed(1)}%`);
      console.log(`   Downloaded: ${downloadManager.formatBytes(download.progress.downloaded_bytes)} / ${downloadManager.formatBytes(download.progress.total_bytes)}`);
      console.log(`   Speed: ${downloadManager.formatSpeed(download.progress.download_speed_bps)}`);
      console.log(`   Elapsed: ${downloadManager.formatTime(download.timing.elapsed_seconds)}`);
      
      if (download.timing.estimated_remaining_seconds !== undefined) {
        console.log(`   Remaining: ${downloadManager.formatTime(download.timing.estimated_remaining_seconds)}`);
      }
      
      if (download.error_message) {
        console.log(`   ❌ Error: ${download.error_message}`);
      }
      
      if (download.engine_creation) {
        console.log(`   🔧 Engine: ${download.engine_creation.model_id} (GPU ${download.engine_creation.main_gpu_id})`);
      }
    }

  } catch (error) {
    console.error('❌ Failed to monitor downloads:', error.message);
  }
}

async function demonstrateDownloadControl() {
  const modelId = 'text-embedding-3-small';
  
  try {
    console.log(`\n🔍 Getting status for ${modelId}...`);
    const status = await downloadManager.getDownloadStatus(modelId);
    
    console.log(`Status: ${status.status}`);
    console.log(`Progress: ${status.progress.percentage.toFixed(1)}%`);
    
    if (status.status === 'downloading') {
      console.log(`\n⏸️ Pausing download...`);
      const pauseResult = await downloadManager.pauseDownload(modelId);
      console.log(`✅ ${pauseResult.message}`);
      
      // Wait a moment
      await new Promise(resolve => setTimeout(resolve, 2000));
      
      console.log(`\n▶️ Resuming download...`);
      const resumeResult = await downloadManager.resumeDownload(modelId);
      console.log(`✅ ${resumeResult.message}`);
    }
    
  } catch (error) {
    if (error.message.includes('not found')) {
      console.log(`ℹ️ No active download found for ${modelId}`);
    } else {
      console.error('❌ Error:', error.message);
    }
  }
}

// Monitor downloads every 5 seconds
async function startMonitoring() {
  console.log('🚀 Starting download monitoring...\n');
  
  // Initial check
  await monitorDownloads();
  
  // Demonstrate download control if there are active downloads
  await demonstrateDownloadControl();
  
  // Continue monitoring
  const interval = setInterval(async () => {
    console.log('\n' + '='.repeat(50));
    console.log('🔄 Refreshing download status...\n');
    await monitorDownloads();
  }, 5000);
  
  // Stop monitoring after 30 seconds
  setTimeout(() => {
    clearInterval(interval);
    console.log('\n🛑 Monitoring stopped');
  }, 30000);
}

startMonitoring();
```

### Advanced Download Management with Real-time Updates

```javascript
const axios = require('axios');
const EventEmitter = require('events');

class AdvancedDownloadManager extends EventEmitter {
  constructor(baseURL = 'http://localhost:8080', apiKey = null) {
    super();
    this.baseURL = baseURL;
    this.client = axios.create({
      baseURL: baseURL,
      headers: {
        'Content-Type': 'application/json',
        ...(apiKey && { 'X-API-Key': apiKey })
      },
      timeout: 30000
    });
    
    this.monitoringActive = false;
    this.monitoringInterval = null;
    this.downloadCache = new Map();
  }

  async makeRequest(method, endpoint, data = null) {
    try {
      const response = await this.client.request({
        method,
        url: endpoint,
        data
      });
      return response.data;
    } catch (error) {
      if (error.response) {
        const errorData = error.response.data;
        throw new Error(`API Error (${error.response.status}): ${errorData.error?.message || 'Unknown error'}`);
      } else if (error.request) {
        throw new Error('No response from server - check if server is running');
      } else {
        throw new Error(`Request error: ${error.message}`);
      }
    }
  }

  async getAllDownloads() {
    return this.makeRequest('GET', '/downloads');
  }

  async getDownloadStatus(modelId) {
    return this.makeRequest('GET', `/downloads/${encodeURIComponent(modelId)}`);
  }

  async cancelDownload(modelId) {
    const result = await this.makeRequest('POST', `/downloads/${encodeURIComponent(modelId)}/cancel`);
    this.emit('downloadCancelled', { modelId, ...result });
    return result;
  }

  async pauseDownload(modelId) {
    const result = await this.makeRequest('POST', `/downloads/${encodeURIComponent(modelId)}/pause`);
    this.emit('downloadPaused', { modelId, ...result });
    return result;
  }

  async resumeDownload(modelId) {
    const result = await this.makeRequest('POST', `/downloads/${encodeURIComponent(modelId)}/resume`);
    this.emit('downloadResumed', { modelId, ...result });
    return result;
  }

  async cancelAllDownloads() {
    const result = await this.makeRequest('POST', '/downloads/cancel');
    this.emit('allDownloadsCancelled', result);
    return result;
  }

  startMonitoring(intervalMs = 2000) {
    if (this.monitoringActive) {
      console.log('⚠️ Monitoring already active');
      return;
    }

    this.monitoringActive = true;
    console.log(`🔄 Starting download monitoring (${intervalMs}ms interval)`);

    const monitor = async () => {
      if (!this.monitoringActive) return;

      try {
        const downloads = await this.getAllDownloads();
        this.processDownloadUpdates(downloads);
      } catch (error) {
        this.emit('monitoringError', error);
      }
    };

    // Initial check
    monitor();

    // Set up interval
    this.monitoringInterval = setInterval(monitor, intervalMs);
    this.emit('monitoringStarted', { intervalMs });
  }

  stopMonitoring() {
    if (!this.monitoringActive) {
      console.log('⚠️ Monitoring not active');
      return;
    }

    this.monitoringActive = false;
    if (this.monitoringInterval) {
      clearInterval(this.monitoringInterval);
      this.monitoringInterval = null;
    }

    console.log('🛑 Download monitoring stopped');
    this.emit('monitoringStopped');
  }

  processDownloadUpdates(downloads) {
    const currentDownloads = new Map();

    // Process current downloads
    for (const download of downloads.active_downloads) {
      const modelId = download.model_id;
      const cached = this.downloadCache.get(modelId);
      
      currentDownloads.set(modelId, download);

      if (!cached) {
        // New download detected
        this.emit('downloadStarted', download);
      } else {
        // Check for status changes
        if (cached.status !== download.status) {
          this.emit('downloadStatusChanged', {
            modelId,
            oldStatus: cached.status,
            newStatus: download.status,
            download
          });
        }

        // Check for progress updates (only emit if significant change)
        const progressDiff = Math.abs(download.progress.percentage - cached.progress.percentage);
        if (progressDiff >= 1.0) {
          this.emit('downloadProgress', {
            modelId,
            progress: download.progress,
            timing: download.timing
          });
        }

        // Check for completion
        if (cached.status !== 'completed' && download.status === 'completed') {
          this.emit('downloadCompleted', download);
        }

        // Check for errors
        if (!cached.error_message && download.error_message) {
          this.emit('downloadError', {
            modelId,
            error: download.error_message,
            download
          });
        }
      }
    }

    // Check for removed downloads
    for (const [modelId, cached] of this.downloadCache) {
      if (!currentDownloads.has(modelId)) {
        this.emit('downloadRemoved', { modelId, lastStatus: cached.status });
      }
    }

    // Update cache
    this.downloadCache = currentDownloads;

    // Emit summary update
    this.emit('downloadSummaryUpdate', downloads.summary);
  }

  async waitForDownloadCompletion(modelId, timeoutMs = 300000) {
    return new Promise((resolve, reject) => {
      const timeout = setTimeout(() => {
        this.removeAllListeners(`completion_${modelId}`);
        this.removeAllListeners(`error_${modelId}`);
        reject(new Error(`Download timeout after ${timeoutMs}ms`));
      }, timeoutMs);

      const completionHandler = (download) => {
        if (download.model_id === modelId || download.modelId === modelId) {
          clearTimeout(timeout);
          this.removeAllListeners(`completion_${modelId}`);
          this.removeAllListeners(`error_${modelId}`);
          resolve(download);
        }
      };

      const errorHandler = (error) => {
        if (error.modelId === modelId) {
          clearTimeout(timeout);
          this.removeAllListeners(`completion_${modelId}`);
          this.removeAllListeners(`error_${modelId}`);
          reject(new Error(`Download failed: ${error.error}`));
        }
      };

      this.on('downloadCompleted', completionHandler);
      this.on('downloadError', errorHandler);

      // Store handlers for cleanup
      this.once(`completion_${modelId}`, completionHandler);
      this.once(`error_${modelId}`, errorHandler);
    });
  }

  getDownloadStatistics() {
    const stats = {
      totalDownloads: this.downloadCache.size,
      byStatus: {},
      byType: {},
      totalBytes: 0,
      downloadedBytes: 0,
      averageSpeed: 0,
      estimatedTimeRemaining: 0
    };

    let activeDownloads = 0;
    let totalSpeed = 0;

    for (const [modelId, download] of this.downloadCache) {
      // Count by status
      stats.byStatus[download.status] = (stats.byStatus[download.status] || 0) + 1;
      
      // Count by type
      stats.byType[download.download_type] = (stats.byType[download.download_type] || 0) + 1;
      
      // Accumulate bytes
      stats.totalBytes += download.progress.total_bytes;
      stats.downloadedBytes += download.progress.downloaded_bytes;
      
      // Calculate average speed for active downloads
      if (download.status === 'downloading') {
        activeDownloads++;
        totalSpeed += download.progress.download_speed_bps;
      }
    }

    if (activeDownloads > 0) {
      stats.averageSpeed = totalSpeed / activeDownloads;
      const remainingBytes = stats.totalBytes - stats.downloadedBytes;
      if (stats.averageSpeed > 0) {
        stats.estimatedTimeRemaining = Math.ceil(remainingBytes / stats.averageSpeed);
      }
    }

    stats.overallProgress = stats.totalBytes > 0 ? 
      (stats.downloadedBytes / stats.totalBytes) * 100 : 0;

    return stats;
  }

  formatBytes(bytes) {
    if (bytes === 0) return '0 Bytes';
    const k = 1024;
    const sizes = ['Bytes', 'KB', 'MB', 'GB', 'TB'];
    const i = Math.floor(Math.log(bytes) / Math.log(k));
    return parseFloat((bytes / Math.pow(k, i)).toFixed(2)) + ' ' + sizes[i];
  }

  formatSpeed(bytesPerSecond) {
    return this.formatBytes(bytesPerSecond) + '/s';
  }

  formatDuration(seconds) {
    if (seconds < 0 || !isFinite(seconds)) return 'Unknown';
    
    const hours = Math.floor(seconds / 3600);
    const minutes = Math.floor((seconds % 3600) / 60);
    const secs = Math.floor(seconds % 60);

    if (hours > 0) {
      return `${hours}h ${minutes}m ${secs}s`;
    } else if (minutes > 0) {
      return `${minutes}m ${secs}s`;
    } else {
      return `${secs}s`;
    }
  }

  printDownloadSummary() {
    const stats = this.getDownloadStatistics();
    
    console.log('\n📊 Download Statistics:');
    console.log('======================');
    console.log(`Total Downloads: ${stats.totalDownloads}`);
    console.log(`Overall Progress: ${stats.overallProgress.toFixed(1)}%`);
    console.log(`Total Size: ${this.formatBytes(stats.totalBytes)}`);
    console.log(`Downloaded: ${this.formatBytes(stats.downloadedBytes)}`);
    
    if (stats.averageSpeed > 0) {
      console.log(`Average Speed: ${this.formatSpeed(stats.averageSpeed)}`);
      console.log(`Est. Time Remaining: ${this.formatDuration(stats.estimatedTimeRemaining)}`);
    }
    
    console.log('\nStatus Breakdown:');
    for (const [status, count] of Object.entries(stats.byStatus)) {
      console.log(`  ${status}: ${count}`);
    }
    
    console.log('\nType Breakdown:');
    for (const [type, count] of Object.entries(stats.byType)) {
      console.log(`  ${type}: ${count}`);
    }
  }
}

// Example usage with event handling
async function demonstrateAdvancedMonitoring() {
  const manager = new AdvancedDownloadManager('http://localhost:8080');

  // Set up event listeners
  manager.on('downloadStarted', (download) => {
    console.log(`🚀 Download started: ${download.model_id} (${download.download_type})`);
  });

  manager.on('downloadProgress', (data) => {
    console.log(`📈 Progress update: ${data.modelId} - ${data.progress.percentage.toFixed(1)}%`);
  });

  manager.on('downloadCompleted', (download) => {
    console.log(`✅ Download completed: ${download.model_id}`);
    if (download.engine_creation) {
      console.log(`   🔧 Engine created for ${download.engine_creation.model_type} model`);
    }
  });

  manager.on('downloadError', (data) => {
    console.error(`❌ Download error: ${data.modelId} - ${data.error}`);
  });

  manager.on('downloadStatusChanged', (data) => {
    console.log(`🔄 Status change: ${data.modelId} - ${data.oldStatus} → ${data.newStatus}`);
  });

  manager.on('downloadCancelled', (data) => {
    console.log(`🛑 Download cancelled: ${data.modelId}`);
  });

  manager.on('downloadPaused', (data) => {
    console.log(`⏸️ Download paused: ${data.modelId}`);
  });

  manager.on('downloadResumed', (data) => {
    console.log(`▶️ Download resumed: ${data.modelId}`);
  });

  manager.on('monitoringError', (error) => {
    console.error('⚠️ Monitoring error:', error.message);
  });

  // Start monitoring
  manager.startMonitoring(1000); // Check every second

  // Print statistics every 10 seconds
  const statsInterval = setInterval(() => {
    manager.printDownloadSummary();
  }, 10000);

  // Example: Wait for a specific download to complete
  try {
    console.log('⏳ Waiting for text-embedding-3-small to complete...');
    const completedDownload = await manager.waitForDownloadCompletion('text-embedding-3-small', 60000);
    console.log(`🎉 Download completed successfully: ${completedDownload.model_id}`);
  } catch (error) {
    console.log(`ℹ️ ${error.message}`);
  }

  // Stop monitoring after 2 minutes
  setTimeout(() => {
    clearInterval(statsInterval);
    manager.stopMonitoring();
    console.log('Demo completed');
  }, 120000);
}

// Run the demonstration
demonstrateAdvancedMonitoring();
```

### Browser-based Download Monitor

```html
<!DOCTYPE html>
<html>
<head>
    <title>Kolosal Downloads Monitor</title>
    <style>
        .container { max-width: 1200px; margin: 20px auto; padding: 20px; font-family: Arial, sans-serif; }
        .header { text-align: center; margin-bottom: 30px; }
        .stats-grid { display: grid; grid-template-columns: repeat(auto-fit, minmax(200px, 1fr)); gap: 15px; margin: 20px 0; }
        .stat-card { background: #f8f9fa; padding: 20px; border-radius: 8px; text-align: center; border-left: 4px solid #007bff; }
        .stat-value { font-size: 24px; font-weight: bold; color: #007bff; }
        .stat-label { color: #6c757d; margin-top: 5px; }
        .download-list { margin: 20px 0; }
        .download-item { background: white; border: 1px solid #ddd; border-radius: 8px; padding: 20px; margin: 10px 0; box-shadow: 0 2px 4px rgba(0,0,0,0.1); }
        .download-header { display: flex; justify-content: between; align-items: center; margin-bottom: 15px; }
        .model-id { font-size: 18px; font-weight: bold; color: #333; }
        .status { padding: 4px 12px; border-radius: 20px; font-size: 12px; font-weight: bold; text-transform: uppercase; }
        .status-downloading { background: #cce5ff; color: #0066cc; }
        .status-completed { background: #d4edda; color: #155724; }
        .status-paused { background: #fff3cd; color: #856404; }
        .status-cancelled { background: #f8d7da; color: #721c24; }
        .status-failed { background: #f8d7da; color: #721c24; }
        .progress-bar { width: 100%; height: 20px; background: #f0f0f0; border-radius: 10px; overflow: hidden; margin: 10px 0; }
        .progress-fill { height: 100%; background: linear-gradient(90deg, #4CAF50, #45a049); transition: width 0.3s; }
        .download-details { display: grid; grid-template-columns: repeat(auto-fit, minmax(150px, 1fr)); gap: 10px; margin: 15px 0; }
        .detail-item { font-size: 14px; }
        .detail-label { color: #6c757d; font-weight: bold; }
        .detail-value { color: #333; }
        .controls { margin-top: 15px; }
        .btn { padding: 8px 16px; border: none; border-radius: 4px; cursor: pointer; margin: 0 5px; font-size: 14px; }
        .btn-pause { background: #ffc107; color: #212529; }
        .btn-resume { background: #28a745; color: white; }
        .btn-cancel { background: #dc3545; color: white; }
        .btn-refresh { background: #007bff; color: white; }
        .btn:hover { opacity: 0.8; }
        .btn:disabled { opacity: 0.5; cursor: not-allowed; }
        .loading { text-align: center; padding: 40px; color: #6c757d; }
        .error { background: #f8d7da; border: 1px solid #f5c6cb; color: #721c24; padding: 15px; border-radius: 4px; margin: 15px 0; }
        .no-downloads { text-align: center; padding: 40px; color: #6c757d; }
        .auto-refresh { margin: 20px 0; text-align: center; }
        .refresh-indicator { display: inline-block; margin-left: 10px; }
        .engine-info { background: #e7f3ff; border-left: 4px solid #007bff; padding: 10px; margin: 10px 0; border-radius: 4px; }
        .bulk-controls { margin: 20px 0; text-align: center; }
    </style>
</head>
<body>
    <div class="container">
        <div class="header">
            <h1>📥 Kolosal Downloads Monitor</h1>
            <p>Real-time monitoring of model downloads and progress</p>
        </div>

        <div class="auto-refresh">
            <label>
                <input type="checkbox" id="auto-refresh" checked> Auto-refresh every 
                <select id="refresh-interval">
                    <option value="1000">1 second</option>
                    <option value="2000" selected>2 seconds</option>
                    <option value="5000">5 seconds</option>
                    <option value="10000">10 seconds</option>
                </select>
            </label>
            <button class="btn btn-refresh" onclick="refreshDownloads()">🔄 Refresh Now</button>
            <span class="refresh-indicator" id="refresh-indicator"></span>
        </div>

        <div class="stats-grid" id="stats-grid">
            <!-- Stats will be populated here -->
        </div>

        <div class="bulk-controls">
            <button class="btn btn-cancel" onclick="cancelAllDownloads()" id="cancel-all-btn">🛑 Cancel All Downloads</button>
        </div>

        <div id="error-container"></div>
        <div id="downloads-container"></div>
    </div>

    <script>
        class DownloadsMonitor {
            constructor() {
                this.baseURL = 'http://localhost:8080';
                this.autoRefresh = true;
                this.refreshInterval = 2000;
                this.intervalId = null;
                this.isRefreshing = false;
                
                this.initializeEventListeners();
                this.startAutoRefresh();
                this.refreshDownloads();
            }

            initializeEventListeners() {
                document.getElementById('auto-refresh').addEventListener('change', (e) => {
                    this.autoRefresh = e.target.checked;
                    if (this.autoRefresh) {
                        this.startAutoRefresh();
                    } else {
                        this.stopAutoRefresh();
                    }
                });

                document.getElementById('refresh-interval').addEventListener('change', (e) => {
                    this.refreshInterval = parseInt(e.target.value);
                    if (this.autoRefresh) {
                        this.stopAutoRefresh();
                        this.startAutoRefresh();
                    }
                });
            }

            startAutoRefresh() {
                if (this.intervalId) return;
                
                this.intervalId = setInterval(() => {
                    if (!this.isRefreshing) {
                        this.refreshDownloads();
                    }
                }, this.refreshInterval);
            }

            stopAutoRefresh() {
                if (this.intervalId) {
                    clearInterval(this.intervalId);
                    this.intervalId = null;
                }
            }

            async makeRequest(method, endpoint, data = null) {
                const options = {
                    method,
                    headers: {
                        'Content-Type': 'application/json'
                    }
                };

                if (data) {
                    options.body = JSON.stringify(data);
                }

                const response = await fetch(`${this.baseURL}${endpoint}`, options);
                
                if (!response.ok) {
                    const errorData = await response.json();
                    throw new Error(errorData.error?.message || `HTTP ${response.status}`);
                }

                return await response.json();
            }

            async refreshDownloads() {
                if (this.isRefreshing) return;
                
                this.isRefreshing = true;
                const indicator = document.getElementById('refresh-indicator');
                if (indicator) indicator.textContent = '🔄';

                try {
                    const data = await this.makeRequest('GET', '/downloads');
                    this.updateDisplay(data);
                    this.clearError();
                } catch (error) {
                    this.showError(`Failed to fetch downloads: ${error.message}`);
                } finally {
                    this.isRefreshing = false;
                    if (indicator) indicator.textContent = '';
                }
            }

            updateDisplay(data) {
                this.updateStats(data.summary);
                this.updateDownloadsList(data.active_downloads);
            }

            updateStats(summary) {
                const statsGrid = document.getElementById('stats-grid');
                
                statsGrid.innerHTML = `
                    <div class="stat-card">
                        <div class="stat-value">${summary.total_active}</div>
                        <div class="stat-label">Total Active</div>
                    </div>
                    <div class="stat-card">
                        <div class="stat-value">${summary.startup_downloads}</div>
                        <div class="stat-label">Startup Downloads</div>
                    </div>
                    <div class="stat-card">
                        <div class="stat-value">${summary.regular_downloads}</div>
                        <div class="stat-label">Regular Downloads</div>
                    </div>
                    <div class="stat-card">
                        <div class="stat-value">${summary.embedding_model_downloads}</div>
                        <div class="stat-label">Embedding Models</div>
                    </div>
                    <div class="stat-card">
                        <div class="stat-value">${summary.llm_model_downloads}</div>
                        <div class="stat-label">LLM Models</div>
                    </div>
                `;
            }

            updateDownloadsList(downloads) {
                const container = document.getElementById('downloads-container');
                
                if (downloads.length === 0) {
                    container.innerHTML = '<div class="no-downloads">📭 No active downloads</div>';
                    return;
                }

                let html = '<div class="download-list"><h3>📋 Active Downloads</h3>';
                
                downloads.forEach(download => {
                    html += this.createDownloadItem(download);
                });
                
                html += '</div>';
                container.innerHTML = html;
            }

            createDownloadItem(download) {
                const canPause = download.status === 'downloading';
                const canResume = download.status === 'paused';
                const canCancel = ['downloading', 'paused', 'creating_engine'].includes(download.status);

                let engineInfo = '';
                if (download.engine_creation) {
                    engineInfo = `
                        <div class="engine-info">
                            <strong>🔧 Engine Creation:</strong> ${download.engine_creation.model_id}
                            <br><small>GPU: ${download.engine_creation.main_gpu_id} | Auto-load: ${download.engine_creation.load_immediately ? 'Yes' : 'No'}</small>
                        </div>
                    `;
                }

                return `
                    <div class="download-item">
                        <div class="download-header">
                            <div>
                                <div class="model-id">${download.model_id}</div>
                                <small style="color: #6c757d;">${download.download_type} download</small>
                            </div>
                            <div class="status status-${download.status}">${download.status}</div>
                        </div>
                        
                        <div class="progress-bar">
                            <div class="progress-fill" style="width: ${download.progress.percentage}%"></div>
                        </div>
                        <div style="text-align: center; font-size: 14px; margin: 5px 0;">
                            ${download.progress.percentage.toFixed(1)}%
                        </div>
                        
                        <div class="download-details">
                            <div class="detail-item">
                                <div class="detail-label">Downloaded</div>
                                <div class="detail-value">${this.formatBytes(download.progress.downloaded_bytes)}</div>
                            </div>
                            <div class="detail-item">
                                <div class="detail-label">Total Size</div>
                                <div class="detail-value">${this.formatBytes(download.progress.total_bytes)}</div>
                            </div>
                            <div class="detail-item">
                                <div class="detail-label">Speed</div>
                                <div class="detail-value">${this.formatSpeed(download.progress.download_speed_bps)}</div>
                            </div>
                            <div class="detail-item">
                                <div class="detail-label">Elapsed</div>
                                <div class="detail-value">${this.formatDuration(download.timing.elapsed_seconds)}</div>
                            </div>
                            ${download.timing.estimated_remaining_seconds !== undefined ? `
                                <div class="detail-item">
                                    <div class="detail-label">Remaining</div>
                                    <div class="detail-value">${this.formatDuration(download.timing.estimated_remaining_seconds)}</div>
                                </div>
                            ` : ''}
                        </div>
                        
                        ${download.error_message ? `
                            <div class="error">❌ ${download.error_message}</div>
                        ` : ''}
                        
                        ${engineInfo}
                        
                        <div class="controls">
                            <button class="btn btn-pause" onclick="monitor.pauseDownload('${download.model_id}')" ${!canPause ? 'disabled' : ''}>⏸️ Pause</button>
                            <button class="btn btn-resume" onclick="monitor.resumeDownload('${download.model_id}')" ${!canResume ? 'disabled' : ''}>▶️ Resume</button>
                            <button class="btn btn-cancel" onclick="monitor.cancelDownload('${download.model_id}')" ${!canCancel ? 'disabled' : ''}>🛑 Cancel</button>
                        </div>
                    </div>
                `;
            }

            async pauseDownload(modelId) {
                try {
                    await this.makeRequest('POST', `/downloads/${encodeURIComponent(modelId)}/pause`);
                    this.refreshDownloads();
                } catch (error) {
                    this.showError(`Failed to pause download: ${error.message}`);
                }
            }

            async resumeDownload(modelId) {
                try {
                    await this.makeRequest('POST', `/downloads/${encodeURIComponent(modelId)}/resume`);
                    this.refreshDownloads();
                } catch (error) {
                    this.showError(`Failed to resume download: ${error.message}`);
                }
            }

            async cancelDownload(modelId) {
                if (!confirm(`Are you sure you want to cancel the download for ${modelId}?`)) {
                    return;
                }

                try {
                    await this.makeRequest('POST', `/downloads/${encodeURIComponent(modelId)}/cancel`);
                    this.refreshDownloads();
                } catch (error) {
                    this.showError(`Failed to cancel download: ${error.message}`);
                }
            }

            async cancelAllDownloads() {
                if (!confirm('Are you sure you want to cancel ALL active downloads?')) {
                    return;
                }

                const button = document.getElementById('cancel-all-btn');
                button.disabled = true;
                button.textContent = '🔄 Cancelling...';

                try {
                    const result = await this.makeRequest('POST', '/downloads/cancel');
                    this.refreshDownloads();
                    alert(`Cancelled ${result.summary.successful_cancellations} downloads successfully`);
                } catch (error) {
                    this.showError(`Failed to cancel all downloads: ${error.message}`);
                } finally {
                    button.disabled = false;
                    button.textContent = '🛑 Cancel All Downloads';
                }
            }

            formatBytes(bytes) {
                if (bytes === 0) return '0 Bytes';
                const k = 1024;
                const sizes = ['Bytes', 'KB', 'MB', 'GB', 'TB'];
                const i = Math.floor(Math.log(bytes) / Math.log(k));
                return parseFloat((bytes / Math.pow(k, i)).toFixed(2)) + ' ' + sizes[i];
            }

            formatSpeed(bytesPerSecond) {
                return this.formatBytes(bytesPerSecond) + '/s';
            }

            formatDuration(seconds) {
                if (seconds < 0 || !isFinite(seconds)) return 'Unknown';
                
                const hours = Math.floor(seconds / 3600);
                const minutes = Math.floor((seconds % 3600) / 60);
                const secs = Math.floor(seconds % 60);

                if (hours > 0) {
                    return `${hours}h ${minutes}m ${secs}s`;
                } else if (minutes > 0) {
                    return `${minutes}m ${secs}s`;
                } else {
                    return `${secs}s`;
                }
            }

            showError(message) {
                const container = document.getElementById('error-container');
                container.innerHTML = `<div class="error">${message}</div>`;
            }

            clearError() {
                const container = document.getElementById('error-container');
                container.innerHTML = '';
            }
        }

        // Global functions for button handlers
        function refreshDownloads() {
            monitor.refreshDownloads();
        }

        function cancelAllDownloads() {
            monitor.cancelAllDownloads();
        }

        // Initialize monitor
        const monitor = new DownloadsMonitor();
    </script>
</body>
</html>
```

## Error Responses

### 400 Bad Request

```json
{
  "error": {
    "message": "Cannot extract model ID from request path",
    "type": "invalid_request_error",
    "param": "path",
    "code": "invalid_path_format"
  }
}
```

### 404 Not Found

```json
{
  "error": {
    "message": "No download found for model ID: model-name",
    "type": "not_found_error",
    "param": "model_id",
    "code": "download_not_found"
  }
}
```

### 500 Server Error

```json
{
  "error": {
    "message": "Server error: Internal processing failed",
    "type": "server_error",
    "param": null,
    "code": null
  }
}
```

## Best Practices

1. **Polling Frequency**: Don't poll too frequently (recommended: 1-5 seconds)
2. **Error Handling**: Always handle network errors and API errors gracefully
3. **Progress Tracking**: Use percentage and bytes for accurate progress display
4. **Status Monitoring**: Check for status changes to update UI appropriately
5. **Batch Operations**: Use cancel-all for bulk operations when needed
6. **Model ID Encoding**: Always URL-encode model IDs in path parameters
7. **Timeout Handling**: Set appropriate timeouts for long-running operations

## Response Headers

The API includes CORS headers for cross-origin requests:

```http
Access-Control-Allow-Origin: *
Access-Control-Allow-Methods: GET, POST, DELETE, OPTIONS
Access-Control-Allow-Headers: Content-Type, Authorization, X-API-Key
```

## Authentication

If your server is configured with authentication, include the appropriate headers:

```javascript
headers: {
  'Content-Type': 'application/json',
  'Authorization': 'Bearer your-jwt-token',
  // or
  'X-API-Key': 'your-api-key'
}
```

## Use Cases

1. **Download Monitoring**: Real-time tracking of model download progress
2. **Batch Management**: Managing multiple simultaneous downloads
3. **Error Recovery**: Pausing and resuming failed downloads
4. **Resource Management**: Canceling downloads to free up bandwidth
5. **UI Integration**: Building download progress interfaces
6. **Automation**: Scripted download management and monitoring
7. **System Administration**: Bulk operations for system maintenance

## Performance Considerations

- **Network Monitoring**: Track download speeds and adjust expectations
- **Resource Usage**: Monitor disk space and bandwidth consumption
- **Concurrent Downloads**: Limit simultaneous downloads based on system capacity
- **Error Resilience**: Implement retry logic for network failures
- **Progress Caching**: Cache progress data to reduce API calls
- **Status Optimization**: Only update UI when significant changes occur

## Advanced: Progress & Percentage Notes

`percentage = (downloaded_bytes / total_bytes) * 100` (guarded; invalid values coerced to 0.0). `download_speed_bps` is average since `start_time` (not instantaneous). `estimated_remaining_seconds` appears only while actively downloading with valid speed & percentage.

## Advanced: Curl Examples

```bash
# All downloads
curl -s http://localhost:8080/v1/downloads | jq

# Single download
curl -s http://localhost:8080/v1/downloads/my-model-id | jq

# Cancel (DELETE)
curl -X DELETE http://localhost:8080/v1/downloads/my-model-id

# Cancel (POST action)
curl -X POST http://localhost:8080/v1/downloads/my-model-id/cancel

# Pause / Resume
curl -X POST http://localhost:8080/v1/downloads/my-model-id/pause
curl -X POST http://localhost:8080/v1/downloads/my-model-id/resume

# Cancel all
curl -X POST http://localhost:8080/v1/downloads/cancel
```

## Advanced: Error Structure

All errors follow:

```json
{
  "error": {
    "message": "string",
    "type": "invalid_request_error | not_found_error | server_error",
    "param": "string|null",
    "code": "string|null"
  }
}
```

| Scenario | HTTP | type | code | Notes |
|----------|------|------|------|-------|
| Unknown model id (GET) | 404 | not_found_error | download_not_found | Re-check id casing |
| Unknown model id (cancel/pause/resume) | 404 | not_found_error | (null) | Operation on non-existent download |
| Cancel already terminal | 400 | invalid_request_error | (null) | Status completed/failed/cancelled |
| Bad path | 400 | invalid_request_error | invalid_endpoint | Regex failed to match |
| Bad model id segment | 400 | invalid_request_error | invalid_path_format | Missing id component |
| Internal exception | 500 | server_error | (null) | Inspect server logs |

## Advanced: Polling Guidance

Polling every 1–2s per active item is usually sufficient. For many simultaneous downloads prefer a single `GET /v1/downloads` then derive UI deltas client-side.

## Advanced: Pause / Resume Semantics

Pause is best-effort; a short delay before transition to `paused` may occur. Resume may restart from 0 if underlying source lacks range support.

## Advanced: Roadmap Ideas

- SSE / WebSocket streaming updates
- Prioritized queue & reordering
- Throttling & rate shaping
- Retry policy configuration via API

## Advanced: Change Log (Doc Additions)

- Added method matrix, curl samples, error table, progress computation notes, polling & roadmap sections.

## Progress & Percentage Notes
`percentage = (downloaded_bytes / total_bytes) * 100` (guarded; invalid values coerced to 0.0). `download_speed_bps` is average since `start_time` (not instantaneous). `estimated_remaining_seconds` appears only while actively downloading with valid speed & percentage.

## Curl Examples
```bash
# All downloads
curl -s http://localhost:8080/v1/downloads | jq

# Single download
curl -s http://localhost:8080/v1/downloads/my-model-id | jq

# Cancel (DELETE)
curl -X DELETE http://localhost:8080/v1/downloads/my-model-id

# Cancel (POST action)
curl -X POST http://localhost:8080/v1/downloads/my-model-id/cancel

# Pause / Resume
curl -X POST http://localhost:8080/v1/downloads/my-model-id/pause
curl -X POST http://localhost:8080/v1/downloads/my-model-id/resume

# Cancel all
curl -X POST http://localhost:8080/v1/downloads/cancel
```

## Error Structure
All errors follow:
```json
{
  "error": {
    "message": "string",
    "type": "invalid_request_error | not_found_error | server_error",
    "param": "string|null",
    "code": "string|null"
  }
}
```

| Scenario | HTTP | type | code | Notes |
|----------|------|------|------|-------|
| Unknown model id (GET) | 404 | not_found_error | download_not_found | Re-check id casing |
| Unknown model id (cancel/pause/resume) | 404 | not_found_error | (null) | Operation on non-existent download |
| Cancel already terminal | 400 | invalid_request_error | (null) | Status completed/failed/cancelled |
| Bad path | 400 | invalid_request_error | invalid_endpoint | Regex failed to match |
| Bad model id segment | 400 | invalid_request_error | invalid_path_format | Missing id component |
| Internal exception | 500 | server_error | (null) | Inspect server logs |

## Polling Guidance
Polling every 1–2s per active item is usually sufficient. For many simultaneous downloads prefer a single `GET /v1/downloads` then derive UI deltas client-side.

## Pause / Resume Semantics
Pause is best-effort; a short delay before transition to `paused` may occur. Resume may restart from 0 if underlying source lacks range support.

## Roadmap Ideas
- SSE / WebSocket streaming updates
- Prioritized queue & reordering
- Throttling & rate shaping
- Retry policy configuration via API

## Change Log (Doc Additions)
- Added method matrix, curl samples, error table, progress computation notes, polling & roadmap sections.
