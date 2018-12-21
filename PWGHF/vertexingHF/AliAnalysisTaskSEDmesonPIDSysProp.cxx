/* Copyright(c) 1998-2008, ALICE Experiment at CERN, All rights reserved. */

///////////////////////////////////////////////////////////////////////////////////////////////////////////
// \class AliAnalysisTaskSEDmesonPIDSysProp                                                              //
// \brief analysis task for PID Systematic uncertainty propagation from the single track to the D mesons //
// \author: A. M. Barbano, anastasia.maria.barbano@cern.ch                                               //
// \author: F. Grosa, fabrizio.grosa@cern.ch                                                             //
///////////////////////////////////////////////////////////////////////////////////////////////////////////

#include "AliAnalysisTaskSEDmesonPIDSysProp.h"
#include "AliAnalysisTaskSE.h"
#include "AliAnalysisManager.h"
#include "AliAODHandler.h"
#include "AliLog.h"
#include "AliMCEventHandler.h"
#include "AliMCEvent.h"
#include "AliRDHFCutsDplustoKpipi.h"
#include "AliRDHFCutsDstoKKpi.h"
#include "AliRDHFCutsD0toKpi.h"
#include "AliRDHFCutsDStartoKpipi.h"
#include "AliAODMCParticle.h"
#include "AliAODMCHeader.h"
#include "AliAODRecoDecay.h"
#include "AliAODRecoDecayHF3Prong.h"
#include "AliAODRecoDecayHF2Prong.h"
#include "AliAODRecoCascadeHF.h"
#include "AliAnalysisVertexingHF.h"
#include "AliVertexingHFUtils.h"
#include "AliPIDResponse.h"
#include <TFile.h>
#include <TH1F.h>
#include <TH2F.h>
#include <TChain.h>

ClassImp(AliAnalysisTaskSEDmesonPIDSysProp)

//________________________________________________________________________
AliAnalysisTaskSEDmesonPIDSysProp::AliAnalysisTaskSEDmesonPIDSysProp():
AliAnalysisTaskSE("taskPIDSysProp"),
fOutput(nullptr),
fHistNEvents(nullptr),
fHistPtDauVsD(nullptr),
fHistSystPIDEffD(nullptr),
fHistEffPionTOF(nullptr),
fHistEffKaonTOF(nullptr),
fHistSystPionTOF(nullptr),
fHistSystKaonTOF(nullptr),
fPartName(""),
fPIDresp(nullptr),
fSystFileName(""),
fPIDstrategy(kConservativePID),
fnSigma(3.),
fDecayChannel(kD0toKpi),
fKaonTPCHistoOpt(kKaonTOFtag),
fKaonTOFHistoOpt(kSamePionV0tag),
fAODProtection(1),
fNPtBins(0),
fPtLimits(nullptr),
fAnalysisCuts(nullptr)
{
  for(int iHist=0; iHist<2; iHist++) {
    fHistEffPionTPC[iHist]=nullptr;
    fHistEffKaonTPC[iHist]=nullptr;
    fHistSystPionTPC[iHist]=nullptr;
    fHistSystKaonTPC[iHist]=nullptr;
  }

  DefineInput(0, TChain::Class());
  DefineOutput(1, TList::Class());
}

//________________________________________________________________________
AliAnalysisTaskSEDmesonPIDSysProp::AliAnalysisTaskSEDmesonPIDSysProp(int ch, AliRDHFCuts* cuts, TString systfilename):
AliAnalysisTaskSE("taskPIDSysProp"),
fOutput(nullptr),
fHistNEvents(nullptr),
fHistPtDauVsD(nullptr),
fHistSystPIDEffD(nullptr),
fHistEffPionTOF(nullptr),
fHistEffKaonTOF(nullptr),
fHistSystPionTOF(nullptr),
fHistSystKaonTOF(nullptr),
fPartName(""),
fPIDresp(nullptr),
fSystFileName(systfilename),
fPIDstrategy(kConservativePID),
fnSigma(3.),
fDecayChannel(ch),
fKaonTPCHistoOpt(kKaonTOFtag),
fKaonTOFHistoOpt(kSamePionV0tag),
fAODProtection(1),
fNPtBins(0),
fPtLimits(nullptr),
fAnalysisCuts(cuts)
{
  for(int iHist=0; iHist<2; iHist++) {
    fHistEffPionTPC[iHist]=nullptr;
    fHistEffKaonTPC[iHist]=nullptr;
    fHistSystPionTPC[iHist]=nullptr;
    fHistSystKaonTPC[iHist]=nullptr;
  }
  
  DefineInput(0, TChain::Class());
  DefineOutput(1, TList::Class());
}

