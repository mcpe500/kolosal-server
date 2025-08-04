# Kolosal Server - Add Documents API Documentation

## Overview

The `/add_documents` API endpoint allows you to add multiple documents to the vector database for semantic search and retrieval. Documents are automatically processed, embedded, and indexed for later retrieval.

**Endpoint:** `POST /add_documents`  
**Content-Type:** `application/json`

## Request Format

### Request Body Schema

```json
{
  "documents": [
    {
      "text": "string (required)",
      "metadata": "object (optional)"
    }
  ]
}
```

### Request Parameters

| Parameter | Type | Required | Description |
|-----------|------|----------|-------------|
| `documents` | array | Yes | Array of document objects to be indexed |
| `documents[].text` | string | Yes | The text content of the document |
| `documents[].metadata` | object | No | Additional metadata associated with the document |

### Metadata Field

The `metadata` field can contain any JSON object with key-value pairs. Common metadata fields include:

- `source`: Source of the document (e.g., "web", "pdf", "database")
- `author`: Author of the document
- `title`: Title of the document
- `created_at`: Creation timestamp
- `category`: Document category
- `tags`: Array of tags

## Response Format

### Success Response (200 OK)

```json
{
  "results": [
    {
      "id": "string",
      "success": "boolean",
      "error": "string"
    }
  ],
  "successful_count": "integer",
  "failed_count": "integer",
  "collection_name": "string"
}
```

### Error Response (4xx/5xx)

```json
{
  "error": "string",
  "error_type": "string",
  "param": "string",
  "code": "string"
}
```

## JavaScript Examples

### Basic Usage with Fetch API

```javascript
async function addDocuments(documents) {
  const requestBody = {
    documents: documents
  };

  try {
    const response = await fetch('http://localhost:8080/add_documents', {
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
      throw new Error(`API Error (${response.status}): ${errorData.error}`);
    }

    const data = await response.json();
    return data;
  } catch (error) {
    console.error('Add documents failed:', error);
    throw error;
  }
}

// Example usage
const documentsToAdd = [
  {
    text: "Machine learning is a subset of artificial intelligence that focuses on algorithms that can learn from data.",
    metadata: {
      title: "Introduction to Machine Learning",
      category: "AI",
      source: "textbook",
      chapter: 1
    }
  },
  {
    text: "Deep learning uses neural networks with multiple layers to model and understand complex patterns.",
    metadata: {
      title: "Deep Learning Fundamentals",
      category: "AI",
      source: "research_paper",
      authors: ["John Doe", "Jane Smith"]
    }
  },
  {
    text: "Natural language processing enables computers to understand and process human language.",
    metadata: {
      title: "NLP Overview",
      category: "AI",
      tags: ["nlp", "linguistics", "computation"]
    }
  }
];

addDocuments(documentsToAdd)
.then(result => {
  console.log(`Successfully added ${result.successful_count} documents`);
  console.log(`Failed to add ${result.failed_count} documents`);
  
  // Process individual results
  result.results.forEach((docResult, index) => {
    if (docResult.success) {
      console.log(`Document ${index + 1} added with ID: ${docResult.id}`);
    } else {
      console.log(`Document ${index + 1} failed: ${docResult.error}`);
    }
  });
})
.catch(error => {
  console.error('Error adding documents:', error);
});
```

### Using Axios with Error Handling

