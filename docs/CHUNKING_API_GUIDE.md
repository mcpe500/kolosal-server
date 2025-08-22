# Kolosal Server - Chunking API Documentation

## Overview

The `/chunking` API endpoint splits text into smaller, manageable chunks for processing by language models. It supports both regular (token-based) and semantic (meaning-based) chunking methods, making it ideal for preparing long documents for embedding generation, RAG systems, and other NLP tasks.

**Endpoint:** `POST /chunking`  
**Content-Type:** `application/json`

## Request Format

### Request Body Schema

```json
{
  "model_name": "string (required)",
  "text": "string (required)",
  "method": "string (optional, default: 'regular')",
  "chunk_size": "integer (optional, default: 128)",
  "max_chunk_size": "integer (optional, default: 512)",
  "overlap": "integer (optional, default: 64)",
  "similarity_threshold": "number (optional, default: 0.7)"
}
```

### Request Parameters

| Parameter | Type | Required | Default | Range | Description |
|-----------|------|----------|---------|-------|-------------|
| `model_name` | string | Yes | - | - | Name of the model to use for tokenization and embeddings |
| `text` | string | Yes | - | - | Text content to be chunked |
| `method` | string | No | "regular" | "regular", "semantic" | Chunking method to use |
| `chunk_size` | integer | No | 128 | 1-2048 | Base chunk size in tokens |
| `max_chunk_size` | integer | No | 512 | 1-4096 | Maximum chunk size when merging (semantic only) |
| `overlap` | integer | No | 64 | 0 to chunk_size-1 | Overlap between consecutive chunks in tokens |
| `similarity_threshold` | number | No | 0.7 | 0.0-1.0 | Similarity threshold for semantic chunking |

### Chunking Methods

1. **Regular Chunking (`"regular"`)**:
   - Token-based splitting with fixed chunk sizes
   - Fast and deterministic
   - Good for general purpose text processing

2. **Semantic Chunking (`"semantic"`)**:
   - Content-aware splitting based on semantic similarity
   - Preserves contextual meaning across chunks
   - Better for embedding generation and retrieval systems

## Response Format

### Success Response (200 OK)

```json
{
  "model_name": "string",
  "method": "string",
  "total_chunks": "integer",
  "chunks": [
    {
      "text": "string",
      "index": "integer",
      "token_count": "integer"
    }
  ],
  "usage": {
    "original_tokens": "integer",
    "total_chunk_tokens": "integer",
    "processing_time_ms": "number"
  }
}
```

### Response Fields

| Field | Type | Description |
|-------|------|-------------|
| `model_name` | string | Model used for processing |
| `method` | string | Chunking method used ("regular" or "semantic") |
| `total_chunks` | integer | Total number of chunks generated |
| `chunks` | array | Array of chunk objects |
| `chunks[].text` | string | Text content of the chunk |
| `chunks[].index` | integer | Sequential index of the chunk |
| `chunks[].token_count` | integer | Number of tokens in this chunk |
| `usage.original_tokens` | integer | Total tokens in the original text |
| `usage.total_chunk_tokens` | integer | Sum of tokens across all chunks |
| `usage.processing_time_ms` | number | Processing time in milliseconds |

### Error Response (4xx/5xx)

```json
{
  "error": {
    "message": "string",
    "type": "string",
    "code": "string",
    "param": "string"
  }
}
```

## JavaScript Examples

### Basic Usage with Fetch API

```javascript
async function chunkText(text, modelName, options = {}) {
  const requestBody = {
    model_name: modelName,
    text: text,
    method: options.method || 'regular',
    chunk_size: options.chunkSize || 128,
    max_chunk_size: options.maxChunkSize || 512,
    overlap: options.overlap || 64,
    similarity_threshold: options.similarityThreshold || 0.7
  };

  try {
    const response = await fetch('http://localhost:8080/chunking', {
      method: 'POST',
      headers: {
        'Content-Type': 'application/json',
        // Add authentication headers if required
        // 'Authorization': 'Bearer your-token',
        // 'X-API-Key': 'your-api-key'
      },
      body: JSON.stringify(requestBody)
    });

    if (!response.ok) {
      const errorData = await response.json();
      throw new Error(`API Error (${response.status}): ${errorData.error.message}`);
    }

    const data = await response.json();
    return data;
  } catch (error) {
    console.error('Chunking failed:', error);
    throw error;
  }
}

// Example usage - Regular chunking
const longText = `
Artificial intelligence (AI) is intelligence demonstrated by machines, in contrast to the natural intelligence displayed by humans and animals. Leading AI textbooks define the field as the study of "intelligent agents": any device that perceives its environment and takes actions that maximize its chance of successfully achieving its goals.

Machine learning is a subset of AI that focuses on the development of algorithms that can learn from and make decisions or predictions based on data. Deep learning, a subset of machine learning, uses neural networks with multiple layers to model and understand complex patterns in data.

Natural language processing (NLP) is a branch of AI that helps computers understand, interpret and manipulate human language. NLP draws from many disciplines, including computer science and computational linguistics, in its pursuit to help computers understand human language.
`;

chunkText(longText, 'text-embedding-3-small', {
  method: 'regular',
  chunkSize: 100,
  overlap: 20
})
.then(result => {
  console.log(`📊 Chunking Results:`);
  console.log(`Model: ${result.model_name}`);
  console.log(`Method: ${result.method}`);
  console.log(`Total chunks: ${result.total_chunks}`);
  console.log(`Processing time: ${result.usage.processing_time_ms}ms`);
  console.log(`Original tokens: ${result.usage.original_tokens}`);
  console.log(`Total chunk tokens: ${result.usage.total_chunk_tokens}`);
  
  console.log('\n📄 Chunks:');
  result.chunks.forEach((chunk, index) => {
    console.log(`\n${index + 1}. [${chunk.token_count} tokens]`);
    console.log(`${chunk.text.substring(0, 100)}...`);
  });
})
.catch(error => {
  console.error('Error chunking text:', error);
});
```

