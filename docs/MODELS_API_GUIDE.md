# Kolosal Server - Models API Documentation

## Overview

The Models API provides comprehensive model management capabilities for the Kolosal server. It supports listing, adding, removing, and monitoring AI models including both Large Language Models (LLMs) and embedding models. The API handles model downloads from URLs, local file management, and engine creation with configurable parameters.

**Base Endpoints:** `/models` or `/v1/models`  
**Supported Models:** LLM (text generation) and Embedding (text vectorization)  
**Supported Operations:** GET, POST, DELETE

## API Endpoints

### 1. List All Models

**Endpoint:** `GET /models` or `GET /v1/models`  
**Description:** Retrieve information about all registered models including their status and capabilities

#### Response Format (200 OK)

```json
{
  "models": [
    {
      "model_id": "string",
      "status": "string",
      "available": "boolean",
      "model_type": "string",
      "capabilities": ["string"],
      "inference_ready": "boolean",
      "last_accessed": "string"
    }
  ],
  "total_count": "integer",
  "summary": {
    "total_models": "integer",
    "embedding_models": "integer",
    "llm_models": "integer",
    "loaded_models": "integer",
    "unloaded_models": "integer"
  }
}
```

### 2. Add New Model

**Endpoint:** `POST /models` or `POST /v1/models`  
**Description:** Add a new model to the server with configurable loading parameters

#### Request Body Schema

```json
{
  "model_id": "string (required)",
  "model_path": "string (required)",
  "model_type": "string (required)",
  "inference_engine": "string (optional)",
  "main_gpu_id": "integer (optional, default: -1)",
  "load_immediately": "boolean (optional, default: true)",
  "loading_parameters": {
    "n_ctx": "integer (optional, default: 4096)",
    "n_keep": "integer (optional, default: 0)",
    "n_batch": "integer (optional, default: 512)",
    "n_ubatch": "integer (optional, default: 512)",
    "n_parallel": "integer (optional, default: 1)",
    "n_gpu_layers": "integer (optional, default: 0)",
    "use_mmap": "boolean (optional, default: true)",
    "use_mlock": "boolean (optional, default: false)",
    "cont_batching": "boolean (optional, default: false)",
    "warmup": "boolean (optional, default: true)"
  }
}
```

#### Request Parameters

| Parameter | Type | Required | Default | Description |
|-----------|------|----------|---------|-------------|
| `model_id` | string | Yes | - | Unique identifier for the model |
| `model_path` | string | Yes | - | Local file path or URL to the model file |
| `model_type` | string | Yes | - | Type of model: "llm" or "embedding" |
| `inference_engine` | string | No | "llama-cpu" | Inference engine to use |
| `main_gpu_id` | integer | No | -1 | GPU ID to use (-1 for auto-select) |
| `load_immediately` | boolean | No | true | Whether to load the model immediately |

#### Loading Parameters

| Parameter | Type | Default | Range | Description |
|-----------|------|---------|-------|-------------|
| `n_ctx` | integer | 4096 | 1-32768+ | Context window size in tokens |
| `n_keep` | integer | 0 | 0+ | Number of tokens to keep from initial prompt |
| `n_batch` | integer | 512 | 1-4096+ | Batch size for processing |
| `n_ubatch` | integer | 512 | 1-4096+ | Micro-batch size |
| `n_parallel` | integer | 1 | 1-16+ | Number of parallel sequences |
| `n_gpu_layers` | integer | 0 | 0-100+ | Number of layers to offload to GPU |
| `use_mmap` | boolean | true | - | Use memory mapping for model loading |
| `use_mlock` | boolean | false | - | Lock model in memory |
| `cont_batching` | boolean | false | - | Enable continuous batching |
| `warmup` | boolean | true | - | Perform model warmup on load |

#### Success Response (201 Created)

```json
{
  "model_id": "string",
  "model_path": "string",
  "model_type": "string",
  "status": "string",
  "load_immediately": "boolean",
  "loading_parameters": {
    "n_ctx": "integer",
    "n_keep": "integer",
    "n_batch": "integer",
    "n_ubatch": "integer",
    "n_parallel": "integer",
    "n_gpu_layers": "integer",
    "use_mmap": "boolean",
    "use_mlock": "boolean",
    "cont_batching": "boolean",
    "warmup": "boolean"
  },
  "main_gpu_id": "integer",
  "message": "string",
  "download_info": {
    "source_url": "string",
    "local_path": "string",
    "was_downloaded": "boolean"
  }
}
```

#### Download Response (202 Accepted)

When adding a model from URL that requires downloading:

```json
{
  "model_id": "string",
  "model_type": "string",
  "status": "downloading",
  "message": "Download started in background",
  "download_url": "string",
  "local_path": "string"
}
```

### 3. Get Specific Model

**Endpoint:** `GET /models/{model_id}` or `GET /v1/models/{model_id}`  
**Description:** Get detailed information about a specific model

#### Model Details Response (200 OK)

```json
{
  "model_id": "string",
  "status": "string",
  "available": "boolean",
  "message": "string",
  "engine_loaded": "boolean",
  "inference_ready": "boolean",
  "capabilities": ["string"],
  "performance": {
    "last_activity": "string",
    "request_count": "integer"
  }
}
```

### 4. Remove Model

**Endpoint:** `DELETE /models/{model_id}` or `DELETE /v1/models/{model_id}`  
**Description:** Remove a model from the server