```javascript
const axios = require('axios');

class DocumentManager {
  constructor(baseURL = 'http://localhost:8080', apiKey = null) {
    this.client = axios.create({
      baseURL: baseURL,
      headers: {
        'Content-Type': 'application/json',
        ...(apiKey && { 'X-API-Key': apiKey })
      },
      timeout: 30000 // 30 second timeout for large document sets
    });
  }

  async addDocuments(documents) {
    // Validate documents before sending
    if (!Array.isArray(documents) || documents.length === 0) {
      throw new Error('Documents must be a non-empty array');
    }

    for (let i = 0; i < documents.length; i++) {
      const doc = documents[i];
      if (!doc.text || typeof doc.text !== 'string' || doc.text.trim() === '') {
        throw new Error(`Document at index ${i} must have non-empty text field`);
      }
    }

    try {
      const response = await this.client.post('/add_documents', {
        documents: documents
      });
      
      return response.data;
    } catch (error) {
      if (error.response) {
        const errorData = error.response.data;
        throw new Error(`API Error (${error.response.status}): ${errorData.error}`);
      } else if (error.request) {
        throw new Error('No response from server - check if server is running');
      } else {
        throw new Error(`Request error: ${error.message}`);
      }
    }
  }

  async addSingleDocument(text, metadata = {}) {
    return this.addDocuments([{ text, metadata }]);
  }
}

// Example usage
const docManager = new DocumentManager('http://localhost:8080');

async function bulkUploadDocuments() {
  const documents = [
    {
      text: "Quantum computing leverages quantum mechanical phenomena to process information.",
      metadata: {
        title: "Quantum Computing Basics",
        difficulty: "advanced",
        field: "physics"
      }
    },
    {
      text: "Blockchain technology provides a decentralized ledger for secure transactions.",
      metadata: {
        title: "Blockchain Introduction",
        difficulty: "intermediate",
        field: "technology"
      }
    }
  ];

  try {
    const result = await docManager.addDocuments(documents);
    
    console.log('Upload Results:');
    console.log(`✅ Successfully added: ${result.successful_count}`);
    console.log(`❌ Failed to add: ${result.failed_count}`);
    console.log(`📁 Collection: ${result.collection_name}`);
    
    // Log details for each document
    result.results.forEach((docResult, index) => {
      const docTitle = documents[index].metadata.title || `Document ${index + 1}`;
      if (docResult.success) {
        console.log(`✅ "${docTitle}" → ID: ${docResult.id}`);
      } else {
        console.log(`❌ "${docTitle}" → Error: ${docResult.error}`);
      }
    });
    
  } catch (error) {
    console.error('❌ Bulk upload failed:', error.message);
  }
}

bulkUploadDocuments();
```

### Node.js with Streaming for Large Documents

```javascript
const https = require('https');
const http = require('http');
const url = require('url');

function addDocumentsBatch(serverUrl, documents, options = {}) {
  return new Promise((resolve, reject) => {
    // Validate input
    if (!Array.isArray(documents) || documents.length === 0) {
      reject(new Error('Documents must be a non-empty array'));
      return;
    }

    const parsedUrl = url.parse(serverUrl);
    const requestModule = parsedUrl.protocol === 'https:' ? https : http;
    
    const requestData = JSON.stringify({
      documents: documents
    });

    const requestOptions = {
      hostname: parsedUrl.hostname,
      port: parsedUrl.port,
      path: '/add_documents',
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
            reject(new Error(`API Error (${res.statusCode}): ${parsedResponse.error}`));
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
      reject(new Error('Request timeout - try with smaller batches'));
    });

    req.setTimeout(60000); // 60 second timeout for large batches
    req.write(requestData);
    req.end();
  });
}

// Batch processing for large document sets
async function addDocumentsInBatches(serverUrl, allDocuments, batchSize = 10) {
  const results = [];
  
  for (let i = 0; i < allDocuments.length; i += batchSize) {
    const batch = allDocuments.slice(i, i + batchSize);
    
    console.log(`Processing batch ${Math.floor(i / batchSize) + 1}/${Math.ceil(allDocuments.length / batchSize)}`);
    
    try {
      const batchResult = await addDocumentsBatch(serverUrl, batch);
      results.push(batchResult);
      
      console.log(`Batch completed: ${batchResult.successful_count} success, ${batchResult.failed_count} failed`);
      
      // Small delay between batches to avoid overwhelming the server
      await new Promise(resolve => setTimeout(resolve, 100));
    } catch (error) {
      console.error(`Batch failed:`, error.message);
      results.push({ successful_count: 0, failed_count: batch.length, error: error.message });
    }
  }
  
  // Aggregate results
  const totalSuccess = results.reduce((sum, r) => sum + (r.successful_count || 0), 0);
  const totalFailed = results.reduce((sum, r) => sum + (r.failed_count || 0), 0);
  
  return {
    total_documents: allDocuments.length,
    successful_count: totalSuccess,
    failed_count: totalFailed,
    batches: results
  };
}

// Example usage with large document set
const largeDocumentSet = [
  // ... array of hundreds or thousands of documents
];

addDocumentsInBatches('http://localhost:8080', largeDocumentSet, 20)
.then(aggregatedResult => {
  console.log('\n📊 Final Results:');
  console.log(`Total documents processed: ${aggregatedResult.total_documents}`);
  console.log(`Successfully added: ${aggregatedResult.successful_count}`);
  console.log(`Failed to add: ${aggregatedResult.failed_count}`);
  console.log(`Success rate: ${((aggregatedResult.successful_count / aggregatedResult.total_documents) * 100).toFixed(1)}%`);
})
.catch(error => {
  console.error('❌ Batch processing failed:', error.message);
});
```