### Using Axios with Different Chunking Methods

```javascript
const axios = require('axios');

class TextChunker {
  constructor(baseURL = 'http://localhost:8080', apiKey = null) {
    this.client = axios.create({
      baseURL: baseURL,
      headers: {
        'Content-Type': 'application/json',
        ...(apiKey && { 'X-API-Key': apiKey })
      },
      timeout: 60000 // 60 second timeout for long texts
    });
  }

  async chunkText(text, modelName, options = {}) {
    // Validate input
    if (!text || typeof text !== 'string' || text.trim() === '') {
      throw new Error('Text must be a non-empty string');
    }
    
    if (!modelName || typeof modelName !== 'string') {
      throw new Error('Model name must be a valid string');
    }

    const requestData = {
      model_name: modelName,
      text: text.trim(),
      method: options.method || 'regular',
      chunk_size: options.chunkSize || 128,
      max_chunk_size: options.maxChunkSize || 512,
      overlap: options.overlap || 64,
      similarity_threshold: options.similarityThreshold || 0.7
    };

    try {
      const response = await this.client.post('/chunking', requestData);
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

  async regularChunking(text, modelName, chunkSize = 128, overlap = 64) {
    return this.chunkText(text, modelName, {
      method: 'regular',
      chunkSize,
      overlap
    });
  }

  async semanticChunking(text, modelName, options = {}) {
    return this.chunkText(text, modelName, {
      method: 'semantic',
      chunkSize: options.chunkSize || 128,
      maxChunkSize: options.maxChunkSize || 512,
      overlap: options.overlap || 64,
      similarityThreshold: options.similarityThreshold || 0.7
    });
  }

  async compareChunkingMethods(text, modelName, chunkSize = 128) {
    try {
      console.log('🔄 Comparing chunking methods...\n');

      // Regular chunking
      const regularResult = await this.regularChunking(text, modelName, chunkSize, 32);
      
      // Semantic chunking
      const semanticResult = await this.semanticChunking(text, modelName, {
        chunkSize,
        similarityThreshold: 0.6
      });

      const comparison = {
        original_text_length: text.length,
        original_tokens: regularResult.usage.original_tokens,
        
        regular: {
          chunks: regularResult.total_chunks,
          processing_time: regularResult.usage.processing_time_ms,
          total_tokens: regularResult.usage.total_chunk_tokens,
          avg_chunk_length: regularResult.chunks.reduce((sum, chunk) => sum + chunk.text.length, 0) / regularResult.total_chunks,
          avg_tokens_per_chunk: regularResult.usage.total_chunk_tokens / regularResult.total_chunks
        },
        
        semantic: {
          chunks: semanticResult.total_chunks,
          processing_time: semanticResult.usage.processing_time_ms,
          total_tokens: semanticResult.usage.total_chunk_tokens,
          avg_chunk_length: semanticResult.chunks.reduce((sum, chunk) => sum + chunk.text.length, 0) / semanticResult.total_chunks,
          avg_tokens_per_chunk: semanticResult.usage.total_chunk_tokens / semanticResult.total_chunks
        }
      };

      console.log('📊 Chunking Method Comparison:');
      console.log(`Original text: ${comparison.original_text_length} chars, ${comparison.original_tokens} tokens\n`);
      
      console.log('📋 Regular Chunking:');
      console.log(`  Chunks: ${comparison.regular.chunks}`);
      console.log(`  Processing time: ${comparison.regular.processing_time.toFixed(2)}ms`);
      console.log(`  Avg chunk length: ${comparison.regular.avg_chunk_length.toFixed(0)} chars`);
      console.log(`  Avg tokens per chunk: ${comparison.regular.avg_tokens_per_chunk.toFixed(1)}`);
      
      console.log('\n🧠 Semantic Chunking:');
      console.log(`  Chunks: ${comparison.semantic.chunks}`);
      console.log(`  Processing time: ${comparison.semantic.processing_time.toFixed(2)}ms`);
      console.log(`  Avg chunk length: ${comparison.semantic.avg_chunk_length.toFixed(0)} chars`);
      console.log(`  Avg tokens per chunk: ${comparison.semantic.avg_tokens_per_chunk.toFixed(1)}`);

      return {
        regular: regularResult,
        semantic: semanticResult,
        comparison
      };

    } catch (error) {
      console.error('❌ Comparison failed:', error.message);
      throw error;
    }
  }

  async batchChunkTexts(texts, modelName, options = {}) {
    const results = [];
    const batchSize = options.batchSize || 5;
    
    console.log(`📦 Processing ${texts.length} texts in batches of ${batchSize}`);
    
    for (let i = 0; i < texts.length; i += batchSize) {
      const batch = texts.slice(i, i + batchSize);
      const batchNumber = Math.floor(i / batchSize) + 1;
      const totalBatches = Math.ceil(texts.length / batchSize);
      
      console.log(`Processing batch ${batchNumber}/${totalBatches}`);
      
      const batchPromises = batch.map(async (text, index) => {
        try {
          const result = await this.chunkText(text, modelName, options);
          return {
            index: i + index,
            success: true,
            result,
            originalLength: text.length
          };
        } catch (error) {
          console.error(`Failed to chunk text ${i + index}:`, error.message);
          return {
            index: i + index,
            success: false,
            error: error.message,
            originalLength: text.length
          };
        }
      });
      
      const batchResults = await Promise.all(batchPromises);
      results.push(...batchResults);
      
      // Small delay between batches
      if (i + batchSize < texts.length) {
        await new Promise(resolve => setTimeout(resolve, 100));
      }
    }
    
    const successful = results.filter(r => r.success);
    const failed = results.filter(r => !r.success);
    
    console.log(`\n✅ Successfully processed: ${successful.length}/${texts.length}`);
    console.log(`❌ Failed: ${failed.length}/${texts.length}`);
    
    return {
      results: successful.map(r => r.result),
      failed: failed,
      summary: {
        total: texts.length,
        successful: successful.length,
        failed: failed.length,
        totalChunks: successful.reduce((sum, r) => sum + r.result.total_chunks, 0),
        totalProcessingTime: successful.reduce((sum, r) => sum + r.result.usage.processing_time_ms, 0)
      }
    };
  }
}

// Example usage
const chunker = new TextChunker('http://localhost:8080');

async function demonstrateChunking() {
  const sampleTexts = [
    `Machine learning is a method of data analysis that automates analytical model building. It is a branch of artificial intelligence based on the idea that systems can learn from data, identify patterns and make decisions with minimal human intervention.`,
    
    `Deep learning is part of a broader family of machine learning methods based on artificial neural networks with representation learning. Learning can be supervised, semi-supervised or unsupervised.`,
    
    `Natural language processing (NLP) is a subfield of linguistics, computer science, and artificial intelligence concerned with the interactions between computers and human language, in particular how to program computers to process and analyze large amounts of natural language data.`
  ];

  try {
    // Single text chunking with different methods
    console.log('🔍 Comparing chunking methods on sample text...\n');
    const comparison = await chunker.compareChunkingMethods(sampleTexts[0], 'text-embedding-3-small', 50);
    
    // Batch processing
    console.log('\n📦 Batch processing multiple texts...\n');
    const batchResult = await chunker.batchChunkTexts(sampleTexts, 'text-embedding-3-small', {
      method: 'semantic',
      chunkSize: 75,
      similarityThreshold: 0.65
    });
    
    console.log('\n📈 Batch Summary:');
    console.log(`Total chunks generated: ${batchResult.summary.totalChunks}`);
    console.log(`Total processing time: ${batchResult.summary.totalProcessingTime.toFixed(2)}ms`);
    console.log(`Average time per text: ${(batchResult.summary.totalProcessingTime / batchResult.summary.successful).toFixed(2)}ms`);

  } catch (error) {
    console.error('❌ Demo failed:', error.message);
  }
}

demonstrateChunking();
```

