#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <iomanip>
#include <cmath>
#include <deque>
#include <bitset>
#include <map>
#include <limits>

using namespace std;

const int POSITIVE_INFINITY = numeric_limits<int>::max();

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

struct TLBData
{
    int tag;
    int physicalPageNumber;
    bool valid;
    int index;
    int count;
};

struct Cache
{
    int tag;
    bool valid;
    bool dirty;
    int physicalPage;
    int index;
    int count;
};

struct Page
{
    bool dirty;
    int physicalPage;
    int index;
    int virtualPage;
    int valid;
    int count;
};

vector<TLBData> tlbDataList; // Data TLB
vector<Cache> dcList;        // Data Cache
vector<Cache> l2CacheList;   // L2 Cache
vector<Page> pageTableList;  // Page Table

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
int pageOffSetBits, VPNBits, indexBits, tagBits, totalBits,physicalPageBits;
int dcIndexBits, dcOffsetBits, dcTagBits, dcTotalBits;
int l2IndexBits, l2OffsetBits, l2TagBits, l2TotalBits;
const int MAX_BITS = 32;
int currenPhysicalPageAddress = -1;


int concatBits(int value1, int value2)
{
    // Convert integers to binary strings
    bitset<MAX_BITS / 2> binaryValue1(value1);
    bitset<MAX_BITS / 2> binaryValue2(value2);
    // cout<<binaryValue1.to_string()<<endl;
    // cout<<binaryValue2.to_string()<<endl;

    // Concatenate integers
    string binaryString1 = binaryValue1.to_string();
    binaryString1.erase(0, binaryString1.find_first_not_of('0'));

    string binaryString2 = binaryValue2.to_string();
    binaryString2.erase(0, binaryString2.find_first_not_of('0'));

    // Concatenate binary strings
    string concatenatedBinary = binaryString1 + binaryString2;
    // cout<<concatenatedBinary<<endl;
    // Convert concatenated binary string to an integer
    int result = std::bitset<MAX_BITS>(concatenatedBinary).to_ulong();

    return result;
}

int concatAsHex(string hexValue1, string hexValue2)
{
    string concatenatedHex = hexValue1 + hexValue2;

    // Convert concatenated hex string to an integer
    int result;
    istringstream(concatenatedHex) >> hex >> result;

    return result;
}

string convertHex(int value, int offSet)
{
    if(offSet==0){
        offSet=1;
    }
    ostringstream stream;
    stream << hex << std::setw(offSet) << setfill('0') << value;
    return stream.str();
}

template <typename T>
int LRU(const std::vector<T> &list)
{
    int index = 0;
    int lruIndex = 0;
    int minValue = POSITIVE_INFINITY;
    for (const auto &item : list)
    {
        if (minValue > item.count)
        {
            minValue = item.count;
            lruIndex = index;
        }
        index++;
    }
    return lruIndex;
}

