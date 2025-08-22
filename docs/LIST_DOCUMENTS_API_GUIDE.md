# Kolosal Server - List Documents API Documentation

## Overview

The `/list_documents` API endpoint retrieves a list of all document IDs currently stored in the vector database. This is useful for getting an overview of your document collection, implementing document management interfaces, or preparing for bulk operations.

**Endpoint:** `GET /list_documents`  
**Content-Type:** Not required (GET request)

## Request Format

This endpoint requires **no request body** as it's a GET request. Simply make a GET request to the endpoint.

### Query Parameters

None. This endpoint returns all document IDs in the collection.

## Response Format

### Success Response (200 OK)

```json
{
  "document_ids": ["string", "string", "..."],
  "total_count": "integer",
  "collection_name": "string"
}
```

### Response Fields

| Field | Type | Description |
|-------|------|-------------|
| `document_ids` | array | Array of all document IDs in the collection |
| `total_count` | integer | Total number of documents in the collection |
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
async function listDocuments() {
  try {
    const response = await fetch('http://localhost:8080/list_documents', {
      method: 'GET',
      headers: {
        // Add authentication headers if required
        // 'Authorization': 'Bearer your-token',
        // 'X-API-Key': 'your-api-key'
      }
    });

    if (!response.ok) {
      const errorData = await response.json();
      throw new Error(`API Error (${response.status}): ${errorData.error}`);
    }

    const data = await response.json();
    return data;
  } catch (error) {
    console.error('List documents failed:', error);
    throw error;
  }
}

// Example usage
listDocuments()
.then(result => {
  console.log(`📁 Collection: ${result.collection_name}`);
  console.log(`📊 Total documents: ${result.total_count}`);
  
  if (result.document_ids.length > 0) {
    console.log('📋 Document IDs:');
    result.document_ids.forEach((id, index) => {
      console.log(`  ${index + 1}. ${id}`);
    });
  } else {
    console.log('📭 No documents found in the collection.');
  }
})
.catch(error => {
  console.error('Error listing documents:', error);
});
```

### Using Axios with Filtering and Pagination

```javascript
const axios = require('axios');

class DocumentManager {
  constructor(baseURL = 'http://localhost:8080', apiKey = null) {
    this.client = axios.create({
      baseURL: baseURL,
      headers: {
        ...(apiKey && { 'X-API-Key': apiKey })
      },
      timeout: 30000
    });
  }