### Node.js with File Processing

```javascript
const fs = require('fs').promises;
const path = require('path');
const https = require('https');
const http = require('http');
const url = require('url');

function chunkTextRequest(serverUrl, text, modelName, options = {}) {
  return new Promise((resolve, reject) => {
    const parsedUrl = url.parse(serverUrl);
    const requestModule = parsedUrl.protocol === 'https:' ? https : http;
    
    const requestData = JSON.stringify({
      model_name: modelName,
      text: text,
      method: options.method || 'regular',
      chunk_size: options.chunkSize || 128,
      max_chunk_size: options.maxChunkSize || 512,
      overlap: options.overlap || 64,
      similarity_threshold: options.similarityThreshold || 0.7
    });

    const requestOptions = {
      hostname: parsedUrl.hostname,
      port: parsedUrl.port,
      path: '/chunking',
      method: 'POST',
      headers: {
        'Content-Type': 'application/json',
        'Content-Length': Buffer.byteLength(requestData),
        ...(options.apiKey && { 'X-API-Key': options.apiKey })
      }
    };

    const req = requestModule.request(requestOptions, (res) => {
      let responseBody = '';

      res.on('data', (chunk) => {
        responseBody += chunk;
      });

      res.on('end', () => {
        try {
          const parsedResponse = JSON.parse(responseBody);
          
          if (res.statusCode >= 200 && res.statusCode < 300) {
            resolve(parsedResponse);
          } else {
            reject(new Error(`API Error (${res.statusCode}): ${parsedResponse.error?.message || 'Unknown error'}`));
          }
        } catch (error) {
          reject(new Error(`Failed to parse response: ${error.message}`));
        }
      });
    });

    req.on('error', (error) => {
      reject(new Error(`Request failed: ${error.message}`));
    });

    req.on('timeout', () => {
      req.destroy();
      reject(new Error('Request timeout'));
    });

    req.setTimeout(120000); // 2 minute timeout for very long texts
    req.write(requestData);
    req.end();
  });
}

async function processTextFile(filePath, serverUrl, modelName, options = {}) {
  try {
    console.log(`📄 Processing file: ${filePath}`);
    
    // Read the file
    const text = await fs.readFile(filePath, 'utf-8');
    console.log(`File size: ${text.length} characters`);
    
    // Chunk the text
    const startTime = Date.now();
    const result = await chunkTextRequest(serverUrl, text, modelName, options);
    const endTime = Date.now();
    
    console.log(`✅ Chunking completed in ${endTime - startTime}ms`);
    console.log(`Generated ${result.total_chunks} chunks using ${result.method} method`);
    
    // Save results
    const outputDir = options.outputDir || './chunking-output';
    await fs.mkdir(outputDir, { recursive: true });
    
    const timestamp = new Date().toISOString().replace(/[:.]/g, '-');
    const basename = path.basename(filePath, path.extname(filePath));
    const outputFile = path.join(outputDir, `${basename}-chunks-${timestamp}.json`);
    
    const outputData = {
      source_file: filePath,
      processed_at: new Date().toISOString(),
      chunking_options: {
        model_name: modelName,
        method: options.method || 'regular',
        chunk_size: options.chunkSize || 128,
        overlap: options.overlap || 64
      },
      results: result
    };
    
    await fs.writeFile(outputFile, JSON.stringify(outputData, null, 2));
    console.log(`💾 Results saved to: ${outputFile}`);
    
    // Create a text summary
    const summaryFile = path.join(outputDir, `${basename}-summary-${timestamp}.txt`);
    let summary = `Text Chunking Summary\n`;
    summary += `=====================\n\n`;
    summary += `Source File: ${filePath}\n`;
    summary += `Processed: ${new Date().toISOString()}\n`;
    summary += `Model: ${result.model_name}\n`;
    summary += `Method: ${result.method}\n`;
    summary += `Total Chunks: ${result.total_chunks}\n`;
    summary += `Original Tokens: ${result.usage.original_tokens}\n`;
    summary += `Total Chunk Tokens: ${result.usage.total_chunk_tokens}\n`;
    summary += `Processing Time: ${result.usage.processing_time_ms}ms\n\n`;
    
    summary += `Chunk Details:\n`;
    summary += `==============\n\n`;
    
    result.chunks.forEach((chunk, index) => {
      summary += `Chunk ${index + 1} (${chunk.token_count} tokens):\n`;
      summary += `${chunk.text.substring(0, 200)}${chunk.text.length > 200 ? '...' : ''}\n\n`;
      summary += `---\n\n`;
    });
    
    await fs.writeFile(summaryFile, summary);
    console.log(`📋 Summary saved to: ${summaryFile}`);
    
    return {
      result,
      outputFile,
      summaryFile
    };
    
  } catch (error) {
    console.error(`❌ Failed to process file ${filePath}:`, error.message);
    throw error;
  }
}

async function batchProcessFiles(directory, serverUrl, modelName, options = {}) {
  try {
    console.log(`📁 Processing files in directory: ${directory}`);
    
    const files = await fs.readdir(directory);
    const textFiles = files.filter(file => 
      file.endsWith('.txt') || file.endsWith('.md') || file.endsWith('.csv')
    );
    
    if (textFiles.length === 0) {
      console.log('No text files found to process.');
      return;
    }
    
    console.log(`Found ${textFiles.length} text files to process`);
    
    const results = [];
    
    for (const file of textFiles) {
      const filePath = path.join(directory, file);
      
      try {
        const result = await processTextFile(filePath, serverUrl, modelName, options);
        results.push({
          file: file,
          success: true,
          chunks: result.result.total_chunks,
          processingTime: result.result.usage.processing_time_ms
        });
        
        // Small delay between files
        await new Promise(resolve => setTimeout(resolve, 500));
        
      } catch (error) {
        console.error(`❌ Failed to process ${file}:`, error.message);
        results.push({
          file: file,
          success: false,
          error: error.message
        });
      }
    }
    
    // Summary report
    const successful = results.filter(r => r.success);
    const failed = results.filter(r => !r.success);
    
    console.log(`\n📊 Batch Processing Summary:`);
    console.log(`Total files: ${textFiles.length}`);
    console.log(`Successfully processed: ${successful.length}`);
    console.log(`Failed: ${failed.length}`);
    
    if (successful.length > 0) {
      const totalChunks = successful.reduce((sum, r) => sum + r.chunks, 0);
      const totalTime = successful.reduce((sum, r) => sum + r.processingTime, 0);
      console.log(`Total chunks generated: ${totalChunks}`);
      console.log(`Average chunks per file: ${(totalChunks / successful.length).toFixed(1)}`);
      console.log(`Total processing time: ${totalTime.toFixed(2)}ms`);
      console.log(`Average time per file: ${(totalTime / successful.length).toFixed(2)}ms`);
    }
    
    if (failed.length > 0) {
      console.log('\n❌ Failed files:');
      failed.forEach(f => console.log(`  ${f.file}: ${f.error}`));
    }
    
    return results;
    
  } catch (error) {
    console.error('❌ Batch processing failed:', error.message);
    throw error;
  }
}

// Example usage
async function main() {
  const serverUrl = 'http://localhost:8080';
  const modelName = 'text-embedding-3-small';
  
  try {
    // Process a single file
    if (process.argv[2] && process.argv[2].endsWith('.txt')) {
      await processTextFile(process.argv[2], serverUrl, modelName, {
        method: 'semantic',
        chunkSize: 150,
        similarityThreshold: 0.6,
        outputDir: './output'
      });
    } else {
      // Process directory
      const directory = process.argv[2] || './documents';
      await batchProcessFiles(directory, serverUrl, modelName, {
        method: 'regular',
        chunkSize: 128,
        overlap: 32,
        outputDir: './chunking-results'
      });
    }
    
  } catch (error) {
    console.error('❌ Operation failed:', error.message);
    process.exit(1);
  }
}

// Run if called directly
if (require.main === module) {
  main();
}

module.exports = {
  chunkTextRequest,
  processTextFile,
  batchProcessFiles
};
```

