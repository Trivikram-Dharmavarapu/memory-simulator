#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <iomanip>
#include <cmath>
#include <deque>
#include <bitset>
#include <map>

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
} config;

// Define TLB entry structure
struct TLBData
{
    int tag;
    int physicalPageNumber;
    bool valid;
    int index;
};

// Define Cache entry structure
struct CacheEntry
{
    int tag;
    bool valid;
    bool dirty;
    int physicalPage;
    int index;
};

// Define Memory Page structure
struct Page
{
    bool dirty;
    int physicalPage;
    int index;
    int virtualPage;
    int valid;
};

// Define Memory Hierarchy components
vector<TLBData> tlbDataList;      // Data TLB
vector<CacheEntry> dc;      // Data Cache
vector<CacheEntry> l2Cache; // L2 Cache
vector<Page> pageTableList;     // Page Table

int ptHits;
int ptFaults;
int dcHits;
int dcMisses;
int l2Hits;
int l2Misses;
int totalReads;
int totalWrites;
double ratioOfReads;
int mainMemoryRefs;
int pageTableRefs;
int diskRefs;
int dtlbHits = 0;
int dtlbMisses = 0;
double dtlbHitRatio;
double ptHitRatio;
double dcHitRatio;
double l2HitRatio;
int pageOffSetBits;
int VPNBits;
int indexBits;
int tagBits;
int totalBits;
const int MAX_BITS= 32;
int currenPhysicalPageAddress=-1;

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
                        currentData = "";
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
        }
        file.close();
    }
    else
    {
        cerr << "Error: Unable to open trace file." << endl;
    }
    return traceData;
}

int extractBits(int value, int startBit, int endBit) {
   bitset<MAX_BITS> binaryValue(value);
   string result;
    int paddingZeros=0;
    // cout<<value<<endl;
    // cout<<startBit<<" "<<endBit<<endl;
    // cout<<binaryValue<<endl;
    if (binaryValue.size() > totalBits)
    {
        paddingZeros = binaryValue.size() - totalBits +1;
        // cout<<binaryValue.to_string().substr(paddingZeros-1,binaryValue.size())<<endl;
        result = (binaryValue.to_string().substr(paddingZeros-1,binaryValue.size())).substr(startBit, endBit - startBit);
    }
    else
    {
        paddingZeros = totalBits - binaryValue.size();
        string padding ="";
        for(int i=0;i<paddingZeros;i++){
            padding = padding+'0';
        }
        result = (padding +binaryValue.to_string()).substr(startBit, endBit - startBit);
    }
    // cout<<result<<endl;
    return bitset<MAX_BITS>(result).to_ulong();;
}

int getVirtualPageNumber(int tag, int index) {
   // Convert integers to binary strings
    bitset<MAX_BITS/2> binaryValue1(tag);
    bitset<MAX_BITS/2> binaryValue2(index);


    // Concatenate binary strings
    string concatenatedBinary = binaryValue1.to_string() + binaryValue2.to_string();

    // Convert concatenated binary string to an integer
    int result = bitset<MAX_BITS>(concatenatedBinary).to_ulong();

    return result;
}


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
void printConfiguration()
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