  async listAllDocuments() {
    try {
      const response = await this.client.get('/list_documents');
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

  async listDocumentsWithFilter(filterOptions = {}) {
    const allDocuments = await this.listAllDocuments();
    let filteredIds = allDocuments.document_ids;

    // Apply filters
    if (filterOptions.pattern) {
      const regex = new RegExp(filterOptions.pattern, 'i');
      filteredIds = filteredIds.filter(id => regex.test(id));
    }

    if (filterOptions.startsWith) {
      filteredIds = filteredIds.filter(id => id.startsWith(filterOptions.startsWith));
    }

    if (filterOptions.contains) {
      filteredIds = filteredIds.filter(id => id.includes(filterOptions.contains));
    }

    if (filterOptions.limit) {
      filteredIds = filteredIds.slice(0, filterOptions.limit);
    }

    return {
      document_ids: filteredIds,
      total_count: filteredIds.length,
      original_count: allDocuments.total_count,
      collection_name: allDocuments.collection_name,
      filtered: filteredIds.length !== allDocuments.total_count
    };
  }

  async paginateDocuments(page = 1, pageSize = 50) {
    const allDocuments = await this.listAllDocuments();
    const startIndex = (page - 1) * pageSize;
    const endIndex = startIndex + pageSize;
    
    const paginatedIds = allDocuments.document_ids.slice(startIndex, endIndex);
    const totalPages = Math.ceil(allDocuments.total_count / pageSize);

    return {
      document_ids: paginatedIds,
      current_page: page,
      page_size: pageSize,
      total_pages: totalPages,
      total_count: allDocuments.total_count,
      showing_count: paginatedIds.length,
      collection_name: allDocuments.collection_name,
      has_next: page < totalPages,
      has_previous: page > 1
    };
  }

  async getDocumentStatistics() {
    const allDocuments = await this.listAllDocuments();
    const ids = allDocuments.document_ids;

    // Analyze document ID patterns
    const patterns = {};
    const prefixes = {};
    const dateCounts = {};

    ids.forEach(id => {
      // Extract prefix (part before first dash)
      const parts = id.split('-');
      if (parts.length > 1) {
        const prefix = parts[0];
        prefixes[prefix] = (prefixes[prefix] || 0) + 1;

        // Try to extract date pattern
        const dateMatch = id.match(/(\d{4})-?(\d{2})-?(\d{2})/);
        if (dateMatch) {
          const year = dateMatch[1];
          dateCounts[year] = (dateCounts[year] || 0) + 1;
        }
      }

      // Analyze general patterns
      const pattern = id.replace(/\d+/g, 'N').replace(/[a-f0-9]{8,}/g, 'HASH');
      patterns[pattern] = (patterns[pattern] || 0) + 1;
    });

    return {
      total_count: allDocuments.total_count,
      collection_name: allDocuments.collection_name,
      patterns: patterns,
      prefixes: prefixes,
      yearly_distribution: dateCounts,
      sample_ids: ids.slice(0, 5) // First 5 IDs as samples
    };
  }
}

// Example usage
const docManager = new DocumentManager('http://localhost:8080');

async function exploreDocuments() {
  try {
    console.log('🔍 Exploring document collection...\n');

    // Get basic list
    const allDocs = await docManager.listAllDocuments();
    console.log(`📊 Found ${allDocs.total_count} documents in "${allDocs.collection_name}"`);

    // Get statistics
    const stats = await docManager.getDocumentStatistics();
    console.log('\n📈 Document Statistics:');
    console.log('Prefixes:', stats.prefixes);
    console.log('Patterns:', stats.patterns);
    console.log('Yearly distribution:', stats.yearly_distribution);

    // Filter examples
    const tempDocs = await docManager.listDocumentsWithFilter({
      startsWith: 'temp-',
      limit: 10
    });
    console.log(`\n🔍 Temporary documents: ${tempDocs.total_count}`);

    // Pagination example
    const firstPage = await docManager.paginateDocuments(1, 20);
    console.log(`\n📄 Page 1: Showing ${firstPage.showing_count}/${firstPage.total_count} documents`);
    console.log(`Total pages: ${firstPage.total_pages}`);

  } catch (error) {
    console.error('❌ Exploration failed:', error.message);
  }
}

exploreDocuments();
```

### Node.js with Export/Backup Functionality

```javascript
const fs = require('fs').promises;
const path = require('path');
const https = require('https');
const http = require('http');
const url = require('url');

function listDocuments(serverUrl, options = {}) {
  return new Promise((resolve, reject) => {
    const parsedUrl = url.parse(serverUrl);
    const requestModule = parsedUrl.protocol === 'https:' ? https : http;
    
    const requestOptions = {
      hostname: parsedUrl.hostname,
      port: parsedUrl.port,
      path: '/list_documents',
      method: 'GET',
      headers: {
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

    req.setTimeout(30000);
    req.end();
  });
}

async function exportDocumentList(serverUrl, outputDir = './exports', options = {}) {
  try {
    console.log('📋 Fetching document list...');
    const result = await listDocuments(serverUrl, options);
    
    // Create export directory if it doesn't exist
    await fs.mkdir(outputDir, { recursive: true });
    
    const timestamp = new Date().toISOString().replace(/[:.]/g, '-');
    const filename = `document-list-${timestamp}.json`;
    const filepath = path.join(outputDir, filename);
    
    // Prepare export data
    const exportData = {
      export_timestamp: new Date().toISOString(),
      server_url: serverUrl,
      collection_name: result.collection_name,
      total_count: result.total_count,
      document_ids: result.document_ids,
      metadata: {
        export_version: '1.0',
        format: 'kolosal-document-list'
      }
    };
    
    // Write to file
    await fs.writeFile(filepath, JSON.stringify(exportData, null, 2));
    
    console.log(`✅ Document list exported to: ${filepath}`);
    console.log(`📊 Total documents: ${result.total_count}`);
    
    // Also create a simple text file with just IDs
    const textFilename = `document-ids-${timestamp}.txt`;
    const textFilepath = path.join(outputDir, textFilename);
    await fs.writeFile(textFilepath, result.document_ids.join('\n'));
    
    console.log(`📄 Text file created: ${textFilepath}`);
    
    return {
      json_file: filepath,
      text_file: textFilepath,
      document_count: result.total_count
    };
    
  } catch (error) {
    console.error('❌ Export failed:', error.message);
    throw error;
  }
}

async function generateDocumentReport(serverUrl, options = {}) {
  try {
    const result = await listDocuments(serverUrl, options);
    
    // Generate comprehensive report
    const report = {
      summary: {
        collection: result.collection_name,
        total_documents: result.total_count,
        generated_at: new Date().toISOString()
      },
      analysis: {
        id_patterns: {},
        prefixes: {},
        suffixes: {},
        length_distribution: {},
        date_patterns: []
      },
      sample_ids: result.document_ids.slice(0, 10)
    };
    
    // Analyze document IDs
    result.document_ids.forEach(id => {
      // Length distribution
      const length = id.length;
      const lengthBucket = Math.floor(length / 10) * 10;
      const lengthKey = `${lengthBucket}-${lengthBucket + 9}`;
      report.analysis.length_distribution[lengthKey] = 
        (report.analysis.length_distribution[lengthKey] || 0) + 1;
      
      // Prefix analysis (first part before -)
      const parts = id.split('-');
      if (parts.length > 0) {
        const prefix = parts[0];
        report.analysis.prefixes[prefix] = (report.analysis.prefixes[prefix] || 0) + 1;
      }
      
      // Suffix analysis (last part after -)
      if (parts.length > 1) {
        const suffix = parts[parts.length - 1];
        // Only track if it's not too long (likely not a hash)
        if (suffix.length <= 20) {
          report.analysis.suffixes[suffix] = (report.analysis.suffixes[suffix] || 0) + 1;
        }
      }
      
      // Pattern analysis
      const pattern = id.replace(/\d+/g, 'N').replace(/[a-f0-9]{32,}/g, 'HASH');
      report.analysis.id_patterns[pattern] = (report.analysis.id_patterns[pattern] || 0) + 1;
      
      // Date extraction
      const dateMatch = id.match(/(\d{13})/); // Unix timestamp in milliseconds
      if (dateMatch) {
        const timestamp = parseInt(dateMatch[1]);
        const date = new Date(timestamp);
        if (date.getFullYear() > 2020 && date.getFullYear() < 2030) {
          report.analysis.date_patterns.push({
            id: id,
            extracted_date: date.toISOString(),
            timestamp: timestamp
          });
        }
      }
    });
    
    // Sort analysis results
    report.analysis.prefixes = Object.fromEntries(
      Object.entries(report.analysis.prefixes).sort((a, b) => b[1] - a[1])
    );
    
    console.log('📊 Document Collection Report:');
    console.log(`Collection: ${report.summary.collection}`);
    console.log(`Total Documents: ${report.summary.total_documents}`);
    console.log('\nTop Prefixes:');
    Object.entries(report.analysis.prefixes).slice(0, 5).forEach(([prefix, count]) => {
      console.log(`  ${prefix}: ${count} documents`);
    });
    
    console.log('\nID Length Distribution:');
    Object.entries(report.analysis.length_distribution).forEach(([range, count]) => {
      console.log(`  ${range} chars: ${count} documents`);
    });
    
    if (report.analysis.date_patterns.length > 0) {
      console.log(`\nFound ${report.analysis.date_patterns.length} documents with timestamp patterns`);
    }
    
    return report;
    
  } catch (error) {
    console.error('❌ Report generation failed:', error.message);
    throw error;
  }
}

// Example usage
async function main() {
  const serverUrl = 'http://localhost:8080';
  
  try {
    // Basic listing
    const result = await listDocuments(serverUrl);
    console.log(`📋 Listed ${result.total_count} documents\n`);
    
    // Export functionality
    await exportDocumentList(serverUrl);
    
    // Generate report
    await generateDocumentReport(serverUrl);
    
  } catch (error) {
    console.error('❌ Operation failed:', error.message);
  }
}

main();
```

### Browser Implementation with Management Interface

```html
<!DOCTYPE html>
<html>
<head>
    <title>Document List Manager</title>
    <style>
        .container { max-width: 1200px; margin: 20px auto; padding: 20px; }
        .controls { display: flex; gap: 10px; margin: 20px 0; align-items: center; }
        .search-box { flex: 1; padding: 8px; border: 1px solid #ddd; border-radius: 4px; }
        .button { padding: 8px 16px; border: none; border-radius: 4px; cursor: pointer; }
        .primary { background: #007bff; color: white; }
        .secondary { background: #6c757d; color: white; }
        .stats { display: grid; grid-template-columns: repeat(auto-fit, minmax(200px, 1fr)); gap: 15px; margin: 20px 0; }
        .stat-card { background: white; padding: 15px; border-radius: 8px; box-shadow: 0 2px 4px rgba(0,0,0,0.1); border-left: 4px solid #007bff; }
        .document-list { max-height: 500px; overflow-y: auto; border: 1px solid #ddd; border-radius: 4px; margin: 20px 0; }
        .document-item { padding: 10px; border-bottom: 1px solid #eee; display: flex; justify-content: between; align-items: center; }
        .document-item:hover { background: #f8f9fa; }
        .document-item:last-child { border-bottom: none; }
        .document-id { font-family: monospace; flex: 1; }
        .document-actions { display: flex; gap: 5px; }
        .document-actions button { padding: 4px 8px; font-size: 12px; }
        .loading { text-align: center; padding: 40px; color: #6c757d; }
        .error { background: #f8d7da; border: 1px solid #f5c6cb; color: #721c24; padding: 15px; border-radius: 4px; margin: 15px 0; }
        .pagination { display: flex; justify-content: center; gap: 10px; margin: 20px 0; }
        .pagination button { padding: 8px 12px; border: 1px solid #ddd; background: white; cursor: pointer; border-radius: 4px; }
        .pagination button.active { background: #007bff; color: white; border-color: #007bff; }
        .pagination button:disabled { opacity: 0.5; cursor: not-allowed; }
    </style>
</head>
<body>
    <div class="container">
        <h1>Document Collection Manager</h1>
        
        <div class="controls">
            <input type="text" class="search-box" id="search-box" placeholder="Search document IDs...">
            <button class="button primary" onclick="refreshDocumentList()">Refresh</button>
            <button class="button secondary" onclick="exportDocumentList()">Export</button>
            <button class="button secondary" onclick="selectAll()">Select All</button>
            <button class="button secondary" onclick="clearSelection()">Clear Selection</button>
        </div>
        
        <div class="stats" id="stats" style="display: none;">
            <div class="stat-card">
                <strong id="total-count">0</strong><br>
                Total Documents
            </div>
            <div class="stat-card">
                <strong id="filtered-count">0</strong><br>
                Shown
            </div>
            <div class="stat-card">
                <strong id="selected-count">0</strong><br>
                Selected
            </div>
            <div class="stat-card">
                <strong id="collection-name">-</strong><br>
                Collection
            </div>
        </div>
        
        <div class="pagination" id="pagination" style="display: none;"></div>
        
        <div id="document-list-container">
            <div class="loading">Click "Refresh" to load documents</div>
        </div>
        
        <div class="pagination" id="pagination-bottom" style="display: none;"></div>
        
        <div id="error-container"></div>
    </div>

    <script>
        let allDocuments = [];
        let filteredDocuments = [];
        let selectedDocuments = new Set();
        let currentPage = 1;
        const documentsPerPage = 50;

        async function refreshDocumentList() {
            const container = document.getElementById('document-list-container');
            const errorContainer = document.getElementById('error-container');
            
            container.innerHTML = '<div class="loading">Loading documents...</div>';
            errorContainer.innerHTML = '';
            
            try {
                const response = await fetch('http://localhost:8080/list_documents');
                
                if (!response.ok) {
                    const errorData = await response.json();
                    throw new Error(errorData.error);
                }
                
                const result = await response.json();
                allDocuments = result.document_ids;
                filteredDocuments = [...allDocuments];
                
                updateStats(result);
                updateDocumentList();
                updatePagination();
                
                document.getElementById('stats').style.display = 'grid';
                
            } catch (error) {
                errorContainer.innerHTML = `<div class="error">❌ Failed to load documents: ${error.message}</div>`;
                container.innerHTML = '<div class="loading">Failed to load documents</div>';
            }
        }

        function updateStats(result) {
            document.getElementById('total-count').textContent = result.total_count;
            document.getElementById('filtered-count').textContent = filteredDocuments.length;
            document.getElementById('selected-count').textContent = selectedDocuments.size;
            document.getElementById('collection-name').textContent = result.collection_name;
        }

        function updateDocumentList() {
            const container = document.getElementById('document-list-container');
            const startIndex = (currentPage - 1) * documentsPerPage;
            const endIndex = startIndex + documentsPerPage;
            const pageDocuments = filteredDocuments.slice(startIndex, endIndex);
            
            if (pageDocuments.length === 0) {
                container.innerHTML = '<div class="loading">No documents found</div>';
                return;
            }
            
            let html = '<div class="document-list">';
            
            pageDocuments.forEach(id => {
                const isSelected = selectedDocuments.has(id);
                html += `
                    <div class="document-item">
                        <input type="checkbox" ${isSelected ? 'checked' : ''} 
                               onchange="toggleSelection('${id}')" style="margin-right: 10px;">
                        <span class="document-id">${id}</span>
                        <div class="document-actions">
                            <button class="button" style="background: #ffc107; color: black;" 
                                    onclick="copyToClipboard('${id}')">Copy</button>
                            <button class="button" style="background: #dc3545; color: white;" 
                                    onclick="removeDocument('${id}')">Remove</button>
                        </div>
                    </div>
                `;
            });
            
            html += '</div>';
            container.innerHTML = html;
        }

        function updatePagination() {
            const totalPages = Math.ceil(filteredDocuments.length / documentsPerPage);
            const paginationTop = document.getElementById('pagination');
            const paginationBottom = document.getElementById('pagination-bottom');
            
            if (totalPages <= 1) {
                paginationTop.style.display = 'none';
                paginationBottom.style.display = 'none';
                return;
            }
            
            let html = '';
            
            // Previous button
            html += `<button ${currentPage === 1 ? 'disabled' : ''} onclick="goToPage(${currentPage - 1})">‹ Previous</button>`;
            
            // Page numbers
            for (let i = 1; i <= totalPages; i++) {
                if (i === 1 || i === totalPages || (i >= currentPage - 2 && i <= currentPage + 2)) {
                    const activeClass = i === currentPage ? 'active' : '';
                    html += `<button class="${activeClass}" onclick="goToPage(${i})">${i}</button>`;
                } else if (i === currentPage - 3 || i === currentPage + 3) {
                    html += '<span>...</span>';
                }
            }
            
            // Next button
            html += `<button ${currentPage === totalPages ? 'disabled' : ''} onclick="goToPage(${currentPage + 1})">Next ›</button>`;
            
            paginationTop.innerHTML = html;
            paginationBottom.innerHTML = html;
            paginationTop.style.display = 'flex';
            paginationBottom.style.display = 'flex';
        }

        function goToPage(page) {
            currentPage = page;
            updateDocumentList();
            updatePagination();
        }

        function toggleSelection(documentId) {
            if (selectedDocuments.has(documentId)) {
                selectedDocuments.delete(documentId);
            } else {
                selectedDocuments.add(documentId);
            }
            document.getElementById('selected-count').textContent = selectedDocuments.size;
        }

        function selectAll() {
            filteredDocuments.forEach(id => selectedDocuments.add(id));
            document.getElementById('selected-count').textContent = selectedDocuments.size;
            updateDocumentList();
        }

        function clearSelection() {
            selectedDocuments.clear();
            document.getElementById('selected-count').textContent = 0;
            updateDocumentList();
        }

        function copyToClipboard(text) {
            navigator.clipboard.writeText(text).then(() => {
                // Could add a toast notification here
                console.log('Copied to clipboard:', text);
            });
        }

        async function removeDocument(documentId) {
            if (!confirm(`Are you sure you want to remove document: ${documentId}?`)) {
                return;
            }
            
            try {
                const response = await fetch('http://localhost:8080/remove_documents', {
                    method: 'POST',
                    headers: { 'Content-Type': 'application/json' },
                    body: JSON.stringify({ document_ids: [documentId] })
                });
                
                if (!response.ok) {
                    const errorData = await response.json();
                    throw new Error(errorData.error);
                }
                
                // Refresh the list
                await refreshDocumentList();
                
            } catch (error) {
                alert('Failed to remove document: ' + error.message);
            }
        }

        function exportDocumentList() {
            const dataStr = JSON.stringify({
                export_date: new Date().toISOString(),
                total_count: allDocuments.length,
                document_ids: allDocuments
            }, null, 2);
            
            const dataBlob = new Blob([dataStr], {type: 'application/json'});
            const url = URL.createObjectURL(dataBlob);
            
            const link = document.createElement('a');
            link.href = url;
            link.download = `document-list-${new Date().toISOString().split('T')[0]}.json`;
            document.body.appendChild(link);
            link.click();
            document.body.removeChild(link);
            URL.revokeObjectURL(url);
        }

        // Search functionality
        document.getElementById('search-box').addEventListener('input', function(e) {
            const searchTerm = e.target.value.toLowerCase();
            filteredDocuments = allDocuments.filter(id => 
                id.toLowerCase().includes(searchTerm)
            );
            currentPage = 1;
            updateDocumentList();
            updatePagination();
            document.getElementById('filtered-count').textContent = filteredDocuments.length;
        });

        // Auto-refresh every 30 seconds
        setInterval(() => {
            if (allDocuments.length > 0) {
                refreshDocumentList();
            }
        }, 30000);
    </script>
</body>
</html>
```

## Common Error Scenarios

### 1. Service Unavailable

```javascript
// When database connection fails
// Error response: 503 Service Unavailable
// { "error": "Database connection failed", "error_type": "service_unavailable" }
```

### 2. Server Error

```javascript
// When document service initialization fails
// Error response: 500 Internal Server Error
// { "error": "Failed to initialize document service", "error_type": "service_error" }
```

## Best Practices

1. **Caching**: Cache the document list locally to reduce API calls
2. **Pagination**: Implement client-side pagination for large collections
3. **Filtering**: Add search and filter capabilities for better UX
4. **Auto-refresh**: Periodically refresh the list to stay up-to-date
5. **Error Handling**: Always handle network and API errors gracefully
6. **Performance**: Consider using virtual scrolling for very large lists
7. **Export**: Provide export functionality for backup purposes

## Response Headers

The API includes CORS headers for cross-origin requests:

```http
Access-Control-Allow-Origin: *
Access-Control-Allow-Methods: GET, OPTIONS
Access-Control-Allow-Headers: Content-Type, Authorization, X-API-Key
```

## Authentication

If your server is configured with authentication, include the appropriate headers:

```javascript
headers: {
  'Authorization': 'Bearer your-jwt-token',
  // or
  'X-API-Key': 'your-api-key'
}
```

## Use Cases

1. **Document Management**: Build admin interfaces to manage documents
2. **Bulk Operations**: Get IDs for bulk removal or modification operations
3. **Monitoring**: Track collection growth and document patterns
4. **Backup**: Export document lists for backup and migration
5. **Analytics**: Analyze document ID patterns and collection statistics
6. **Debugging**: Verify document addition and removal operations
