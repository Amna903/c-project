#pragma once
#include <string>

class PdfTextExtractor {
public:
    std::string extractText(const std::string& pdfPath) const;
};