void printDTLB() {
    cout<<"DTLB Data"<<endl;
    for (TLBData tlbData  : tlbDataList) {
        cout<<" inex: "<<tlbData.index<<" VPN: "<<hex<<getVirtualPageNumber(tlbData.tag,tlbData.index)<<" PP: "<<" "<<tlbData.physicalPageNumber<<endl;
    }
}
void printPageTable() {
    cout<<"DTLB Data"<<endl;
    for (Page page  : pageTableList) {
        cout<<" physicalPage: "<<page.physicalPage<<" VPN: "<<page.virtualPage<<endl;
    }
}
void initializeMemoryHierarchy()
{
    dtlbHits = 0;
    dtlbMisses = 0;
    ptHits = 0;
    ptFaults = 0;
    dcHits = 0;
    dcMisses = 0;
    l2Hits = 0;
    l2Misses = 0;
    totalReads = 0;
    totalWrites = 0;
    mainMemoryRefs = 0;
    pageTableRefs = 0;
    diskRefs = 0;
    pageOffSetBits = log2(config.ptConfig.pageSize);
    //cout<<pageOffSetBits<<endl;
    indexBits = log2(config.dtlbConfig.numSets);
    //cout<<indexBits<<endl;
    totalBits = log2(config.ptConfig.numPhysicalPages*config.ptConfig.numVirtualPages*config.ptConfig.pageSize);
    tagBits = totalBits - pageOffSetBits - indexBits;
    VPNBits = tagBits+indexBits;
    //cout<<tagBits<<endl;

    // Initialize TLB
    tlbDataList.resize(config.dtlbConfig.numSets * config.dtlbConfig.setSize);
    // Initialize Data Cache
    dc.resize(config.dcConfig.numSets * config.dcConfig.setSize);

    // Initialize L2 Cache
    l2Cache.resize(config.l2Config.numSets * config.l2Config.setSize);

    // Initialize Page Table
    pageTableList.resize(config.ptConfig.numPhysicalPages);

    // Initialize physical pages
    for (int i = 0; i < config.ptConfig.numPhysicalPages; ++i)
    {
        Page page;
        page.physicalPage = -1;
        page.index =-1;
        page.valid = false;
        page.dirty = false;
        pageTableList[i] = page;
    }

    // Initialization of TLB entries
    for (int i = 0; i < tlbDataList.size(); ++i)
    {
        TLBData entry;
        entry.valid = false;
        entry.physicalPageNumber=-1;
        tlbDataList[i] = entry;
    }

    // Initialization of Data Cache entries
    for (int i = 0; i < dc.size(); ++i)
    {
        CacheEntry entry;
        entry.valid = false;
        // Initialize other fields as needed
        dc[i] = entry;
    }

    // Initialization of L2 Cache entries
    for (int i = 0; i < l2Cache.size(); ++i)
    {
        CacheEntry entry;
        entry.valid = false;
        // Initialize other fields as needed
        l2Cache[i] = entry;
    }
}

// Helper function for Data Cache access
CacheEntry performDataCacheAccess(const Page &page, int pageOffset, char accessType)
{
    int index = (page.physicalPage * config.ptConfig.pageSize + pageOffset) % config.dcConfig.numSets;

    if (dc[index].valid && dc[index].tag == page.physicalPage)
    {
        // Data Cache hit
        dcHits++;
        return dc[index];
    }
    else
    {
        // Data Cache miss
        dcMisses++;
        CacheEntry dcEntry;
        dcEntry.valid = false;

        // Fetch data from the next level (e.g., memory) and update Data Cache
        // For simplicity, let's assume data is always present in the next level
        dcEntry.tag = page.physicalPage;
        dcEntry.valid = true;
        dcEntry.dirty = false;

        // Update Data Cache
        dc[index] = dcEntry;

        return dcEntry;
    }
}

// Helper function for L2 Cache access
CacheEntry performL2CacheAccess(const CacheEntry &dcEntry, int pageOffset, char accessType)
{
    int index = (dcEntry.physicalPage * config.ptConfig.pageSize + pageOffset) % config.l2Config.numSets;

    if (l2Cache[index].valid && l2Cache[index].tag == dcEntry.physicalPage)
    {
        // L2 Cache hit
        l2Hits++;
        return l2Cache[index];
    }
    else
    {
        // L2 Cache miss
        l2Misses++;
        CacheEntry l2Entry;
        l2Entry.valid = false;

        // Fetch data from the next level (e.g., memory) and update L2 Cache
        // For simplicity, let's assume data is always present in the next level
        l2Entry.tag = dcEntry.physicalPage;
        l2Entry.valid = true;
        l2Entry.dirty = false;

        // Update L2 Cache
        l2Cache[index] = l2Entry;

        return l2Entry;
    }
}

