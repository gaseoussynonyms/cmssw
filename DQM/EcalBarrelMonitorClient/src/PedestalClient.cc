#include "../interface/PedestalClient.h"

#include "DQM/EcalCommon/interface/MESetMulti.h"

#include "DataFormats/EcalDetId/interface/EcalPnDiodeDetId.h"

#include "CondFormats/EcalObjects/interface/EcalDQMStatusHelper.h"

namespace ecaldqm
{

  PedestalClient::PedestalClient(edm::ParameterSet const& _workerParams, edm::ParameterSet const& _commonParams) :
    DQWorkerClient(_workerParams, _commonParams, "PedestalClient"),
    expectedMean_(0),
    toleranceMean_(0),
    toleranceRMS_(0),
    expectedPNMean_(0),
    tolerancePNMean_(0),
    tolerancePNRMS_(0)
  {
    using namespace std;

    vector<int> MGPAGains(_commonParams.getUntrackedParameter<vector<int> >("MGPAGains"));
    vector<int> MGPAGainsPN(_commonParams.getUntrackedParameter<vector<int> >("MGPAGainsPN"));

    unsigned iMEGain(0);
    for(vector<int>::iterator gainItr(MGPAGains.begin()); gainItr != MGPAGains.end(); ++gainItr){
      if(*gainItr != 1 && *gainItr != 6 && *gainItr != 12) throw cms::Exception("InvalidConfiguration") << "MGPA gain" << endl;
      gainToME_[*gainItr] = iMEGain++;
    }

    unsigned iMEPNGain(0);
    for(vector<int>::iterator gainItr(MGPAGainsPN.begin()); gainItr != MGPAGainsPN.end(); ++gainItr){
      if(*gainItr != 1 && *gainItr != 16) throw cms::Exception("InvalidConfiguration") << "PN diode gain" << endl;	
      pnGainToME_[*gainItr] = iMEPNGain++;
    }

    stringstream ss;

    expectedMean_.resize(iMEGain);
    toleranceMean_.resize(iMEGain);
    toleranceRMS_.resize(iMEGain);

    for(map<int, unsigned>::iterator gainItr(gainToME_.begin()); gainItr != gainToME_.end(); ++gainItr){
      ss.str("");
      ss << "G" << gainItr->first;

      expectedMean_[gainItr->second] = _workerParams.getUntrackedParameter<double>("expectedMean" + ss.str());
      toleranceMean_[gainItr->second] = _workerParams.getUntrackedParameter<double>("toleranceMean" + ss.str());
      toleranceRMS_[gainItr->second] = _workerParams.getUntrackedParameter<double>("toleranceRMS" + ss.str());
    }

    expectedPNMean_.resize(iMEPNGain);
    tolerancePNMean_.resize(iMEPNGain);
    tolerancePNRMS_.resize(iMEPNGain);

    for(map<int, unsigned>::iterator gainItr(pnGainToME_.begin()); gainItr != pnGainToME_.end(); ++gainItr){
      ss.str("");
      ss << "G" << gainItr->first;

      expectedPNMean_[gainItr->second] = _workerParams.getUntrackedParameter<double>("expectedPNMean" + ss.str());
      tolerancePNMean_[gainItr->second] = _workerParams.getUntrackedParameter<double>("tolerancePNMean" + ss.str());
      tolerancePNRMS_[gainItr->second] = _workerParams.getUntrackedParameter<double>("tolerancePNRMS" + ss.str());
    }

    map<string, string> replacements;

    unsigned apdPlots[] = {kQuality, kMean, kRMS, kQualitySummary};
    for(unsigned iS(0); iS < sizeof(apdPlots) / sizeof(unsigned); ++iS){
      unsigned plot(apdPlots[iS]);
      MESetMulti* multi(static_cast<MESetMulti*>(MEs_[plot]));

      for(map<int, unsigned>::iterator gainItr(gainToME_.begin()); gainItr != gainToME_.end(); ++gainItr){
        multi->use(gainItr->second);

        ss.str("");
        ss << gainItr->first;
        replacements["gain"] = ss.str();

        multi->formPath(replacements);
      }
    }

    unsigned pnPlots[] = {kPNRMS, kPNQualitySummary};
    for(unsigned iS(0); iS < sizeof(pnPlots) / sizeof(unsigned); ++iS){
      unsigned plot(pnPlots[iS]);
      MESetMulti* multi(static_cast<MESetMulti*>(MEs_[plot]));

      for(map<int, unsigned>::iterator gainItr(pnGainToME_.begin()); gainItr != pnGainToME_.end(); ++gainItr){
        multi->use(gainItr->second);

        ss.str("");
        ss << gainItr->first;
        replacements["pngain"] = ss.str();

        multi->formPath(replacements);
      }
    }

    unsigned apdSources[] = {kPedestal};
    for(unsigned iS(0); iS < sizeof(apdSources) / sizeof(unsigned); ++iS){
      unsigned plot(apdSources[iS]);
      MESetMulti const* multi(static_cast<MESetMulti const*>(sources_[plot]));

      for(map<int, unsigned>::iterator gainItr(gainToME_.begin()); gainItr != gainToME_.end(); ++gainItr){
        multi->use(gainItr->second);

        ss.str("");
        ss << gainItr->first;
        replacements["gain"] = ss.str();

        multi->formPath(replacements);
      }
    }

    unsigned pnSources[] = {kPNPedestal};
    for(unsigned iS(0); iS < sizeof(pnSources) / sizeof(unsigned); ++iS){
      unsigned plot(pnSources[iS]);
      MESetMulti const* multi(static_cast<MESetMulti const*>(sources_[plot]));

      for(map<int, unsigned>::iterator gainItr(pnGainToME_.begin()); gainItr != pnGainToME_.end(); ++gainItr){
        multi->use(gainItr->second);

        ss.str("");
        ss << gainItr->first;
        replacements["pngain"] = ss.str();

        multi->formPath(replacements);
      }
    }
  }

