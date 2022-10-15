#include "request_queue.h"

using namespace std;

RequestQueue::RequestQueue(const SearchServer& search_server) {
    server = &search_server;
    number_of_empty_docs_ = 0;
}

vector<Document> RequestQueue::AddFindRequest(const string& raw_query, DocumentStatus status) {
    QueryResult result;
    result.raw_query = raw_query;
    result.results = server->FindTopDocuments(raw_query, status);
    AddResult(result);
    return result.results;
}

vector<Document> RequestQueue::AddFindRequest(const string& raw_query) {
    QueryResult result;
    result.raw_query = raw_query;
    result.results = server->FindTopDocuments(raw_query);
    AddResult(result);
    return result.results;
}

int RequestQueue::GetNoResultRequests() const {
    return number_of_empty_docs_;
}

void RequestQueue::AddResult(const QueryResult &result) {
    if (requests_.size() == sec_in_day_) {
        if (requests_.front().results.empty() == true) {
            --number_of_empty_docs_;
        }
        requests_.pop_front();
    }
    if (result.results.empty() == true) {
        ++number_of_empty_docs_;
    }
    requests_.push_back(result);
}