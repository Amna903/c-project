#include "pdf_extractor.h"
#include <poppler/cpp/poppler-document.h>
#include <poppler/cpp/poppler-page.h>

#include <iostream>
#include <string>

std::string PdfTextExtractor::extractText(const std::string& pdfPath) const {
    std::string fullText;

    // Load document
    std::unique_ptr<poppler::document> doc(poppler::document::load_from_file(pdfPath));
    if (!doc) {
        std::cerr << "Error: Could not open PDF: " << pdfPath << "\n";
        return "";
    }

    int numPages = doc->pages();
    for (int i = 0; i < numPages; ++i) {

        std::unique_ptr<poppler::page> page(doc->create_page(i));
        if (!page) continue;

        // Extract text (Poppler 25 returns std::vector<char>)
        auto bytes = page->text().to_utf8();

        std::string text(bytes.begin(), bytes.end());
        fullText += text + "\n\n";
    }

    return fullText;
}