### Browser Implementation with Interactive Chunking

```html
<!DOCTYPE html>
<html>
<head>
    <title>Text Chunking Tool</title>
    <style>
        .container { max-width: 1200px; margin: 20px auto; padding: 20px; }
        .input-section { margin: 20px 0; }
        .text-input { width: 100%; height: 200px; margin: 10px 0; padding: 10px; border: 1px solid #ddd; border-radius: 4px; font-family: monospace; }
        .controls { display: grid; grid-template-columns: repeat(auto-fit, minmax(200px, 1fr)); gap: 15px; margin: 20px 0; }
        .control-group { background: #f8f9fa; padding: 15px; border-radius: 8px; }
        .control-group label { display: block; margin-bottom: 5px; font-weight: bold; }
        .control-group input, .control-group select { width: 100%; padding: 8px; border: 1px solid #ddd; border-radius: 4px; }
        .button { padding: 10px 20px; border: none; border-radius: 5px; cursor: pointer; margin: 5px; }
        .primary { background: #007bff; color: white; }
        .secondary { background: #6c757d; color: white; }
        .success { background: #28a745; color: white; }
        .warning { background: #ffc107; color: black; }
        .stats { display: grid; grid-template-columns: repeat(auto-fit, minmax(150px, 1fr)); gap: 15px; margin: 20px 0; }
        .stat-card { background: white; padding: 15px; border-radius: 8px; box-shadow: 0 2px 4px rgba(0,0,0,0.1); border-left: 4px solid #007bff; text-align: center; }
        .chunks-container { margin: 20px 0; }
        .chunk { background: white; border: 1px solid #ddd; border-radius: 8px; padding: 15px; margin: 10px 0; box-shadow: 0 2px 4px rgba(0,0,0,0.1); }
        .chunk-header { display: flex; justify-content: space-between; align-items: center; margin-bottom: 10px; padding-bottom: 10px; border-bottom: 1px solid #eee; }
        .chunk-text { background: #f8f9fa; padding: 10px; border-radius: 4px; white-space: pre-wrap; font-family: monospace; font-size: 14px; line-height: 1.4; max-height: 150px; overflow-y: auto; }
        .chunk-text.expanded { max-height: none; }
        .loading { text-align: center; padding: 40px; color: #6c757d; }
        .error { background: #f8d7da; border: 1px solid #f5c6cb; color: #721c24; padding: 15px; border-radius: 4px; margin: 15px 0; }
        .method-comparison { display: grid; grid-template-columns: 1fr 1fr; gap: 20px; margin: 20px 0; }
        .comparison-section { background: #f8f9fa; padding: 15px; border-radius: 8px; }
        .progress { margin: 20px 0; }
        .progress-bar { width: 100%; height: 20px; background: #f0f0f0; border-radius: 10px; overflow: hidden; }
        .progress-fill { height: 100%; background: #4CAF50; transition: width 0.3s; }
    </style>
</head>
<body>
    <div class="container">
        <h1>🔧 Text Chunking Tool</h1>
        
        <div class="input-section">
            <h3>Input Text</h3>
            <textarea 
                id="input-text" 
                class="text-input"
                placeholder="Enter your text here to be chunked. You can paste long articles, documents, or any text content that needs to be split into manageable pieces for processing by language models..."
            ></textarea>
        </div>
        
        <div class="controls">
            <div class="control-group">
                <label for="model-name">Model Name</label>
                <input type="text" id="model-name" value="text-embedding-3-small" placeholder="e.g., text-embedding-3-small">
            </div>
            
            <div class="control-group">
                <label for="chunking-method">Chunking Method</label>
                <select id="chunking-method" onchange="toggleMethodOptions()">
                    <option value="regular">Regular (Token-based)</option>
                    <option value="semantic">Semantic (Context-aware)</option>
                </select>
            </div>
            
            <div class="control-group">
                <label for="chunk-size">Chunk Size (tokens)</label>
                <input type="number" id="chunk-size" value="128" min="1" max="2048">
            </div>
            
            <div class="control-group">
                <label for="overlap">Overlap (tokens)</label>
                <input type="number" id="overlap" value="64" min="0" max="512">
            </div>
            
            <div class="control-group" id="semantic-options" style="display: none;">
                <label for="max-chunk-size">Max Chunk Size</label>
                <input type="number" id="max-chunk-size" value="512" min="1" max="4096">
            </div>
            
            <div class="control-group" id="semantic-threshold" style="display: none;">
                <label for="similarity-threshold">Similarity Threshold</label>
                <input type="number" id="similarity-threshold" value="0.7" min="0" max="1" step="0.1">
            </div>
        </div>
        
        <div style="text-align: center; margin: 20px 0;">
            <button class="button primary" onclick="performChunking()" id="chunk-btn">🔧 Chunk Text</button>
            <button class="button secondary" onclick="compareChunkingMethods()" id="compare-btn">📊 Compare Methods</button>
            <button class="button success" onclick="exportResults()" id="export-btn" disabled>💾 Export Results</button>
            <button class="button warning" onclick="loadSampleText()">📝 Load Sample Text</button>
        </div>
        
        <div class="progress" id="progress" style="display: none;">
            <div class="progress-bar">
                <div class="progress-fill" id="progress-fill" style="width: 0%;"></div>
            </div>
            <div id="progress-text" style="text-align: center; margin-top: 10px;">Processing...</div>
        </div>
        
        <div class="stats" id="stats" style="display: none;">
            <div class="stat-card">
                <strong id="total-chunks">0</strong><br>
                Total Chunks
            </div>
            <div class="stat-card">
                <strong id="original-tokens">0</strong><br>
                Original Tokens
            </div>
            <div class="stat-card">
                <strong id="total-chunk-tokens">0</strong><br>
                Chunk Tokens
            </div>
            <div class="stat-card">
                <strong id="processing-time">0</strong><br>
                Processing Time (ms)
            </div>
            <div class="stat-card">
                <strong id="avg-tokens-per-chunk">0</strong><br>
                Avg Tokens/Chunk
            </div>
        </div>
        
        <div id="results-container"></div>
        <div id="error-container"></div>
    </div>

    <script>
        let currentResults = null;

        function toggleMethodOptions() {
            const method = document.getElementById('chunking-method').value;
            const semanticOptions = document.getElementById('semantic-options');
            const semanticThreshold = document.getElementById('semantic-threshold');
            
            if (method === 'semantic') {
                semanticOptions.style.display = 'block';
                semanticThreshold.style.display = 'block';
            } else {
                semanticOptions.style.display = 'none';
                semanticThreshold.style.display = 'none';
            }
        }

        function loadSampleText() {
            const sampleTexts = [
                `Artificial intelligence (AI) is intelligence demonstrated by machines, in contrast to the natural intelligence displayed by humans and animals. Leading AI textbooks define the field as the study of "intelligent agents": any device that perceives its environment and takes actions that maximize its chance of successfully achieving its goals.

