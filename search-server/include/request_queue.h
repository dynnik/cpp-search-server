#pragma once

#include <deque>
#include <string>
#include <vector>

#include "search_server.h"

class RequestQueue {
public:
    explicit RequestQueue(const SearchServer& search_server);
    
    template <typename DocumentPredicate>
    std::vector<Document> AddFindRequest(const std::string& raw_query, DocumentPredicate document_predicate);
    std::vector<Document> AddFindRequest(const std::string& raw_query, DocumentStatus status);
    std::vector<Document> AddFindRequest(const std::string& raw_query);
    
    int GetNoResultRequests() const;
    
private:
    struct QueryResult {
        std::string raw_query;
        std::vector<Document> results;
    };
    
    std::deque<QueryResult> requests_;
    const static int sec_in_day_ = 1440;
    const SearchServer *server;
    int number_of_empty_docs_;

    void AddResult(const QueryResult &result);
};


template <typename DocumentPredicate>
std::vector<Document> RequestQueue::AddFindRequest(const std::string& raw_query,
                                                   DocumentPredicate document_predicate) {
    QueryResult result;
    result.raw_query = raw_query;
    result.results = server->FindTopDocuments(raw_query, document_predicate);
    AddResult(result);
    return result.results;
}
