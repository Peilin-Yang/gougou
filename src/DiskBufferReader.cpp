#include "DiskBufferReader.hpp"
#include "DAMreader.hpp"
#include <climits>

using namespace std;

DiskBufferReader::DiskBufferReader(DiskAsMemoryReader *theIOP,uint64_t beginP,uint64_t endP,int fileHandle,int offset) : theIO(theIOP),begin(beginP),end(endP),itemSize(theIOP->getItemSize())
{
	usage = 0;
	if(begin==ULLONG_MAX) //head buffer
	{
		buffer = (char*)mmap(0,end*itemSize,PROT_READ,MAP_SHARED,fileHandle,offset);
	}
	else
	{
		if(end<=begin)
		{
			cerr<<"incorrect offset!"<<endl;
			return;
		}
		buffer = (char*)mmap(0,(end-begin)*itemSize,PROT_READ,MAP_SHARED,fileHandle,offset);
	}
	if(buffer == MAP_FAILED)
	{
		cerr<<"Could not mmap the buffer!"<<beginP<<" : "<<endP<<endl;
	}
}

DiskItemReader* DiskBufferReader::localItem(uint64_t itemNum)
{
	if(begin>end) // head buffer
	{
		if(itemNum<end)
		{
			usage++;
			DiskItemReader* DI = new DiskItemReader(buffer+itemNum*itemSize,itemNum,this);
			return DI;
		}
		else return NULL;
	}
	else
	{
		if(itemNum<end && itemNum>=begin)
		{
			usage++;
			DiskItemReader* DI = new DiskItemReader(buffer+(itemNum-begin)*itemSize,itemNum,this);
			return DI;
		}
		else return NULL;
	}
}

bool DiskBufferReader::PutMultiItems(DiskMultiItemReader* DMI,uint64_t begin_num,unsigned item_num) {
  if(begin>end) { // head buffer
    if(begin_num + item_num <= end) {
      usage++;
      DMI->PutItems(this, buffer+itemSize*item_num, item_num);
      return true;
    } else {
      cerr << "Locate multiple items failed at the buffer which is "<<begin<<" : "<<end<<endl;
      return false;
    }
  } else {
    if(begin_num >= begin && begin_num+item_num <= end) {
      usage++;
      DMI->PutItems(this, buffer+itemSize*(begin_num-begin), item_num);
      return true;
    } else {
      cerr << "Locate multiple items failed at the buffer which is "<<begin<<" : "<<end<<endl;
      return false;
    }
  }
}

void DiskBufferReader::freeUsage()
{
	if(usage>1) usage--;
	else
	{
		usage = 0;
		if(begin!=ULLONG_MAX) theIO->freeBuffer(this);
	}
}

DiskBufferReader::~DiskBufferReader()
{
	if(begin==ULLONG_MAX) munmap(buffer,end*itemSize);
	else munmap(buffer,(end-begin)*itemSize);
}