//___________________________________________________________________________
AliAnalysisTaskSEDmesonPIDSysProp::~AliAnalysisTaskSEDmesonPIDSysProp()
{  
  if(fOutput && !fOutput->IsOwner()){
    delete fHistNEvents;
    for(int iHist=0; iHist<2; iHist++) {
      if(fHistSystPionTPC[iHist]) delete fHistSystPionTPC[iHist];
      if(fHistSystKaonTPC[iHist]) delete fHistSystKaonTPC[iHist];
    }
    delete fHistSystPionTOF;
    delete fHistSystKaonTOF;
    
    delete fHistPtDauVsD;
    delete fHistSystPIDEffD;
    delete fOutput;
  }
  if(fPIDresp) delete fPIDresp;
  if(fAnalysisCuts) delete fAnalysisCuts;
  if(fPtLimits) delete[] fPtLimits; 
  fOutput = nullptr;
}

//________________________________________________________________________
void AliAnalysisTaskSEDmesonPIDSysProp::UserCreateOutputObjects()
{  
  int load = LoadEffSystFile();
  if(load>0) AliFatal("Impossible to load single track systematic file, check if it is correct! Exit.");

  fOutput = new TList();
  fOutput->SetOwner();
  fOutput->SetName("OutputHistos");
  
  fHistNEvents = new TH1F("hNEvents", "number of events ",15,-0.5,13.5);
  fHistNEvents->GetXaxis()->SetBinLabel(1,"nEventsRead");
  fHistNEvents->GetXaxis()->SetBinLabel(2,"nEvents Matched dAOD");
  fHistNEvents->GetXaxis()->SetBinLabel(3,"nEvents Mismatched dAOD");
  fHistNEvents->GetXaxis()->SetBinLabel(4,"nEventsAnal");
  fHistNEvents->GetXaxis()->SetBinLabel(5,"n. passing IsEvSelected");
  fHistNEvents->GetXaxis()->SetBinLabel(6,"n. rejected due to trigger");
  fHistNEvents->GetXaxis()->SetBinLabel(7,"n. rejected due to not reco vertex");
  fHistNEvents->GetXaxis()->SetBinLabel(8,"n. rejected for contr vertex");
  fHistNEvents->GetXaxis()->SetBinLabel(9,"n. rejected for vertex out of accept");
  fHistNEvents->GetXaxis()->SetBinLabel(10,"n. rejected for pileup events");
  fHistNEvents->GetXaxis()->SetBinLabel(11,"no. of out centrality events");
  fHistNEvents->GetXaxis()->SetBinLabel(12,"no. of D candidates");
  fHistNEvents->GetXaxis()->SetBinLabel(13,"no. of D after PID cuts");
  fHistNEvents->GetXaxis()->SetBinLabel(14,"no. of not on-the-fly rec D");
  
  fHistNEvents->GetXaxis()->SetNdivisions(1,false);
  
  fHistNEvents->Sumw2();
  fHistNEvents->SetMinimum(0);
  fOutput->Add(fHistNEvents);
  
  fNPtBins = fAnalysisCuts->GetNPtBins();
  float* ptlims = fAnalysisCuts->GetPtBinLimits();
  fPtLimits = new double[fNPtBins+1];
  for(int iPt=0; iPt<=fNPtBins; iPt++) {
    fPtLimits[iPt] = static_cast<double>(ptlims[iPt]);
  }
  if(fPtLimits[fNPtBins]>100) fPtLimits[fNPtBins] = 100.;
  
  fHistSystPIDEffD = new TH2F("fHistSystPIDEffD","PID efficiency systematic uncertainty; #it{p}_{T}^{D} (GeV/#it{c}); relative systematic uncertainty",fNPtBins,fPtLimits,150,0.,0.15);
  fHistPtDauVsD = new TH2F("fHistPtDauVsD","#it{p}_{T} Dau vs #it{p}_{T} D; #it{p}_{T}^{D} (GeV/#it{c}); #it{p}_{T}^{daugh} (GeV/#it{c})",static_cast<int>(fPtLimits[fNPtBins] * 10),0.,fPtLimits[fNPtBins],static_cast<int>(fPtLimits[fNPtBins] * 10),0.,fPtLimits[fNPtBins]);
  fOutput->Add(fHistSystPIDEffD);
  fOutput->Add(fHistPtDauVsD);
    
  PostData(1,fOutput);
}

