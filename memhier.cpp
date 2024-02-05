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
#include <cstring>

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

struct TraceData
{
    int virtualAddress = -1;
    int virtualPage = -1;
    int pageOffset = -1;
    int tlbTag = -1;
    int tlbIndex = -1;
    char tlbRes[10];
    char ptRes[10];
    int physicalPage = -1;
    int dcTag = -1;
    int dcIndex = -1;
    char dcRes[10];
    int l2Tag = -1;
    int l2Index = -1;
    char l2Res[10];
};

struct DCSet
{
    int setIndex;
    vector<Cache> dcList;
};

struct L2Set
{
    int setIndex;
    vector<Cache> l2CacheList;
};
struct TLBSet
{
    int setIndex;
    vector<TLBData> tlbDataList;
};

vector<Page> pageTableList; // Page Table
vector<TraceData> traceDataList;
vector<DCSet> dcSetsList;
vector<L2Set> l2SetsList;
vector<TLBSet> tlbSetsList;

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
int pageOffSetBits, VPNBits, indexBits, tagBits, totalBits, physicalPageBits;
int dcIndexBits, dcOffsetBits, dcTagBits, dcTotalBits;
int l2IndexBits, l2OffsetBits, l2TagBits, l2TotalBits;
const int MAX_BITS = 32;
int currenPhysicalPageAddress = -1;
int trace = 0;

