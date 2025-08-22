# Kolosal Server - Document Parsing API Guide (/parse_pdf, /parse_docx, /parse_html)

## Overview

These endpoints let you extract plain text (PDF, DOCX) or Markdown (HTML) from raw document content supplied in a single POST request. PDF and DOCX inputs must be base64-encoded binary data; HTML input is raw HTML text (string). Returned content can be used for downstream indexing, chunking, or retrieval.

| Endpoint | Purpose | Input Key | Input Encoding | Output Key | Notes |
|----------|---------|-----------|----------------|------------|-------|
| `POST /parse_pdf`  | Parse PDF to text | `data` | base64 string | `text` | Supports parsing methods: `fast`, `ocr`, `visual` |
| `POST /parse_docx` | Parse DOCX to text | `data` | base64 string | `text` | Single pass; pages count reported as 1 |
| `POST /parse_html` | Convert HTML to Markdown | `html` | raw string | `markdown` | Cleans & normalizes output |

All three endpoints return a JSON object with `success` plus additional metadata. On failure `success=false` and `error` (and sometimes `details`) are provided.

CORS preflight (`OPTIONS`) returns a small JSON capability description.

## Shared Behavior
- Method: `POST`
- Content-Type: `application/json`
- CORS: `Access-Control-Allow-Origin: *`
- Authentication headers (e.g., `Authorization`, `X-API-Key`) are accepted if server config requires them.
- Empty or malformed JSON returns `400` with an error message.
- Internal exceptions return `500`.

## 1. Parse PDF (`POST /parse_pdf`)
Extract text from a PDF file in memory. Choose among three parsing strategies:

| Method | Value | When to Use | Trade-offs |
|--------|-------|-------------|------------|
| Fast   | `fast` (default) | Digital PDFs with embedded text | Fastest; no OCR for scanned pages |
| OCR    | `ocr`  | Scanned or image-based PDFs | Slower; requires OCR language; higher CPU usage |
| Visual | `visual` | Preserve some layout cues | Slower than `fast`; experimental formatting |

### Request Body
```json
{
  "data": "<base64-pdf-bytes>",
  "method": "fast|ocr|visual (optional)",
  "language": "eng|fra|... (optional, OCR only)",
  "progress": false
}
```

Field details:
- `data` (string, required): Base64-encoded raw PDF bytes.
- `method` (string, optional): Parsing method; defaults to `fast` if missing or invalid.
- `language` (string, optional): ISO 3-letter language code for OCR; defaults to `eng`.
- `progress` (bool, optional): If true, server logs per-page progress (not streamed to client).

### Success Response (200)
```json
{
  "success": true,
  "text": "...extracted text...",
  "pages_processed": 12,
  "method": "fast",
  "language": "eng",
  "data_size_bytes": 458732
}
```

### Error Response (4xx/5xx)
```json
{
  "success": false,
  "error": "Failed to decode base64 data",
  "details": "<optional internal detail>",
  "method": "fast",
  "language": "eng",
  "pages_processed": 3,
  "data_size_bytes": 458732
}
```

## 2. Parse DOCX (`POST /parse_docx`)
Extract plain text from a DOCX file provided as base64.

### Request Body
```json
{
  "data": "<base64-docx-bytes>"
}
```

### Success Response (200)
```json
{
  "success": true,
  "text": "...extracted text...",
  "pages_processed": 1,
  "data_size_bytes": 12345
}
```
`pages_processed` is always `1` (DOCX page concept not tracked by parser).

### Error Response
```json
{
  "success": false,
  "error": "Empty document data",
  "data_size_bytes": 0,
  "pages_processed": 1
}
```

## 3. Parse HTML (`POST /parse_html`)
Convert raw HTML string content into cleaned Markdown.

### Request Body
```json
{
  "html": "<div><h1>Title</h1><p>Paragraph</p></div>"
}
```

### Success Response (200)
```json
{
  "success": true,
  "markdown": "# Title\n\nParagraph\n",
  "elements_processed": 42
}
```

### Error Response
```json
{
  "success": false,
  "error": "Failed to parse HTML content",
  "elements_processed": 10
}
```

## Example (JavaScript Fetch) - PDF
```javascript
async function parsePdf(base64Data) {
  const res = await fetch('http://localhost:8080/parse_pdf', {
    method: 'POST',
    headers: { 'Content-Type': 'application/json' },
    body: JSON.stringify({ data: base64Data, method: 'fast', language: 'eng' })
  });
  if (!res.ok) throw new Error(await res.text());
  return res.json();
}
```

## Example (JavaScript Fetch) - DOCX
```javascript
async function parseDocx(base64Data) {
  const res = await fetch('http://localhost:8080/parse_docx', {
    method: 'POST',
    headers: { 'Content-Type': 'application/json' },
    body: JSON.stringify({ data: base64Data })
  });
  if (!res.ok) throw new Error(await res.text());
  return res.json();
}
```

## Example (JavaScript Fetch) - HTML
```javascript
async function parseHtml(html) {
  const res = await fetch('http://localhost:8080/parse_html', {
    method: 'POST',
    headers: { 'Content-Type': 'application/json' },
    body: JSON.stringify({ html })
  });
  if (!res.ok) throw new Error(await res.text());
  return res.json();
}
```

## Base64 Encoding Helpers
Node.js:
```javascript
const fs = require('fs');
const pdfB64 = fs.readFileSync('sample.pdf').toString('base64');
```
Browser:
```javascript
function fileToBase64(file) {
  return new Promise((resolve, reject) => {
    const reader = new FileReader();
    reader.onload = () => resolve(reader.result.split(',')[1]);
    reader.onerror = reject;
    reader.readAsDataURL(file);
  });
}
```

## Common Errors & Causes
| Error Message | Cause | Fix |
|---------------|-------|-----|
| Missing or invalid 'data' field | `data` absent or not a string (PDF/DOCX) | Supply base64 string under `data` |
| Missing or invalid 'html' field | `html` absent or not a string | Provide HTML string |
| Empty document data | Base64 decoded to empty buffer | Ensure file read correctly; check size |
| Failed to decode base64 data | Corrupt/incorrect base64 | Re-encode file bytes |
| Invalid JSON format | Malformed JSON body | Validate JSON before sending |
| Internal server error | Unexpected exception | Retry; inspect server logs |

## Performance Tips
- Prefer `fast` for most digital PDFs. Use `ocr` only when necessary (scanned pages). 
- Large PDFs: consider splitting before upload to reduce single request size.
- Avoid sending excessively large HTML blobs with unused script/style sections—strip them client-side.
- OCR increases CPU usage and latency; batch such requests conservatively.

## Security Considerations
- Only accept documents from trusted sources to avoid malicious payloads.
- Enforce size limits at reverse proxy or prior middleware if needed.
- Sanitize/strip HTML client-side if you only need textual content.

## Response Headers
```http
Access-Control-Allow-Origin: *
Access-Control-Allow-Methods: POST, OPTIONS
Access-Control-Allow-Headers: Content-Type, Authorization, X-API-Key
```

## Quick Integration Flow
1. Read file bytes (PDF/DOCX) or gather HTML string.
2. Base64 encode binary files.
3. POST to the appropriate endpoint.
4. On success, feed `text` or `markdown` into chunking / embedding pipeline.
5. Handle and log errors with original file reference for retry.

## Versioning & Changes
Current implementation auto-detects path to choose parser. Future changes may introduce streaming or partial-page outputs for very large PDFs.

---
If you need batch ingestion, combine these parse calls with your `/add_documents` and `/chunking` flows.
