
// TEST: how easy is it in C++ to read the x86 instruction reference index.html file from
// internet, parse it, and print out the mnemonic-summary pairs to stdout
// TypeScript needs require and htmlparser2
// C++/C needs curl and ?
// Do it also in C and time them to see which one is fastest: C++/C/TypeScript?
//
//std::dload("https://www.felixcloutier.com/x86/index.html", "~/Downloads/x86.html");
//////////////////////////////

#include "HTTPDownloader.hpp"
#include <cstdio>
#include <iostream>
#include <string>
#include <array>


// For Methods 3 and 4
#include <curl/curl.h>
#include <curl/easy.h>
#include <curl/curlbuild.h>


// Method 2
// NOTE: When curl output is redirected to a file it also outputs some statistics.
// Use --silent option.
std::string exec(const char* cmd) 
{
    std::array<char, 128> buffer;
    std::string result;
    std::shared_ptr<FILE> pipe(popen(cmd, "r"), pclose);
    if (!pipe) throw std::runtime_error("popen() failed!");
    while (!feof(pipe.get())) {
        if (fgets(buffer.data(), 128, pipe.get()) != nullptr)
            result += buffer.data();
    }
    return result;
}


// Method 4
// Operator overloading
size_t curlCbToStream (
    char* buffer,
    size_t nitems,
    size_t size,
    std::ostream * sout
) 
{
    *sout << buffer;
    return nitems * size;
}

std::ostream & operator<< (
    std::ostream & sout,
    CURL* request
)
{
    ::curl_easy_setopt(request, CURLOPT_WRITEDATA, & sout);
    ::curl_easy_setopt(request, CURLOPT_WRITEFUNCTION, curlCbToStream);
    ::curl_easy_perform(request);
    return sout;
} 



int main(int argc, char** argv) 
{
    // Method 1:
    // The clean OO way
    HTTPDownloader downloader;
    std::string content = downloader.download("https://www.felixcloutier.com/x86/index.html");
    std::cout << content << std::endl;

    // Method 2:
    // System call
    // But still need to get output in string (using > file)
    //system("curl www.example.com");
    // Or via popen
    //std::cout << exec("curl --silent www.example.com");

    // Method 3:
    // C++11 lambdas (less code but confusing)
    // std::string resultBody { };
    // void* curl = curl_easy_init();
    // curl_easy_setopt(curl, CURLOPT_URL, "www.example.com");
    // curl_easy_setopt(curl, CURLOPT_WRITEDATA, &resultBody);
    // curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, static_cast<size_t (*)(char*, size_t, size_t, void*)>(
    //     [](char* ptr, size_t size, size_t nmemb, void* resultBody){
    //         *(static_cast<std::string*>(resultBody)) += std::string {ptr, size * nmemb};
    //         return size * nmemb;
    //     }
    // ));
    //
    // CURLcode curlResult = curl_easy_perform(curl);
    // if (curlResult != CURLE_OK) {
    //     std::cerr << "curl_easy_perform() failed: ", curl_easy_strerror(curlResult);
    // }
    // std::cout << "RESULT BODY:\n" << resultBody << std::endl;

    // Method 4:
    // Nice one!
    //void* curl = curl_easy_init();
    //curl_easy_setopt(curl, CURLOPT_URL, "www.example.com");
    //std::cout << curl;



}