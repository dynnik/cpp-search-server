#pragma once

#include <deque>
#include <string>
#include <vector>

#include "search_server.h"

class RequestQueue {
public:
    explicit RequestQueue(const SearchServer& search_server);
    
    template <typename DocumentPredicate>
    int AddFindRequest(const std::string& raw_query, DocumentPredicate document_predicate);
    int AddFindRequest(const std::string& raw_query, DocumentStatus status);
    int AddFindRequest(const std::string& raw_query);
    
    int GetNoResultRequests() const;
    
private:
    struct QueryResult {
        int results;
    };
    
    std::deque<QueryResult> requests_;
    const static int sec_in_day_ = 1440;
    const SearchServer *server;
    int number_of_empty_docs_;

    void AddResult(const QueryResult &result);
};


template <typename DocumentPredicate>
int RequestQueue::AddFindRequest(const std::string& raw_query,
                                                   DocumentPredicate document_predicate) {
    QueryResult result;
    result.results = server->FindTopDocuments(raw_query, document_predicate).size();
    AddResult(result);
    return result.results;
}