// Helper function for Page Table lookup
Page performPageTableLookup( int virtualPageNumber)
{
    for(Page pageData:pageTableList){
        if(pageData.virtualPage==virtualPageNumber){
            cout<<" page hit |";
            ptHits++;
            return pageData;
        }

    }
    if(currenPhysicalPageAddress < config.ptConfig.numPhysicalPages-1 ){
        currenPhysicalPageAddress++;
    }
    else{
        currenPhysicalPageAddress=0;
    }
    cout<<" page Miss |";
    ptFaults++;
    Page pageData;
    pageData.physicalPage = currenPhysicalPageAddress;
    pageData.index = currenPhysicalPageAddress;
    pageData.virtualPage = virtualPageNumber;
    pageTableList[currenPhysicalPageAddress] = pageData;

    return pageData;
}

TLBData performTLBLookup(int virtualAddress)
{
    TLBData tLBData;
    int virtualPageNumber =  extractBits(virtualAddress,0,VPNBits);
    int index = extractBits(virtualAddress,tagBits,tagBits+indexBits);

    if (getVirtualPageNumber(tlbDataList[index].tag,tlbDataList[index].index) == virtualPageNumber)
    {
        dtlbHits++;
         cout<<" TLB hit |";
        tLBData = tlbDataList[index];
    }
    else
    {
        dtlbMisses++;
         cout<<" TLB Miss |";
        Page pageData = performPageTableLookup(virtualPageNumber);
        TLBData tlbEntry;
        tlbEntry.tag = extractBits(virtualAddress,0,tagBits);
        tlbEntry.index = index;
        tlbEntry.physicalPageNumber = pageData.physicalPage;
        tlbEntry.valid = false;
        tlbDataList[index] = tlbEntry;

        tLBData = tlbEntry;
    }

    return tLBData;
}


void simulateMemoryAccess(string address, char accessType)
{
    int virtualAddress = stoi(address, nullptr, 16);

    cout<<address<<" | ";
    cout<<"VPN : " << hex << extractBits(virtualAddress,0,VPNBits) <<" | ";
    cout<<"pageOffset : " <<hex<< extractBits(virtualAddress,tagBits+indexBits,tagBits+indexBits+pageOffSetBits) <<" | ";
    cout<<"Tag : " << extractBits(virtualAddress,0,tagBits) <<" | ";
    cout<<"Tlb index : " << extractBits(virtualAddress,tagBits,tagBits+indexBits) <<" | ";

    // Simulate TLB lookup
    TLBData tlbEntry = performTLBLookup(virtualAddress);
    cout<<"Page Index: "<<tlbEntry.physicalPageNumber<<endl;
    // printDTLB();
    // printPageTable();
    // cout<<tlbEntry.physicalPageNumber<<endl;

    // Simulate Data Cache access
    // CacheEntry dcEntry = performDataCacheAccess( page, pageOffset, accessType);

    // // Simulate L2 Cache access if enabled
    // CacheEntry l2Entry;
    // if (config.useL2Cache)
    // {
    //     l2Entry = performL2CacheAccess( dcEntry, pageOffset, accessType);
    // }

    // // Increment total reads or writes based on access type
    // if (accessType == 'R')
    // {
    //     totalReads++;
    // }
    // else if (accessType == 'W')
    // {
    //     totalWrites++;
    // }

    // // Update cache hit ratios
    // dtlbHitRatio = (dtlbHits * 1.0) / (dtlbHits + dtlbMisses);
    // ptHitRatio = (ptHits * 1.0) / (ptHits + ptFaults);
    // dcHitRatio = (dcHits * 1.0) / (dcHits + dcMisses);
    // l2HitRatio = (l2Hits * 1.0) / (l2Hits + l2Misses);

    // // Increment main memory references
    // mainMemoryRefs++;

    // // Increment page table references if TLB is not used or TLB miss
    // if (!config.useTLB || (config.useTLB && !tlbEntry.valid))
    // {
    //     pageTableRefs++;
    // }

    // // Increment disk references in case of a TLB miss or a page table fault
    // if (!tlbEntry.valid || !page.valid)
    // {
    //     diskRefs++;
    // }
}