#### Removal Response (200 OK)

```json
{
  "model_id": "string",
  "status": "removed",
  "message": "Model removed successfully"
}
```

### 5. Get Model Status

**Endpoint:** `GET /models/{model_id}/status` or `GET /v1/models/{model_id}/status`  
**Description:** Get detailed status information about a specific model

#### Status Response (200 OK)

```json
{
  "model_id": "string",
  "status": "string",
  "available": "boolean",
  "message": "string",
  "engine_loaded": "boolean",
  "inference_ready": "boolean",
  "capabilities": ["string"],
  "performance": {
    "last_activity": "string",
    "request_count": "integer"
  }
}
```

## Field Descriptions

### Model Status Values

| Status | Description |
|--------|-------------|
| `loaded` | Model is loaded and ready for inference |
| `unloaded` | Model exists but is not currently loaded |
| `downloading` | Model is being downloaded from URL |
| `created` | Model was registered but not immediately loaded |
| `removed` | Model has been removed from the server |

### Model Types

| Type | Description | Capabilities |
|------|-------------|--------------|
| `llm` | Large Language Model | text_generation, chat |
| `embedding` | Embedding Model | embedding, retrieval |

### Inference Engines

| Engine | Description | Best For |
|--------|-------------|----------|
| `llama-cpu` | CPU-based inference using llama.cpp | General purpose, no GPU required |
| `llama-gpu` | GPU-accelerated inference using llama.cpp | High performance with CUDA support |

## JavaScript Examples

### Basic Model Management

```javascript
class ModelManager {
  constructor(baseURL = 'http://localhost:8080', apiKey = null) {
    this.baseURL = baseURL;
    this.headers = {
      'Content-Type': 'application/json',
      ...(apiKey && { 'X-API-Key': apiKey })
    };
  }

  async listModels() {
    try {
      const response = await fetch(`${this.baseURL}/models`, {
        method: 'GET',
        headers: this.headers
      });

      if (!response.ok) {
        const errorData = await response.json();
        throw new Error(`API Error (${response.status}): ${errorData.error?.message || 'Unknown error'}`);
      }

      return await response.json();
    } catch (error) {
      console.error('Failed to list models:', error);
      throw error;
    }
  }

  async addModel(modelConfig) {
    try {
      const response = await fetch(`${this.baseURL}/models`, {
        method: 'POST',
        headers: this.headers,
        body: JSON.stringify(modelConfig)
      });

      if (!response.ok) {
        const errorData = await response.json();
        throw new Error(`API Error (${response.status}): ${errorData.error?.message || 'Failed to add model'}`);
      }

      return await response.json();
    } catch (error) {
      console.error('Failed to add model:', error);
      throw error;
    }
  }

  async getModel(modelId) {
    try {
      const response = await fetch(`${this.baseURL}/models/${encodeURIComponent(modelId)}`, {
        method: 'GET',
        headers: this.headers
      });

      if (!response.ok) {
        const errorData = await response.json();
        throw new Error(`API Error (${response.status}): ${errorData.error?.message || 'Model not found'}`);
      }

      return await response.json();
    } catch (error) {
      console.error(`Failed to get model ${modelId}:`, error);
      throw error;
    }
  }

  async removeModel(modelId) {
    try {
      const response = await fetch(`${this.baseURL}/models/${encodeURIComponent(modelId)}`, {
        method: 'DELETE',
        headers: this.headers
      });

      if (!response.ok) {
        const errorData = await response.json();
        throw new Error(`API Error (${response.status}): ${errorData.error?.message || 'Failed to remove model'}`);
      }

      return await response.json();
    } catch (error) {
      console.error(`Failed to remove model ${modelId}:`, error);
      throw error;
    }
  }

  async getModelStatus(modelId) {
    try {
      const response = await fetch(`${this.baseURL}/models/${encodeURIComponent(modelId)}/status`, {
        method: 'GET',
        headers: this.headers
      });

      if (!response.ok) {
        const errorData = await response.json();
        throw new Error(`API Error (${response.status}): ${errorData.error?.message || 'Model not found'}`);
      }

      return await response.json();
    } catch (error) {
      console.error(`Failed to get model status ${modelId}:`, error);
      throw error;
    }
  }
}

// Example usage
const modelManager = new ModelManager('http://localhost:8080');

async function demonstrateModelManagement() {
  try {
    console.log('🔍 Listing all models...\n');
    
    const modelsList = await modelManager.listModels();
    
    console.log(`📊 Model Summary:`);
    console.log(`Total models: ${modelsList.summary.total_models}`);
    console.log(`LLM models: ${modelsList.summary.llm_models}`);
    console.log(`Embedding models: ${modelsList.summary.embedding_models}`);
    console.log(`Loaded models: ${modelsList.summary.loaded_models}`);
    console.log(`Unloaded models: ${modelsList.summary.unloaded_models}\n`);

    if (modelsList.models.length === 0) {
      console.log('📭 No models found');
      return;
    }

    console.log('📋 Available Models:');
    console.log('===================');
    
    for (const model of modelsList.models) {
      console.log(`\n🤖 ${model.model_id}`);
      console.log(`   Type: ${model.model_type}`);
      console.log(`   Status: ${model.status}`);
      console.log(`   Available: ${model.available}`);
      console.log(`   Inference Ready: ${model.inference_ready}`);
      console.log(`   Capabilities: ${model.capabilities.join(', ')}`);
      
      if (model.last_accessed) {
        console.log(`   Last Accessed: ${model.last_accessed}`);
      }
    }

    // Demonstrate getting specific model details
    if (modelsList.models.length > 0) {
      const firstModel = modelsList.models[0];
      console.log(`\n🔍 Getting detailed info for ${firstModel.model_id}...`);
      
      const modelDetails = await modelManager.getModel(firstModel.model_id);
      console.log(`Status: ${modelDetails.status}`);
      console.log(`Message: ${modelDetails.message}`);
      console.log(`Engine Loaded: ${modelDetails.engine_loaded}`);
      console.log(`Inference Ready: ${modelDetails.inference_ready}`);
    }

  } catch (error) {
    console.error('❌ Demo failed:', error.message);
  }
}

demonstrateModelManagement();
```

