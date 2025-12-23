clang++ -std=c++20 pdf_extractor.cpp main.cpp \
  -I/opt/homebrew/Cellar/poppler/25.11.0/include \
  -I/opt/homebrew/Cellar/poppler/25.11.0/include/poppler \
  -L/opt/homebrew/lib \
  -lpoppler -o pdf_extractor_app
