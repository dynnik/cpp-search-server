#include "request_queue.h"

using namespace std;

RequestQueue::RequestQueue(const SearchServer& search_server) {
    server = &search_server;
    number_of_empty_docs_ = 0;
}

int RequestQueue::AddFindRequest(const string& raw_query, DocumentStatus status) {
    QueryResult result;
    result.results = server->FindTopDocuments(raw_query, status).size();
    AddResult(result);
    return result.results;
}

int RequestQueue::AddFindRequest(const string& raw_query) {
    QueryResult result;
    result.results = server->FindTopDocuments(raw_query).size();
    AddResult(result);
    return result.results;
}

int RequestQueue::GetNoResultRequests() const {
    return number_of_empty_docs_;
}

void RequestQueue::AddResult(const QueryResult &result) {
    if (requests_.size() == sec_in_day_) {
        if (requests_.front().results == 0) {
            --number_of_empty_docs_;
        }
        requests_.pop_front();
    }
    if (result.results == 0) {
        ++number_of_empty_docs_;
    }
    requests_.push_back(result);
}
