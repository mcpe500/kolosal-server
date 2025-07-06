#pragma once

#include "kolosal/models/completion_metrics_response_model.hpp"
#include "kolosal/completion_monitor.hpp"

namespace kolosal {
    namespace utils {

        // Convert AggregatedCompletionMetrics to CompletionMetricsResponseModel
        CompletionMetricsResponseModel convertToCompletionMetricsResponse(const AggregatedCompletionMetrics& metrics);

        // Convert CompletionMetrics to EngineMetricsResponseModel
        EngineMetricsResponseModel convertToEngineMetricsResponse(const CompletionMetrics& metrics);

    } // namespace utils
} // namespace kolosal
