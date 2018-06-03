
// TEST: how easy is it in C++ to read the x86 instruction reference index.html file from
// the internet, parse it, and print out the mnemonic-summary pairs to stdout
// TypeScript needs require and htmlparser2 (install with npm)
// C++/C needs curl and tidy (both preinstalled on macOS)
// Do it also in C and time them to see which one is fastest: C++/C/TypeScript?
//
//dload("https://www.felixcloutier.com/x86/index.html", "~/Downloads/x86.html");
//////////////////////////////

#include "HTTPDownloader.hpp"
#include <cstdio>
#include <iostream>
#include <string>
#include <array>
#include <vector>

// For Methods 3 and 4
#include <curl/curl.h>
#include <curl/easy.h>
#include <curl/curlbuild.h>

// HTML Parsing
#include <tidy/tidy.h>
#include <tidy/buffio.h>


using namespace std;


// Method 2
// NOTE: When curl output is redirected to a file it also outputs some statistics.
// Use --silent option.
string exec(const char* cmd) 
{
    array<char, 128> buffer;
    string result;
    shared_ptr<FILE> pipe(popen(cmd, "r"), pclose);
    if (!pipe) throw runtime_error("popen() failed!");
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
    ostream * sout
) 
{
    *sout << buffer;
    return nitems * size;
}

ostream & operator<< (
    ostream & sout,
    CURL* request
)
{
    curl_easy_setopt(request, CURLOPT_WRITEDATA, & sout);
    curl_easy_setopt(request, CURLOPT_WRITEFUNCTION, curlCbToStream);
    curl_easy_perform(request);
    return sout;
}



// Curl and Tidy
//bool found_td = false;
//bool found_a = false;
//int column = 1;
int td = 0;
char mnemonic[128];

struct Dep {
    string mnemonic;
    string summary;
    string link;
} dep;

vector<Dep> deps;



/* curl write callback, to fill tidy's input buffer...  */
uint write_cb(char *in, uint size, uint nmemb, TidyBuffer *out)
{
    uint r;
    r = size * nmemb;
    tidyBufAppend(out, in, r);
    return r;
}

/* Traverse the document tree */
void dumpNode(TidyDoc doc, TidyNode tnod, int indent)
{
    TidyNode child;
    for (child = tidyGetChild(tnod); child; child = tidyGetNext(child))
    {
        ctmbstr name = tidyNodeGetName(child);
        if (name)
        {
            /* if it has a name, then it's an HTML tag ... */
            // TidyAttr attr;
            // printf("%*.*s%s ", indent, indent, "<", name);
            // /* walk the attribute list */
            // for (attr = tidyAttrFirst(child); attr; attr = tidyAttrNext(attr))
            // {
            //     printf(tidyAttrName(attr));
            //     tidyAttrValue(attr) ? printf("=\"%s\" ",
            //                                  tidyAttrValue(attr))
            //                         : printf(" ");
            // }
            // printf(">\n");

            // TEST: Tidy never finds end nodes only start nodes
            // Is eigenlijk ook niet nodig
            // if (tidyNodeGetType(child) == TidyNode_End) {
            //     printf(name);
            // }

            ///////
            if (0 == strcmp(name, "td"))
            {
                //found_td = true;
                td++;
                if (td > 2) td = 0;
                //printf("Found TD!");
            }
            if (0 == strcmp(name, "a") && td == 1)
            {
                //found_a = true;
                //printf("Found A!");
                TidyAttr attr;
                attr = tidyAttrFirst(child);
                //printf(tidyAttrName(attr));
                //printf(tidyAttrValue(attr));
                dep.link = tidyAttrValue(attr);
            }
        }
        else
        {
            /* if it doesn't have a name, then it's probably text, cdata, etc... */
            TidyBuffer buf;
            tidyBufInit(&buf);
            tidyNodeGetText(doc, child, &buf);
            //printf("%*.*s\n", indent, indent, buf.bp ? (char *)buf.bp : "");

            // HELL: tidyNodeGetText puts \n at end of buf
            // FYL2X gaat fout en geeft html codes ipv text 

            // Get rid of trailing \n
            if (buf.bp) buf.bp[strlen((char*)buf.bp)-1] = '\0';
            
            //////
            //if (found_td) {
                if (td == 1) {  // mnemonic                  
                    //printf("%s\n", buf.bp);// ? (char *)buf.bp : "");
                    strcat(mnemonic, (char*)buf.bp);
                    //column = 2;
                    
                    dep.mnemonic = mnemonic;
                         
                }
                if (td == 2) {  // summary
                    //printf("%s\n\n", buf.bp ? (char *)buf.bp : "");
                    //column = 1;

                    dep.summary = (char*)buf.bp;
                    deps.push_back(dep);
                    //cerr << "Pushback.\n";

                    // reset
                    td = 0;
                    mnemonic[0] = '\0';    
                }
         
            //}

            // Hier zit prob. Je mag niet zomaar td afzetten. Dat mag een /td alleen.
            // Echter tidy kan die niet parsen. Zo kunnen we dus niet goed kludgen.
            //found_td = false;

            // NOTE: Tidy kan weldegelijk end tags parsen. Zie tidy.h
            //  case TidyNode_Start:
            //  case TidyNode_End:
            // NONO: TidyNode_End wordt nooit gevonden en is een loze waarde.


            // kludge for (1)            
            // if (NULL != strstr((char *)buf.bp, "(1)") ||
            //     NULL != strstr((char *)buf.bp, "(2)")
            // ) {
            //     //printf("%s\n", buf.bp);
            //     strcat(mnemonic, (char *)buf.bp);
            //     //printf("%s\n", mnemonic);
            //     dep.mnemonic = mnemonic;
            //     mnemonic[0] = '\0';
            // }
           

            tidyBufFree(&buf);
        }
        dumpNode(doc, child, indent + 4); /* recursive */
    }
}