Machine learning is a subset of AI that focuses on the development of algorithms that can learn from and make decisions or predictions based on data. These algorithms build a model based on training data in order to make predictions or decisions without being explicitly programmed to do so.

Deep learning is part of a broader family of machine learning methods based on artificial neural networks with representation learning. Learning can be supervised, semi-supervised or unsupervised. Deep learning architectures such as deep neural networks, deep belief networks, recurrent neural networks and convolutional neural networks have been applied to fields including computer vision, speech recognition, natural language processing, and bioinformatics.

Natural language processing (NLP) is a subfield of linguistics, computer science, and artificial intelligence concerned with the interactions between computers and human language, in particular how to program computers to process and analyze large amounts of natural language data. The goal is a computer capable of understanding the contents of documents, including the contextual nuances of the language within them.`,
                
                `Quantum computing is a type of computation that harnesses the collective properties of quantum states, such as superposition, interference, and entanglement, to perform calculations. The devices that perform quantum computations are known as quantum computers.

Quantum computers are believed to be able to solve certain computational problems, such as integer factorization, substantially faster than classical computers. The study of quantum computing is a subfield of quantum information science.

Quantum computing began in the early 1980s, when physicist Paul Benioff proposed a quantum mechanical model of the Turing machine. Richard Feynman and Yuri Manin later suggested that a quantum computer had the potential to simulate things that a classical computer could not.`
            ];
            
            const randomSample = sampleTexts[Math.floor(Math.random() * sampleTexts.length)];
            document.getElementById('input-text').value = randomSample;
        }

        async function performChunking() {
            const text = document.getElementById('input-text').value.trim();
            const modelName = document.getElementById('model-name').value.trim();
            
            if (!text) {
                alert('Please enter some text to chunk.');
                return;
            }
            
            if (!modelName) {
                alert('Please enter a model name.');
                return;
            }

            const method = document.getElementById('chunking-method').value;
            const chunkSize = parseInt(document.getElementById('chunk-size').value);
            const overlap = parseInt(document.getElementById('overlap').value);
            const maxChunkSize = parseInt(document.getElementById('max-chunk-size').value);
            const similarityThreshold = parseFloat(document.getElementById('similarity-threshold').value);

            const requestBody = {
                model_name: modelName,
                text: text,
                method: method,
                chunk_size: chunkSize,
                overlap: overlap,
                ...(method === 'semantic' && {
                    max_chunk_size: maxChunkSize,
                    similarity_threshold: similarityThreshold
                })
            };

            const chunkBtn = document.getElementById('chunk-btn');
            const compareBtn = document.getElementById('compare-btn');
            const exportBtn = document.getElementById('export-btn');
            const progress = document.getElementById('progress');
            const progressFill = document.getElementById('progress-fill');
            const progressText = document.getElementById('progress-text');
            const errorContainer = document.getElementById('error-container');

            chunkBtn.disabled = true;
            compareBtn.disabled = true;
            exportBtn.disabled = true;
            chunkBtn.textContent = '🔄 Processing...';
            progress.style.display = 'block';
            errorContainer.innerHTML = '';

            // Simulate progress
            let progressValue = 0;
            const progressInterval = setInterval(() => {
                progressValue += 10;
                if (progressValue <= 90) {
                    progressFill.style.width = progressValue + '%';
                    progressText.textContent = `Processing... ${progressValue}%`;
                }
            }, 200);

            try {
                const response = await fetch('http://localhost:8080/chunking', {
                    method: 'POST',
                    headers: {
                        'Content-Type': 'application/json',
                    },
                    body: JSON.stringify(requestBody)
                });

                clearInterval(progressInterval);
                progressFill.style.width = '100%';
                progressText.textContent = 'Completed!';

                if (!response.ok) {
                    const errorData = await response.json();
                    throw new Error(errorData.error?.message || 'Chunking failed');
                }

                const result = await response.json();
                currentResults = result;
                displayResults(result);
                updateStats(result);
                exportBtn.disabled = false;

            } catch (error) {
                clearInterval(progressInterval);
                errorContainer.innerHTML = `<div class="error">❌ Chunking failed: ${error.message}</div>`;
            } finally {
                chunkBtn.disabled = false;
                compareBtn.disabled = false;
                chunkBtn.textContent = '🔧 Chunk Text';
                setTimeout(() => {
                    progress.style.display = 'none';
                }, 2000);
            }
        }

        function updateStats(result) {
            document.getElementById('total-chunks').textContent = result.total_chunks;
            document.getElementById('original-tokens').textContent = result.usage.original_tokens;
            document.getElementById('total-chunk-tokens').textContent = result.usage.total_chunk_tokens;
            document.getElementById('processing-time').textContent = result.usage.processing_time_ms.toFixed(1);
            
            const avgTokens = result.total_chunks > 0 ? 
                (result.usage.total_chunk_tokens / result.total_chunks).toFixed(1) : 0;
            document.getElementById('avg-tokens-per-chunk').textContent = avgTokens;
            
            document.getElementById('stats').style.display = 'grid';
        }

        function displayResults(result) {
            const container = document.getElementById('results-container');
            
            let html = `
                <h3>📄 Chunking Results</h3>
                <div style="background: #e7f3ff; padding: 15px; border-radius: 8px; margin: 15px 0;">
                    <strong>Model:</strong> ${result.model_name} | 
                    <strong>Method:</strong> ${result.method} | 
                    <strong>Chunks:</strong> ${result.total_chunks}
                </div>
                <div class="chunks-container">
            `;

            result.chunks.forEach((chunk, index) => {
                const isLong = chunk.text.length > 300;
                html += `
                    <div class="chunk" id="chunk-${index}">
                        <div class="chunk-header">
                            <div>
                                <strong>Chunk ${index + 1}</strong>
                                <span style="color: #6c757d; margin-left: 10px;">
                                    ${chunk.token_count} tokens | ${chunk.text.length} chars
                                </span>
                            </div>
                            <div>
                                <button class="button" style="background: #ffc107; color: black; padding: 5px 10px; font-size: 12px;" 
                                        onclick="copyChunkText(${index})">Copy</button>
                                ${isLong ? 
                                    `<button class="button" style="background: #17a2b8; color: white; padding: 5px 10px; font-size: 12px;" 
                                             onclick="toggleChunkExpansion(${index})" id="expand-btn-${index}">Expand</button>` : ''}
                            </div>
                        </div>
                        <div class="chunk-text" id="chunk-text-${index}">
                            ${escapeHtml(chunk.text)}
                        </div>
                    </div>
                `;
            });

            html += '</div>';
            container.innerHTML = html;
        }

        function toggleChunkExpansion(index) {
            const chunkText = document.getElementById(`chunk-text-${index}`);
            const expandBtn = document.getElementById(`expand-btn-${index}`);
            
            if (chunkText.classList.contains('expanded')) {
                chunkText.classList.remove('expanded');
                expandBtn.textContent = 'Expand';
            } else {
                chunkText.classList.add('expanded');
                expandBtn.textContent = 'Collapse';
            }
        }

        function copyChunkText(index) {
            const text = currentResults.chunks[index].text;
            navigator.clipboard.writeText(text).then(() => {
                // Could add a toast notification here
                console.log('Chunk text copied to clipboard');
            });
        }

        function escapeHtml(text) {
            const div = document.createElement('div');
            div.textContent = text;
            return div.innerHTML;
        }

        async function compareChunkingMethods() {
            const text = document.getElementById('input-text').value.trim();
            const modelName = document.getElementById('model-name').value.trim();
            
            if (!text || !modelName) {
                alert('Please enter text and model name first.');
                return;
            }

            const chunkSize = 100; // Fixed size for comparison
            
            try {
                document.getElementById('results-container').innerHTML = '<div class="loading">🔄 Comparing chunking methods...</div>';
                
                // Regular chunking
                const regularResponse = await fetch('http://localhost:8080/chunking', {
                    method: 'POST',
                    headers: { 'Content-Type': 'application/json' },
                    body: JSON.stringify({
                        model_name: modelName,
                        text: text,
                        method: 'regular',
                        chunk_size: chunkSize,
                        overlap: 32
                    })
                });
                
                const regularResult = await regularResponse.json();
                
                // Semantic chunking
                const semanticResponse = await fetch('http://localhost:8080/chunking', {
                    method: 'POST',
                    headers: { 'Content-Type': 'application/json' },
                    body: JSON.stringify({
                        model_name: modelName,
                        text: text,
                        method: 'semantic',
                        chunk_size: chunkSize,
                        max_chunk_size: 300,
                        overlap: 32,
                        similarity_threshold: 0.6
                    })
                });
                
                const semanticResult = await semanticResponse.json();
                
                displayComparison(regularResult, semanticResult);
                
            } catch (error) {
                document.getElementById('error-container').innerHTML = 
                    `<div class="error">❌ Comparison failed: ${error.message}</div>`;
            }
        }

        function displayComparison(regularResult, semanticResult) {
            const container = document.getElementById('results-container');
            
            let html = `
                <h3>📊 Method Comparison</h3>
                <div class="method-comparison">
                    <div class="comparison-section">
                        <h4>📋 Regular Chunking</h4>
                        <div><strong>Chunks:</strong> ${regularResult.total_chunks}</div>
                        <div><strong>Processing time:</strong> ${regularResult.usage.processing_time_ms.toFixed(2)}ms</div>
                        <div><strong>Avg tokens/chunk:</strong> ${(regularResult.usage.total_chunk_tokens / regularResult.total_chunks).toFixed(1)}</div>
                        <div><strong>Total tokens:</strong> ${regularResult.usage.total_chunk_tokens}</div>
                    </div>
                    <div class="comparison-section">
                        <h4>🧠 Semantic Chunking</h4>
                        <div><strong>Chunks:</strong> ${semanticResult.total_chunks}</div>
                        <div><strong>Processing time:</strong> ${semanticResult.usage.processing_time_ms.toFixed(2)}ms</div>
                        <div><strong>Avg tokens/chunk:</strong> ${(semanticResult.usage.total_chunk_tokens / semanticResult.total_chunks).toFixed(1)}</div>
                        <div><strong>Total tokens:</strong> ${semanticResult.usage.total_chunk_tokens}</div>
                    </div>
                </div>
                
                <div style="margin: 20px 0;">
                    <button class="button primary" onclick="showRegularChunks()">Show Regular Chunks</button>
                    <button class="button primary" onclick="showSemanticChunks()">Show Semantic Chunks</button>
                </div>
                
                <div id="chunk-display"></div>
            `;
            
            container.innerHTML = html;
            
            // Store results for display
            window.comparisonResults = {
                regular: regularResult,
                semantic: semanticResult
            };
        }

        function showRegularChunks() {
            currentResults = window.comparisonResults.regular;
            displayChunksOnly(window.comparisonResults.regular, 'Regular');
        }

        function showSemanticChunks() {
            currentResults = window.comparisonResults.semantic;
            displayChunksOnly(window.comparisonResults.semantic, 'Semantic');
        }

        function displayChunksOnly(result, methodName) {
            const display = document.getElementById('chunk-display');
            
            let html = `<h4>📄 ${methodName} Chunks</h4><div class="chunks-container">`;
            
            result.chunks.forEach((chunk, index) => {
                html += `
                    <div class="chunk">
                        <div class="chunk-header">
                            <div>
                                <strong>Chunk ${index + 1}</strong>
                                <span style="color: #6c757d; margin-left: 10px;">
                                    ${chunk.token_count} tokens | ${chunk.text.length} chars
                                </span>
                            </div>
                        </div>
                        <div class="chunk-text">
                            ${escapeHtml(chunk.text)}
                        </div>
                    </div>
                `;
            });
            
            html += '</div>';
            display.innerHTML = html;
        }

        function exportResults() {
            if (!currentResults) {
                alert('No results to export');
                return;
            }

            const exportData = {
                export_metadata: {
                    timestamp: new Date().toISOString(),
                    export_type: 'text_chunking',
                    original_text_length: document.getElementById('input-text').value.length
                },
                chunking_results: currentResults
            };

            const dataStr = JSON.stringify(exportData, null, 2);
            const dataBlob = new Blob([dataStr], {type: 'application/json'});
            const url = URL.createObjectURL(dataBlob);
            
            const link = document.createElement('a');
            link.href = url;
            link.download = `chunking-results-${new Date().toISOString().split('T')[0]}.json`;
            document.body.appendChild(link);
            link.click();
            document.body.removeChild(link);
            URL.revokeObjectURL(url);
        }

        // Initialize
        toggleMethodOptions();
    </script>
</body>
</html>
```

