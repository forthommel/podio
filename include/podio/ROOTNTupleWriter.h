#ifndef RNTUPLEWRITER_H
#define RNTUPLEWRITER_H

#include <ROOT/RNTuple.hxx>
#include <ROOT/RNTupleModel.hxx>
#include <cstdint>
#include <iostream>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "TBranch.h"
#include "podio/CollectionBase.h"
#include "podio/CollectionBranches.h"
#include "podio/EventStore.h"

namespace rnt = ROOT::Experimental;
// forward declarations
class TFile;
class TTree;

namespace podio {
class ROOTNTupleWriter {
public:
  ROOTNTupleWriter(const std::string& filename, EventStore* store);
  ~ROOTNTupleWriter();

  // TODO: delete copy ctors
  //
  bool registerForWrite(const std::string& name);
  void writeEvent();
  void finish();

  using StoreCollection = std::pair<const std::string&, podio::CollectionBase*>;
  void createModels(const std::vector<StoreCollection>& collections);

private:
  std::unique_ptr<rnt::RNTupleModel> m_events{nullptr};
  std::unique_ptr<rnt::RNTupleModel> m_metadata{nullptr};
  std::unique_ptr<rnt::RNTupleModel> m_runMD{nullptr};
  std::unique_ptr<rnt::RNTupleModel> m_colMD{nullptr};
  std::unique_ptr<rnt::RNTupleModel> m_evtMD{nullptr};

  TFile* m_file{nullptr};

  std::vector<std::string> m_collectionsToWrite;

  EventStore* m_store{nullptr};
  bool m_firstEvent{true};
  std::unique_ptr<rnt::RNTupleWriter> m_ntuple_events{nullptr};
  std::unique_ptr<rnt::RNTupleWriter> m_ntuple_metadata{nullptr};
  std::unique_ptr<rnt::RNTupleWriter> m_ntuple_runMD{nullptr};
  std::unique_ptr<rnt::RNTupleWriter> m_ntuple_colMD{nullptr};
  std::unique_ptr<rnt::RNTupleWriter> m_ntuple_evtMD{nullptr};
};

} // namespace podio

#endif // RNTUPLEWRITER_H
