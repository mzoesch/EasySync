#include<iostream>
#include<fstream>
#include<string.h>
#include<ctime>
#include<sys/types.h>
#include<sys/stat.h>
#include<cerrno>
#include<cstring>
#include<filesystem>
#include<vector>
#include<chrono>

using namespace std;
namespace fs = std::filesystem;

int writeMostRecentPathSource(string path) {
    
    ofstream f;
    f.open("pathSource.txt", ofstream::out
    | ofstream::trunc
    );
    if (!f.is_open()) { // Exception handling
        cout << "Error creating file for source path." << endl;
        return 1;
    }

    f << path;
    f.close();
    return 0;
}

int writeMostRecentPathTarget(string path) {
    
    ofstream f;
    f.open("pathTarget.txt", ofstream::out
    | ofstream::trunc
    );
    if (!f.is_open()) { // Exception handling
        cout << "Error creating file for target path." << endl;
        return 1;
    }

    f << path;
    f.close();
    return 0;
}

string getMostRecentPathSource() {

    ifstream f;
    f.open("pathSource.txt");
    if (!f.is_open()) { // Exception handling
        cout << "Error loading most recent source path." << endl;
        return "e";
    }

    stringstream buffer;
    buffer << f.rdbuf();
    f.close();
    return buffer.str();
}

string getMostRecentPathTarget() {
    
    ifstream f;
    f.open("pathTarget.txt");
    if (!f.is_open()) { // Exception handling
        cout << "Error loading most recent target path." << endl;
        return "e";
    }
    
    stringstream buffer;
    buffer << f.rdbuf();
    f.close();
    return buffer.str();
}

string enterAnAbsolutePath() {

    string absolutePath;
    cout << "Enter absolute path: ";
    getline(cin, absolutePath); // Get line will get console with spaces!!!
    return absolutePath;
}