//________________________________________________________________________
void AliAnalysisTaskSEDmesonPIDSysProp::Init()
{    
  if(fDecayChannel == kDplustoKpipi) {
    fAnalysisCuts = new AliRDHFCutsDplustoKpipi(*(dynamic_cast<AliRDHFCutsDplustoKpipi*>(fAnalysisCuts)));
  }
  else if(fDecayChannel == kDstoKKpi) {
    fAnalysisCuts = new AliRDHFCutsDstoKKpi(*(dynamic_cast<AliRDHFCutsDstoKKpi*>(fAnalysisCuts)));
  }
  else if(fDecayChannel == kD0toKpi) {
    fAnalysisCuts = new AliRDHFCutsD0toKpi(*(dynamic_cast<AliRDHFCutsD0toKpi*>(fAnalysisCuts)));
  }
  else if(fDecayChannel == kDstartoKpipi) {
    fAnalysisCuts = new AliRDHFCutsDStartoKpipi(*(dynamic_cast<AliRDHFCutsDStartoKpipi*>(fAnalysisCuts)));
  }
  else {
    AliFatal("The decay channel MUST be defined according to AliCFVertexing::DecayChannel - Exit.");
  }
}

//________________________________________________________________________
void AliAnalysisTaskSEDmesonPIDSysProp::UserExec(Option_t *)
{    
  AliAODEvent* aod = dynamic_cast<AliAODEvent*>(fInputEvent);
  fHistNEvents->Fill(0); // all events
  
  if(fAODProtection>=0){
    //   Protection against different number of events in the AOD and deltaAOD
    //   In case of discrepancy the event is rejected.
    int matchingAODdeltaAODlevel = AliRDHFCuts::CheckMatchingAODdeltaAODevents();
    if (matchingAODdeltaAODlevel<0 || (matchingAODdeltaAODlevel==0 && fAODProtection==1)) {
      // AOD/deltaAOD trees have different number of entries || TProcessID do not match while it was required
      fHistNEvents->Fill(2);
      PostData(1,fOutput);
      return;
    }
  }
  TClonesArray *arrayBranch=0;
  
  if(!aod && AODEvent() && IsStandardAOD()) {
    // In case there is an AOD handler writing a standard AOD, use the AOD
    // event in memory rather than the input (ESD) event.
    aod = dynamic_cast<AliAODEvent*> (AODEvent());
    // in this case the braches in the deltaAOD (AliAOD.VertexingHF.root)
    // have to taken from the AOD event hold by the AliAODExtension
    AliAODHandler* aodHandler = (AliAODHandler*)
    ((AliAnalysisManager::GetAnalysisManager())->GetOutputEventHandler());
    if(aodHandler->GetExtensions()) {
      AliAODExtension *ext = (AliAODExtension*)aodHandler->GetExtensions()->FindObject("AliAOD.VertexingHF.root");
      AliAODEvent *aodFromExt = ext->GetAOD();
      if(fDecayChannel == kDplustoKpipi || fDecayChannel == kDstoKKpi)
      arrayBranch=(TClonesArray*)aodFromExt->GetList()->FindObject("Charm3Prong");
      else if(fDecayChannel == kD0toKpi)
      arrayBranch=(TClonesArray*)aodFromExt->GetList()->FindObject("D0toKpi");
      else if(fDecayChannel == kDstartoKpipi)
      arrayBranch=(TClonesArray*)aodFromExt->GetList()->FindObject("Dstar");
    }
  }
  else {
    if(fDecayChannel == kDplustoKpipi || fDecayChannel == kDstoKKpi)
    arrayBranch=(TClonesArray*)aod->GetList()->FindObject("Charm3Prong");
    else if(fDecayChannel == kD0toKpi)
    arrayBranch=(TClonesArray*)aod->GetList()->FindObject("D0toKpi");
    else if(fDecayChannel == kDstartoKpipi)
    arrayBranch=(TClonesArray*)aod->GetList()->FindObject("Dstar");
  }
  
  if (!arrayBranch) {
    AliError("Could not find array of HF vertices");
    PostData(1,fOutput);
    return;
  }
  
  if (!fPIDresp) fPIDresp = ((AliInputEventHandler*)(AliAnalysisManager::GetAnalysisManager()->GetInputEventHandler()))->GetPIDResponse();
  
  AliAODVertex *aodVtx = (AliAODVertex*)aod->GetPrimaryVertex();
  if (!aodVtx || TMath::Abs(aod->GetMagneticField())<0.001) {
    AliDebug(3, "The event was skipped due to missing vertex or magnetic field issue");
    PostData(1,fOutput);
    return;
  }
  fHistNEvents->Fill(3); // count event
  
  bool isEvSel  = fAnalysisCuts->IsEventSelected(aod);
  float ntracks = aod->GetNumberOfTracks();
  
  if(fAnalysisCuts->IsEventRejectedDueToTrigger()) fHistNEvents->Fill(5);
  if(fAnalysisCuts->IsEventRejectedDueToNotRecoVertex()) fHistNEvents->Fill(6);
  if(fAnalysisCuts->IsEventRejectedDueToVertexContributors()) fHistNEvents->Fill(7);
  if(fAnalysisCuts->IsEventRejectedDueToZVertexOutsideFiducialRegion()) fHistNEvents->Fill(8);
  if(fAnalysisCuts->IsEventRejectedDueToPileup()) fHistNEvents->Fill(9);
  if(fAnalysisCuts->IsEventRejectedDueToCentrality()) fHistNEvents->Fill(10);
  
  int runNumber = aod->GetRunNumber();
  
  TClonesArray *arrayMC    =  (TClonesArray*)aod->GetList()->FindObject(AliAODMCParticle::StdBranchName());
  AliAODMCHeader *mcHeader =  (AliAODMCHeader*)aod->GetList()->FindObject(AliAODMCHeader::StdBranchName());
  
  if(!arrayMC) {
    AliError("AliAnalysisTaskSEDmesonPIDSysProp::UserExec: MC particles branch not found!\n");
    PostData(1,fOutput);
    return;
  }
  if(!mcHeader) {
    AliError("AliAnalysisTaskSEDmesonPIDSysProp::UserExec: MC header branch not found!\n");
    PostData(1,fOutput);
    return;
  }
  
  if(aod->GetTriggerMask()==0 && (runNumber>=195344 && runNumber<=195677)) {
    // protection for events with empty trigger mask in p-Pb (Run1)
    PostData(1,fOutput);
    return;
  }
  if(fAnalysisCuts->GetUseCentrality()>0 && fAnalysisCuts->IsEventSelectedInCentrality(aod)!=0) {
    // events not passing the centrality selection can be removed immediately.
    PostData(1,fOutput);
    return;
  }
  double zMCVertex = mcHeader->GetVtxZ();
  if (TMath::Abs(zMCVertex) > fAnalysisCuts->GetMaxVtxZ()) {
    PostData(1,fOutput);
    return;
  }
  if(!isEvSel){
    PostData(1,fOutput);
    return;
  }
  fHistNEvents->Fill(4);
  
  int nCand = arrayBranch->GetEntriesFast();
  
  int nprongs = -1;
  int pdgcode = -1;
  int pdgDaughter[3];
  int pdg2Daughter[2];
  if(fDecayChannel == kDplustoKpipi){
    pdgcode = 411;
    nprongs  = 3;
    fPartName="Dplus";
    pdgDaughter[0]=321;
    pdgDaughter[1]=211;
    pdgDaughter[2]=211;
  }else if(fDecayChannel == kDstoKKpi){
    pdgcode = 431;
    nprongs  = 3;
    fPartName="Ds";
    pdgDaughter[0]=321;
    pdgDaughter[1]=321;
    pdgDaughter[2]=211;
  }else if(fDecayChannel == kD0toKpi){
    pdgcode = 421;
    nprongs  = 2;
    fPartName="D0";
    pdgDaughter[0]=321;
    pdgDaughter[1]=211;
  }else if(fDecayChannel == kDstartoKpipi){
    pdgcode = 413;
    nprongs  = 2;
    fPartName="Dstar";
    pdgDaughter[0]=421;
    pdgDaughter[1]=211;
    pdg2Daughter[0]=321;
    pdg2Daughter[1]=211;
  }else{
    AliError("Wrong decay setting");
    PostData(1,fOutput);
    return;
  }
  
  // vHF object is needed to call the method that refills the missing info of the candidates
  // if they have been deleted in dAOD reconstruction phase
  // in order to reduce the size of the file
  AliAnalysisVertexingHF *vHF=new AliAnalysisVertexingHF();
  
  for (int iCand = 0; iCand < nCand; iCand++) {
    
    AliAODRecoDecayHF* d = nullptr;
    bool isDStarCand = false;

    if(fDecayChannel == kDplustoKpipi || fDecayChannel == kDstoKKpi) {
      d = dynamic_cast<AliAODRecoDecayHF3Prong*>(arrayBranch->UncheckedAt(iCand));
      if(!vHF->FillRecoCand(aod,dynamic_cast<AliAODRecoDecayHF3Prong*>(d))) {
        fHistNEvents->Fill(13);
        continue;
      }
    }
    else if(fDecayChannel == kD0toKpi) {
      d = dynamic_cast<AliAODRecoDecayHF2Prong*>(arrayBranch->UncheckedAt(iCand));
      if(!vHF->FillRecoCand(aod,dynamic_cast<AliAODRecoDecayHF2Prong*>(d))) {
        fHistNEvents->Fill(13);
        continue;
      }
    }
    else if(fDecayChannel == kDstartoKpipi) {
      d = dynamic_cast<AliAODRecoCascadeHF*>(arrayBranch->UncheckedAt(iCand));
      if(!d) continue;
      isDStarCand = true;
      if(!vHF->FillRecoCasc(aod,dynamic_cast<AliAODRecoCascadeHF*>(d),isDStarCand)) {
        fHistNEvents->Fill(13);
        continue;
      }
      if(!d->GetSecondaryVtx()) continue;
    }
    
    fHistNEvents->Fill(11);
    bool unsetvtx=false;
    if(!d->GetOwnPrimaryVtx()){
      d->SetOwnPrimaryVtx(aodVtx);
      unsetvtx=true;
    }
    
    bool recVtx=false;
    AliAODVertex *origownvtx = nullptr;
    
    double ptD = d->Pt();
    double rapid  = d->Y(pdgcode);
    bool isFidAcc = fAnalysisCuts->IsInFiducialAcceptance(ptD,rapid);
    
    if(isFidAcc){
      int retCodeAnalysisCuts = fAnalysisCuts->IsSelectedPID(d);
      if(retCodeAnalysisCuts==0) continue;
      fHistNEvents->Fill(12);

      int mcLabel=-1;
      int orig = 0;
      if(!isDStarCand) mcLabel = d->MatchToMC(pdgcode,arrayMC,nprongs,pdgDaughter);
      else mcLabel = (dynamic_cast<AliAODRecoCascadeHF*>(d))->MatchToMC(pdgcode,421,pdgDaughter,pdg2Daughter,arrayMC);
      if(mcLabel<0) continue;
      AliAODMCParticle* partD = dynamic_cast<AliAODMCParticle*>(arrayMC->At(mcLabel));
      if(partD) orig = AliVertexingHFUtils::CheckOrigin(arrayMC,partD,kTRUE);
      if(orig<4) continue;
      
      const int nDau = d->GetNDaughters();
      AliAODTrack* dautrack[nDau];
      if(fDecayChannel != kDstartoKpipi) {     
        for(int iDau=0; iDau<nDau; iDau++)
          dautrack[iDau] = dynamic_cast<AliAODTrack*>(d->GetDaughter(iDau));
      }
      else {
        AliAODRecoDecayHF2Prong* D0prong = dynamic_cast<AliAODRecoDecayHF2Prong*>((dynamic_cast<AliAODRecoCascadeHF*>(d))->Get2Prong());
        if(!D0prong) {
          continue;
        }
        for(int iDau=0; iDau<nDau; iDau++) {
          dautrack[iDau] = dynamic_cast<AliAODTrack*>(D0prong->GetDaughter(iDau));
        }
      }

      double syst = GetDmesonPIDuncertainty(dautrack,nDau,arrayMC,ptD);
      if(syst==-999. || syst==0.) continue;
      fHistSystPIDEffD->Fill(ptD,syst);
    }
    if(unsetvtx) d->UnsetOwnPrimaryVtx();
  }

  PostData(1,fOutput);
  return;
}

