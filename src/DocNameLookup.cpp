#include "DocNameLookup.hpp"

#include <stdint.h>
#include <string.h>

using namespace std;

DocNameLookup::DocNameLookup(char* path) {
  char summary_path[256],body_path[256];
  strcpy(summary_path,path);
  strcpy(body_path,path);
  strcat(summary_path,"/summary");
  strcat(body_path,"/body");
  summary_ = new DiskAsMemory(summary_path,sizeof(uint64_t),1024);
  body_ = new DiskAsMemory(body_path,sizeof(char),4096);
  summary_->addNewItem();
}

void DocNameLookup::AddDoc(char* doc_name,unsigned docID) {
  unsigned length = strlen(doc_name);
  DiskItem* DI = summary_->addNewItem();
  uint64_t body_usage = body_->getItemCounter();
  memcpy(DI->content,&body_usage,sizeof(uint64_t));
  delete DI;
  DiskMultiItem* items = body_->AddMultiNewItem(length);
  items->MemcpyIn(doc_name,0,length);
  delete items;
}

void DocNameLookup::AddDoc(string doc_name,unsigned docID) {
  AddDoc(doc_name.c_str(),docID);
}

DocNameLookup::~DocNameLookup() {
  DiskItem* DI = summary_->addNewItem();
  delete DI;
  delete summary_;
  delete body_;
}