TraceData initTrace()
{
    TraceData traceData;
    traceData.tlbRes[0] = '\0';
    traceData.ptRes[0] = '\0';
    traceData.dcRes[0] = '\0';
    traceData.l2Res[0] = '\0';
    return traceData;
}

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
    if (offSet == 0)
    {
        offSet = 1;
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
    totalBits = log2(config.ptConfig.numPhysicalPages * config.ptConfig.numVirtualPages * config.ptConfig.pageSize); // virtual
    tagBits = totalBits - pageOffSetBits - indexBits;
    VPNBits = tagBits + indexBits;
    physicalPageBits = log2(config.ptConfig.numVirtualPages);

    // DC
    dcIndexBits = log2(config.dcConfig.numSets);
    dcOffsetBits = log2(config.dcConfig.lineSize);
    dcTotalBits = log2(config.ptConfig.numPhysicalPages * config.ptConfig.pageSize);
    dcTagBits = dcTotalBits - dcIndexBits - dcOffsetBits;
    // cout<<"dcOffsetBits: "<<dcOffsetBits<<endl;
    // cout<<"dcIndexBits :"<<dcIndexBits<<endl;
    // cout<<"dcTagBits: "<<dcTagBits<<endl;
    // cout<<"dcTotalBits :"<<dcTotalBits<<endl;

    // L2
    l2IndexBits = log2(config.l2Config.numSets);
    l2OffsetBits = log2(config.l2Config.lineSize);
    l2TotalBits = log2(config.ptConfig.numPhysicalPages * config.ptConfig.pageSize);
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
    for (TLBSet tlbSet : tlbSetsList)
    {
        cout << "Set: " << tlbSet.setIndex << endl;
        for (TLBData tlbData : tlbSet.tlbDataList)
        {
            cout << " inex: " << tlbData.index << " VPN: " << hex << concatBits(tlbData.tag, tlbData.index) << " PP: "
                 << " " << tlbData.physicalPageNumber << endl;
        }
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
    cout << endl
         << "DC DATA" << endl;
    for (DCSet dcSet : dcSetsList)
    {
        cout << "set : " << dcSet.setIndex << endl;
        for (Cache dcData : dcSet.dcList)
        {
            cout << " dc: " << dcData.index << " tag: " << dcData.tag << " count:" << dcData.count << endl;
        }
    }
}

void printSimulationStatistics()
{
    dtlbHitRatio = static_cast<double>(dtlbHits) / (dtlbHits + dtlbMisses);
    ptHitRatio = static_cast<double>(ptHits) / (ptHits + ptFaults);
    dcHitRatio = static_cast<double>(dcHits) / (dcHits + dcMisses);
    l2HitRatio = static_cast<double>(l2Hits) / (l2Hits + l2Misses);

    // cout << endl<< "Simulation statistics" << endl<<endl;
    // cout << "dtlb hits : " << dtlbHits << endl;
    // cout << "dtlb misses : " << dtlbMisses << endl;
    // cout << "dtlb hit ratio : " << fixed << setprecision(6) << dtlbHitRatio << endl<< endl;
    // cout << "pt hits : " << ptHits << endl;
    // cout << "pt faults : " << ptFaults << endl;
    // cout << "pt hit ratio : " << fixed << setprecision(6) << ptHitRatio << endl<< endl;
    // cout << "dc hits : " << dcHits << endl;
    // cout << "dc misses : " << dcMisses << endl;
    // cout << "dc hit ratio : " << fixed << setprecision(6) << dcHitRatio << endl<< endl;
    // cout << "L2 hits : " << l2Hits << endl;
    // cout << "L2 misses : " << l2Misses << endl;
    // cout << "L2 hit ratio : " << fixed << setprecision(6) << l2HitRatio << endl<< endl;
    // cout << "Total reads : " << totalReads << endl;
    // cout << "Total writes : " << totalWrites << endl;
    // cout << "Ratio of reads : " << fixed << setprecision(6) << ratioOfReads << endl<< endl;
    // cout << "main memory refs : " << mainMemoryRefs << endl;
    // cout << "page table refs : " << pageTableRefs << endl;
    // cout << "disk refs : " << diskRefs << endl;

    cout << endl
         << "Simulation statistics" << endl
         << endl;
    cout << left << setw(20) << "dtlb hits : " << dtlbHits << endl;
    cout << left << setw(20) << "dtlb misses : " << dtlbMisses << endl;
    cout << left << setw(20) << "dtlb hit ratio : " << fixed << setprecision(6) << dtlbHitRatio << endl
         << endl;
    cout << left << setw(20) << "pt hits : " << ptHits << endl;
    cout << left << setw(20) << "pt faults : " << ptFaults << endl;
    cout << left << setw(20) << "pt hit ratio : " << fixed << setprecision(6) << ptHitRatio << endl
         << endl;
    cout << left << setw(20) << "dc hits : " << dcHits << endl;
    cout << left << setw(20) << "dc misses : " << dcMisses << endl;
    cout << left << setw(20) << "dc hit ratio : " << fixed << setprecision(6) << dcHitRatio << endl
         << endl;
    cout << left << setw(20) << "L2 hits : " << l2Hits << endl;
    cout << left << setw(20) << "L2 misses : " << l2Misses << endl;
    cout << left << setw(20) << "L2 hit ratio : " << fixed << setprecision(6) << l2HitRatio << endl
         << endl;
    cout << left << setw(20) << "Total reads : " << totalReads << endl;
    cout << left << setw(20) << "Total writes : " << totalWrites << endl;
    cout << left << setw(20) << "Ratio of reads : " << fixed << setprecision(6) << ratioOfReads << endl
         << endl;
    cout << left << setw(20) << "main memory refs : " << mainMemoryRefs << endl;
    cout << left << setw(20) << "page table refs : " << pageTableRefs << endl;
    cout << left << setw(20) << "disk refs : " << diskRefs << endl;
}

void printConfig()
{
    
    cout << "Data TLB contains " << config.dtlbConfig.numSets << " sets." << endl;
    cout << "Each set contains " << config.dtlbConfig.setSize << " entries." << endl;
    cout << "Number of bits used for the index is " << indexBits << "." << endl
         << endl;

    cout << "Number of virtual pages is " << config.ptConfig.numVirtualPages << "." << endl;
    cout << "Number of physical pages is " << config.ptConfig.numPhysicalPages << "." << endl;
    cout << "Each page contains " << config.ptConfig.pageSize << " bytes." << endl;
    cout << "Number of bits used for the page table index is " << physicalPageBits << "." << endl;
    cout << "Number of bits used for the page offset is " << pageOffSetBits << "." << endl
         << endl;

    cout << "D-cache contains " << config.dcConfig.numSets << " sets." << endl;
    cout << "Each set contains " << config.dcConfig.setSize << " entries." << endl;
    cout << "Each line is " << config.dcConfig.lineSize << " bytes." << endl;
    if (config.dcConfig.writeThroughOrNoWriteAllocate == 1)
    {
        cout << "The cache uses a no write-allocate and write-through policy." << endl;
    }
    cout << "Number of bits used for the index is " << dcIndexBits << "." << endl;
    cout << "Number of bits used for the offset is " << dcOffsetBits << "." << endl
         << endl;

    cout << "L2-cache contains " << config.l2Config.numSets << " sets." << endl;
    cout << "Each set contains " << config.l2Config.setSize << " entries." << endl;
    cout << "Each line is " << config.l2Config.lineSize << " bytes." << endl;
    cout << "Number of bits used for the index is " << l2IndexBits << "." << endl;
    cout << "Number of bits used for the offset is " << l2OffsetBits << "." << endl
         << endl;
    if (config.useVirtualAddresses == 'y')
    {
        cout << "The addresses read in are virtual addresses." << endl
             << endl;
    }
    else
    {
        cout << "The addresses read in are physical addresses." << endl
             << endl;
    }
}

void printTraceData()
{
    size_t arraySize = traceDataList.size();
    for (size_t i = 0; i < arraySize; ++i)
    {
        // printf("%08x %6x %4x %8x %3x %4s %4s %3x %8x %3x %4s %8x %3x %4s\n",
        //        traceDataList[i].virtualAddress, traceDataList[i].virtualPage, traceDataList[i].pageOffset,
        //        traceDataList[i].tlbTag, traceDataList[i].tlbIndex, traceDataList[i].tlbRes, traceDataList[i].ptRes,
        //        traceDataList[i].physicalPage, traceDataList[i].dcTag, traceDataList[i].dcIndex, traceDataList[i].dcRes,
        //        traceDataList[i].l2Tag, traceDataList[i].l2Index, traceDataList[i].l2Res);
        char formattedVirtualAddress[9];
        char formattedVirtualPage[7];
        char formattedPageOffset[5];
        char formattedTlbTag[9];
        char formattedTlbIndex[4];
        char formattedTlbRes[5];
        char formattedPtRes[5];
        char formattedPhysicalPage[4];
        char formattedDcTag[9];
        char formattedDcIndex[4];
        char formattedDcRes[5];
        char formattedL2Tag[9];
        char formattedL2Index[4];
        char formattedL2Res[5];
        // %08x %6x %4x %6x %3x %4s %4s %4x %6x %3x %4s %6x %3x %4s
        // Format each field using snprintf
        snprintf(formattedVirtualAddress, sizeof(formattedVirtualAddress), "%08x", traceDataList[i].virtualAddress);
        snprintf(formattedVirtualPage, sizeof(formattedVirtualPage), traceDataList[i].virtualPage >= 0 ? "%6x" : "      ", traceDataList[i].virtualPage);
        snprintf(formattedPageOffset, sizeof(formattedPageOffset), traceDataList[i].pageOffset >= 0 ? "%4x" : "    ", traceDataList[i].pageOffset);
        snprintf(formattedTlbTag, sizeof(formattedTlbTag), traceDataList[i].tlbTag >= 0 ? "%6x" : "        ", traceDataList[i].tlbTag);
        snprintf(formattedTlbIndex, sizeof(formattedTlbIndex), traceDataList[i].tlbIndex >= 0 ? "%3x" : "   ", traceDataList[i].tlbIndex);
        snprintf(formattedTlbRes, sizeof(formattedTlbRes), traceDataList[i].tlbRes[0] != '\0' ? "%4s" : "    ", traceDataList[i].tlbRes);
        snprintf(formattedPtRes, sizeof(formattedPtRes), traceDataList[i].ptRes[0] != '\0' ? "%4s" : "    ", traceDataList[i].ptRes);
        snprintf(formattedPhysicalPage, sizeof(formattedPhysicalPage), traceDataList[i].physicalPage >= 0 ? "%4x" : "   ", traceDataList[i].physicalPage);
        snprintf(formattedDcTag, sizeof(formattedDcTag), traceDataList[i].dcTag >= 0 ? "%6x" : "        ", traceDataList[i].dcTag);
        snprintf(formattedDcIndex, sizeof(formattedDcIndex), traceDataList[i].dcIndex >= 0 ? "%3x" : "   ", traceDataList[i].dcIndex);
        snprintf(formattedDcRes, sizeof(formattedDcRes), traceDataList[i].dcRes[0] != '\0' ? "%4s" : "    ", traceDataList[i].dcRes);
        snprintf(formattedL2Tag, sizeof(formattedL2Tag), traceDataList[i].l2Tag >= 0 ? "%6x" : "        ", traceDataList[i].l2Tag);
        snprintf(formattedL2Index, sizeof(formattedL2Index), traceDataList[i].l2Index >= 0 ? "%3x" : "   ", traceDataList[i].l2Index);
        snprintf(formattedL2Res, sizeof(formattedL2Res), traceDataList[i].l2Res[0] != '\0' ? "%4s" : "    ", traceDataList[i].l2Res);

        // Print all fields
        printf("%s %s %s %s %s %s %s %4x %s %s %s %s %s %s\n",
               formattedVirtualAddress,
               formattedVirtualPage,
               formattedPageOffset,
               formattedTlbTag,
               formattedTlbIndex,
               formattedTlbRes,
               formattedPtRes,
               traceDataList[i].physicalPage,
               formattedDcTag,
               formattedDcIndex,
               formattedDcRes,
               formattedL2Tag,
               formattedL2Index,
               formattedL2Res);
    }
}

void printHeader()
{
    printf("Virtual  Virt.  Page TLB    TLB TLB  PT   Phys        DC  DC          L2  L2\n");
    printf("Address  Page # Off  Tag    Ind Res. Res. Pg # DC Tag Ind Res  L2 Tag Ind Res.\n");
    printf("-------- ------ ---- ------ --- ---- ---- ---- ------ --- ---- ------ --- ----\n");
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

void initCache()
{
    dcSetsList.resize(config.dcConfig.numSets);
    for (int i = 0; i < dcSetsList.size(); i++)
    {
        dcSetsList[i].setIndex = i;
        dcSetsList[i].dcList.resize(config.dcConfig.setSize);
        for (int j = 0; j < dcSetsList[i].dcList.size(); j++)
        {
            Cache entry;
            entry.valid = false;
            entry.count = 0;
            entry.index = -1;
            entry.tag = -1;
            dcSetsList[i].dcList[j] = entry;
        }
    }
}

void initL2Cache()
{
    l2SetsList.resize(config.l2Config.numSets);
    for (int i = 0; i < l2SetsList.size(); i++)
    {
        l2SetsList[i].setIndex = i;
        l2SetsList[i].l2CacheList.resize(config.l2Config.setSize);
        for (int j = 0; j < l2SetsList[i].l2CacheList.size(); j++)
        {
            Cache entry;
            entry.valid = false;
            entry.count = 0;
            entry.index = -1;
            entry.tag = -1;
            l2SetsList[i].l2CacheList[j] = entry;
        }
    }
}

void initTlb()
{
    tlbSetsList.resize(config.dtlbConfig.numSets);
    for (int i = 0; i < tlbSetsList.size(); i++)
    {
        tlbSetsList[i].setIndex = i;
        tlbSetsList[i].tlbDataList.resize(config.dtlbConfig.setSize);
        for (int j = 0; j < tlbSetsList[i].tlbDataList.size(); j++)
        {
            TLBData entry;
            entry.valid = false;
            entry.physicalPageNumber = -1;
            entry.count = 0;
            tlbSetsList[i].tlbDataList[j] = entry;
        }
    }
}

void ptinit()
{
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
    initCache();
    initL2Cache();
    initTlb();
    ptinit();
}

Cache performL2CacheAccess(int physicalAddess, int pageOffset, char accessType)
{
    Cache l2Cache;
    int index = extractBits(physicalAddess, l2TagBits, l2TagBits + l2IndexBits, l2TotalBits);
    int tag = extractBits(physicalAddess, 0, l2TagBits, l2TotalBits);
    // cout<<" l2tag: "<< hex << tag <<" | ";
    // cout<<" l2Index: "<<index<<" | ";

    traceDataList[trace].l2Index = index;
    traceDataList[trace].l2Tag = tag;
    bool flag = false;
    for (int i = 0; i < l2SetsList[index].l2CacheList.size(); i++)
    {
        if (l2SetsList[index].l2CacheList[i].tag == tag)
        {
            strcpy(traceDataList[trace].l2Res, "hit ");
            l2Hits++;
            l2SetsList[index].l2CacheList[i].count++;
            l2Cache = l2SetsList[index].l2CacheList[i];
            flag = true;
        }
    }
    if (flag == false)
    {
        strcpy(traceDataList[trace].l2Res, "miss");
        l2Misses++;
        int lruIndex = LRU(l2SetsList[index].l2CacheList);
        Cache l2Entry;
        l2Entry.tag = tag;
        l2Entry.valid = true;
        l2Entry.dirty = false;
        l2Entry.index = lruIndex;
        l2Entry.count = 0;
        l2SetsList[index].l2CacheList[lruIndex] = l2Entry;
        l2Cache = l2SetsList[index].l2CacheList[lruIndex];
    }
    return l2Cache;
}

Cache performDataCacheAccess(int physicalAddess, int pageOffSet, char accessType)
{
    Cache dcCache;
    int index = extractBits(physicalAddess, dcTagBits, dcTagBits + dcIndexBits, dcTotalBits);
    int tag = extractBits(physicalAddess, 0, dcTagBits, dcTotalBits);
    // cout<<" dctag: "<< hex << extractBits(physicalAddess,0,dcTagBits,dcTotalBits)<<" | ";
    // cout<<" dcIndex: "<<index<<" | ";

    traceDataList[trace].dcIndex = index;
    traceDataList[trace].dcTag = tag;
    bool flag = false;
    for (int i = 0; i < dcSetsList[index].dcList.size(); i++)
    {
        if (dcSetsList[index].dcList[i].tag == tag)
        {
            strcpy(traceDataList[trace].dcRes, "hit ");
            dcHits++;
            dcSetsList[index].dcList[i].count++;
            dcCache = dcSetsList[index].dcList[i];
            flag = true;
        }
    }
    if (flag == false)
    {
        strcpy(traceDataList[trace].dcRes, "miss");
        dcMisses++;
        Cache l2Cache = performL2CacheAccess(physicalAddess, pageOffSet, accessType);
        Cache dcEntry;
        int lruIndex = LRU(dcSetsList[index].dcList);
        dcEntry.tag = tag;
        dcEntry.valid = true;
        dcEntry.dirty = false;
        dcEntry.index = lruIndex;
        dcEntry.count = 0;
        dcSetsList[index].dcList[lruIndex] = dcEntry;
        dcCache = dcSetsList[index].dcList[lruIndex];
    }
    return dcCache;
}

Page performPageTableLookup(int virtualPageNumber)
{
    for (int i = 0; i < pageTableList.size(); i++)
    {
        if (pageTableList[i].virtualPage == virtualPageNumber)
        {
            strcpy(traceDataList[trace].ptRes, "hit ");
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
    strcpy(traceDataList[trace].ptRes, "miss");
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
    int tag = extractBits(virtualAddress, 0, tagBits, totalBits);

    traceDataList[trace].virtualPage = virtualPageNumber;
    traceDataList[trace].tlbIndex = index;
    traceDataList[trace].tlbTag = tag;
    bool flag = false;
    for (int i = 0; i < tlbSetsList[index].tlbDataList.size(); i++)
    {
        if (tlbSetsList[index].tlbDataList[i].tag == tag)
        {
            dtlbHits++;
            strcpy(traceDataList[trace].tlbRes, "hit ");
            tlbSetsList[index].tlbDataList[i].count++;
            tLBData = tlbSetsList[index].tlbDataList[i];
            flag = true;
        }
    }
    if (flag == false)
    {
        dtlbMisses++;
        strcpy(traceDataList[trace].tlbRes, "miss");
        Page pageData = performPageTableLookup(virtualPageNumber);
        int lruIndex = LRU(tlbSetsList[index].tlbDataList);
        TLBData tlbEntry;
        tlbEntry.tag = tag;
        tlbEntry.index = lruIndex;
        tlbEntry.physicalPageNumber = pageData.physicalPage;
        tlbEntry.valid = false;
        tlbEntry.count = 0;
        tlbSetsList[index].tlbDataList[lruIndex] = tlbEntry;

        tLBData = tlbEntry;
    }

    return tLBData;
}

void simulateMemoryAccess(string address, char accessType)
{
    int virtualAddress = stoi(address, nullptr, 16);
    int pageOffSet = extractBits(virtualAddress, tagBits + indexBits, tagBits + indexBits + pageOffSetBits, totalBits);
    traceDataList[trace].virtualAddress = virtualAddress;
    traceDataList[trace].pageOffset = pageOffSet;

    // Simulate TLB lookup
    TLBData tlbEntry = performTLBLookup(virtualAddress);
    traceDataList[trace].physicalPage = tlbEntry.physicalPageNumber;
    // printDTLB();
    // printPageTable();

    // DC LookUP
    string pageValue = convertHex(tlbEntry.physicalPageNumber, config.ptConfig.numPhysicalPages / 4);
    string pageOffsetValue = convertHex(pageOffSet, pageOffSetBits / 4);
    int physicalAddress = concatAsHex(pageValue, pageOffsetValue);
    // cout<<pageValue<<" : "<<pageOffsetValue<<" : "<<physicalAddress;
    Cache dcCache = performDataCacheAccess(physicalAddress, pageOffSet, accessType);
    // printDC();

    if (accessType == 'R')
    {
        totalReads++;
    }
    else if (accessType == 'W')
    {
        totalWrites++;
    }

    // Update cache hit ratios
    dtlbHitRatio = (dtlbHits * 1.0) / (dtlbHits + dtlbMisses);
    ptHitRatio = (ptHits * 1.0) / (ptHits + ptFaults);
    dcHitRatio = (dcHits * 1.0) / (dcHits + dcMisses);
    l2HitRatio = (l2Hits * 1.0) / (l2Hits + l2Misses);

    // Increment main memory references
    mainMemoryRefs++;

    // Increment page table references if TLB is not used or TLB miss
    if (!config.useTLB || (config.useTLB && !tlbEntry.valid))
    {
        pageTableRefs++;
    }

    // Increment disk references in case of a TLB miss or a page table fault
    if (!tlbEntry.valid)
    {
        diskRefs++;
    }
}

int main()
{
    config = readConfigFile("./trace.config");

    // printConfiguration();

    // Read trace file
    vector<string> traceData = readTraceFile("./trace.dat");
    // Iterate over each trace entry and simulate memory access
    initializeMemoryHierarchy();
    printConfig();
    for (const string &traceEntry : traceData)
    {
        char accessType = traceEntry[0];
        string hexAddress = traceEntry.substr(2); // Assuming hex address starts at index 2
        traceDataList.push_back(initTrace());
        simulateMemoryAccess(hexAddress, accessType);
        trace++;
    }
    printHeader();
    printTraceData();
    printSimulationStatistics();
    return 0;
}
