# Kolosal Server - Retrieve API Documentation

## Overview

The `/retrieve` API endpoint performs vector similarity search to find documents most similar to a given query. This endpoint uses semantic embeddings to find relevant documents based on meaning rather than exact keyword matches.

**Endpoint:** `POST /retrieve`  
**Content-Type:** `application/json`

## Request Format

### Request Body Schema

```json
{
  "query": "string (required)",
  "k": "integer (optional, default: 10)",
  "collection_name": "string (optional)",
  "score_threshold": "number (optional, default: 0.0)"
}
```

### Request Parameters

| Parameter | Type | Required | Default | Description |
|-----------|------|----------|---------|-------------|
| `query` | string | Yes | - | The search query text to find similar documents |
| `k` | integer | No | 10 | Number of top similar documents to return (1-1000) |
| `collection_name` | string | No | "documents" | Name of the collection to search in |
| `score_threshold` | number | No | 0.0 | Minimum similarity score threshold (0.0-1.0) |

## Response Format

### Success Response (200 OK)

```json
{
  "query": "string",
  "k": "integer",
  "collection_name": "string",
  "score_threshold": "number",
  "total_found": "integer",
  "documents": [
    {
      "id": "string",
      "text": "string",
      "metadata": "object",
      "score": "number"
    }
  ]
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
async function searchDocuments(query, options = {}) {
  const requestBody = {
    query: query,
    k: options.k || 10,
    ...(options.collectionName && { collection_name: options.collectionName }),
    ...(options.scoreThreshold && { score_threshold: options.scoreThreshold })
  };

  try {
    const response = await fetch('http://localhost:8080/retrieve', {
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
    console.error('Search failed:', error);
    throw error;
  }
}

// Example usage
searchDocuments("machine learning algorithms", {
  k: 5,
  scoreThreshold: 0.7
})
.then(results => {
  console.log(`Found ${results.total_found} documents:`);
  results.documents.forEach((doc, index) => {
    console.log(`${index + 1}. Score: ${doc.score.toFixed(3)}`);
    console.log(`   ID: ${doc.id}`);
    console.log(`   Text: ${doc.text.substring(0, 100)}...`);
    console.log(`   Metadata:`, doc.metadata);
  });
})
.catch(error => {
  console.error('Search error:', error);
});
```

### Using Axios

```javascript
const axios = require('axios');

class KolosalSearchClient {
  constructor(baseURL = 'http://localhost:8080', apiKey = null) {
    this.client = axios.create({
      baseURL: baseURL,
      headers: {
        'Content-Type': 'application/json',
        ...(apiKey && { 'X-API-Key': apiKey })
      }
    });
  }

  async search(query, options = {}) {
    const requestData = {
      query: query,
      k: options.k || 10,
      ...(options.collectionName && { collection_name: options.collectionName }),
      ...(options.scoreThreshold && { score_threshold: options.scoreThreshold })
    };

    try {
      const response = await this.client.post('/retrieve', requestData);
      return response.data;
    } catch (error) {
      if (error.response) {
        // Server responded with error status
        const errorData = error.response.data;
        throw new Error(`API Error (${error.response.status}): ${errorData.error}`);
      } else if (error.request) {
        // Request was made but no response received
        throw new Error('No response from server');
      } else {
        // Something else happened
        throw new Error(`Request error: ${error.message}`);
      }
    }
  }
}

// Example usage
const client = new KolosalSearchClient('http://localhost:8080');

async function performSearch() {
  try {
    const results = await client.search("artificial intelligence trends", {
      k: 3,
      scoreThreshold: 0.8
    });
    
    console.log('Search Results:', results);
  } catch (error) {
    console.error('Search failed:', error.message);
  }
}

performSearch();
```

### Node.js with Error Handling

```javascript
const https = require('https');
const http = require('http');
const url = require('url');

function searchDocuments(serverUrl, query, options = {}) {
  return new Promise((resolve, reject) => {
    const parsedUrl = url.parse(serverUrl);
    const requestModule = parsedUrl.protocol === 'https:' ? https : http;
    
    const requestData = JSON.stringify({
      query: query,
      k: options.k || 10,
      ...(options.collectionName && { collection_name: options.collectionName }),
      ...(options.scoreThreshold && { score_threshold: options.scoreThreshold })
    });

    const requestOptions = {
      hostname: parsedUrl.hostname,
      port: parsedUrl.port,
      path: '/retrieve',
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

// Example usage
searchDocuments('http://localhost:8080', 'deep learning neural networks', {
  k: 5,
  scoreThreshold: 0.6
})
.then(results => {
  console.log(`Query: "${results.query}"`);
  console.log(`Found ${results.total_found} documents:`);
  
  results.documents.forEach((doc, index) => {
    console.log(`\n${index + 1}. Document ID: ${doc.id}`);
    console.log(`   Score: ${doc.score.toFixed(4)}`);
    console.log(`   Text: ${doc.text}`);
    if (Object.keys(doc.metadata).length > 0) {
      console.log(`   Metadata:`, JSON.stringify(doc.metadata, null, 2));
    }
  });
})
.catch(error => {
  console.error('Error:', error.message);
});
```

