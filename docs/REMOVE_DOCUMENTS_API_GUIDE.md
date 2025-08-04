# Kolosal Server - Remove Documents API Documentation

## Overview

The `/remove_documents` API endpoint allows you to remove multiple documents from the vector database by their IDs. This operation permanently deletes the documents and their associated embeddings from the collection.

**Endpoint:** `POST /remove_documents`  
**Content-Type:** `application/json`

## Request Format

### Request Body Schema

```json
{
  "document_ids": ["string", "string", "..."]
}
```

### Request Parameters

| Parameter | Type | Required | Description |
|-----------|------|----------|-------------|
| `document_ids` | array | Yes | Array of document IDs to be removed from the collection |

## Response Format

### Success Response (200 OK)

```json
{
  "results": [
    {
      "id": "string",
      "status": "string"
    }
  ],
  "collection_name": "string",
  "removed_count": "integer",
  "failed_count": "integer",
  "not_found_count": "integer"
}
```

### Response Fields

| Field | Type | Description |
|-------|------|-------------|
| `results` | array | Array of individual document removal results |
| `results[].id` | string | Document ID that was processed |
| `results[].status` | string | Status: "removed", "failed", or "not_found" |
| `collection_name` | string | Name of the collection (typically "documents") |
| `removed_count` | integer | Number of documents successfully removed |
| `failed_count` | integer | Number of documents that failed to be removed |
| `not_found_count` | integer | Number of documents that were not found |

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
async function removeDocuments(documentIds) {
  if (!Array.isArray(documentIds) || documentIds.length === 0) {
    throw new Error('Document IDs must be a non-empty array');
  }

  const requestBody = {
    document_ids: documentIds
  };

  try {
    const response = await fetch('http://localhost:8080/remove_documents', {
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
    console.error('Remove documents failed:', error);
    throw error;
  }
}

// Example usage
const documentIdsToRemove = [
  "doc-123-1691234567890",
  "doc-124-1691234567891",
  "doc-125-1691234567892"
];

removeDocuments(documentIdsToRemove)
.then(result => {
  console.log(`✅ Successfully removed: ${result.removed_count} documents`);
  console.log(`❌ Failed to remove: ${result.failed_count} documents`);
  console.log(`🔍 Not found: ${result.not_found_count} documents`);
  
  // Process individual results
  result.results.forEach(docResult => {
    switch (docResult.status) {
      case 'removed':
        console.log(`✅ Removed: ${docResult.id}`);
        break;
      case 'failed':
        console.log(`❌ Failed: ${docResult.id}`);
        break;
      case 'not_found':
        console.log(`🔍 Not found: ${docResult.id}`);
        break;
    }
  });
})
.catch(error => {
  console.error('Error removing documents:', error);
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
      timeout: 30000 // 30 second timeout
    });
  }

  async removeDocuments(documentIds) {
    // Validate input
    if (!Array.isArray(documentIds) || documentIds.length === 0) {
      throw new Error('Document IDs must be a non-empty array');
    }

    // Check for valid ID format
    const invalidIds = documentIds.filter(id => !id || typeof id !== 'string' || id.trim() === '');
    if (invalidIds.length > 0) {
      throw new Error('All document IDs must be non-empty strings');
    }

    try {
      const response = await this.client.post('/remove_documents', {
        document_ids: documentIds
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

  async removeSingleDocument(documentId) {
    return this.removeDocuments([documentId]);
  }

  async removeDocumentsByPattern(pattern) {
    // First, get all document IDs
    const listResponse = await this.client.get('/list_documents');
    const allIds = listResponse.data.document_ids;
    
    // Filter IDs based on pattern
    const regex = new RegExp(pattern);
    const matchingIds = allIds.filter(id => regex.test(id));
    
    if (matchingIds.length === 0) {
      return {
        removed_count: 0,
        failed_count: 0,
        not_found_count: 0,
        message: 'No documents matched the pattern'
      };
    }

    // Remove matching documents
    return this.removeDocuments(matchingIds);
  }
}

// Example usage
const docManager = new DocumentManager('http://localhost:8080');

async function cleanupOldDocuments() {
  try {
    // Remove documents with specific pattern
    const result = await docManager.removeDocumentsByPattern('^temp-');
    
    console.log('🧹 Cleanup Results:');
    console.log(`✅ Removed: ${result.removed_count}`);
    console.log(`❌ Failed: ${result.failed_count}`);
    console.log(`🔍 Not found: ${result.not_found_count}`);
    
    if (result.results) {
      result.results.forEach(docResult => {
        console.log(`${getStatusIcon(docResult.status)} ${docResult.id}`);
      });
    }
    
  } catch (error) {
    console.error('❌ Cleanup failed:', error.message);
  }
}

function getStatusIcon(status) {
  switch (status) {
    case 'removed': return '✅';
    case 'failed': return '❌';
    case 'not_found': return '🔍';
    default: return '❓';
  }
}

// Remove specific documents
async function removeSpecificDocuments() {
  const idsToRemove = [
    'doc-123-1691234567890',
    'doc-456-1691234567891',
    'invalid-id-123'
  ];

  try {
    const result = await docManager.removeDocuments(idsToRemove);
    
    console.log('📊 Removal Summary:');
    console.log(`Total processed: ${idsToRemove.length}`);
    console.log(`Successfully removed: ${result.removed_count}`);
    console.log(`Failed to remove: ${result.failed_count}`);
    console.log(`Not found: ${result.not_found_count}`);
    
    // Show details for each document
    console.log('\n📋 Detailed Results:');
    result.results.forEach(docResult => {
      console.log(`${getStatusIcon(docResult.status)} ${docResult.id} → ${docResult.status}`);
    });
    
  } catch (error) {
    console.error('❌ Removal failed:', error.message);
  }
}

// Execute examples
removeSpecificDocuments();
cleanupOldDocuments();
```

### Node.js with Batch Processing

```javascript
const https = require('https');
const http = require('http');
const url = require('url');

function removeDocumentsBatch(serverUrl, documentIds, options = {}) {
  return new Promise((resolve, reject) => {
    if (!Array.isArray(documentIds) || documentIds.length === 0) {
      reject(new Error('Document IDs must be a non-empty array'));
      return;
    }

    const parsedUrl = url.parse(serverUrl);
    const requestModule = parsedUrl.protocol === 'https:' ? https : http;
    
    const requestData = JSON.stringify({
      document_ids: documentIds
    });

    const requestOptions = {
      hostname: parsedUrl.hostname,
      port: parsedUrl.port,
      path: '/remove_documents',
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
      reject(new Error('Request timeout'));
    });

    req.setTimeout(30000); // 30 second timeout
    req.write(requestData);
    req.end();
  });
}

// Safe bulk removal with confirmation
async function safeBulkRemoval(serverUrl, documentIds, options = {}) {
  const batchSize = options.batchSize || 50;
  const confirmBeforeRemoval = options.confirm !== false;
  
  console.log(`\n🗂️  Preparing to remove ${documentIds.length} documents`);
  
  if (confirmBeforeRemoval) {
    // In a real application, you might use a proper prompt library
    console.log('⚠️  This will permanently delete the documents. Continue? (This example assumes yes)');
    // For demo purposes, we continue automatically
  }

  const results = [];
  let totalRemoved = 0;
  let totalFailed = 0;
  let totalNotFound = 0;
  
  for (let i = 0; i < documentIds.length; i += batchSize) {
    const batch = documentIds.slice(i, i + batchSize);
    const batchNumber = Math.floor(i / batchSize) + 1;
    const totalBatches = Math.ceil(documentIds.length / batchSize);
    
    console.log(`\n📦 Processing batch ${batchNumber}/${totalBatches} (${batch.length} documents)`);
    
    try {
      const batchResult = await removeDocumentsBatch(serverUrl, batch, options);
      results.push(batchResult);
      
      totalRemoved += batchResult.removed_count;
      totalFailed += batchResult.failed_count;
      totalNotFound += batchResult.not_found_count;
      
      console.log(`   ✅ Removed: ${batchResult.removed_count}`);
      console.log(`   ❌ Failed: ${batchResult.failed_count}`);
      console.log(`   🔍 Not found: ${batchResult.not_found_count}`);
      
      // Small delay between batches
      if (i + batchSize < documentIds.length) {
        await new Promise(resolve => setTimeout(resolve, 100));
      }
    } catch (error) {
      console.error(`❌ Batch ${batchNumber} failed:`, error.message);
      results.push({ 
        removed_count: 0, 
        failed_count: batch.length, 
        not_found_count: 0,
        error: error.message 
      });
      totalFailed += batch.length;
    }
  }
  
  console.log(`\n📊 Final Results:`);
  console.log(`Total documents processed: ${documentIds.length}`);
  console.log(`Successfully removed: ${totalRemoved}`);
  console.log(`Failed to remove: ${totalFailed}`);
  console.log(`Not found: ${totalNotFound}`);
  console.log(`Success rate: ${((totalRemoved / documentIds.length) * 100).toFixed(1)}%`);
  
  return {
    total_documents: documentIds.length,
    removed_count: totalRemoved,
    failed_count: totalFailed,
    not_found_count: totalNotFound,
    batches: results
  };
}

// Example usage
const documentsToRemove = [
  'doc-1-1691234567890',
  'doc-2-1691234567891', 
  'doc-3-1691234567892',
  // ... more document IDs
];

safeBulkRemoval('http://localhost:8080', documentsToRemove, {
  batchSize: 25,
  confirm: false // Set to true in production for safety
})
.then(result => {
  console.log('\n🎉 Bulk removal completed!');
})
.catch(error => {
  console.error('💥 Bulk removal failed:', error.message);
});
```

### Browser Implementation with Confirmation

```html
<!DOCTYPE html>
<html>
<head>
    <title>Document Removal Demo</title>
    <style>
        .container { max-width: 800px; margin: 20px auto; padding: 20px; }
        .id-input { width: 100%; height: 200px; margin: 10px 0; }
        .warning { background: #fff3cd; border: 1px solid #ffeaa7; padding: 15px; border-radius: 5px; margin: 15px 0; }
        .results { margin: 20px 0; padding: 10px; background: #f5f5f5; border-radius: 5px; }
        .success { color: green; }
        .error { color: red; }
        .not-found { color: orange; }
        .button { padding: 10px 20px; margin: 5px; border: none; border-radius: 5px; cursor: pointer; }
        .danger { background: #dc3545; color: white; }
        .primary { background: #007bff; color: white; }
        .secondary { background: #6c757d; color: white; }
        .stats { display: flex; gap: 20px; margin: 15px 0; }
        .stat-card { background: white; padding: 15px; border-radius: 5px; border-left: 4px solid #007bff; flex: 1; }
    </style>
</head>
<body>
    <div class="container">
        <h1>Remove Documents from Kolosal Server</h1>
        
        <div>
            <h3>Document IDs to Remove</h3>
            <textarea 
                id="document-ids" 
                class="id-input"
                placeholder="Enter document IDs, one per line or comma-separated&#10;&#10;Example:&#10;doc-123-1691234567890&#10;doc-124-1691234567891&#10;doc-125-1691234567892"
            ></textarea>
        </div>
        
        <div class="warning">
            <strong>⚠️ Warning:</strong> This action will permanently delete the selected documents from the database. This operation cannot be undone.
        </div>
        
        <div>
            <button class="button secondary" onclick="previewRemoval()">Preview Removal</button>
            <button class="button danger" onclick="removeDocuments()" id="remove-btn" disabled>Remove Documents</button>
            <button class="button primary" onclick="listAllDocuments()">Show All Document IDs</button>
        </div>
        
        <div id="preview" style="display: none;">
            <h3>Preview</h3>
            <div id="preview-content"></div>
        </div>
        
        <div class="results" id="results" style="display: none;"></div>
    </div>

    <script>
        let documentIdsToRemove = [];

        function parseDocumentIds() {
            const input = document.getElementById('document-ids').value.trim();
            if (!input) return [];
            
            // Split by newlines and commas, then clean up
            const ids = input.split(/[,\n]/)
                           .map(id => id.trim())
                           .filter(id => id.length > 0);
            
            return [...new Set(ids)]; // Remove duplicates
        }

        function previewRemoval() {
            documentIdsToRemove = parseDocumentIds();
            const previewDiv = document.getElementById('preview');
            const previewContent = document.getElementById('preview-content');
            const removeBtn = document.getElementById('remove-btn');
            
            if (documentIdsToRemove.length === 0) {
                alert('Please enter at least one document ID.');
                return;
            }
            
            previewContent.innerHTML = `
                <div class="stats">
                    <div class="stat-card">
                        <strong>${documentIdsToRemove.length}</strong><br>
                        Documents to remove
                    </div>
                </div>
                <h4>Document IDs:</h4>
                <ul>
                    ${documentIdsToRemove.map(id => `<li><code>${id}</code></li>`).join('')}
                </ul>
                <p><strong>Are you sure you want to remove these documents?</strong></p>
            `;
            
            previewDiv.style.display = 'block';
            removeBtn.disabled = false;
        }

        async function removeDocuments() {
            if (documentIdsToRemove.length === 0) {
                alert('Please preview the removal first.');
                return;
            }

            const removeBtn = document.getElementById('remove-btn');
            const resultsDiv = document.getElementById('results');

            // Final confirmation
            if (!confirm(`Are you absolutely sure you want to permanently remove ${documentIdsToRemove.length} documents?`)) {
                return;
            }

            removeBtn.disabled = true;
            removeBtn.textContent = 'Removing...';
            resultsDiv.innerHTML = '<div>🔄 Removing documents...</div>';
            resultsDiv.style.display = 'block';

            try {
                const response = await fetch('http://localhost:8080/remove_documents', {
                    method: 'POST',
                    headers: {
                        'Content-Type': 'application/json',
                    },
                    body: JSON.stringify({
                        document_ids: documentIdsToRemove
                    })
                });

                if (!response.ok) {
                    const errorData = await response.json();
                    throw new Error(errorData.error);
                }

                const result = await response.json();
                displayResults(result);

            } catch (error) {
                resultsDiv.innerHTML = `<div class="error">❌ Removal failed: ${error.message}</div>`;
            } finally {
                removeBtn.disabled = false;
                removeBtn.textContent = 'Remove Documents';
            }
        }

        function displayResults(result) {
            const resultsDiv = document.getElementById('results');
            
            let html = `
                <h3>Removal Results</h3>
                <div class="stats">
                    <div class="stat-card" style="border-left-color: #28a745;">
                        <strong>${result.removed_count}</strong><br>
                        Successfully Removed
                    </div>
                    <div class="stat-card" style="border-left-color: #dc3545;">
                        <strong>${result.failed_count}</strong><br>
                        Failed to Remove
                    </div>
                    <div class="stat-card" style="border-left-color: #ffc107;">
                        <strong>${result.not_found_count}</strong><br>
                        Not Found
                    </div>
                </div>
                <div>📁 Collection: ${result.collection_name}</div>
                <hr>
                <h4>Detailed Results:</h4>
            `;

            result.results.forEach(docResult => {
                let statusClass, statusIcon, statusText;
                
                switch (docResult.status) {
                    case 'removed':
                        statusClass = 'success';
                        statusIcon = '✅';
                        statusText = 'Successfully removed';
                        break;
                    case 'failed':
                        statusClass = 'error';
                        statusIcon = '❌';
                        statusText = 'Failed to remove';
                        break;
                    case 'not_found':
                        statusClass = 'not-found';
                        statusIcon = '🔍';
                        statusText = 'Document not found';
                        break;
                    default:
                        statusClass = '';
                        statusIcon = '❓';
                        statusText = 'Unknown status';
                }
                
                html += `
                    <div class="${statusClass}" style="margin: 8px 0; padding: 8px; background: rgba(0,0,0,0.05); border-radius: 3px;">
                        ${statusIcon} <strong>${docResult.id}</strong> → ${statusText}
                    </div>
                `;
            });

            // Clear the input and preview if all documents were processed successfully
            if (result.failed_count === 0) {
                document.getElementById('document-ids').value = '';
                document.getElementById('preview').style.display = 'none';
                document.getElementById('remove-btn').disabled = true;
                documentIdsToRemove = [];
            }

            resultsDiv.innerHTML = html;
        }

        async function listAllDocuments() {
            try {
                const response = await fetch('http://localhost:8080/list_documents');
                if (!response.ok) {
                    throw new Error('Failed to fetch document list');
                }
                
                const result = await response.json();
                const textarea = document.getElementById('document-ids');
                
                if (result.document_ids.length === 0) {
                    alert('No documents found in the collection.');
                    return;
                }
                
                const confirmLoad = confirm(`Found ${result.document_ids.length} documents. Load all IDs into the input field?`);
                if (confirmLoad) {
                    textarea.value = result.document_ids.join('\n');
                }
                
            } catch (error) {
                alert('Failed to fetch document list: ' + error.message);
            }
        }
    </script>
</body>
</html>
```

## Common Error Scenarios

### 1. Empty Document IDs Array

```javascript
// This will return a 400 error
const response = await fetch('/remove_documents', {
  method: 'POST',
  headers: { 'Content-Type': 'application/json' },
  body: JSON.stringify({ document_ids: [] })
});
// Error: "Invalid request parameters: document_ids cannot be empty"
```

### 2. Missing document_ids Field

```javascript
// This will return a 400 error
const response = await fetch('/remove_documents', {
  method: 'POST',
  headers: { 'Content-Type': 'application/json' },
  body: JSON.stringify({ ids: ["doc-123"] }) // Wrong field name
});
// Error: Missing required field 'document_ids'
```

### 3. Invalid JSON

```javascript
// This will return a 400 error
const response = await fetch('/remove_documents', {
  method: 'POST',
  headers: { 'Content-Type': 'application/json' },
  body: '{"document_ids": [invalid json}'
});
// Error: "Invalid JSON: ..."
```

## Best Practices

1. **Confirmation**: Always implement user confirmation for deletion operations
2. **Batch Processing**: For large removals, process in batches to avoid timeouts
3. **Error Handling**: Check individual document results, not just overall success
4. **Validation**: Validate document IDs before sending requests
5. **Logging**: Log removal operations for audit trails
6. **Backup**: Consider backing up documents before bulk removals
7. **Rate Limiting**: Don't overwhelm the server with concurrent removal requests

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

## Safety Considerations

⚠️ **Important**: Document removal is permanent and cannot be undone. Always:

- Implement proper access controls
- Require user confirmation for deletions
- Maintain audit logs of removal operations
- Consider implementing a "soft delete" mechanism for critical systems
- Test with non-production data first