### Advanced Model Configuration

```javascript
const axios = require('axios');

class AdvancedModelManager {
  constructor(baseURL = 'http://localhost:8080', apiKey = null) {
    this.baseURL = baseURL;
    this.client = axios.create({
      baseURL: baseURL,
      headers: {
        'Content-Type': 'application/json',
        ...(apiKey && { 'X-API-Key': apiKey })
      },
      timeout: 300000 // 5 minute timeout for model loading
    });
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

  async addLLMModel(modelId, modelPath, options = {}) {
    const config = {
      model_id: modelId,
      model_path: modelPath,
      model_type: 'llm',
      inference_engine: options.inferenceEngine || 'llama-cpu',
      main_gpu_id: options.mainGpuId ?? -1,
      load_immediately: options.loadImmediately ?? true,
      loading_parameters: {
        n_ctx: options.contextSize || 4096,
        n_keep: options.keepTokens || 0,
        n_batch: options.batchSize || 512,
        n_ubatch: options.microBatchSize || 512,
        n_parallel: options.parallelSequences || 1,
        n_gpu_layers: options.gpuLayers || 0,
        use_mmap: options.useMemoryMapping ?? true,
        use_mlock: options.lockMemory ?? false,
        cont_batching: options.continuousBatching ?? false,
        warmup: options.warmup ?? true
      }
    };

    return this.makeRequest('POST', '/models', config);
  }

  async addEmbeddingModel(modelId, modelPath, options = {}) {
    const config = {
      model_id: modelId,
      model_path: modelPath,
      model_type: 'embedding',
      inference_engine: options.inferenceEngine || 'llama-cpu',
      main_gpu_id: options.mainGpuId ?? -1,
      load_immediately: options.loadImmediately ?? true,
      loading_parameters: {
        n_ctx: options.contextSize || 2048, // Smaller context for embeddings
        n_keep: 0,
        n_batch: options.batchSize || 512,
        n_ubatch: options.microBatchSize || 512,
        n_parallel: options.parallelSequences || 1,
        n_gpu_layers: options.gpuLayers || 0,
        use_mmap: options.useMemoryMapping ?? true,
        use_mlock: options.lockMemory ?? false,
        cont_batching: false, // Usually not needed for embeddings
        warmup: options.warmup ?? true
      }
    };

    return this.makeRequest('POST', '/models', config);
  }

  async listModels() {
    return this.makeRequest('GET', '/models');
  }

  async getModel(modelId) {
    return this.makeRequest('GET', `/models/${encodeURIComponent(modelId)}`);
  }

  async removeModel(modelId) {
    return this.makeRequest('DELETE', `/models/${encodeURIComponent(modelId)}`);
  }

  async getModelStatus(modelId) {
    return this.makeRequest('GET', `/models/${encodeURIComponent(modelId)}/status`);
  }

  async addModelFromHuggingFace(repoId, options = {}) {
    const modelId = options.modelId || repoId.replace('/', '_');
    const huggingFaceUrl = `https://huggingface.co/${repoId}/resolve/main/ggml-model-q4_0.gguf`;
    
    console.log(`📥 Adding model from Hugging Face: ${repoId}`);
    console.log(`🔗 URL: ${huggingFaceUrl}`);
    console.log(`🏷️ Model ID: ${modelId}`);

    const modelType = options.modelType || 'llm'; // Default to LLM
    
    if (modelType === 'embedding') {
      return this.addEmbeddingModel(modelId, huggingFaceUrl, {
        ...options,
        loadImmediately: false // Start with lazy loading for downloads
      });
    } else {
      return this.addLLMModel(modelId, huggingFaceUrl, {
        ...options,
        loadImmediately: false // Start with lazy loading for downloads
      });
    }
  }

  async waitForModelReady(modelId, timeoutMs = 300000, intervalMs = 2000) {
    const startTime = Date.now();
    
    while (Date.now() - startTime < timeoutMs) {
      try {
        const status = await this.getModelStatus(modelId);
        
        if (status.status === 'loaded' && status.inference_ready) {
          return status;
        }
        
        if (status.status === 'failed') {
          throw new Error(`Model loading failed: ${status.message || 'Unknown error'}`);
        }
        
        console.log(`⏳ Waiting for model ${modelId}... Status: ${status.status}`);
        await new Promise(resolve => setTimeout(resolve, intervalMs));
        
      } catch (error) {
        if (error.message.includes('not found')) {
          console.log(`⏳ Model ${modelId} not yet available...`);
          await new Promise(resolve => setTimeout(resolve, intervalMs));
          continue;
        }
        throw error;
      }
    }
    
    throw new Error(`Timeout waiting for model ${modelId} to be ready`);
  }

  async setupDevelopmentEnvironment() {
    console.log('🚀 Setting up development environment with common models...\n');

    const models = [
      {
        id: 'llama-2-7b-chat',
        type: 'llm',
        description: 'Llama 2 7B Chat model for conversational AI',
        config: {
          contextSize: 4096,
          gpuLayers: 35, // Adjust based on GPU memory
          batchSize: 512
        }
      },
      {
        id: 'text-embedding-ada-002',
        type: 'embedding',
        description: 'Text embedding model for semantic search',
        config: {
          contextSize: 2048,
          gpuLayers: 20,
          batchSize: 256
        }
      }
    ];

    for (const model of models) {
      try {
        console.log(`📦 Setting up ${model.description}...`);
        
        // Check if model already exists
        try {
          const existing = await this.getModelStatus(model.id);
          if (existing.status === 'loaded') {
            console.log(`✅ ${model.id} already loaded and ready`);
            continue;
          }
        } catch (error) {
          // Model doesn't exist, continue with setup
        }

        // For demo purposes, we'll use placeholder paths
        // In real usage, these would be actual model file paths or URLs
        const modelPath = `./models/${model.id}.gguf`;
        
        const result = model.type === 'embedding' 
          ? await this.addEmbeddingModel(model.id, modelPath, model.config)
          : await this.addLLMModel(model.id, modelPath, model.config);
          
        if (result.status === 'downloading') {
          console.log(`📥 ${model.id} download started, will be ready soon`);
        } else {
          console.log(`✅ ${model.id} added successfully`);
        }
        
      } catch (error) {
        console.error(`❌ Failed to setup ${model.id}: ${error.message}`);
      }
    }

    console.log('\n🎉 Development environment setup completed!');
  }

  async performHealthCheck() {
    try {
      console.log('🔍 Performing models health check...\n');
      
      const models = await this.listModels();
      
      console.log(`📊 Health Check Results:`);
      console.log(`========================`);
      console.log(`Total models: ${models.summary.total_models}`);
      console.log(`Loaded models: ${models.summary.loaded_models}`);
      console.log(`Unloaded models: ${models.summary.unloaded_models}`);
      
      let healthyModels = 0;
      let unhealthyModels = 0;
      
      for (const model of models.models) {
        try {
          const status = await this.getModelStatus(model.model_id);
          
          if (status.inference_ready) {
            console.log(`✅ ${model.model_id}: Healthy (${status.status})`);
            healthyModels++;
          } else {
            console.log(`⚠️  ${model.model_id}: Not ready (${status.status})`);
            unhealthyModels++;
          }
          
        } catch (error) {
          console.log(`❌ ${model.model_id}: Error - ${error.message}`);
          unhealthyModels++;
        }
      }
      
      console.log(`\n📈 Summary: ${healthyModels} healthy, ${unhealthyModels} need attention`);
      
      return {
        total: models.summary.total_models,
        healthy: healthyModels,
        unhealthy: unhealthyModels,
        healthScore: models.summary.total_models > 0 ? (healthyModels / models.summary.total_models) * 100 : 0
      };
      
    } catch (error) {
      console.error('❌ Health check failed:', error.message);
      throw error;
    }
  }

  getOptimizedConfig(modelType, gpuMemoryGB = 8) {
    const configs = {
      llm: {
        small: { // < 4GB GPU memory
          contextSize: 2048,
          gpuLayers: 10,
          batchSize: 256,
          parallelSequences: 1
        },
        medium: { // 4-8GB GPU memory
          contextSize: 4096,
          gpuLayers: 25,
          batchSize: 512,
          parallelSequences: 2
        },
        large: { // > 8GB GPU memory
          contextSize: 8192,
          gpuLayers: 40,
          batchSize: 1024,
          parallelSequences: 4
        }
      },
      embedding: {
        small: {
          contextSize: 1024,
          gpuLayers: 15,
          batchSize: 256,
          parallelSequences: 1
        },
        medium: {
          contextSize: 2048,
          gpuLayers: 25,
          batchSize: 512,
          parallelSequences: 2
        },
        large: {
          contextSize: 4096,
          gpuLayers: 35,
          batchSize: 1024,
          parallelSequences: 4
        }
      }
    };

    let sizeCategory = 'small';
    if (gpuMemoryGB >= 8) sizeCategory = 'large';
    else if (gpuMemoryGB >= 4) sizeCategory = 'medium';

    return configs[modelType][sizeCategory];
  }
}