//________________________________________________________________________
int AliAnalysisTaskSEDmesonPIDSysProp::LoadEffSystFile()
{  
  TFile* infile = TFile::Open(fSystFileName.Data());
  if(!infile) return 1;
  
  if(fPIDstrategy==kConservativePID || fPIDstrategy==kStrongPID) {
    fHistEffPionTPC[0] = (TH1F*)infile->Get("hEffPionTPCDataV0tag_3sigma");
    fHistSystPionTPC[0] = (TH1F*)infile->Get("hRatioEffPionTPCDataV0tag_3sigma");
    if(!fHistSystPionTPC[0] || !fHistEffPionTPC[0]) return 2;
    if(fKaonTPCHistoOpt==kKaonTOFtag) {
      fHistEffKaonTPC[0] = (TH1F*)infile->Get("hEffKaonTPCDataTOFtag_3sigma");
      fHistSystKaonTPC[0] = (TH1F*)infile->Get("hRatioEffKaonTPCDataTOFtag_3sigma");
    }
    else if(fKaonTPCHistoOpt==kKaonKinkstag) {
      fHistEffKaonTPC[0] = (TH1F*)infile->Get("hEffKaonTPCDataKinktag_3sigma");
      fHistSystKaonTPC[0] = (TH1F*)infile->Get("hRatioEffKaonTPCDataKinktag_3sigma");
    }
    if(!fHistSystKaonTPC[0] || !fHistEffKaonTPC[0]) return 3;
    fHistEffPionTOF = (TH1F*)infile->Get("hEffPionTOFDataV0tag_3sigma");
    fHistSystPionTOF = (TH1F*)infile->Get("hRatioEffPionTOFDataV0tag_3sigma");
    if(!fHistSystPionTOF || !fHistEffPionTOF) return 4;
    if(fKaonTOFHistoOpt==kKaonTPCtag) {
      fHistEffKaonTOF = (TH1F*)infile->Get("hEffKaonTOFDataTPCtag_3sigma");
      fHistSystKaonTOF = (TH1F*)infile->Get("hRatioEffKaonTOFDataTPCtag_3sigma");
    }
    else if(fKaonTOFHistoOpt==kSamePionV0tag) {
      fHistEffKaonTOF = (TH1F*)infile->Get("hEffPionTOFDataV0tag_3sigma");
      fHistSystKaonTOF = (TH1F*)infile->Get("hRatioEffPionTOFDataV0tag_3sigma");
    }
    if(!fHistSystKaonTOF || !fHistEffKaonTOF) return 5;
    if(fPIDstrategy==kStrongPID) {
      fHistEffPionTPC[1] = (TH1F*)infile->Get("hEffPionTPCDataV0tag_2sigma");
      fHistSystPionTPC[1] = (TH1F*)infile->Get("hRatioEffPionTPCDataV0tag_2sigma");
      if(!fHistSystPionTPC[1] || !fHistEffPionTPC[1]) return 6;
      if(fKaonTPCHistoOpt==kKaonTOFtag) {
        fHistEffKaonTPC[1] = (TH1F*)infile->Get("hEffKaonTPCDataTOFtag_2sigma");
        fHistSystKaonTPC[1] = (TH1F*)infile->Get("hRatioEffKaonTPCDataTOFtag_2sigma");
      }
      else if(fKaonTPCHistoOpt==kKaonKinkstag) {
        fHistEffKaonTPC[1] = (TH1F*)infile->Get("hEffKaonTPCDataKinktag_2sigma");
        fHistSystKaonTPC[1] = (TH1F*)infile->Get("hRatioEffKaonTPCDataKinktag_2sigma");
      }
      if(!fHistSystKaonTPC[1] || !fHistEffKaonTPC[1]) return 7;
    }
  }
  else if(fPIDstrategy==knSigmaPID) {
    fHistEffPionTPC[0] = (TH1F*)infile->Get(Form("hEffPionTPCDataV0tag_%0.fsigma",fnSigma));
    fHistSystPionTPC[0] = (TH1F*)infile->Get(Form("hRatioEffPionTPCDataV0tag_%0.fsigma",fnSigma));
    if(!fHistSystPionTPC[0] || !fHistEffPionTPC[0]) return 8;
    if(fKaonTPCHistoOpt==kKaonTOFtag) {
      fHistEffKaonTPC[0] = (TH1F*)infile->Get(Form("hEffKaonTPCDataTOFtag_%0.fsigma",fnSigma));
      fHistSystKaonTPC[0] = (TH1F*)infile->Get(Form("hRatioEffKaonTPCDataTOFtag_%0.fsigma",fnSigma));
    }
    else if(fKaonTPCHistoOpt==kKaonKinkstag) {
      fHistEffKaonTPC[0] = (TH1F*)infile->Get(Form("hEffKaonTPCDataKinktag_%0.fsigma",fnSigma));
      fHistSystKaonTPC[0] = (TH1F*)infile->Get(Form("hRatioEffKaonTPCDataKinktag_%0.fsigma",fnSigma));
    }
    if(!fHistSystKaonTPC[0] || !fHistEffKaonTPC[0]) return 9;
    fHistEffPionTOF = (TH1F*)infile->Get(Form("hEffPionTOFDataV0tag_%0.fsigma",fnSigma));
    fHistSystPionTOF = (TH1F*)infile->Get(Form("hRatioEffPionTOFDataV0tag_%0.fsigma",fnSigma));
    if(!fHistSystPionTOF || !fHistEffPionTOF) return 10;
    if(fKaonTOFHistoOpt==kKaonTPCtag) {
      fHistEffKaonTOF = (TH1F*)infile->Get(Form("hEffKaonTOFDataTPCtag_%0.fsigma",fnSigma));
      fHistSystKaonTOF = (TH1F*)infile->Get(Form("hRatioEffKaonTOFDataTPCtag_%0.fsigma",fnSigma));
    }
    else if(fKaonTOFHistoOpt==kSamePionV0tag) {
      fHistEffKaonTOF = (TH1F*)infile->Get(Form("hEffPionTOFDataV0tag_%0.fsigma",fnSigma));
      fHistSystKaonTOF = (TH1F*)infile->Get(Form("hRatioEffPionTOFDataV0tag_%0.fsigma",fnSigma));
    }
    fHistEffKaonTOF = (TH1F*)infile->Get(Form("hEffKaonTOFDataTPCtag_%0.fsigma",fnSigma));
    fHistSystKaonTOF = (TH1F*)infile->Get(Form("hRatioEffKaonTOFDataTPCtag_%0.fsigma",fnSigma));
    if(!fHistSystKaonTOF || !fHistEffKaonTOF) return 11;
  }
  
  return 0;
}

