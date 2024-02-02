#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <iomanip>
#include <cmath>
#include <deque>

using namespace std;

struct DataTLBConfig
{
    int numSets;
    int setSize;
};

struct DataCacheConfig
{
    int numSets;
    int setSize;
    int lineSize;
    bool writeThroughOrNoWriteAllocate;
};

struct L2CacheConfig
{
    int numSets;
    int setSize;
    int lineSize;
};

struct MemoryConfig
{
    int numVirtualPages;
    int numPhysicalPages;
    int pageSize;
};

// Main Configuration structure
struct Configuration
{
    DataTLBConfig dtlbConfig;
    MemoryConfig ptConfig;
    DataCacheConfig dcConfig;
    L2CacheConfig l2Config;
    bool useVirtualAddresses;
    bool useTLB;
    bool useL2Cache;
};

// Function to print cache configuration details
void printDataTLBConfig(const DataTLBConfig &tlbConfig)
{
    cout << "Number of Sets: " << tlbConfig.numSets << endl;
    cout << "Set Size: " << tlbConfig.setSize << endl;
}

// Function to print cache configuration details
void printDataCacheConfig(const DataCacheConfig &cacheConfig)
{
    cout << "Number of Sets: " << cacheConfig.numSets << endl;
    cout << "Set Size: " << cacheConfig.setSize << endl;
    cout << "Line Size: " << cacheConfig.lineSize << endl;
    cout << "write Through Or No Write Allocate: " << (cacheConfig.writeThroughOrNoWriteAllocate ? "yes" : "no") << endl;
}

void printL2CacheConfig(const L2CacheConfig &cacheConfig)
{
    cout << "Number of Sets: " << cacheConfig.numSets << endl;
    cout << "Set Size: " << cacheConfig.setSize << endl;
    cout << "Line Size: " << cacheConfig.lineSize << endl;
}

// Function to print memory configuration details
void printMemoryConfig(const MemoryConfig &memoryConfig)
{
    cout << "Number of Virtual Pages: " << memoryConfig.numVirtualPages << endl;
    cout << "Number of Physical Pages: " << memoryConfig.numPhysicalPages << endl;
    cout << "Page Size: " << memoryConfig.pageSize << endl;
}

// Function to print configuration details
void printConfiguration(const Configuration &config)
{
    cout << "Data TLB configuration:" << endl;
    printDataTLBConfig(config.dtlbConfig);

    cout << "Page Table configuration:" << endl;
    printMemoryConfig(config.ptConfig);

    cout << "Data Cache configuration:" << endl;
    printDataCacheConfig(config.dcConfig);

    cout << "L2 Cache configuration:" << endl;
    printL2CacheConfig(config.l2Config);

    cout << "Addresses: " << (config.useVirtualAddresses ? "virtual" : "physical") << endl;
    cout << "TLB: " << (config.useTLB ? "yes" : "no") << endl;
    cout << "L2 Cache: " << (config.useL2Cache ? "yes" : "no") << endl;
}

Configuration readConfigFile(const string &filename)
{
    ifstream file(filename);
    Configuration config;
    string line;
    string currentData;
    if (file.is_open())
    {

        while (getline(file, line))
        {
            if (line.find(':') != string::npos)
            {
                istringstream iss(line);
                string key, value;
                getline(iss, key, ':');
                getline(iss, value);

                key.erase(0, key.find_first_not_of(" \t"));
                key.erase(key.find_last_not_of(" \t") + 1);
                value.erase(0, value.find_first_not_of(" \t"));
                value.erase(value.find_last_not_of(" \t") + 1);

                if (currentData.find("Data TLB configuration") != string::npos)
                {
                    if (key == "Number of sets")
                    {
                        config.dtlbConfig.numSets = stoi(value);
                    }
                    else if (key == "Set size")
                    {
                        config.dtlbConfig.setSize = stoi(value);
                    }
                }
                else if (currentData.find("Page Table configuration") != string::npos)
                {
                    if (key == "Number of virtual pages")
                    {
                        config.ptConfig.numVirtualPages = stoi(value);
                    }
                    else if (key == "Number of physical pages")
                    {
                        config.ptConfig.numPhysicalPages = stoi(value);
                    }
                    else if (key == "Page size")
                    {
                        config.ptConfig.pageSize = stoi(value);
                    }
                }
                else if (currentData.find("Data Cache configuration") != string::npos)
                {
                    if (key == "Number of sets")
                    {
                        config.dcConfig.numSets = stoi(value);
                    }
                    else if (key == "Set size")
                    {
                        config.dcConfig.setSize = stoi(value);
                    }
                    else if (key == "Line size")
                    {
                        config.dcConfig.lineSize = stoi(value);
                    }
                    else if (key == "Write through/no write allocate")
                    {
                        config.dcConfig.writeThroughOrNoWriteAllocate = (value == "y");
                    }
                }
                else if (currentData.find("L2 Cache configuration") != string::npos)
                {
                    if (key == "Number of sets")
                    {
                        config.l2Config.numSets = stoi(value);
                    }
                    else if (key == "Set size")
                    {
                        config.l2Config.setSize = stoi(value);
                    }
                    else if (key == "Line size")
                    {
                        config.l2Config.lineSize = stoi(value);
                        currentData ="";
                    }
                }
                else
                {
                    if (key == "Virtual addresses")
                    {
                        config.useVirtualAddresses = (value == "y");
                    }
                    else if (key == "TLB")
                    {
                        config.useTLB = (value == "y");
                    }
                    else if (key == "L2 cache")
                    {
                        config.useL2Cache = (value == "y");
                    }
                }
            }
            else if (line.size() > 0)
            {
                currentData = line;
            }
        }

        file.close();
    }
    else
    {
        cerr << "Error: Unable to open trace file." << endl;
    }
    return config;
}

// Function to read the trace file
vector<string> readTraceFile(const string &traceFile)
{
    vector<string> traceData;
    ifstream file(traceFile);
    if (file.is_open())
    {
        string line;
        while (getline(file, line))
        {
            traceData.push_back(line);
            cout<<line<<endl;
        }
        file.close();
    }
    else
    {
        cerr << "Error: Unable to open trace file." << endl;
    }
    return traceData;
}

int main()
{
    // Read configuration file
    Configuration config = readConfigFile("./trace.config");

    // Print configuration details
    printConfiguration(config);

    // Read trace file
    vector<string> traceData = readTraceFile("./trace.dat");
    // Simulate memory hierarchy using the provided trace file
    // ...

    return 0;
}

// int extractBits(int value, int startBit, int endBit) {
//     // Convert the integer to binary
//     std::bitset<MAX_BITS> binaryValue(value);

//     // Calculate the number of padding zeros needed
//     int paddingZeros = totalBits - binaryValue.size();

//     // Add padding zeros to the binary representation
//     for (int i = 0; i < paddingZeros; ++i) {
//         binaryValue <<= 1;
//     }

//     // Extract bits using startBit and endBit
//     int numBits = endBit - startBit + 1;
//     int result = 0;

//     for (int i = 0; i < numBits; ++i) {
//         result |= (binaryValue[startBit + i] & 1) << i;
//     }

//     return result;
// }



// int extractBits(int value, int startBit, int endBit) {
//     int numBits = endBit - startBit + 1;
//     int result = 0;
//     // cout<<"StartBit :" << startBit <<" Endbit: " << endBit<<endl;

//     for (int i = 0; i < numBits; ++i) {
//         result |= ((value >> (startBit + i)) & 1) << i;
//     }

//     return result;
// }