void calculateBits()
{
    pageOffSetBits = log2(config.ptConfig.pageSize);
    indexBits = log2(config.dtlbConfig.numSets);
    totalBits = log2(config.ptConfig.numPhysicalPages * config.ptConfig.numVirtualPages * config.ptConfig.pageSize);//virtual
    tagBits = totalBits - pageOffSetBits - indexBits;
    VPNBits = tagBits + indexBits;
    physicalPageBits = log2(config.ptConfig.numPhysicalPages);

    //DC
    dcIndexBits=log2(config.dcConfig.numSets);
    dcOffsetBits=log2(config.dcConfig.lineSize);
    dcTotalBits = log2(config.ptConfig.numPhysicalPages*config.ptConfig.pageSize);
    dcTagBits = dcTotalBits - dcIndexBits - dcOffsetBits;
    // cout<<"dcOffsetBits: "<<dcOffsetBits<<endl;
    // cout<<"dcIndexBits :"<<dcIndexBits<<endl;
    // cout<<"dcTagBits: "<<dcTagBits<<endl;
    // cout<<"dcTotalBits :"<<dcTotalBits<<endl;

    //L2
    l2IndexBits=log2(config.l2Config.numSets);
    l2OffsetBits=log2(config.l2Config.lineSize);
    l2TotalBits = log2(config.ptConfig.numPhysicalPages*config.ptConfig.pageSize);
    l2TagBits = l2TotalBits - l2IndexBits - l2OffsetBits;
    // cout<<"l2OffsetBits: "<<dec<<l2OffsetBits<<endl;
    // cout<<"l2IndexBits :"<<l2IndexBits<<endl;
    // cout<<"l2TagBits: "<<l2TagBits<<endl;
    // cout<<"l2TotalBits :"<<l2TotalBits<<endl;
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

void printDataTLBConfig(const DataTLBConfig &tlbConfig)
{
    cout << "Number of Sets: " << tlbConfig.numSets << endl;
    cout << "Set Size: " << tlbConfig.setSize << endl;
}

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

void printMemoryConfig(const MemoryConfig &memoryConfig)
{
    cout << "Number of Virtual Pages: " << memoryConfig.numVirtualPages << endl;
    cout << "Number of Physical Pages: " << memoryConfig.numPhysicalPages << endl;
    cout << "Page Size: " << memoryConfig.pageSize << endl;
}

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

void printDTLB()
{
    cout << "DTLB Data" << endl;
    for (TLBData tlbData : tlbDataList)
    {
        cout << " inex: " << tlbData.index << " VPN: " << hex << concatBits(tlbData.tag, tlbData.index) << " PP: "
             << " " << tlbData.physicalPageNumber << endl;
    }
}

void printPageTable()
{
    cout << "Page Table Data" << endl;
    for (Page page : pageTableList)
    {
        cout << " physicalPage: " << page.physicalPage << " VPN: " << page.virtualPage << " count:" << page.count << endl;
    }
}

void printDC()
{
    cout <<endl<< "DC DATA" << endl;
    for (Cache dcData : dcList)
    {
        cout << " dc: " << dcData.index << " tag: " << dcData.tag << " count:" << dcData.count << endl;
    }
}

void printMemoryAccessInfo(const Configuration &config, int address, const TLBData &tlbResult, const Page &ptResult, const Cache &dcResult, const Cache &l2Result)
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

int extractBits(int value, int startBit, int endBit, int totalBits)
{
    bitset<MAX_BITS> binaryValue(value);
    string result;
    int paddingZeros = 0;
    // cout<<value<<endl;
    // cout<<"total :"<<totalBits<<endl;
    // cout<<startBit<<" "<<endBit<<endl;
    // cout<<binaryValue<<endl;
    if (binaryValue.size() > totalBits)
    {
        paddingZeros = binaryValue.size() - totalBits + 1;
        // cout<<binaryValue.to_string().substr(paddingZeros-1,binaryValue.size())<<endl;
        result = (binaryValue.to_string().substr(paddingZeros - 1, binaryValue.size())).substr(startBit, endBit - startBit);
    }
    else
    {
        paddingZeros = totalBits - binaryValue.size();
        string padding = "";
        for (int i = 0; i < paddingZeros; i++)
        {
            padding = padding + '0';
        }
        // result = (padding + binaryValue.to_string()).substr(startBit, endBit - startBit);
    }
    // cout<<result<<endl;
    return bitset<MAX_BITS>(result).to_ulong();
    ;
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

    calculateBits();

    tlbDataList.resize(config.dtlbConfig.numSets * config.dtlbConfig.setSize);
    dcList.resize(config.dcConfig.numSets * config.dcConfig.setSize);
    l2CacheList.resize(config.l2Config.numSets * config.l2Config.setSize);
    pageTableList.resize(config.ptConfig.numPhysicalPages);
    for (int i = 0; i < config.ptConfig.numPhysicalPages; ++i)
    {
        Page page;
        page.physicalPage = -1;
        page.index = -1;
        page.valid = false;
        page.dirty = false;
        page.count = 0;
        pageTableList[i] = page;
    }

    for (int i = 0; i < tlbDataList.size(); ++i)
    {
        TLBData entry;
        entry.valid = false;
        entry.physicalPageNumber = -1;
        entry.count = 0;
        tlbDataList[i] = entry;
    }
    for (int i = 0; i < dcList.size(); ++i)
    {
        Cache entry;
        entry.valid = false;
        entry.count = 0;
        entry.index=-1;
        entry.tag=-1;
        dcList[i] = entry;
    }
    for (int i = 0; i < l2CacheList.size(); ++i)
    {
        Cache entry;
        entry.valid = false;
        entry.count = 0;
        entry.index=-1;
        entry.tag=-1;
        l2CacheList[i] = entry;
    }
}

Cache performL2CacheAccess(int physicalAddess ,int pageOffset, char accessType)
{
    Cache l2Cache;
    int index = extractBits(physicalAddess,l2TagBits,l2TagBits+l2IndexBits, l2TotalBits);
    int tag = extractBits(physicalAddess,0, l2TagBits,l2TotalBits);
    cout<<" l2tag: "<< hex << tag <<" | ";
    cout<<" l2Index: "<<index<<" | ";
    if (l2CacheList[index].tag == tag)
    {
        cout<<" l2Hit | ";
        l2Hits++;
        l2CacheList[index].count++;
        l2Cache = l2CacheList[index];
    }
    else
    {
        cout<<" l2Miss | ";
        l2Misses++;
        Cache l2Entry;
        l2Entry.tag = tag;
        l2Entry.valid = true;
        l2Entry.dirty = false;
        l2Entry.index = index;
        l2Entry.count =0;
        l2CacheList[index] = l2Entry;
        l2Cache = l2CacheList[index];
    }
    return l2Cache;
}

Cache performDataCacheAccess(int physicalAddess, int pageOffSet, char accessType)
{
    Cache dcCache;
    int index = extractBits(physicalAddess,dcTagBits,dcTagBits+dcIndexBits, dcTotalBits);
    int tag = extractBits(physicalAddess,0,dcTagBits,dcTotalBits);
    // cout<<" dctag: "<< hex << extractBits(physicalAddess,0,dcTagBits,dcTotalBits)<<" | ";
    // cout<<" dcIndex: "<<index<<" | ";
    if (dcList[index].tag == tag)
    {
        cout<<" dcHit | ";
        dcHits++;
        dcList[index].count++;
        dcCache = dcList[index];
    }
    else
    {
        cout<<" dcMiss | ";
        dcMisses++;
        Cache l2Cache = performL2CacheAccess(physicalAddess,pageOffSet, accessType);
        Cache dcEntry;
        dcEntry.tag = tag;
        dcEntry.valid = true;
        dcEntry.dirty = false;
        dcEntry.index = index;
        dcEntry.count =0;
        dcList[index] = dcEntry;
        dcCache = dcList[index];
    }
    return dcCache;
}

Page performPageTableLookup(int virtualPageNumber)
{
    for (int i = 0; i < pageTableList.size(); i++)
    {
        if (pageTableList[i].virtualPage == virtualPageNumber)
        {
            cout << " page hit |";
            ptHits++;
            pageTableList[i].count++;
            return pageTableList[i];
        }
    }
    if (currenPhysicalPageAddress < config.ptConfig.numPhysicalPages - 1)
    {
        currenPhysicalPageAddress++;
    }
    else
    {
        currenPhysicalPageAddress = LRU(pageTableList);
    }
    cout << " page Miss |";
    ptFaults++;
    Page pageData;
    pageData.physicalPage = currenPhysicalPageAddress;
    pageData.index = currenPhysicalPageAddress;
    pageData.virtualPage = virtualPageNumber;
    pageData.count = 0;
    pageTableList[currenPhysicalPageAddress] = pageData;

    return pageData;
}

TLBData performTLBLookup(int virtualAddress)
{
    TLBData tLBData;
    int virtualPageNumber = extractBits(virtualAddress, 0, VPNBits, totalBits);
    int index = extractBits(virtualAddress, tagBits, tagBits + indexBits, totalBits);

    if (concatBits(tlbDataList[index].tag, tlbDataList[index].index)== virtualPageNumber)
    {
        dtlbHits++;
        cout << " TLB hit | Page     | ";
        tlbDataList[index].count++;
        tLBData = tlbDataList[index];
    }
    else
    {
        dtlbMisses++;
        cout << " TLB Miss |";
        Page pageData = performPageTableLookup(virtualPageNumber);
        TLBData tlbEntry;
        tlbEntry.tag = extractBits(virtualAddress, 0, tagBits, totalBits);
        tlbEntry.index = index;
        tlbEntry.physicalPageNumber = pageData.physicalPage;
        tlbEntry.valid = false;
        tlbEntry.count = 0;
        tlbDataList[index] = tlbEntry;

        tLBData = tlbEntry;
    }

    return tLBData;
}

void simulateMemoryAccess(string address, char accessType)
{
    int virtualAddress = stoi(address, nullptr, 16);
    int pageOffSet =  extractBits(virtualAddress, tagBits + indexBits, tagBits + indexBits + pageOffSetBits, totalBits);
    cout << address << " | ";
    cout << "VPN : " << hex << extractBits(virtualAddress, 0, VPNBits, totalBits) << " | ";
    cout << "pageOffset : "  << extractBits(virtualAddress, tagBits + indexBits, tagBits + indexBits + pageOffSetBits, totalBits) << " | ";
    cout << "Tag : " << extractBits(virtualAddress, 0, tagBits, totalBits) << " | ";
    cout << "Tlb index : " << extractBits(virtualAddress, tagBits, tagBits + indexBits, totalBits) << " | ";

    // Simulate TLB lookup
    TLBData tlbEntry = performTLBLookup(virtualAddress);
    cout << "Page Index: " << tlbEntry.physicalPageNumber<<" | ";
    // printDTLB();
    // printPageTable();

    //DC LookUP
    string pageValue = convertHex(tlbEntry.physicalPageNumber,config.ptConfig.numPhysicalPages/4);
    string pageOffsetValue = convertHex(pageOffSet,pageOffSetBits/4);
    int physicalAddress = concatAsHex(pageValue, pageOffsetValue);
    // cout<<pageValue<<" : "<<pageOffsetValue<<" : "<<physicalAddress;
    Cache dcCache = performDataCacheAccess( physicalAddress,pageOffSet, accessType);
    // printDC();

    if (accessType == 'R')
    {
        totalReads++;
    }
    else if (accessType == 'W')
    {
        totalWrites++;
    }

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

int main()
{
    config = readConfigFile("./trace.config");

    // printConfiguration();

    // Read trace file
    vector<string> traceData = readTraceFile("./trace.dat");
    // Iterate over each trace entry and simulate memory access
    initializeMemoryHierarchy();
    for (const string &traceEntry : traceData)
    {
        char accessType = traceEntry[0];
        string hexAddress = traceEntry.substr(2); // Assuming hex address starts at index 2

        simulateMemoryAccess(hexAddress, accessType);
        // performDataCacheAccess(284, 28);
        cout<<endl;
    }

    // printSimulationStatistics();

    return 0;
}
