# Kolosal Server - Info Documents API Documentation

## Overview

The `/info_documents` API endpoint retrieves detailed information for specific documents by their IDs. This includes the full text content and metadata for each requested document, allowing you to inspect and analyze document details.

**Endpoint:** `POST /info_documents`  
**Content-Type:** `application/json`

## Request Format

### Request Body Schema

```json
{
  "ids": ["string", "string", "..."]
}
```

### Request Parameters

| Parameter | Type | Required | Description |
|-----------|------|----------|-------------|
| `ids` | array | Yes | Array of document IDs to retrieve information for |

## Response Format

### Success Response (200 OK)

```json
{
  "documents": [
    {
      "id": "string",
      "text": "string",
      "metadata": "object"
    }
  ],
  "found_count": "integer",
  "not_found_count": "integer",
  "not_found_ids": ["string"],
  "collection_name": "string"
}
```

### Response Fields

| Field | Type | Description |
|-------|------|-------------|
| `documents` | array | Array of found document information objects |
| `documents[].id` | string | Document ID |
| `documents[].text` | string | Full text content of the document |
| `documents[].metadata` | object | Metadata associated with the document |
| `found_count` | integer | Number of documents successfully found |
| `not_found_count` | integer | Number of documents that were not found |
| `not_found_ids` | array | Array of document IDs that were not found |
| `collection_name` | string | Name of the collection (typically "documents") |

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
async function getDocumentsInfo(documentIds) {
  if (!Array.isArray(documentIds) || documentIds.length === 0) {
    throw new Error('Document IDs must be a non-empty array');
  }

  const requestBody = {
    ids: documentIds
  };

  try {
    const response = await fetch('http://localhost:8080/info_documents', {
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
    console.error('Get documents info failed:', error);
    throw error;
  }
}

// Example usage
const documentIdsToQuery = [
  "doc-123-1691234567890",
  "doc-124-1691234567891",
  "nonexistent-id-123"
];

getDocumentsInfo(documentIdsToQuery)
.then(result => {
  console.log(`📁 Collection: ${result.collection_name}`);
  console.log(`✅ Found: ${result.found_count} documents`);
  console.log(`❌ Not found: ${result.not_found_count} documents`);
  
  // Display found documents
  result.documents.forEach((doc, index) => {
    console.log(`\n📄 Document ${index + 1}:`);
    console.log(`   ID: ${doc.id}`);
    console.log(`   Text: ${doc.text.substring(0, 100)}...`);
    console.log(`   Metadata:`, doc.metadata);
  });
  
  // Display not found IDs
  if (result.not_found_ids.length > 0) {
    console.log('\n🔍 Not found:');
    result.not_found_ids.forEach(id => {
      console.log(`   ${id}`);
    });
  }
})
.catch(error => {
  console.error('Error getting document info:', error);
});
```

### Using Axios with Advanced Document Analysis

```javascript
const axios = require('axios');

class DocumentInfoManager {
  constructor(baseURL = 'http://localhost:8080', apiKey = null) {
    this.client = axios.create({
      baseURL: baseURL,
      headers: {
        'Content-Type': 'application/json',
        ...(apiKey && { 'X-API-Key': apiKey })
      },
      timeout: 30000
    });
  }

  async getDocumentsInfo(documentIds) {
    // Validate input
    if (!Array.isArray(documentIds) || documentIds.length === 0) {
      throw new Error('Document IDs must be a non-empty array');
    }

    const uniqueIds = [...new Set(documentIds)]; // Remove duplicates

    try {
      const response = await this.client.post('/info_documents', {
        ids: uniqueIds
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

  async getSingleDocumentInfo(documentId) {
    const result = await this.getDocumentsInfo([documentId]);
    
    if (result.found_count === 0) {
      return null;
    }
    
    return result.documents[0];
  }

  async analyzeDocuments(documentIds) {
    const result = await this.getDocumentsInfo(documentIds);
    
    const analysis = {
      summary: {
        total_requested: documentIds.length,
        found: result.found_count,
        not_found: result.not_found_count,
        collection: result.collection_name
      },
      text_analysis: {
        total_characters: 0,
        total_words: 0,
        avg_length: 0,
        length_distribution: {},
        language_patterns: {}
      },
      metadata_analysis: {
        common_fields: {},
        field_types: {},
        metadata_stats: {}
      },
      documents: result.documents.map(doc => ({
        id: doc.id,
        text_length: doc.text.length,
        word_count: doc.text.split(/\s+/).length,
        metadata_fields: Object.keys(doc.metadata),
        has_metadata: Object.keys(doc.metadata).length > 0
      }))
    };

    // Text analysis
    result.documents.forEach(doc => {
      const textLength = doc.text.length;
      const wordCount = doc.text.split(/\s+/).length;
      
      analysis.text_analysis.total_characters += textLength;
      analysis.text_analysis.total_words += wordCount;
      
      // Length distribution
      const lengthBucket = Math.floor(textLength / 500) * 500;
      const lengthKey = `${lengthBucket}-${lengthBucket + 499}`;
      analysis.text_analysis.length_distribution[lengthKey] = 
        (analysis.text_analysis.length_distribution[lengthKey] || 0) + 1;
      
      // Metadata analysis
      Object.entries(doc.metadata).forEach(([key, value]) => {
        analysis.metadata_analysis.common_fields[key] = 
          (analysis.metadata_analysis.common_fields[key] || 0) + 1;
        
        const valueType = Array.isArray(value) ? 'array' : typeof value;
        analysis.metadata_analysis.field_types[key] = valueType;
      });
    });

    // Calculate averages
    if (result.found_count > 0) {
      analysis.text_analysis.avg_length = 
        Math.round(analysis.text_analysis.total_characters / result.found_count);
    }

    return analysis;
  }

  async compareDocuments(documentIds) {
    const result = await this.getDocumentsInfo(documentIds);
    
    if (result.found_count < 2) {
      throw new Error('Need at least 2 documents for comparison');
    }

    const comparison = {
      documents: result.documents.map(doc => ({
        id: doc.id,
        text_length: doc.text.length,
        word_count: doc.text.split(/\s+/).length,
        unique_words: new Set(doc.text.toLowerCase().match(/\b\w+\b/g) || []).size,
        metadata_fields: Object.keys(doc.metadata),
        preview: doc.text.substring(0, 200) + '...'
      })),
      similarities: {},
      differences: {
        length_variance: 0,
        metadata_overlap: {},
        common_metadata_fields: []
      }
    };

    // Calculate similarities and differences
    const docs = result.documents;
    for (let i = 0; i < docs.length; i++) {
      for (let j = i + 1; j < docs.length; j++) {
        const doc1 = docs[i];
        const doc2 = docs[j];
        const key = `${doc1.id}_vs_${doc2.id}`;
        
        // Simple text similarity (Jaccard similarity of words)
        const words1 = new Set(doc1.text.toLowerCase().match(/\b\w+\b/g) || []);
        const words2 = new Set(doc2.text.toLowerCase().match(/\b\w+\b/g) || []);
        const intersection = new Set([...words1].filter(x => words2.has(x)));
        const union = new Set([...words1, ...words2]);
        const textSimilarity = intersection.size / union.size;
        
        // Metadata similarity
        const meta1Keys = new Set(Object.keys(doc1.metadata));
        const meta2Keys = new Set(Object.keys(doc2.metadata));
        const metaIntersection = new Set([...meta1Keys].filter(x => meta2Keys.has(x)));
        const metaUnion = new Set([...meta1Keys, ...meta2Keys]);
        const metaSimilarity = metaUnion.size > 0 ? metaIntersection.size / metaUnion.size : 0;
        
        comparison.similarities[key] = {
          text_similarity: Math.round(textSimilarity * 100) / 100,
          metadata_similarity: Math.round(metaSimilarity * 100) / 100,
          length_difference: Math.abs(doc1.text.length - doc2.text.length)
        };
      }
    }

    return comparison;
  }

  async exportDocumentsToJson(documentIds, options = {}) {
    const result = await this.getDocumentsInfo(documentIds);
    
    const exportData = {
      export_metadata: {
        timestamp: new Date().toISOString(),
        requested_ids: documentIds,
        found_count: result.found_count,
        not_found_count: result.not_found_count,
        collection_name: result.collection_name,
        export_version: '1.0'
      },
      documents: result.documents,
      not_found_ids: result.not_found_ids
    };

    if (options.includeAnalysis) {
      exportData.analysis = await this.analyzeDocuments(documentIds);
    }

    return exportData;
  }
}

// Example usage
const docInfoManager = new DocumentInfoManager('http://localhost:8080');

async function demonstrateDocumentInfo() {
  try {
    const testIds = [
      'doc-123-1691234567890',
      'doc-124-1691234567891',
      'doc-125-1691234567892'
    ];

    console.log('🔍 Getting document information...\n');

    // Basic document info
    const result = await docInfoManager.getDocumentsInfo(testIds);
    console.log(`📊 Results: ${result.found_count} found, ${result.not_found_count} not found`);

    // Detailed analysis
    console.log('\n📈 Analyzing documents...');
    const analysis = await docInfoManager.analyzeDocuments(testIds);
    console.log('Text Analysis:', analysis.text_analysis);
    console.log('Metadata Analysis:', analysis.metadata_analysis);

    // Export with analysis
    console.log('\n💾 Exporting documents...');
    const exportData = await docInfoManager.exportDocumentsToJson(testIds, {
      includeAnalysis: true
    });
    
    console.log(`Exported ${exportData.documents.length} documents with analysis`);

  } catch (error) {
    console.error('❌ Demo failed:', error.message);
  }
}

demonstrateDocumentInfo();
```

### Node.js with Batch Processing and Export

```javascript
const fs = require('fs').promises;
const path = require('path');
const https = require('https');
const http = require('http');
const url = require('url');

function getDocumentsInfo(serverUrl, documentIds, options = {}) {
  return new Promise((resolve, reject) => {
    if (!Array.isArray(documentIds) || documentIds.length === 0) {
      reject(new Error('Document IDs must be a non-empty array'));
      return;
    }

    const parsedUrl = url.parse(serverUrl);
    const requestModule = parsedUrl.protocol === 'https:' ? https : http;
    
    const requestData = JSON.stringify({
      ids: documentIds
    });

    const requestOptions = {
      hostname: parsedUrl.hostname,
      port: parsedUrl.port,
      path: '/info_documents',
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

    req.setTimeout(60000); // 60 second timeout for large document sets
    req.write(requestData);
    req.end();
  });
}

async function batchGetDocumentsInfo(serverUrl, allDocumentIds, options = {}) {
  const batchSize = options.batchSize || 25; // Smaller batches for info requests
  const results = [];
  let totalFound = 0;
  let totalNotFound = 0;
  
  console.log(`📊 Processing ${allDocumentIds.length} document IDs in batches of ${batchSize}`);
  
  for (let i = 0; i < allDocumentIds.length; i += batchSize) {
    const batch = allDocumentIds.slice(i, i + batchSize);
    const batchNumber = Math.floor(i / batchSize) + 1;
    const totalBatches = Math.ceil(allDocumentIds.length / batchSize);
    
    console.log(`📦 Processing batch ${batchNumber}/${totalBatches} (${batch.length} documents)`);
    
    try {
      const batchResult = await getDocumentsInfo(serverUrl, batch, options);
      results.push(batchResult);
      
      totalFound += batchResult.found_count;
      totalNotFound += batchResult.not_found_count;
      
      console.log(`   ✅ Found: ${batchResult.found_count}, ❌ Not found: ${batchResult.not_found_count}`);
      
      // Small delay between batches
      if (i + batchSize < allDocumentIds.length) {
        await new Promise(resolve => setTimeout(resolve, 200));
      }
    } catch (error) {
      console.error(`❌ Batch ${batchNumber} failed:`, error.message);
      results.push({ 
        found_count: 0, 
        not_found_count: batch.length,
        documents: [],
        not_found_ids: batch,
        error: error.message 
      });
      totalNotFound += batch.length;
    }
  }
  
  // Aggregate results
  const allDocuments = [];
  const allNotFoundIds = [];
  
  results.forEach(result => {
    if (result.documents) {
      allDocuments.push(...result.documents);
    }
    if (result.not_found_ids) {
      allNotFoundIds.push(...result.not_found_ids);
    }
  });
  
  console.log(`\n📊 Final Results:`);
  console.log(`Total documents requested: ${allDocumentIds.length}`);
  console.log(`Successfully found: ${totalFound}`);
  console.log(`Not found: ${totalNotFound}`);
  console.log(`Success rate: ${((totalFound / allDocumentIds.length) * 100).toFixed(1)}%`);
  
  return {
    documents: allDocuments,
    found_count: totalFound,
    not_found_count: totalNotFound,
    not_found_ids: allNotFoundIds,
    collection_name: results[0]?.collection_name || 'documents',
    batches: results
  };
}

async function exportDocumentDetails(serverUrl, documentIds, outputDir = './exports', options = {}) {
  try {
    console.log('📋 Fetching detailed document information...');
    const result = await batchGetDocumentsInfo(serverUrl, documentIds, options);
    
    // Create export directory
    await fs.mkdir(outputDir, { recursive: true });
    
    const timestamp = new Date().toISOString().replace(/[:.]/g, '-');
    
    // Export full document details
    const detailsFilename = `document-details-${timestamp}.json`;
    const detailsPath = path.join(outputDir, detailsFilename);
    
    const exportData = {
      export_metadata: {
        timestamp: new Date().toISOString(),
        total_requested: documentIds.length,
        found_count: result.found_count,
        not_found_count: result.not_found_count,
        collection_name: result.collection_name
      },
      documents: result.documents,
      not_found_ids: result.not_found_ids
    };
    
    await fs.writeFile(detailsPath, JSON.stringify(exportData, null, 2));
    console.log(`✅ Document details exported to: ${detailsPath}`);
    
    // Export summary report
    const summaryFilename = `document-summary-${timestamp}.txt`;
    const summaryPath = path.join(outputDir, summaryFilename);
    
    let summaryText = `Document Information Summary\n`;
    summaryText += `Generated: ${new Date().toISOString()}\n`;
    summaryText += `Collection: ${result.collection_name}\n`;
    summaryText += `Total Requested: ${documentIds.length}\n`;
    summaryText += `Found: ${result.found_count}\n`;
    summaryText += `Not Found: ${result.not_found_count}\n\n`;
    
    summaryText += `=== FOUND DOCUMENTS ===\n`;
    result.documents.forEach((doc, index) => {
      summaryText += `\n${index + 1}. ID: ${doc.id}\n`;
      summaryText += `   Text Length: ${doc.text.length} characters\n`;
      summaryText += `   Word Count: ~${doc.text.split(/\s+/).length} words\n`;
      summaryText += `   Metadata Fields: ${Object.keys(doc.metadata).join(', ') || 'None'}\n`;
      summaryText += `   Preview: ${doc.text.substring(0, 150)}...\n`;
    });
    
    if (result.not_found_ids.length > 0) {
      summaryText += `\n=== NOT FOUND DOCUMENTS ===\n`;
      result.not_found_ids.forEach((id, index) => {
        summaryText += `${index + 1}. ${id}\n`;
      });
    }
    
    await fs.writeFile(summaryPath, summaryText);
    console.log(`📄 Summary report created: ${summaryPath}`);
    
    // Export individual document files if requested
    if (options.exportIndividualFiles) {
      const docsDir = path.join(outputDir, `documents-${timestamp}`);
      await fs.mkdir(docsDir, { recursive: true });
      
      for (const doc of result.documents) {
        const docFilename = `${doc.id.replace(/[^a-zA-Z0-9-_]/g, '_')}.json`;
        const docPath = path.join(docsDir, docFilename);
        await fs.writeFile(docPath, JSON.stringify(doc, null, 2));
      }
      
      console.log(`📁 Individual document files created in: ${docsDir}`);
    }
    
    return {
      details_file: detailsPath,
      summary_file: summaryPath,
      document_count: result.found_count,
      not_found_count: result.not_found_count
    };
    
  } catch (error) {
    console.error('❌ Export failed:', error.message);
    throw error;
  }
}

// Example usage
async function main() {
  const serverUrl = 'http://localhost:8080';
  const documentIds = [
    'doc-123-1691234567890',
    'doc-124-1691234567891',
    'doc-125-1691234567892',
    'nonexistent-doc-999'
  ];

  try {
    // Get document information
    const result = await getDocumentsInfo(serverUrl, documentIds);
    
    console.log('📋 Document Information Results:');
    console.log(`Found: ${result.found_count} documents`);
    console.log(`Not found: ${result.not_found_count} documents`);
    
    // Export with individual files
    await exportDocumentDetails(serverUrl, documentIds, './document-exports', {
      batchSize: 10,
      exportIndividualFiles: true
    });
    
  } catch (error) {
    console.error('❌ Operation failed:', error.message);
  }
}

main();
```

### Browser Implementation with Document Viewer

```html
<!DOCTYPE html>
<html>
<head>
    <title>Document Information Viewer</title>
    <style>
        .container { max-width: 1200px; margin: 20px auto; padding: 20px; }
        .input-section { margin: 20px 0; }
        .id-input { width: 100%; height: 150px; margin: 10px 0; padding: 10px; border: 1px solid #ddd; border-radius: 4px; }
        .controls { display: flex; gap: 10px; margin: 20px 0; }
        .button { padding: 10px 20px; border: none; border-radius: 5px; cursor: pointer; }
        .primary { background: #007bff; color: white; }
        .secondary { background: #6c757d; color: white; }
        .success { background: #28a745; color: white; }
        .stats { display: grid; grid-template-columns: repeat(auto-fit, minmax(200px, 1fr)); gap: 15px; margin: 20px 0; }
        .stat-card { background: white; padding: 15px; border-radius: 8px; box-shadow: 0 2px 4px rgba(0,0,0,0.1); border-left: 4px solid #007bff; }
        .document-grid { display: grid; grid-template-columns: repeat(auto-fill, minmax(400px, 1fr)); gap: 20px; margin: 20px 0; }
        .document-card { background: white; border: 1px solid #ddd; border-radius: 8px; padding: 20px; box-shadow: 0 2px 4px rgba(0,0,0,0.1); }
        .document-id { font-family: monospace; background: #f8f9fa; padding: 5px 10px; border-radius: 4px; margin-bottom: 10px; }
        .document-text { background: #f8f9fa; padding: 15px; border-radius: 4px; margin: 10px 0; max-height: 200px; overflow-y: auto; white-space: pre-wrap; font-size: 14px; }
        .metadata { background: #fff3cd; padding: 10px; border-radius: 4px; margin: 10px 0; }
        .metadata pre { margin: 0; font-size: 12px; }
        .not-found { background: #f8d7da; border: 1px solid #f5c6cb; color: #721c24; padding: 15px; border-radius: 4px; margin: 15px 0; }
        .loading { text-align: center; padding: 40px; color: #6c757d; }
        .error { background: #f8d7da; border: 1px solid #f5c6cb; color: #721c24; padding: 15px; border-radius: 4px; margin: 15px 0; }
        .text-stats { font-size: 12px; color: #6c757d; margin-top: 10px; }
        .expand-btn { background: #e9ecef; border: 1px solid #ced4da; padding: 5px 10px; border-radius: 4px; cursor: pointer; font-size: 12px; margin-top: 10px; }
        .expanded .document-text { max-height: none; }
    </style>
</head>
<body>
    <div class="container">
        <h1>Document Information Viewer</h1>
        
        <div class="input-section">
            <h3>Document IDs to Query</h3>
            <textarea 
                id="document-ids" 
                class="id-input"
                placeholder="Enter document IDs, one per line or comma-separated&#10;&#10;Example:&#10;doc-123-1691234567890&#10;doc-124-1691234567891&#10;doc-125-1691234567892"
            ></textarea>
        </div>
        
        <div class="controls">
            <button class="button primary" onclick="getDocumentInfo()" id="query-btn">Get Document Info</button>
            <button class="button secondary" onclick="clearResults()">Clear Results</button>
            <button class="button success" onclick="exportResults()" id="export-btn" disabled>Export Results</button>
        </div>
        
        <div class="stats" id="stats" style="display: none;">
            <div class="stat-card">
                <strong id="total-requested">0</strong><br>
                Total Requested
            </div>
            <div class="stat-card">
                <strong id="found-count">0</strong><br>
                Documents Found
            </div>
            <div class="stat-card">
                <strong id="not-found-count">0</strong><br>
                Not Found
            </div>
            <div class="stat-card">
                <strong id="avg-length">0</strong><br>
                Avg Text Length
            </div>
        </div>
        
        <div id="results-container">
            <div class="loading" style="display: none;" id="loading">Loading document information...</div>
        </div>
        
        <div id="error-container"></div>
    </div>

    <script>
        let currentResults = null;

        function parseDocumentIds() {
            const input = document.getElementById('document-ids').value.trim();
            if (!input) return [];
            
            const ids = input.split(/[,\n]/)
                           .map(id => id.trim())
                           .filter(id => id.length > 0);
            
            return [...new Set(ids)]; // Remove duplicates
        }

        async function getDocumentInfo() {
            const documentIds = parseDocumentIds();
            
            if (documentIds.length === 0) {
                alert('Please enter at least one document ID.');
                return;
            }

            const queryBtn = document.getElementById('query-btn');
            const exportBtn = document.getElementById('export-btn');
            const loading = document.getElementById('loading');
            const errorContainer = document.getElementById('error-container');
            const resultsContainer = document.getElementById('results-container');

            queryBtn.disabled = true;
            queryBtn.textContent = 'Loading...';
            exportBtn.disabled = true;
            loading.style.display = 'block';
            errorContainer.innerHTML = '';
            resultsContainer.innerHTML = '';
            resultsContainer.appendChild(loading);

            try {
                const response = await fetch('http://localhost:8080/info_documents', {
                    method: 'POST',
                    headers: {
                        'Content-Type': 'application/json',
                    },
                    body: JSON.stringify({
                        ids: documentIds
                    })
                });

                if (!response.ok) {
                    const errorData = await response.json();
                    throw new Error(errorData.error);
                }

                const result = await response.json();
                currentResults = result;
                displayResults(result, documentIds);
                updateStats(result, documentIds);
                exportBtn.disabled = false;

            } catch (error) {
                errorContainer.innerHTML = `<div class="error">❌ Failed to get document info: ${error.message}</div>`;
                resultsContainer.innerHTML = '';
            } finally {
                queryBtn.disabled = false;
                queryBtn.textContent = 'Get Document Info';
                loading.style.display = 'none';
            }
        }

        function updateStats(result, originalIds) {
            const statsDiv = document.getElementById('stats');
            
            document.getElementById('total-requested').textContent = originalIds.length;
            document.getElementById('found-count').textContent = result.found_count;
            document.getElementById('not-found-count').textContent = result.not_found_count;
            
            if (result.documents.length > 0) {
                const totalLength = result.documents.reduce((sum, doc) => sum + doc.text.length, 0);
                const avgLength = Math.round(totalLength / result.documents.length);
                document.getElementById('avg-length').textContent = avgLength;
            } else {
                document.getElementById('avg-length').textContent = '0';
            }
            
            statsDiv.style.display = 'grid';
        }

        function displayResults(result, originalIds) {
            const resultsContainer = document.getElementById('results-container');
            
            let html = '';
            
            // Found documents
            if (result.documents.length > 0) {
                html += '<h3>📄 Found Documents</h3>';
                html += '<div class="document-grid">';
                
                result.documents.forEach((doc, index) => {
                    const wordCount = doc.text.split(/\s+/).length;
                    const hasMetadata = Object.keys(doc.metadata).length > 0;
                    
                    html += `
                        <div class="document-card" id="doc-card-${index}">
                            <div class="document-id">ID: ${doc.id}</div>
                            
                            <div class="text-stats">
                                📊 ${doc.text.length} characters, ~${wordCount} words
                            </div>
                            
                            <div class="document-text">
                                ${escapeHtml(doc.text)}
                            </div>
                            
                            ${doc.text.length > 500 ? 
                                `<button class="expand-btn" onclick="toggleExpand(${index})">
                                    <span id="expand-text-${index}">Show Full Text</span>
                                </button>` : ''}
                            
                            ${hasMetadata ? `
                                <div class="metadata">
                                    <strong>📋 Metadata:</strong>
                                    <pre>${JSON.stringify(doc.metadata, null, 2)}</pre>
                                </div>
                            ` : '<div style="color: #6c757d; font-style: italic;">No metadata</div>'}
                            
                            <div style="margin-top: 15px;">
                                <button class="button" style="background: #ffc107; color: black; padding: 5px 10px; font-size: 12px;" 
                                        onclick="copyToClipboard('${doc.id}')">Copy ID</button>
                                <button class="button" style="background: #17a2b8; color: white; padding: 5px 10px; font-size: 12px;" 
                                        onclick="copyToClipboard(\`${escapeForJs(doc.text)}\`)">Copy Text</button>
                            </div>
                        </div>
                    `;
                });
                
                html += '</div>';
            }
            
            // Not found documents
            if (result.not_found_ids.length > 0) {
                html += '<h3>🔍 Not Found Documents</h3>';
                html += '<div class="not-found">';
                html += '<strong>The following document IDs were not found:</strong><ul>';
                result.not_found_ids.forEach(id => {
                    html += `<li><code>${id}</code></li>`;
                });
                html += '</ul></div>';
            }
            
            resultsContainer.innerHTML = html;
        }

        function toggleExpand(index) {
            const card = document.getElementById(`doc-card-${index}`);
            const expandText = document.getElementById(`expand-text-${index}`);
            
            if (card.classList.contains('expanded')) {
                card.classList.remove('expanded');
                expandText.textContent = 'Show Full Text';
            } else {
                card.classList.add('expanded');
                expandText.textContent = 'Show Less';
            }
        }

        function escapeHtml(text) {
            const div = document.createElement('div');
            div.textContent = text;
            return div.innerHTML;
        }

        function escapeForJs(text) {
            return text.replace(/`/g, '\\`').replace(/\$/g, '\\$').replace(/\\/g, '\\\\');
        }

        function copyToClipboard(text) {
            navigator.clipboard.writeText(text).then(() => {
                // You could add a toast notification here
                console.log('Copied to clipboard');
            }).catch(err => {
                console.error('Failed to copy: ', err);
            });
        }

        function clearResults() {
            document.getElementById('document-ids').value = '';
            document.getElementById('results-container').innerHTML = '<div class="loading" style="display: none;" id="loading">Loading document information...</div>';
            document.getElementById('error-container').innerHTML = '';
            document.getElementById('stats').style.display = 'none';
            document.getElementById('export-btn').disabled = true;
            currentResults = null;
        }

        function exportResults() {
            if (!currentResults) {
                alert('No results to export');
                return;
            }

            const exportData = {
                export_metadata: {
                    timestamp: new Date().toISOString(),
                    export_type: 'document_info',
                    total_requested: document.getElementById('total-requested').textContent,
                    found_count: currentResults.found_count,
                    not_found_count: currentResults.not_found_count
                },
                collection_name: currentResults.collection_name,
                documents: currentResults.documents,
                not_found_ids: currentResults.not_found_ids
            };

            const dataStr = JSON.stringify(exportData, null, 2);
            const dataBlob = new Blob([dataStr], {type: 'application/json'});
            const url = URL.createObjectURL(dataBlob);
            
            const link = document.createElement('a');
            link.href = url;
            link.download = `document-info-${new Date().toISOString().split('T')[0]}.json`;
            document.body.appendChild(link);
            link.click();
            document.body.removeChild(link);
            URL.revokeObjectURL(url);
        }
    </script>
</body>
</html>
```

## Common Error Scenarios

### 1. Empty IDs Array

```javascript
// This will return a 400 error
const response = await fetch('/info_documents', {
  method: 'POST',
  headers: { 'Content-Type': 'application/json' },
  body: JSON.stringify({ ids: [] })
});
// Error: "Invalid request parameters: ids cannot be empty"
```

### 2. Missing ids Field

```javascript
// This will return a 400 error
const response = await fetch('/info_documents', {
  method: 'POST',
  headers: { 'Content-Type': 'application/json' },
  body: JSON.stringify({ document_ids: ["doc-123"] }) // Wrong field name
});
// Error: Missing required field 'ids'
```

### 3. Invalid JSON

```javascript
// This will return a 400 error
const response = await fetch('/info_documents', {
  method: 'POST',
  headers: { 'Content-Type': 'application/json' },
  body: '{"ids": [invalid json}'
});
// Error: "Invalid JSON: ..."
```

## Best Practices

1. **Batch Size**: Limit batch sizes to 25-50 documents to avoid response timeouts
2. **Error Handling**: Check both found and not_found results in responses
3. **Data Validation**: Validate document IDs before making requests
4. **Caching**: Cache document information for frequently accessed documents
5. **Text Handling**: Handle large text content appropriately in UI
6. **Export**: Provide export functionality for document analysis
7. **Performance**: Consider pagination for large document info requests

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

1. **Document Inspection**: View full content and metadata of specific documents
2. **Content Analysis**: Analyze document text and metadata patterns
3. **Quality Assurance**: Verify document content and indexing accuracy
4. **Data Export**: Export document content for backup or migration
5. **Debugging**: Troubleshoot document-related issues
6. **Content Management**: Build interfaces for document editing and management
7. **Compliance**: Audit document content for regulatory requirements
