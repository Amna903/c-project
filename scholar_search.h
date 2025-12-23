#pragma once
#include <string>
#include <vector>

// Structure to hold a single online search result
struct ScholarResult {
    std::string title;
    std::string url; // Direct PDF link or HTML page link
    std::string snippet;
};

class ScholarSearch {
private:
    // cURL write callback function (C-style function signature required by cURL)
    static size_t WriteCallback(void* contents, size_t size, size_t nmemb, void* userp);

    // Helper function to fetch the raw HTML content from a URL using cURL
    std::string fetchHtml(const std::string& url) const;

    // Helper function to parse the HTML content using libxml2
    std::vector<ScholarResult> parseResults(const std::string& htmlContent) const;

public:
    /**
     * @brief Performs a search on Google Scholar and returns a list of results.
     * @param query The search topic.
     * @return A vector of ScholarResult objects.
     */
    std::vector<ScholarResult> search(const std::string& query) const;
};