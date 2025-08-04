# Kolosal Server - Engines API Documentation

## Overview

The Engines API provides comprehensive management capabilities for inference engines in the Kolosal Server. This API allows you to list available engines, add new engines, and configure the default engine for model inference operations.

**Base URL**: `http://localhost:8080` (or your configured server address)

**API Endpoints**:

- `GET /engines` or `GET /v1/engines` - List all available inference engines
- `POST /engines` or `POST /v1/engines` - Add a new inference engine  
- `PUT /engines` or `PUT /v1/engines` - Set the default inference engine

## Authentication

All endpoints support the same authentication methods as other Kolosal Server APIs:

- **Bearer Token**: `Authorization: Bearer <your-jwt-token>`
- **API Key**: `X-API-Key: <your-api-key>`

## Content-Type

All requests that include a body must use:

```http
Content-Type: application/json
```

---

## 1. List Inference Engines

Retrieves a list of all available inference engines, including their configuration and status.

### List Engines Request

```http
GET /engines
```

### Parameters

None

### List Engines Response (200 OK)

```json
{
  "inference_engines": [
    {
      "name": "llama-cpp",
      "version": "1.0.0",
      "description": "LLaMA.cpp inference engine for efficient CPU/GPU inference",
      "library_path": "/path/to/llama-cpp-engine.dll",
      "is_loaded": true,
      "is_default": true
    },
    {
      "name": "onnx-runtime",
      "version": "1.2.0", 
      "description": "ONNX Runtime for cross-platform ML inference",
      "library_path": "/path/to/onnx-engine.dll",
      "is_loaded": false,
      "is_default": false
    }
  ],
  "default_engine": "llama-cpp",
  "total_count": 2
}
```

### Response Fields

- **inference_engines**: Array of engine objects
  - **name**: Unique identifier for the engine
  - **version**: Engine version string
  - **description**: Human-readable description
  - **library_path**: Absolute path to the engine library
  - **is_loaded**: Whether the engine is currently loaded in memory
  - **is_default**: Whether this is the default engine for new models
- **default_engine**: Name of the currently configured default engine
- **total_count**: Total number of available engines

### List Engines Error Responses

#### 500 Server Error

```json
{
  "error": {
    "message": "Server error: <detailed error message>",
    "type": "server_error",
    "param": null,
    "code": null
  }
}
```

This documentation provides comprehensive coverage of the Engines API, enabling efficient management of inference engines in your Kolosal Server deployment.