void printMemoryAccessInfo(const Configuration &config, int address, const TLBData &tlbResult, const Page &ptResult, const CacheEntry &dcResult, const CacheEntry &l2Result)
{
    cout << "Virtual Address: " << address << endl;

    // Print TLB result
    cout << "TLB Result:" << endl;
    cout << "  Valid: " << (tlbResult.valid ? "Yes" : "No") << endl;
    cout << "  Physical Page: " << tlbResult.physicalPageNumber << endl;
    cout << "  TLB Hits: " << dtlbHits << endl;
    cout << "  TLB Misses: " << dtlbMisses << endl;

    // Print Page Table result
    cout << "Page Table Result:" << endl;
    cout << "  Valid: " << (ptResult.valid ? "Yes" : "No") << endl;
    cout << "  Physical Page: " << ptResult.physicalPage << endl;
    cout << "  Page Table Hits: " << ptHits << endl;
    cout << "  Page Table Faults: " << ptFaults << endl;

    // Print Data Cache result
    cout << "Data Cache Result:" << endl;
    cout << "  Valid: " << (dcResult.valid ? "Yes" : "No") << endl;
    cout << "  Dirty: " << (dcResult.dirty ? "Yes" : "No") << endl;
    cout << "  Data Cache Hits: " << dcHits << endl;
    cout << "  Data Cache Misses: " << dcMisses << endl;

    // Print L2 Cache result if used
    if (config.useL2Cache)
    {
        cout << "L2 Cache Result:" << endl;
        cout << "  Valid: " << (l2Result.valid ? "Yes" : "No") << endl;
        cout << "  Dirty: " << (l2Result.dirty ? "Yes" : "No") << endl;
        cout << "  L2 Cache Hits: " << l2Hits << endl;
        cout << "  L2 Cache Misses: " << l2Misses << endl;
    }

    cout << "------------------------" << endl;
}


void printSimulationStatistics()
{
    dtlbHitRatio = static_cast<double>(dtlbHits) / (dtlbHits + dtlbMisses);
    ptHitRatio = static_cast<double>(ptHits) / (ptHits + ptFaults);
    dcHitRatio = static_cast<double>(dcHits) / (dcHits + dcMisses);
    l2HitRatio = static_cast<double>(l2Hits) / (l2Hits + l2Misses);

    cout << "Simulation statistics" << endl;
    cout << "dtlb hits : " << dtlbHits << endl;
    cout << "dtlb misses : " << dtlbMisses << endl;
    cout << "dtlb hit ratio : " << fixed << setprecision(6) << dtlbHitRatio << endl;
    cout << "pt hits : " << ptHits << endl;
    cout << "pt faults : " << ptFaults << endl;
    cout << "pt hit ratio : " << fixed << setprecision(6) << ptHitRatio << endl;
    cout << "dc hits : " << dcHits << endl;
    cout << "dc misses : " << dcMisses << endl;
    cout << "dc hit ratio : " << fixed << setprecision(6) << dcHitRatio << endl;
    cout << "L2 hits : " << l2Hits << endl;
    cout << "L2 misses : " << l2Misses << endl;
    cout << "L2 hit ratio : " << fixed << setprecision(6) << l2HitRatio << endl;
    cout << "Total reads : " << totalReads << endl;
    cout << "Total writes : " << totalWrites << endl;
    cout << "Ratio of reads : " << fixed << setprecision(6) << ratioOfReads << endl;
    cout << "main memory refs : " << mainMemoryRefs << endl;
    cout << "page table refs : " << pageTableRefs << endl;
    cout << "disk refs : " << diskRefs << endl;
}

int main()
{
    config = readConfigFile("./trace.config");

    //printConfiguration();

    // Read trace file
    vector<string> traceData = readTraceFile("./trace.dat");
    // Iterate over each trace entry and simulate memory access
    initializeMemoryHierarchy();
    for (const string &traceEntry : traceData)
    {
        char accessType = traceEntry[0];
        string hexAddress = traceEntry.substr(2); // Assuming hex address starts at index 2

        simulateMemoryAccess(hexAddress, accessType);
    }

    //printSimulationStatistics();

    return 0;
}
