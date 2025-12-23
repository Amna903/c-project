#include "scholar_search.h"
#include <iostream>
#include <sstream>
#include <iomanip>
#include <curl/curl.h>
#include <libxml/HTMLparser.h>
#include <libxml/xpath.h>

// --- Helper Implementation 1: cURL Write Callback ---
size_t ScholarSearch::WriteCallback(void* contents, size_t size, size_t nmemb, void* userp) {
    ((std::string*)userp)->append((char*)contents, size * nmemb);
    return size * nmemb;
}

// --- Helper Implementation 2: cURL HTML Fetcher ---
std::string ScholarSearch::fetchHtml(const std::string& url) const {
    CURL* curl_handle;
    std::string html_content;
    
    // Initialize cURL
    curl_global_init(CURL_GLOBAL_ALL);
    curl_handle = curl_easy_init();

    if (curl_handle) {
        // Set URL
        curl_easy_setopt(curl_handle, CURLOPT_URL, url.c_str());
        
        // Use a high-quality User-Agent to mimic a real browser
        curl_easy_setopt(curl_handle, CURLOPT_USERAGENT, "Mozilla/5.0 (Macintosh; Intel Mac OS X 10_15_7) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/120.0.0.0 Safari/537.36"); 
        
        // Set write function to append data to the string
        curl_easy_setopt(curl_handle, CURLOPT_WRITEFUNCTION, WriteCallback);
        curl_easy_setopt(curl_handle, CURLOPT_WRITEDATA, &html_content);
        
        // Follow redirects
        curl_easy_setopt(curl_handle, CURLOPT_FOLLOWLOCATION, 1L);

        // Set standard HTTP headers
        struct curl_slist *headers = NULL;
        headers = curl_slist_append(headers, "Accept-Language: en-US,en;q=0.9");
        curl_easy_setopt(curl_handle, CURLOPT_HTTPHEADER, headers);

        // Perform the request
        CURLcode res = curl_easy_perform(curl_handle);

        if (res != CURLE_OK) {
            std::cerr << "[cURL Error] Failed to fetch URL " << url << ": " 
                      << curl_easy_strerror(res) << std::endl;
            html_content.clear(); // Clear content on error
        }

        // Clean up headers list (FIX for memory leak)
        if (headers) {
            curl_slist_free_all(headers);
        }

        // Clean up cURL handle
        curl_easy_cleanup(curl_handle);
    }
    curl_global_cleanup();
    
    return html_content;
}

// --- Helper Implementation 3: libxml2 HTML Parser ---
std::vector<ScholarResult> ScholarSearch::parseResults(const std::string& htmlContent) const {
    std::vector<ScholarResult> results;
    htmlDocPtr doc = NULL;
    xmlXPathContextPtr xpathCtx = NULL;
    xmlXPathObjectPtr xpathObj = NULL;

    // 1. Parse the HTML content
    doc = htmlReadMemory(htmlContent.c_str(), htmlContent.size(), NULL, "UTF-8", HTML_PARSE_NOWARNING | HTML_PARSE_NOERROR);
    if (doc == NULL) {
        std::cerr << "[libxml2 Error] Could not parse HTML document." << std::endl;
        return results;
    }

    // 2. Create XPath context
    xpathCtx = xmlXPathNewContext(doc);
    if (xpathCtx == NULL) {
        std::cerr << "[libxml2 Error] Could not create XPath context." << std::endl;
        xmlFreeDoc(doc);
        return results;
    }

    // 3. Define the main result block XPath
    // FIX: Simplified XPath to use 'contains' and only the stable 'gs_r' class.
    const xmlChar* xpathExpr = (const xmlChar*)"//div[contains(@class, 'gs_r')]";
    xpathObj = xmlXPathEvalExpression(xpathExpr, xpathCtx);

    if (xpathObj && xpathObj->nodesetval) {
        xmlNodeSetPtr nodes = xpathObj->nodesetval;
        
        for (int i = 0; i < nodes->nodeNr; ++i) {
            xmlNodePtr resultNode = nodes->nodeTab[i];
            ScholarResult res;
            
            // Temporary XPath context for the current result block
            xmlXPathContextPtr itemCtx = xmlXPathNewContext(doc);
            itemCtx->node = resultNode;

            // --- Extract Title and URL ---
            // Using './h3' ensures the search is relative to the current resultNode (fixes duplication bug)
            xmlXPathObjectPtr titleObj = xmlXPathEvalExpression((const xmlChar*)"./h3[@class='gs_rt']/a", itemCtx); 
            if (titleObj && titleObj->nodesetval && titleObj->nodesetval->nodeNr > 0) {
                xmlNodePtr titleNode = titleObj->nodesetval->nodeTab[0];
                res.title = (char*)xmlNodeGetContent(titleNode);
                res.url = (char*)xmlGetProp(titleNode, (const xmlChar*)"href");
                
                xmlXPathFreeObject(titleObj);
            }
            
            // --- Extract Snippet ---
           xmlXPathObjectPtr snippetObj = xmlXPathEvalExpression((const xmlChar*)"./div[@class='gs_rs']", itemCtx);
            if (snippetObj && snippetObj->nodesetval && snippetObj->nodesetval->nodeNr > 0) {
                xmlNodePtr snippetNode = snippetObj->nodesetval->nodeTab[0];
                res.snippet = (char*)xmlNodeGetContent(snippetNode);
                
                // Simple cleanup of the snippet text
                if (!res.snippet.empty() && res.snippet.length() > 200) {
                    res.snippet = res.snippet.substr(0, 200) + "...";
                }

                xmlXPathFreeObject(snippetObj);
            }

            // Only add the result if we successfully got a title and URL
            if (!res.title.empty() && !res.url.empty()) {
                results.push_back(res);
            }
            xmlXPathFreeContext(itemCtx);
        }
    }

    // 4. Clean up libxml2 resources
    if (xpathObj) xmlXPathFreeObject(xpathObj);
    if (xpathCtx) xmlXPathFreeContext(xpathCtx);
    if (doc) xmlFreeDoc(doc);
    
    xmlCleanupParser();
    xmlMemoryDump();

    return results;
}

// --- Core Search Function (DEBUG ENABLED) ---
std::vector<ScholarResult> ScholarSearch::search(const std::string& query) const {
    // URL-encode the query
    std::string encodedQuery;
    char* escaped = curl_easy_escape(NULL, query.c_str(), query.length());
    if (escaped) {
        encodedQuery = escaped;
        curl_free(escaped);
    }

    // Construct the Google Scholar URL
    std::string scholarUrl = "https://scholar.google.com/scholar?q=" + encodedQuery;
    
    std::cout << "\n[Status] Fetching results from Google Scholar: " << scholarUrl << std::endl;

    // 1. Fetch HTML
    std::string html = fetchHtml(scholarUrl);
    
    if (html.empty()) {
        std::cerr << "[Error] Failed to get content from Google Scholar. Check network or User-Agent." << std::endl;
        return {};
    }

    // DEBUG: Print the start of the received HTML to diagnose blocking/CAPTCHA
    std::cout << "\n[DEBUG] HTML Snippet Received (First 500 chars):\n";
    std::cout << "---------------------------------------------------\n";
    std::cout << (html.length() > 500 ? html.substr(0, 500) : html) << "\n";
    std::cout << "---------------------------------------------------\n" << std::endl;

    // 2. Parse Results
    std::cout << "[Status] Parsing HTML to extract search results..." << std::endl;
    return parseResults(html);
}