int main() {

    int countDirsSource = 0;
    int countFilesSource = 0;
    unsigned long long int bytesSource = 0; // Max 18,446,744,073,709,551,615
    int countFilesToCopy = 0;
    unsigned long long int bytesToCopy = 0; // Max 18,446,744,073,709,551,615
    int filesNotCopied = 0;
    int filesTimestampIncorrect = 0;
    
    vector<fs::path> filesToCopySourcePath; // This attracts bugs; fix
    vector<fs::path> filesToCopyTargetPath;

    vector<fs::path> filesThatCouldNotBeCopied;
    vector<fs::path> filesWhereTimestampIsIncorrect;

    string inputSource;
    string inputTarget;

    cout << endl;
    cout << "Empty directories are not copied!" << endl;
    cout << "Press Ctrl + c to abort at any given time." << endl;

    // Source
    cout << endl << "Enter absolute source path ('r' for most recent path): ";
    getline(cin, inputSource); // Get line will get console with spaces!!!
    if (inputSource == "r") {
        inputSource = getMostRecentPathSource();
        if (inputSource == "e") {
            inputSource = enterAnAbsolutePath();
            if (writeMostRecentPathSource(inputSource) != 0) cout << "Error: Writing most recent source path." << endl;
        }
    } else if (writeMostRecentPathSource(inputSource) != 0) cout << "Error: Writing most recent source path." << endl;

    // Target
    cout << "Enter absolute target path ('r' for most recent path): ";
    getline(cin, inputTarget); // Get line will get console with spaces!!!
    if (inputTarget == "r") {
        inputTarget = getMostRecentPathTarget();
        if (inputTarget == "e") {
            inputTarget = enterAnAbsolutePath();
            if (writeMostRecentPathTarget(inputTarget) != 0) cout << "Error: Writing most recent target path." << endl;
        }
    } else if (writeMostRecentPathTarget(inputTarget) != 0) cout << "Error: Writing most recent target path." << endl;

    cout << endl << "Using source path:     : " << inputSource << endl;
    cout << "Using target path:     : " << inputTarget << endl << endl;

    fs::path absoluteSourcePath = inputSource;
    fs::path absoluteTargetPath = inputTarget;
    
    cout << "Checking stats . . ." << endl;
    cout << "This could take a while." << endl;

    // Get files to copy
    for (const auto & entry : fs::recursive_directory_iterator(absoluteSourcePath)) {
        
        string modifiedSource;
        string modifiedTarget;
        bool existsInTarget = false;
        bool copyToTarget = true;

        string strAbsolutePathSourceFile = entry.path().string();
        char absolutePathSourceFile[strAbsolutePathSourceFile.length() + 1];
        strcpy(absolutePathSourceFile, strAbsolutePathSourceFile.c_str());

        // File stats
        struct stat sourceFileInfo;
        if (stat(absolutePathSourceFile, &sourceFileInfo) != 0) { // Exception handling
            cout << "Error in source path: " << entry.path() << endl;
            cout << "Error: " << strerror(errno) << endl;
            continue;
        }
        if ((sourceFileInfo.st_mode & S_IFMT) == S_IFDIR) {
            countDirsSource++;
            continue;
        } else countFilesSource++;
        bytesSource += sourceFileInfo.st_size;
        modifiedSource = ctime(&sourceFileInfo.st_mtime);
        
        // Check if program has to copy _this_ file to target
        string rPath; // Get rel path
        for (int i = strlen(absoluteSourcePath.string().c_str()) + 1; i < strlen(strAbsolutePathSourceFile.c_str()); i++) {
            char c = absolutePathSourceFile[i];
            rPath.push_back(c);
        }
        char absolutePathTargetFile[strlen(absoluteTargetPath.string().c_str()) + strlen(rPath.c_str())];
        strcpy(absolutePathTargetFile, absoluteTargetPath.string().c_str());
        strcat(absolutePathTargetFile, "\\");
        strcat(absolutePathTargetFile, rPath.c_str());
        ifstream t(absolutePathTargetFile); // Check if file exists
        existsInTarget = t.good();
        if (existsInTarget == 1) { // Check if file has to be copied
            struct stat targetFileInfo;
            if (stat(absolutePathTargetFile, &targetFileInfo) != 0) { // Exception handling
                cerr << "Error in target Path: " << strerror(errno) << endl; // File should always exists
                continue;
            }
            modifiedTarget = ctime(&targetFileInfo.st_mtime);
            if (modifiedSource == modifiedTarget) copyToTarget = false; // Timestamp is the same
        } else copyToTarget = true; // File does not exists
        t.close();
        
        if (!copyToTarget) continue;

        // File has to be copied
        string strTargetPathFile = absolutePathTargetFile;
        fs::path targetPathFile = strTargetPathFile;
        filesToCopySourcePath.push_back(entry.path());
        filesToCopyTargetPath.push_back(targetPathFile);
        countFilesToCopy++;
        bytesToCopy += sourceFileInfo.st_size;
    }
    
    // Exception handling - vectors have different sizes
    if (filesToCopySourcePath.size() != filesToCopyTargetPath.size()) {
        cout << "Fatal error: Vectors have different sizes. ";
        cout << filesToCopySourcePath.size() << " != " << filesToCopyTargetPath.size() << endl;

        system ("PAUSE");
        return 1;
    }

    cout << endl << "All files to copy to target dir:\n" << endl;
    if (filesToCopySourcePath.size() == 0) cout << "N/A" << endl << endl;
    else {
        for (int i = 0; i < filesToCopySourcePath.size(); i++) {
            cout << "From: " << filesToCopySourcePath[i] << endl;
            cout << "To:   " << filesToCopyTargetPath[i] << endl << endl;;
        }
    }

    cout << endl;
    cout << "Overall stats: " << endl << endl;
    cout << "Source -> Dirs:                    :  " << countDirsSource << endl;
    cout << "Source -> Files:                   :  " << countFilesSource << endl;
    cout << "Source -> Overall size (b):        :  " << bytesSource << endl;
    cout << "Source -> Overall size (mb bi):    : ~" << bytesSource * 0.00000095367432 << endl;
    cout << "Source -> Overall size (mb dc):    : ~" << bytesSource * 0.000001 << endl;
    cout << endl;
    cout << "ToCopy -> Files:                   :  " << countFilesToCopy << endl;
    cout << "ToCopy -> Overall size (b):        :  " << bytesToCopy << endl;
    cout << "ToCopy -> Overall size (mb bi):    : ~" << bytesToCopy * 0.00000095367432 << endl;
    cout << "ToCopy -> Overall size (mb dc):    : ~" << bytesToCopy * 0.000001 << endl;
    
    // No files to copy -> abort
    if (countFilesToCopy == 0) {
        cout << endl << "No files to copy.";
        cout << endl << "Closing program." << endl << endl;

        system("PAUSE");
        return 0;
    }

    cout << endl << "Copy all files to target folder (y/n): ";
    string answ;
    cin >> answ;
    cout;
    if (answ != "y") {
        cout << "aborted" << endl << endl;
        
        system("PAUSE");
        return 0;
    }

    // Copy files
    cout << endl << "Copying files." << endl;
    auto tStart = chrono::high_resolution_clock::now();

    for (int i = 0; i < filesToCopySourcePath.size(); i++) {
        fs::path absoluteSourcePathFile = filesToCopySourcePath[i];
        fs::path absoluteTargetPathFile = filesToCopyTargetPath[i];
        fs::path absoluteTargetPath = absoluteTargetPathFile;
        absoluteTargetPath.remove_filename();

        try {
            // Delete existing file
            fs::remove(absoluteTargetPathFile); // Does not throw if file does not exists

            fs::create_directories(absoluteTargetPath); // Recursively create target directory if not existing
            // Does not work
            // https://github.com/msys2/MSYS2-packages/issues/1937 - Bug in msys2 gcc
            const auto copyOptions = fs::copy_options::overwrite_existing
            | fs::copy_options::skip_existing
            ;
            fs::copy_file(absoluteSourcePathFile, absoluteTargetPathFile, copyOptions);
        } catch (exception& e) { // Not using fs::filesystem_error since std::bad_alloc can throw too
            filesNotCopied++;
            filesThatCouldNotBeCopied.push_back(absoluteSourcePathFile);
            cout << endl << "Error in path: " << absoluteSourcePathFile << endl;
            cout << e.what() << endl << endl;
            continue;
        }


        // Correct timestamp
        try {
            fs::file_time_type fLastWriteTime = fs::last_write_time(absoluteSourcePathFile);
            fs::last_write_time(absoluteTargetPathFile, fLastWriteTime);
        } catch (exception& e) {
            filesTimestampIncorrect++;
            filesWhereTimestampIsIncorrect.push_back(absoluteTargetPathFile);
            cout << endl << "Error in path: " << absoluteTargetPathFile << endl;
            cout << "Error while editing the timestamp." << endl;
            cout << e.what() << endl << endl;
            continue;
        }
        
        cout << "Successfully copied file: " << absoluteSourcePathFile << endl;
    }

    cout << endl;

    // Time
    auto tEnd = chrono::high_resolution_clock::now();
    double timeElapsed = chrono::duration<double, milli>(tEnd - tStart).count();
    if (timeElapsed > 60000) cout << "Program finished in " << timeElapsed / 1000 / 60 << " min." << endl;
    else if (timeElapsed > 1000) cout << "Program finished in " << timeElapsed / 1000 << " s." << endl;
    else cout << "Program finished in " << timeElapsed << " ms." << endl;
    cout << endl;
    

    if (filesNotCopied != 0) {
        string answ;
        cout << "Number of files that could not be copied: " << filesNotCopied << endl;
        cout << "Show files (y/n): ";
        cin >> answ;
        if (answ == "y") { // Display failed paths
            cout << endl;
            for (int i = 0; i < filesThatCouldNotBeCopied.size(); i++) {
                try { cout << filesThatCouldNotBeCopied[i] << endl; }
                catch (exception& e) { cout << "Could not display path: " << e.what() << endl; }
            }
        }
    }

    if (filesTimestampIncorrect != 0) {
        string answ;
        cout << "Number of files where the timestamp could not be corrected: " << filesTimestampIncorrect << endl;
        cout << "Show files (y/n): ";
        cin >> answ;
        if (answ == "y") { // Display failed paths
            cout << endl;
            for (int i = 0; i < filesWhereTimestampIsIncorrect.size(); i++) {
                try { cout << filesWhereTimestampIsIncorrect[i] << endl; }
                catch (exception& e) { cout << "Could not display path: " << e.what() << endl; }
            }
        }
    }

    cout << endl;

    system("PAUSE");
    return 0;
}