### Browser Implementation with CORS

```html
<!DOCTYPE html>
<html>
<head>
    <title>Kolosal Search Demo</title>
</head>
<body>
    <div id="search-container">
        <input type="text" id="search-input" placeholder="Enter your search query">
        <button onclick="performSearch()">Search</button>
        <div id="results"></div>
    </div>

    <script>
        async function performSearch() {
            const query = document.getElementById('search-input').value;
            if (!query.trim()) {
                alert('Please enter a search query');
                return;
            }

            const resultsDiv = document.getElementById('results');
            resultsDiv.innerHTML = 'Searching...';

            try {
                const response = await fetch('http://localhost:8080/retrieve', {
                    method: 'POST',
                    headers: {
                        'Content-Type': 'application/json',
                    },
                    body: JSON.stringify({
                        query: query,
                        k: 5,
                        score_threshold: 0.5
                    })
                });

                if (!response.ok) {
                    const errorData = await response.json();
                    throw new Error(errorData.error);
                }

                const data = await response.json();
                displayResults(data);
            } catch (error) {
                resultsDiv.innerHTML = `<p style="color: red;">Error: ${error.message}</p>`;
            }
        }

        function displayResults(data) {
            const resultsDiv = document.getElementById('results');
            
            if (data.total_found === 0) {
                resultsDiv.innerHTML = '<p>No documents found.</p>';
                return;
            }

            let html = `<h3>Found ${data.total_found} documents:</h3>`;
            
            data.documents.forEach((doc, index) => {
                html += `
                    <div style="border: 1px solid #ccc; margin: 10px 0; padding: 10px;">
                        <h4>Document ${index + 1} (Score: ${doc.score.toFixed(3)})</h4>
                        <p><strong>ID:</strong> ${doc.id}</p>
                        <p><strong>Text:</strong> ${doc.text}</p>
                        ${Object.keys(doc.metadata).length > 0 ? 
                          `<p><strong>Metadata:</strong> ${JSON.stringify(doc.metadata)}</p>` : ''}
                    </div>
                `;
            });
            
            resultsDiv.innerHTML = html;
        }
    </script>
</body>
</html>
```

## Common Error Scenarios

### 1. Empty Query

```javascript
// This will return a 400 error
const response = await fetch('/retrieve', {
  method: 'POST',
  headers: { 'Content-Type': 'application/json' },
  body: JSON.stringify({ query: "" })
});
// Error: "Invalid request parameters"
```

### 2. Invalid Parameters

```javascript
// Invalid k value
const response = await fetch('/retrieve', {
  method: 'POST',
  headers: { 'Content-Type': 'application/json' },
  body: JSON.stringify({ 
    query: "test",
    k: 0 // Invalid: k must be positive
  })
});
// Error: "Field 'k' must be a positive integer"
```

### 3. Service Unavailable

```javascript
// When database connection fails
// Error response: 503 Service Unavailable
// { "error": "Database connection failed", "error_type": "service_unavailable" }
```

## Best Practices

1. **Query Optimization**: Use clear, descriptive queries for better semantic matching
2. **Result Limiting**: Set appropriate `k` values to avoid large response payloads
3. **Score Thresholding**: Use `score_threshold` to filter out low-relevance results
4. **Error Handling**: Always implement proper error handling for network and API errors
5. **Connection Pooling**: Reuse HTTP connections for better performance in high-frequency scenarios
6. **Timeout Handling**: Set appropriate timeouts for requests (recommended: 30 seconds)

## Response Headers

The API includes CORS headers for cross-origin requests:

```http
Access-Control-Allow-Origin: *
Access-Control-Allow-Methods: GET, POST, OPTIONS
Access-Control-Allow-Headers: Content-Type, Authorization, X-API-Key
```

## Rate Limiting

Currently, there are no built-in rate limits, but it's recommended to implement client-side throttling for production usage to avoid overwhelming the server.

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