//________________________________________________________________________
double AliAnalysisTaskSEDmesonPIDSysProp::GetDmesonPIDuncertainty(AliAODTrack *track[], const int nDau, TClonesArray *arrayMC, double ptD)
{
  double syst = 0.;

  for(int iDau=0; iDau<nDau; iDau++){
    if(!track[iDau]){
      AliWarning("Daughter-particle track not found");
      return -999.;
    }
    int labDau = track[iDau]->GetLabel();
    if(labDau<0) {
      AliWarning("Daughter particle not found");
      return -999.;
    }
    AliAODMCParticle* p = dynamic_cast<AliAODMCParticle*>(arrayMC->UncheckedAt(TMath::Abs(labDau)));
    if(!p) {
      AliWarning("Daughter particle not found");
      return -999.;
    }
    
    int daupdgcode = TMath::Abs(p->GetPdgCode());
    double daupt = track[iDau]->Pt();

    bool isTPCok = false;
    bool isTOFok = false;
    if(fPIDresp->CheckPIDStatus(AliPIDResponse::kTPC,track[iDau]) == AliPIDResponse::kDetPidOk) isTPCok = true;
    if(fPIDresp->CheckPIDStatus(AliPIDResponse::kTOF,track[iDau]) == AliPIDResponse::kDetPidOk) isTOFok = true;

    int bin = fHistSystPionTPC[0]->GetXaxis()->FindBin(daupt);
    double systTPC=0.;
    double systTOF=0.;
    double probTPC=0.;
    double probTOF=0.;
    double probTPCandTOF=0.;
    double probTPCorTOF=0.;

    if(fPIDstrategy==kConservativePID) {
      if(daupdgcode==211) {
        if(isTPCok) GetSingleTrackSystAndProb(fHistSystPionTPC[0],fHistEffPionTPC[0],bin,systTPC,probTPC);
        if(isTOFok) GetSingleTrackSystAndProb(fHistSystPionTOF,fHistEffPionTOF,bin,systTOF,probTOF);
      }
      else if(daupdgcode==321) {
        if(isTPCok) GetSingleTrackSystAndProb(fHistSystKaonTPC[0],fHistEffKaonTPC[0],bin,systTPC,probTPC);
        if(isTOFok) GetSingleTrackSystAndProb(fHistSystKaonTOF,fHistEffKaonTOF,bin,systTOF,probTOF);
      }
      probTPCorTOF = probTPC+probTOF-probTPC*probTOF;
      if(probTPCorTOF>1.e-20) syst += TMath::Sqrt((1-probTPC)*(1-probTPC)*systTOF*systTOF+(1-probTOF)*(1-probTOF)*systTPC*systTPC)/probTPCorTOF;
    }
    else if(fPIDstrategy==kStrongPID) {
      if(daupdgcode==211) {
        if(isTOFok) {
          if(isTPCok) GetSingleTrackSystAndProb(fHistSystPionTPC[0],fHistEffPionTPC[0],bin,systTPC,probTPC);
          GetSingleTrackSystAndProb(fHistSystPionTOF,fHistEffPionTOF,bin,systTOF,probTOF);
        }
        else if(isTPCok && !isTOFok) {//2sisgma cut in case of no TOF
          GetSingleTrackSystAndProb(fHistSystPionTPC[1],fHistEffPionTPC[1],bin,systTPC,probTPC);
        }
      }
      else if(daupdgcode==321) {
        if(isTOFok) {
          if(isTPCok) GetSingleTrackSystAndProb(fHistSystKaonTPC[0],fHistEffKaonTPC[0],bin,systTPC,probTPC);
          GetSingleTrackSystAndProb(fHistSystKaonTOF,fHistEffKaonTOF,bin,systTOF,probTOF);
        }
        else if(isTPCok && !isTOFok) {//2sisgma cut in case of no TOF
          GetSingleTrackSystAndProb(fHistSystKaonTPC[1],fHistEffKaonTPC[1],bin,systTPC,probTPC);
        }
      }
      probTPCorTOF = probTPC+probTOF-probTPC*probTOF;
      if(probTPCorTOF>1.e-20) syst += TMath::Sqrt((1-probTPC)*(1-probTPC)*systTOF*systTOF+(1-probTOF)*(1-probTOF)*systTPC*systTPC)/probTPCorTOF;
    }
    else if(fPIDstrategy==knSigmaPID) {
      if(daupdgcode==211) {
        if(isTPCok && isTOFok) {
          GetSingleTrackSystAndProb(fHistSystPionTPC[0],fHistEffPionTPC[0],bin,systTPC,probTPC);
          GetSingleTrackSystAndProb(fHistSystPionTOF,fHistEffPionTOF,bin,systTOF,probTOF);
        }
      }
      else if(daupdgcode==321) {
        if(isTPCok && isTOFok) {
          GetSingleTrackSystAndProb(fHistSystKaonTPC[0],fHistEffKaonTPC[0],bin,systTPC,probTPC);
          GetSingleTrackSystAndProb(fHistSystKaonTOF,fHistEffKaonTOF,bin,systTOF,probTOF);
        }
      }
      probTPCandTOF = probTPC*probTOF;
      if(probTPCandTOF>1.e-20) syst += TMath::Sqrt(probTPC*probTPC*systTOF*systTOF+probTOF*probTOF*systTPC*systTPC)/probTPCandTOF;
    }
    fHistPtDauVsD->Fill(ptD,daupt);
  }
  
  return TMath::Abs(syst);
}

//________________________________________________________________________
void AliAnalysisTaskSEDmesonPIDSysProp::GetSingleTrackSystAndProb(TH1F* hSingleTrackSyst, TH1F* hSingleTrackEff, int bin, double &syst, double &prob)
{
  int binmax = hSingleTrackSyst->GetNbinsX();
  int binmin = 1;
  if(bin<binmax && bin>binmin) {
    syst = TMath::Abs(1-hSingleTrackSyst->GetBinContent(bin))*hSingleTrackEff->GetBinContent(bin); //absolute syst
    prob = hSingleTrackEff->GetBinContent(bin);
  }
  else if(bin>=binmax){
    syst = TMath::Abs(1-hSingleTrackSyst->GetBinContent(binmax))*hSingleTrackEff->GetBinContent(binmax); //absolute syst
    prob = hSingleTrackEff->GetBinContent(binmax);
  }
  else if(bin<=binmin){
    syst = TMath::Abs(1-hSingleTrackSyst->GetBinContent(binmax))*hSingleTrackEff->GetBinContent(binmin); //absolute syst
    prob = hSingleTrackEff->GetBinContent(binmin);
  }
}