  void
  PedestalClient::beginRun(edm::Run const&, edm::EventSetup const&)
  {
    for(unsigned iME(0); iME < gainToME_.size(); ++iME){
      static_cast<MESetMulti*>(MEs_[kQuality])->use(iME);
      static_cast<MESetMulti*>(MEs_[kQualitySummary])->use(iME);

      MEs_[kQuality]->resetAll(-1.);
      MEs_[kQualitySummary]->resetAll(-1.);

      MEs_[kQuality]->reset(kUnknown);
      MEs_[kQualitySummary]->reset(kUnknown);
    }

    for(unsigned iME(0); iME < pnGainToME_.size(); ++iME){
      static_cast<MESetMulti*>(MEs_[kPNQualitySummary])->use(iME);

      MEs_[kPNQualitySummary]->resetAll(-1.);

      MEs_[kPNQualitySummary]->reset(kUnknown);
    }
  }

  void
  PedestalClient::producePlots()
  {
    using namespace std;

    for(map<int, unsigned>::iterator gainItr(gainToME_.begin()); gainItr != gainToME_.end(); ++gainItr){
      static_cast<MESetMulti*>(MEs_[kQuality])->use(gainItr->second);
      static_cast<MESetMulti*>(MEs_[kQualitySummary])->use(gainItr->second);
      static_cast<MESetMulti*>(MEs_[kMean])->use(gainItr->second);
      static_cast<MESetMulti*>(MEs_[kRMS])->use(gainItr->second);

      static_cast<MESetMulti const*>(sources_[kPedestal])->use(gainItr->second);

      MEs_[kMean]->reset();
      MEs_[kRMS]->reset();

      uint32_t mask(0);
      switch(gainItr->first){
      case 1:
        mask |= (1 << EcalDQMStatusHelper::PEDESTAL_LOW_GAIN_MEAN_ERROR |
                 1 << EcalDQMStatusHelper::PEDESTAL_LOW_GAIN_RMS_ERROR);
        break;
      case 6:
        mask |= (1 << EcalDQMStatusHelper::PEDESTAL_MIDDLE_GAIN_MEAN_ERROR |
                 1 << EcalDQMStatusHelper::PEDESTAL_MIDDLE_GAIN_RMS_ERROR);
        break;
      case 12:
        mask |= (1 << EcalDQMStatusHelper::PEDESTAL_HIGH_GAIN_MEAN_ERROR |
                 1 << EcalDQMStatusHelper::PEDESTAL_HIGH_GAIN_RMS_ERROR);
        break;
      default:
        break;
      }

      MESet::iterator qEnd(MEs_[kQuality]->end());
      MESet::const_iterator pItr(sources_[kPedestal]);
      for(MESet::iterator qItr(MEs_[kQuality]->beginChannel()); qItr != qEnd; qItr.toNextChannel()){

        DetId id(qItr->getId());

        bool doMask(applyMask_(kQuality, id, mask));

        pItr = qItr;

        float entries(pItr->getBinEntries());

        if(entries < 1.){
          qItr->setBinContent(doMask ? kMUnknown : kUnknown);
          continue;
        }

        float mean(pItr->getBinContent());
        float rms(pItr->getBinError() * sqrt(entries));

        MEs_[kMean]->fill(id, mean);
        MEs_[kRMS]->fill(id, rms);

        if(abs(mean - expectedMean_[gainItr->second]) > toleranceMean_[gainItr->second] || rms > toleranceRMS_[gainItr->second])
          qItr->setBinContent(doMask ? kMBad : kBad);
        else
          qItr->setBinContent(doMask ? kMGood : kGood);
      }

      towerAverage_(kQualitySummary, kQuality, 0.2);
    }

    for(map<int, unsigned>::iterator gainItr(pnGainToME_.begin()); gainItr != pnGainToME_.end(); ++gainItr){
      static_cast<MESetMulti*>(MEs_[kPNQualitySummary])->use(gainItr->second);
      static_cast<MESetMulti*>(MEs_[kPNRMS])->use(gainItr->second);

      static_cast<MESetMulti const*>(sources_[kPNPedestal])->use(gainItr->second);

      MEs_[kPNRMS]->reset();

      uint32_t mask(0);
      switch(gainItr->first){
      case 1:
        mask |= (1 << EcalDQMStatusHelper::PEDESTAL_LOW_GAIN_MEAN_ERROR |
                 1 << EcalDQMStatusHelper::PEDESTAL_LOW_GAIN_RMS_ERROR);
        break;
      case 16:
        mask |= (1 << EcalDQMStatusHelper::PEDESTAL_HIGH_GAIN_MEAN_ERROR |
                 1 << EcalDQMStatusHelper::PEDESTAL_HIGH_GAIN_RMS_ERROR);
        break;
      default:
        break;
      }

      for(unsigned iDCC(0); iDCC < BinService::nDCC; ++iDCC){

        if(memDCCIndex(iDCC + 1) == unsigned(-1)) continue;

        for(unsigned iPN(0); iPN < 10; ++iPN){
          int subdet(0);
          if(iDCC >= kEBmLow && iDCC <= kEBpHigh) subdet = EcalBarrel;
          else subdet = EcalEndcap;

          EcalPnDiodeDetId id(subdet, iDCC + 1, iPN + 1);

          bool doMask(applyMask_(kPNQualitySummary, id, mask));

          float entries(sources_[kPNPedestal]->getBinEntries(id));

          if(entries < 1.){
            MEs_[kPNQualitySummary]->setBinContent(id, doMask ? kMUnknown : kUnknown);
            continue;
          }

          float mean(sources_[kPNPedestal]->getBinContent(id));
          float rms(sources_[kPNPedestal]->getBinError(id) * sqrt(entries));

          MEs_[kPNRMS]->fill(id, rms);

          if(abs(mean - expectedPNMean_[gainItr->second]) > tolerancePNMean_[gainItr->second] || rms > tolerancePNRMS_[gainItr->second])
            MEs_[kPNQualitySummary]->setBinContent(id, doMask ? kMBad : kBad);
          else
            MEs_[kPNQualitySummary]->setBinContent(id, doMask ? kMGood : kGood);
        }
      }
    }
  }

  void
  PedestalClient::setMEOrdering(std::map<std::string, unsigned>& _nameToIndex)
  {
    _nameToIndex["Quality"] = kQuality;
    _nameToIndex["Mean"] = kMean;
    _nameToIndex["RMS"] = kRMS;
    _nameToIndex["PNRMS"] = kPNRMS;
    _nameToIndex["QualitySummary"] = kQualitySummary;
    _nameToIndex["PNQualitySummary"] = kPNQualitySummary;

    _nameToIndex["Pedestal"] = kPedestal;
    _nameToIndex["PNPedestal"] = kPNPedestal;
  }

  DEFINE_ECALDQM_WORKER(PedestalClient);
}