## Common Error Scenarios

### 1. Empty Text

```javascript
// This will return a 400 error
const response = await fetch('/chunking', {
  method: 'POST',
  headers: { 'Content-Type': 'application/json' },
  body: JSON.stringify({
    model_name: "text-embedding-3-small",
    text: ""
  })
});
// Error: "Invalid request parameters"
```

### 2. Invalid Model Name

```javascript
// This will return a 404 error
const response = await fetch('/chunking', {
  method: 'POST',
  headers: { 'Content-Type': 'application/json' },
  body: JSON.stringify({
    model_name: "nonexistent-model",
    text: "Some text to chunk"
  })
});
// Error: "Model 'nonexistent-model' not found or could not be loaded"
```

### 3. Invalid Parameters

```javascript
// This will return a 400 error
const response = await fetch('/chunking', {
  method: 'POST',
  headers: { 'Content-Type': 'application/json' },
  body: JSON.stringify({
    model_name: "text-embedding-3-small",
    text: "Some text",
    chunk_size: 5000 // Exceeds maximum of 2048
  })
});
// Error: "Invalid request parameters"
```

### 4. Invalid Method

```javascript
// This will return a 400 error
const response = await fetch('/chunking', {
  method: 'POST',
  headers: { 'Content-Type': 'application/json' },
  body: JSON.stringify({
    model_name: "text-embedding-3-small",
    text: "Some text",
    method: "invalid_method"
  })
});
// Error: "Invalid method: must be 'regular' or 'semantic'"
```