### Browser Implementation with Progress Tracking

```html
<!DOCTYPE html>
<html>
<head>
    <title>Document Upload Demo</title>
    <style>
        .container { max-width: 800px; margin: 20px auto; padding: 20px; }
        .document-input { margin: 10px 0; }
        .document-input textarea { width: 100%; height: 100px; }
        .document-input input { width: 100%; margin: 5px 0; }
        .progress { margin: 20px 0; }
        .progress-bar { width: 100%; height: 20px; background: #f0f0f0; border-radius: 10px; }
        .progress-fill { height: 100%; background: #4CAF50; border-radius: 10px; transition: width 0.3s; }
        .results { margin: 20px 0; padding: 10px; background: #f5f5f5; border-radius: 5px; }
        .success { color: green; }
        .error { color: red; }
    </style>
</head>
<body>
    <div class="container">
        <h1>Add Documents to Kolosal Server</h1>
        
        <div id="document-inputs">
            <div class="document-input">
                <h3>Document 1</h3>
                <textarea placeholder="Enter document text..."></textarea>
                <input type="text" placeholder="Title (optional)" data-meta="title">
                <input type="text" placeholder="Category (optional)" data-meta="category">
                <input type="text" placeholder="Tags (comma-separated, optional)" data-meta="tags">
            </div>
        </div>
        
        <button onclick="addDocumentInput()">+ Add Another Document</button>
        <button onclick="uploadDocuments()" id="upload-btn">Upload Documents</button>
        
        <div class="progress" id="progress" style="display: none;">
            <div class="progress-bar">
                <div class="progress-fill" id="progress-fill" style="width: 0%;"></div>
            </div>
            <div id="progress-text">Uploading...</div>
        </div>
        
        <div class="results" id="results" style="display: none;"></div>
    </div>

    <script>
        let documentCount = 1;

        function addDocumentInput() {
            documentCount++;
            const container = document.getElementById('document-inputs');
            const newInput = document.createElement('div');
            newInput.className = 'document-input';
            newInput.innerHTML = `
                <h3>Document ${documentCount}</h3>
                <textarea placeholder="Enter document text..."></textarea>
                <input type="text" placeholder="Title (optional)" data-meta="title">
                <input type="text" placeholder="Category (optional)" data-meta="category">
                <input type="text" placeholder="Tags (comma-separated, optional)" data-meta="tags">
                <button onclick="removeDocumentInput(this)">Remove</button>
            `;
            container.appendChild(newInput);
        }

        function removeDocumentInput(button) {
            button.parentElement.remove();
        }

        function collectDocuments() {
            const documentInputs = document.querySelectorAll('.document-input');
            const documents = [];

            documentInputs.forEach((input, index) => {
                const textarea = input.querySelector('textarea');
                const text = textarea.value.trim();
                
                if (text) {
                    const metadata = {};
                    const metaInputs = input.querySelectorAll('input[data-meta]');
                    
                    metaInputs.forEach(metaInput => {
                        const key = metaInput.getAttribute('data-meta');
                        const value = metaInput.value.trim();
                        if (value) {
                            if (key === 'tags') {
                                metadata[key] = value.split(',').map(tag => tag.trim()).filter(tag => tag);
                            } else {
                                metadata[key] = value;
                            }
                        }
                    });

                    documents.push({
                        text: text,
                        ...(Object.keys(metadata).length > 0 && { metadata: metadata })
                    });
                }
            });

            return documents;
        }

        async function uploadDocuments() {
            const documents = collectDocuments();
            
            if (documents.length === 0) {
                alert('Please enter at least one document with text content.');
                return;
            }

            const uploadBtn = document.getElementById('upload-btn');
            const progressDiv = document.getElementById('progress');
            const progressFill = document.getElementById('progress-fill');
            const progressText = document.getElementById('progress-text');
            const resultsDiv = document.getElementById('results');

            // Show progress and disable button
            uploadBtn.disabled = true;
            uploadBtn.textContent = 'Uploading...';
            progressDiv.style.display = 'block';
            resultsDiv.style.display = 'none';

            // Simulate progress (since we don't have real progress from the API)
            let progress = 0;
            const progressInterval = setInterval(() => {
                progress += 10;
                if (progress <= 90) {
                    progressFill.style.width = progress + '%';
                    progressText.textContent = `Uploading... ${progress}%`;
                }
            }, 200);

            try {
                const response = await fetch('http://localhost:8080/add_documents', {
                    method: 'POST',
                    headers: {
                        'Content-Type': 'application/json',
                    },
                    body: JSON.stringify({ documents: documents })
                });

                clearInterval(progressInterval);
                progressFill.style.width = '100%';
                progressText.textContent = 'Processing results...';

                if (!response.ok) {
                    const errorData = await response.json();
                    throw new Error(errorData.error);
                }

                const result = await response.json();
                displayResults(result, documents);

            } catch (error) {
                clearInterval(progressInterval);
                progressDiv.style.display = 'none';
                resultsDiv.innerHTML = `<div class="error">❌ Upload failed: ${error.message}</div>`;
                resultsDiv.style.display = 'block';
            } finally {
                uploadBtn.disabled = false;
                uploadBtn.textContent = 'Upload Documents';
                setTimeout(() => {
                    progressDiv.style.display = 'none';
                }, 2000);
            }
        }

        function displayResults(result, originalDocuments) {
            const resultsDiv = document.getElementById('results');
            
            let html = `
                <h3>Upload Results</h3>
                <div class="success">✅ Successfully added: ${result.successful_count}</div>
                <div class="error">❌ Failed to add: ${result.failed_count}</div>
                <div>📁 Collection: ${result.collection_name}</div>
                <hr>
            `;

            result.results.forEach((docResult, index) => {
                const originalDoc = originalDocuments[index];
                const title = originalDoc.metadata?.title || `Document ${index + 1}`;
                const preview = originalDoc.text.substring(0, 50) + '...';
                
                if (docResult.success) {
                    html += `
                        <div class="success">
                            ✅ <strong>${title}</strong><br>
                            ID: ${docResult.id}<br>
                            Preview: ${preview}
                        </div>
                    `;
                } else {
                    html += `
                        <div class="error">
                            ❌ <strong>${title}</strong><br>
                            Error: ${docResult.error}<br>
                            Preview: ${preview}
                        </div>
                    `;
                }
            });

            resultsDiv.innerHTML = html;
            resultsDiv.style.display = 'block';
        }
    </script>
</body>
</html>
```

