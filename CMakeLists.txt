cmake_minimum_required(VERSION 3.10)
project(PDFOrganizer CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

# --- Find POPPLER-CPP (PDF Library) ---
find_package(PkgConfig REQUIRED)
pkg_check_modules(POPPLER_CPP REQUIRED poppler-cpp)

# --- Find cURL ---
find_package(CURL REQUIRED)

# --- Find libxml2 ---
find_package(LibXml2 REQUIRED)

# --- Add include directories ---
include_directories(
    ${POPPLER_CPP_INCLUDE_DIRS}
    ${LIBXML2_INCLUDE_DIRS}
    /opt/homebrew/include
    /opt/homebrew/include/poppler
    /opt/homebrew/include/poppler/cpp
)

# --- Add link directories ---
link_directories(
    /opt/homebrew/lib
)

# --- Build executable ---
add_executable(pdf_organizer 
    main.cpp
    file_manager.cpp
    relevance_scorer.cpp
    pdf_extractor.cpp
    scholar_search.cpp
)

# --- Link libraries ---
target_link_libraries(pdf_organizer
    ${POPPLER_CPP_LIBRARIES}   # pkg-config output (should pick up libpoppler-cpp)
    ${CURL_LIBRARIES}
    ${LIBXML2_LIBRARIES}
    poppler-cpp                # explicitly add the dylib name
)