## Best Practices

1. **Text Length**: For very long texts (>100KB), consider processing in smaller sections
2. **Chunk Size**: Choose appropriate chunk sizes based on your downstream processing needs
3. **Overlap**: Use overlap to maintain context continuity across chunks
4. **Method Selection**: Use semantic chunking for better content coherence, regular for speed
5. **Model Selection**: Ensure the model is loaded and available before making requests
6. **Timeout Handling**: Set appropriate timeouts for long text processing
7. **Error Handling**: Always handle both network and API errors gracefully

## Response Headers

The API includes CORS headers for cross-origin requests:

```http
Access-Control-Allow-Origin: *
Access-Control-Allow-Methods: POST, OPTIONS
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

1. **Document Processing**: Split long documents for embedding generation
2. **RAG Systems**: Prepare text for retrieval-augmented generation
3. **Content Analysis**: Break down text for detailed analysis
4. **Token Management**: Ensure text fits within model token limits
5. **Semantic Search**: Create semantically coherent chunks for better search results
6. **Batch Processing**: Process multiple documents efficiently
7. **API Integration**: Prepare text for other AI service APIs with token limits

## Performance Considerations

- **Regular chunking** is faster and more predictable
- **Semantic chunking** provides better content quality but takes longer
- Processing time scales with text length and complexity
- Consider batch processing for multiple texts
- Monitor token usage for cost optimization