## Common Error Scenarios

### 1. Empty Documents Array

```javascript
// This will return a 400 error
const response = await fetch('/add_documents', {
  method: 'POST',
  headers: { 'Content-Type': 'application/json' },
  body: JSON.stringify({ documents: [] })
});
// Error: "Invalid request parameters"
```

### 2. Missing Text Field

```javascript
// This will return a 400 error
const response = await fetch('/add_documents', {
  method: 'POST',
  headers: { 'Content-Type': 'application/json' },
  body: JSON.stringify({
    documents: [{ metadata: { title: "Test" } }] // Missing text field
  })
});
// Error: "Document must contain a 'text' field of type string"
```

### 3. Invalid Metadata

```javascript
// This will return a 400 error
const response = await fetch('/add_documents', {
  method: 'POST',
  headers: { 'Content-Type': 'application/json' },
  body: JSON.stringify({
    documents: [{
      text: "Test document",
      metadata: "invalid" // metadata must be an object
    }]
  })
});
// Error: "Document 'metadata' field must be an object"
```

## Best Practices

1. **Batch Size**: For large document sets, process in batches of 10-50 documents to avoid timeouts
2. **Text Quality**: Ensure document text is meaningful and well-formatted for better embeddings
3. **Metadata Standards**: Use consistent metadata schemas across your documents
4. **Error Handling**: Always check both overall success and individual document results
5. **Timeouts**: Set appropriate timeouts (30-60 seconds) for large document uploads
6. **Retry Logic**: Implement retry logic for failed documents
7. **Validation**: Validate document content before sending to reduce failures

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

## Performance Considerations

- **Document Size**: Very large documents may take longer to process
- **Concurrent Requests**: Limit concurrent add_documents requests to avoid overwhelming the server
- **Memory Usage**: Large batches consume more server memory during processing
- **Embedding Processing**: Document embedding generation is CPU-intensive and may affect response times