// Example usage with advanced features
async function demonstrateAdvancedFeatures() {
  const manager = new AdvancedModelManager('http://localhost:8080');

  try {
    // Perform health check
    const healthCheck = await manager.performHealthCheck();
    console.log(`\n🏥 Overall Health Score: ${healthCheck.healthScore.toFixed(1)}%\n`);

    // Add a model with optimized configuration
    const optimizedConfig = manager.getOptimizedConfig('llm', 8); // 8GB GPU
    console.log('⚙️ Using optimized configuration:', optimizedConfig);

    // Example: Add a model from a local file
    try {
      const result = await manager.addLLMModel(
        'local-llm-model',
        './models/my-model.gguf',
        {
          ...optimizedConfig,
          inferenceEngine: 'llama-gpu',
          loadImmediately: true
        }
      );
      
      console.log(`✅ Added local LLM model: ${result.model_id}`);
      
    } catch (error) {
      console.log(`ℹ️ Could not add local model (expected if file doesn't exist): ${error.message}`);
    }

    // Example: Add an embedding model
    try {
      const embeddingResult = await manager.addEmbeddingModel(
        'local-embedding-model',
        './models/embedding-model.gguf',
        manager.getOptimizedConfig('embedding', 4)
      );
      
      console.log(`✅ Added embedding model: ${embeddingResult.model_id}`);
      
    } catch (error) {
      console.log(`ℹ️ Could not add embedding model: ${error.message}`);
    }

  } catch (error) {
    console.error('❌ Advanced demo failed:', error.message);
  }
}