int main(int argc, char** argv) 
{
    // Method 1:
    // The clean OO way
    //HTTPDownloader downloader;
    //string content = downloader.download("https://www.felixcloutier.com/x86/index.html");
    //string content = downloader.download("www.example.com");
    //cout << content << endl;

    // Method 2:
    // System call
    // But still need to get output in string (using > file)
    //system("curl www.example.com");
    // Or via popen
    //cout << exec("curl --silent www.example.com");

    // Method 3:
    // C++11 lambdas (less code but confusing)
    // string resultBody { };
    // void* curl = curl_easy_init();
    // curl_easy_setopt(curl, CURLOPT_URL, "www.example.com");
    // curl_easy_setopt(curl, CURLOPT_WRITEDATA, &resultBody);
    // curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, static_cast<size_t (*)(char*, size_t, size_t, void*)>(
    //     [](char* ptr, size_t size, size_t nmemb, void* resultBody){
    //         *(static_cast<string*>(resultBody)) += string {ptr, size * nmemb};
    //         return size * nmemb;
    //     }
    // ));
    //
    // CURLcode curlResult = curl_easy_perform(curl);
    // if (curlResult != CURLE_OK) {
    //     cerr << "curl_easy_perform() failed: ", curl_easy_strerror(curlResult);
    // }
    // cout << "RESULT BODY:\n" << resultBody << endl;

    // Method 4:
    // Nice one!
    //void* curl = curl_easy_init();
    //curl_easy_setopt(curl, CURLOPT_URL, "www.example.com");
    //cout << curl;


    ///////////////////////////
    CURL *curl;
    char curl_errbuf[CURL_ERROR_SIZE];
    TidyDoc tdoc;
    TidyBuffer docbuf = {0};
    TidyBuffer tidy_errbuf = {0};
    int err;

  
    curl = curl_easy_init();
    curl_easy_setopt(curl, CURLOPT_URL, "https://www.felixcloutier.com/x86/index.html");
    curl_easy_setopt(curl, CURLOPT_ERRORBUFFER, curl_errbuf);
    curl_easy_setopt(curl, CURLOPT_NOPROGRESS, 0L);
    curl_easy_setopt(curl, CURLOPT_VERBOSE, 1L);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_cb);

    tdoc = tidyCreate();
    tidyOptSetBool(tdoc, TidyForceOutput, yes); /* try harder */
    tidyOptSetInt(tdoc, TidyWrapLen, 4096);
    tidySetErrorBuffer(tdoc, &tidy_errbuf);
    tidyBufInit(&docbuf);

    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &docbuf);
    err = curl_easy_perform(curl);
    if (!err)
    {
        err = tidyParseBuffer(tdoc, &docbuf); /* parse the input */
        if (err >= 0)
        {
            err = tidyCleanAndRepair(tdoc); /* fix any problems */
            if (err >= 0)
            {
                err = tidyRunDiagnostics(tdoc); /* load tidy error buffer */
                if (err >= 0)
                {
                    dumpNode(tdoc, tidyGetRoot(tdoc), 0);    /* walk the tree */
                    fprintf(stderr, "%s\n", tidy_errbuf.bp); /* show errors */
                }
            }
        }
    }
    else
        fprintf(stderr, "%s\n", curl_errbuf);

    /* clean-up */
    curl_easy_cleanup(curl);
    tidyBufFree(&docbuf);
    tidyBufFree(&tidy_errbuf);
    tidyRelease(tdoc);


    // Print deps
    //for(vector<Dep>::iterator it = deps.begin(); it != deps.end(); ++it) {
    //for(auto it = deps.begin(); it != deps.end(); ++it) {
    //    cout << it->mnemonic << " - " << it->summary << endl;
    //}
    for (auto i : deps) {
        // kludge for FYL2X/FYL2XP1 &acirc;&circ;&mdash;
        string needle = "&acirc;&circ;&mdash;";
        size_t pos = i.summary.find(needle);
        if (pos != string::npos) {
            i.summary.replace(pos, needle.length(), "*");
        }
        // kludge for F2XM1 &acirc;&euro;&ldquo;
        needle = "&acirc;&euro;&ldquo;";
        pos = i.summary.find(needle);
        if (pos != string::npos) {
            i.summary.replace(pos, needle.length(), "–");
        }
        
        //cout << i.mnemonic << " - " << i.summary << endl;
        cout << i.mnemonic << endl << i.summary << endl << i.link << endl << endl;
    }
    
    
    //return err;
    return 0;
   

}