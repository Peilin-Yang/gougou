#include "InvertIndexBuilder.hpp"

#include "DAM.hpp"
#include "TermMeta.hpp"
#include "TermSummaryForInvert.hpp"

using namespace std;

InvertIndexBuilder::InvertIndexBuilder(char* temp_path) {
  term_meta_ = new TermMetaManager(temp_path);
}

InvertIndexBuilder::~InvertIndexBuilder() {
  delete term_meta_;
}

void InvertIndexBuilder::Finalize(char* term_path, char* invert_path) {
  term_meta_->Finalize();
  DiskAsMemory* invert_DAM = new DiskAsMemory(invert_path,sizeof(unsigned),4096);
  DiskAsMemory* term_DAM = new DiskAsMemory(term_path,sizeof(TermSummaryForInvert),1024);
  DiskItem* DI = term_DAM->addNewItem();
  TermSummaryForInvert* total_summary = (TermSummaryForInvert*)DI->content;
  total_summary->tf = term_meta_->GetTotalTF();
  total_summary->df = term_meta_->GetTermSize();
  total_summary->block_num = 0;
  total_summary->address = 0;
  total_summary->data_length = 0;
  delete DI;
  for (unsigned i=1; i<term_meta_->GetTermSize(); i++) {
    // Fill up the term summary part.
    DI = term_DAM->addNewItem();
    TermSummaryForInvert* summary = (TermSummaryForInvert*)DI->content;
    TermMeta* term_data = term_meta_->GetTermMeta(i);
    summary->df = term_data->GetDF();
    summary->tf = term_data->GetTF();
    summary->block_num = term_data->GetBlock().size();
    summary->address = invert_DAM->getItemCounter();
    // Fill up the block information part of invert index.
    const vector<TermMetaBlock>& blocks = term_data->GetBlock();
    DiskMultiItem* items = invert_DAM->AddMultiNewItem(blocks.size()*sizeof(BlockSummaryForInvert)/sizeof(unsigned));
    for (unsigned l=0; l<blocks.size(); l++) {
      BlockSummaryForInvert block_summary;
      block_summary.a1 = blocks[l].a1;
      block_summary.b1 = blocks[l].b1;
      block_summary.length1 = blocks[l].length1;
      block_summary.a2 = blocks[l].a2;
      block_summary.b2 = blocks[l].b2;
      block_summary.length2 = blocks[l].length2;
      block_summary.docID = blocks[l].docID;
      items->MemcpyIn((char*)&block_summary,l*sizeof(BlockSummaryForInvert)/sizeof(unsigned),sizeof(BlockSummaryForInvert)/sizeof(unsigned));
    }
    delete items;
    // Fill up the main body of invert index.
    for (unsigned l=0; l<blocks.size(); l++) {
      items = invert_DAM->AddMultiNewItem(blocks[l].length1);
      DiskMultiItem* source_items = term_meta_->GetDAM()->LocalMultiItem(blocks[l].address1,blocks[l].length1);
      unsigned temp1[blocks[l].length1];
      source_items->MemcpyOut((char*)temp1,0,blocks[l].length1);
      items->MemcpyIn((char*)temp1,0,blocks[l].length1);
      delete items;
      delete source_items;
      items = invert_DAM->AddMultiNewItem(blocks[l].length2);
      source_items = term_meta_->GetDAM()->LocalMultiItem(blocks[l].address2,blocks[l].length2);
      unsigned temp2[blocks[l].length2];
      source_items->MemcpyOut((char*)temp2,0,blocks[l].length2);

      items->MemcpyIn((char*)temp2,0,blocks[l].length2);
      delete items;
      delete source_items;
    }
    summary->data_length = invert_DAM->getItemCounter() - summary->address;
    delete DI;
  }
  delete invert_DAM;
  delete term_DAM;
}