demonstrateAdvancedFeatures();
```

### Model Management Dashboard

```html
<!DOCTYPE html>
<html>
<head>
    <title>Kolosal Model Management</title>
    <style>
        .container { max-width: 1400px; margin: 20px auto; padding: 20px; font-family: Arial, sans-serif; }
        .header { text-align: center; margin-bottom: 30px; }
        .stats-grid { display: grid; grid-template-columns: repeat(auto-fit, minmax(200px, 1fr)); gap: 15px; margin: 20px 0; }
        .stat-card { background: #f8f9fa; padding: 20px; border-radius: 8px; text-align: center; border-left: 4px solid #007bff; }
        .stat-value { font-size: 24px; font-weight: bold; color: #007bff; }
        .stat-label { color: #6c757d; margin-top: 5px; }
        .model-list { margin: 20px 0; }
        .model-item { background: white; border: 1px solid #ddd; border-radius: 8px; padding: 20px; margin: 10px 0; box-shadow: 0 2px 4px rgba(0,0,0,0.1); }
        .model-header { display: flex; justify-content: space-between; align-items: center; margin-bottom: 15px; }
        .model-id { font-size: 18px; font-weight: bold; color: #333; }
        .model-type { padding: 4px 12px; border-radius: 20px; font-size: 12px; font-weight: bold; text-transform: uppercase; }
        .type-llm { background: #e7f3ff; color: #0066cc; }
        .type-embedding { background: #f0f8f0; color: #006600; }
        .status { padding: 4px 12px; border-radius: 20px; font-size: 12px; font-weight: bold; text-transform: uppercase; margin-left: 10px; }
        .status-loaded { background: #d4edda; color: #155724; }
        .status-unloaded { background: #fff3cd; color: #856404; }
        .status-downloading { background: #cce5ff; color: #0066cc; }
        .model-details { display: grid; grid-template-columns: repeat(auto-fit, minmax(200px, 1fr)); gap: 15px; margin: 15px 0; }
        .detail-group { background: #f8f9fa; padding: 15px; border-radius: 6px; }
        .detail-label { font-weight: bold; color: #6c757d; font-size: 14px; }
        .detail-value { color: #333; margin-top: 5px; }
        .capabilities { display: flex; flex-wrap: wrap; gap: 5px; margin-top: 5px; }
        .capability { background: #007bff; color: white; padding: 2px 8px; border-radius: 12px; font-size: 12px; }
        .controls { margin-top: 15px; }
        .btn { padding: 8px 16px; border: none; border-radius: 4px; cursor: pointer; margin: 0 5px; font-size: 14px; }
        .btn-primary { background: #007bff; color: white; }
        .btn-danger { background: #dc3545; color: white; }
        .btn-info { background: #17a2b8; color: white; }
        .btn:hover { opacity: 0.8; }
        .btn:disabled { opacity: 0.5; cursor: not-allowed; }
        .add-model-form { background: #f8f9fa; padding: 20px; border-radius: 8px; margin: 20px 0; }
        .form-group { margin: 15px 0; }
        .form-group label { display: block; margin-bottom: 5px; font-weight: bold; }
        .form-group input, .form-group select { width: 100%; padding: 8px; border: 1px solid #ddd; border-radius: 4px; }
        .form-grid { display: grid; grid-template-columns: repeat(auto-fit, minmax(250px, 1fr)); gap: 15px; }
        .loading { text-align: center; padding: 40px; color: #6c757d; }
        .error { background: #f8d7da; border: 1px solid #f5c6cb; color: #721c24; padding: 15px; border-radius: 4px; margin: 15px 0; }
        .success { background: #d4edda; border: 1px solid #c3e6cb; color: #155724; padding: 15px; border-radius: 4px; margin: 15px 0; }
        .no-models { text-align: center; padding: 40px; color: #6c757d; }
        .collapsible { cursor: pointer; user-select: none; }
        .collapsible::before { content: '▼ '; }
        .collapsible.collapsed::before { content: '▶ '; }
        .collapsible-content { max-height: 1000px; overflow: hidden; transition: max-height 0.3s; }
        .collapsible-content.collapsed { max-height: 0; }
    </style>
</head>
<body>
    <div class="container">
        <div class="header">
            <h1>🤖 Kolosal Model Management</h1>
            <p>Manage AI models for text generation and embeddings</p>
        </div>

        <div class="stats-grid" id="stats-grid">
            <!-- Stats will be populated here -->
        </div>

        <div class="add-model-form">
            <h3 class="collapsible" onclick="toggleSection('add-model-content')">➕ Add New Model</h3>
            <div class="collapsible-content" id="add-model-content">
                <form id="add-model-form" onsubmit="addModel(event)">
                    <div class="form-grid">
                        <div class="form-group">
                            <label for="model-id">Model ID *</label>
                            <input type="text" id="model-id" required placeholder="e.g., llama-2-7b-chat">
                        </div>
                        <div class="form-group">
                            <label for="model-path">Model Path/URL *</label>
                            <input type="text" id="model-path" required placeholder="./models/model.gguf or https://...">
                        </div>
                        <div class="form-group">
                            <label for="model-type">Model Type *</label>
                            <select id="model-type" required>
                                <option value="">Select type...</option>
                                <option value="llm">LLM (Text Generation)</option>
                                <option value="embedding">Embedding (Text Vectorization)</option>
                            </select>
                        </div>
                        <div class="form-group">
                            <label for="inference-engine">Inference Engine</label>
                            <select id="inference-engine">
                                <option value="llama-cpu">llama-cpu</option>
                                <option value="llama-gpu">llama-gpu</option>
                            </select>
                        </div>
                        <div class="form-group">
                            <label for="main-gpu-id">Main GPU ID</label>
                            <input type="number" id="main-gpu-id" value="-1" min="-1" max="7">
                        </div>
                        <div class="form-group">
                            <label>
                                <input type="checkbox" id="load-immediately" checked> Load Immediately
                            </label>
                        </div>
                    </div>

                    <h4 class="collapsible collapsed" onclick="toggleSection('loading-params')">⚙️ Loading Parameters</h4>
                    <div class="collapsible-content collapsed" id="loading-params">
                        <div class="form-grid">
                            <div class="form-group">
                                <label for="n-ctx">Context Size (n_ctx)</label>
                                <input type="number" id="n-ctx" value="4096" min="1" max="32768">
                            </div>
                            <div class="form-group">
                                <label for="n-batch">Batch Size (n_batch)</label>
                                <input type="number" id="n-batch" value="512" min="1" max="4096">
                            </div>
                            <div class="form-group">
                                <label for="n-gpu-layers">GPU Layers</label>
                                <input type="number" id="n-gpu-layers" value="0" min="0" max="100">
                            </div>
                            <div class="form-group">
                                <label for="n-parallel">Parallel Sequences</label>
                                <input type="number" id="n-parallel" value="1" min="1" max="16">
                            </div>
                            <div class="form-group">
                                <label>
                                    <input type="checkbox" id="use-mmap" checked> Use Memory Mapping
                                </label>
                            </div>
                            <div class="form-group">
                                <label>
                                    <input type="checkbox" id="warmup" checked> Perform Warmup
                                </label>
                            </div>
                        </div>
                    </div>

                    <div style="margin-top: 20px;">
                        <button type="submit" class="btn btn-primary">➕ Add Model</button>
                        <button type="button" class="btn btn-info" onclick="useOptimizedSettings()">⚡ Use Optimized Settings</button>
                        <button type="button" class="btn" onclick="resetForm()">🔄 Reset Form</button>
                    </div>
                </form>
            </div>
        </div>

        <div style="text-align: center; margin: 20px 0;">
            <button class="btn btn-primary" onclick="refreshModels()">🔄 Refresh Models</button>
            <button class="btn btn-info" onclick="performHealthCheck()">🏥 Health Check</button>
        </div>

        <div id="message-container"></div>
        <div id="models-container"></div>
    </div>

    <script>
        class ModelDashboard {
            constructor() {
                this.baseURL = 'http://localhost:8080';
                this.refreshModels();
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

            async refreshModels() {
                try {
                    this.clearMessage();
                    const data = await this.makeRequest('GET', '/models');
                    this.updateDisplay(data);
                } catch (error) {
                    this.showError(`Failed to fetch models: ${error.message}`);
                }
            }

            updateDisplay(data) {
                this.updateStats(data.summary);
                this.updateModelsList(data.models);
            }

            updateStats(summary) {
                const statsGrid = document.getElementById('stats-grid');
                
                statsGrid.innerHTML = `
                    <div class="stat-card">
                        <div class="stat-value">${summary.total_models}</div>
                        <div class="stat-label">Total Models</div>
                    </div>
                    <div class="stat-card">
                        <div class="stat-value">${summary.loaded_models}</div>
                        <div class="stat-label">Loaded Models</div>
                    </div>
                    <div class="stat-card">
                        <div class="stat-value">${summary.unloaded_models}</div>
                        <div class="stat-label">Unloaded Models</div>
                    </div>
                    <div class="stat-card">
                        <div class="stat-value">${summary.llm_models}</div>
                        <div class="stat-label">LLM Models</div>
                    </div>
                    <div class="stat-card">
                        <div class="stat-value">${summary.embedding_models}</div>
                        <div class="stat-label">Embedding Models</div>
                    </div>
                `;
            }

            updateModelsList(models) {
                const container = document.getElementById('models-container');
                
                if (models.length === 0) {
                    container.innerHTML = '<div class="no-models">📭 No models found. Add your first model above!</div>';
                    return;
                }

                let html = '<div class="model-list"><h3>📋 Available Models</h3>';
                
                models.forEach(model => {
                    html += this.createModelItem(model);
                });
                
                html += '</div>';
                container.innerHTML = html;
            }

            createModelItem(model) {
                const canRemove = model.available;
                
                let capabilitiesHtml = '';
                if (model.capabilities && model.capabilities.length > 0) {
                    capabilitiesHtml = '<div class="capabilities">';
                    model.capabilities.forEach(cap => {
                        capabilitiesHtml += `<span class="capability">${cap}</span>`;
                    });
                    capabilitiesHtml += '</div>';
                }

                return `
                    <div class="model-item">
                        <div class="model-header">
                            <div>
                                <div class="model-id">${model.model_id}</div>
                                <div>
                                    <span class="model-type type-${model.model_type}">${model.model_type}</span>
                                    <span class="status status-${model.status}">${model.status}</span>
                                </div>
                            </div>
                        </div>
                        
                        <div class="model-details">
                            <div class="detail-group">
                                <div class="detail-label">Status</div>
                                <div class="detail-value">
                                    ${model.status}
                                    ${model.inference_ready ? '✅ Ready' : '⏳ Not Ready'}
                                </div>
                            </div>
                            <div class="detail-group">
                                <div class="detail-label">Availability</div>
                                <div class="detail-value">
                                    ${model.available ? '✅ Available' : '❌ Unavailable'}
                                </div>
                            </div>
                            <div class="detail-group">
                                <div class="detail-label">Type</div>
                                <div class="detail-value">${model.model_type.toUpperCase()}</div>
                            </div>
                            <div class="detail-group">
                                <div class="detail-label">Capabilities</div>
                                <div class="detail-value">
                                    ${capabilitiesHtml || 'None specified'}
                                </div>
                            </div>
                            ${model.last_accessed ? `
                                <div class="detail-group">
                                    <div class="detail-label">Last Accessed</div>
                                    <div class="detail-value">${model.last_accessed}</div>
                                </div>
                            ` : ''}
                        </div>
                        
                        <div class="controls">
                            <button class="btn btn-info" onclick="dashboard.getModelStatus('${model.model_id}')">📊 Get Status</button>
                            <button class="btn btn-info" onclick="dashboard.getModelDetails('${model.model_id}')">🔍 View Details</button>
                            <button class="btn btn-danger" onclick="dashboard.removeModel('${model.model_id}')" ${!canRemove ? 'disabled' : ''}>🗑️ Remove</button>
                        </div>
                    </div>
                `;
            }

            async addModel(modelConfig) {
                try {
                    this.showMessage('⏳ Adding model...', 'info');
                    
                    const result = await this.makeRequest('POST', '/models', modelConfig);
                    
                    if (result.status === 'downloading') {
                        this.showSuccess(`📥 Model download started: ${result.model_id}. Check downloads for progress.`);
                    } else {
                        this.showSuccess(`✅ Model added successfully: ${result.model_id}`);
                    }
                    
                    this.refreshModels();
                    
                } catch (error) {
                    this.showError(`Failed to add model: ${error.message}`);
                }
            }

            async removeModel(modelId) {
                if (!confirm(`Are you sure you want to remove model "${modelId}"?`)) {
                    return;
                }

                try {
                    await this.makeRequest('DELETE', `/models/${encodeURIComponent(modelId)}`);
                    this.showSuccess(`✅ Model removed: ${modelId}`);
                    this.refreshModels();
                } catch (error) {
                    this.showError(`Failed to remove model: ${error.message}`);
                }
            }

            async getModelStatus(modelId) {
                try {
                    const status = await this.makeRequest('GET', `/models/${encodeURIComponent(modelId)}/status`);
                    
                    let message = `📊 Status for ${modelId}:\n`;
                    message += `Status: ${status.status}\n`;
                    message += `Available: ${status.available}\n`;
                    message += `Engine Loaded: ${status.engine_loaded}\n`;
                    message += `Inference Ready: ${status.inference_ready}\n`;
                    message += `Message: ${status.message}`;
                    
                    alert(message);
                    
                } catch (error) {
                    this.showError(`Failed to get model status: ${error.message}`);
                }
            }

            async getModelDetails(modelId) {
                try {
                    const details = await this.makeRequest('GET', `/models/${encodeURIComponent(modelId)}`);
                    
                    let message = `🔍 Details for ${modelId}:\n`;
                    message += `Status: ${details.status}\n`;
                    message += `Available: ${details.available}\n`;
                    message += `Engine Loaded: ${details.engine_loaded || 'N/A'}\n`;
                    message += `Inference Ready: ${details.inference_ready || 'N/A'}\n`;
                    if (details.capabilities) {
                        message += `Capabilities: ${details.capabilities.join(', ')}\n`;
                    }
                    message += `Message: ${details.message}`;
                    
                    alert(message);
                    
                } catch (error) {
                    this.showError(`Failed to get model details: ${error.message}`);
                }
            }

            async performHealthCheck() {
                try {
                    this.showMessage('🔍 Performing health check...', 'info');
                    
                    const models = await this.makeRequest('GET', '/models');
                    let healthy = 0;
                    let total = models.models.length;
                    
                    for (const model of models.models) {
                        try {
                            const status = await this.makeRequest('GET', `/models/${encodeURIComponent(model.model_id)}/status`);
                            if (status.inference_ready) {
                                healthy++;
                            }
                        } catch (error) {
                            // Count as unhealthy
                        }
                    }
                    
                    const healthScore = total > 0 ? ((healthy / total) * 100).toFixed(1) : 0;
                    this.showSuccess(`🏥 Health Check Complete: ${healthy}/${total} models healthy (${healthScore}%)`);
                    
                } catch (error) {
                    this.showError(`Health check failed: ${error.message}`);
                }
            }

            showMessage(message, type = 'info') {
                const container = document.getElementById('message-container');
                const className = type === 'error' ? 'error' : type === 'success' ? 'success' : 'loading';
                container.innerHTML = `<div class="${className}">${message}</div>`;
            }

            showError(message) {
                this.showMessage(message, 'error');
            }

            showSuccess(message) {
                this.showMessage(message, 'success');
            }

            clearMessage() {
                document.getElementById('message-container').innerHTML = '';
            }
        }

        // Global functions for form handling
        function toggleSection(sectionId) {
            const content = document.getElementById(sectionId);
            const header = content.previousElementSibling;
            
            content.classList.toggle('collapsed');
            header.classList.toggle('collapsed');
        }

        function addModel(event) {
            event.preventDefault();
            
            const modelConfig = {
                model_id: document.getElementById('model-id').value,
                model_path: document.getElementById('model-path').value,
                model_type: document.getElementById('model-type').value,
                inference_engine: document.getElementById('inference-engine').value,
                main_gpu_id: parseInt(document.getElementById('main-gpu-id').value),
                load_immediately: document.getElementById('load-immediately').checked,
                loading_parameters: {
                    n_ctx: parseInt(document.getElementById('n-ctx').value),
                    n_batch: parseInt(document.getElementById('n-batch').value),
                    n_gpu_layers: parseInt(document.getElementById('n-gpu-layers').value),
                    n_parallel: parseInt(document.getElementById('n-parallel').value),
                    use_mmap: document.getElementById('use-mmap').checked,
                    warmup: document.getElementById('warmup').checked
                }
            };
            
            dashboard.addModel(modelConfig);
        }

        function useOptimizedSettings() {
            const modelType = document.getElementById('model-type').value;
            
            if (!modelType) {
                alert('Please select a model type first');
                return;
            }
            
            if (modelType === 'embedding') {
                document.getElementById('n-ctx').value = '2048';
                document.getElementById('n-batch').value = '256';
                document.getElementById('n-gpu-layers').value = '20';
                document.getElementById('n-parallel').value = '1';
            } else {
                document.getElementById('n-ctx').value = '4096';
                document.getElementById('n-batch').value = '512';
                document.getElementById('n-gpu-layers').value = '35';
                document.getElementById('n-parallel').value = '2';
            }
            
            alert('Applied optimized settings for ' + modelType + ' model');
        }

        function resetForm() {
            document.getElementById('add-model-form').reset();
            document.getElementById('n-ctx').value = '4096';
            document.getElementById('n-batch').value = '512';
            document.getElementById('n-gpu-layers').value = '0';
            document.getElementById('n-parallel').value = '1';
            document.getElementById('main-gpu-id').value = '-1';
        }

        function refreshModels() {
            dashboard.refreshModels();
        }

        // Initialize dashboard
        const dashboard = new ModelDashboard();
    </script>
</body>
</html>
```

## Error Responses

### 400 Bad Request

```json
{
  "error": {
    "message": "Invalid model_type. Must be 'llm' or 'embedding'",
    "type": "invalid_request_error",
    "param": "model_type",
    "code": null
  }
}
```

### 404 Not Found

```json
{
  "error": {
    "message": "Model not found",
    "type": "not_found_error",
    "param": "model_id",
    "code": "model_not_found"
  }
}
```

### 409 Conflict

```json
{
  "error": {
    "message": "Model ID 'example-model' is already loaded and functional",
    "type": "invalid_request_error",
    "param": "model_id",
    "code": "model_already_loaded"
  }
}
```

### 422 Unprocessable Entity

```json
{
  "error": {
    "message": "Failed to load model from './models/model.gguf'. This could be due to: insufficient GPU memory, incompatible model format, corrupted model file...",
    "type": "model_loading_error",
    "param": "model_path",
    "code": "model_loading_failed",
    "details": {
      "model_id": "string",
      "model_path": "string",
      "n_ctx": "integer",
      "n_gpu_layers": "integer",
      "main_gpu_id": "integer"
    }
  }
}
```

## Best Practices

1. **Model File Management**: Use `.gguf` format files for optimal compatibility
2. **Memory Management**: Configure `n_ctx` and `n_gpu_layers` based on available resources
3. **GPU Selection**: Use `main_gpu_id: -1` for automatic GPU selection
4. **Lazy Loading**: Set `load_immediately: false` for faster API responses
5. **URL Downloads**: Monitor `/downloads` endpoint for model download progress
6. **Error Handling**: Always check for model existence before performing operations
7. **Resource Planning**: Consider GPU memory requirements when adding multiple models
8. **Model Types**: Use appropriate model types (`llm` vs `embedding`) for your use case

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

1. **Model Management**: Add, remove, and monitor AI models
2. **Development Setup**: Quickly configure models for development environments
3. **Production Deployment**: Manage model lifecycles in production systems
4. **Resource Optimization**: Configure models based on available hardware
5. **Multi-Model Systems**: Deploy multiple specialized models (LLM + embedding)
6. **Model Downloads**: Automatically download and configure models from URLs
7. **Health Monitoring**: Monitor model status and performance
8. **Dynamic Loading**: Load models on-demand to optimize resource usage

## Performance Considerations

- **Model Size**: Larger models require more memory and loading time
- **GPU Memory**: Monitor GPU memory usage when using multiple models
- **Context Size**: Larger context windows increase memory requirements exponentially
- **Batch Size**: Optimize batch sizes for your specific hardware and use case
- **Parallel Processing**: Balance parallel sequences with available resources
- **Loading Strategy**: Use lazy loading for infrequently accessed models
