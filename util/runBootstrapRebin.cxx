//////////////////////////////////////////////////////////////////
// runBootstrapFit.cxx
//////////////////////////////////////////////////////////////////
// Run fits on histograms made from bootstrap toys.
// Allows rebinning based on RMS of bootstrap toy fits.
//////////////////////////////////////////////////////////////////
// jeff.dandoy@cern.ch
//////////////////////////////////////////////////////////////////

#include <vector>
#include <iostream>
#include <string>
#include <sys/time.h>
#include <sys/stat.h>

#include <TFile.h>
#include <TLatex.h>
#include <TCanvas.h>
#include <TKey.h>
#include <TH1.h>
#include <TH2.h>

#include "JES_ResponseFitter/JES_BalanceFitter.h"

using namespace std;

///// Function for plotting fitted distributions //////////////
int SaveCanvas(int startBin, int endBin, string cName, JES_BalanceFitter* m_BalFit){
  TCanvas c1;// = new TCanvas("c1");
  TLatex lt;// = new TLatex();
  lt.SetTextSize(0.04);
  lt.SetNDC();

  c1.cd();
  TF1* thisFit = (TF1*) m_BalFit->GetFit();
  TH1D* thisHisto = (TH1D*) m_BalFit->GetHisto();
  thisHisto->Draw();
  thisFit->Draw("same");

  float thisMean = 0., thisError = 0., thisRedChi = 0., thisMedian = 0., thisWidth = 0., thisMedianHist = 0.;

  thisMean = m_BalFit->GetMean();
  thisError = m_BalFit->GetMeanError();
  thisMedian = m_BalFit->GetMedian();
  thisRedChi = m_BalFit->GetChi2Ndof();
  thisWidth = m_BalFit->GetSigma();

  if (thisError < 0.5)
    thisMedianHist = m_BalFit->GetHistoMedian();
  else
    thisMedianHist = -99.;

  float ltx = 0.62;
  float lty = 0.80;

  char name[200];

  sprintf(name, "Bins: %i to %i", startBin, endBin);
  lt.DrawLatex(ltx,lty,name);
  lty -= 0.05;

  sprintf(name, "pT: %.0f to %.0f", thisHisto->GetXaxis()->GetBinLowEdge(startBin), thisHisto->GetXaxis()->GetBinUpEdge(endBin));
  lt.DrawLatex(ltx,lty,name);
  lty -= 0.05;

  sprintf(name,"Mean: %.3f", thisMean);
  lt.DrawLatex(ltx,lty,name);
  lty -= 0.05;

  sprintf(name,"Fit Median: %.3f", thisMedian);
  lt.DrawLatex(ltx,lty,name);
  lty -= 0.05;

  sprintf(name,"Hist Median: %.3f", thisMedianHist);
  lt.DrawLatex(ltx,lty,name);
  lty -= 0.05;

  sprintf(name,"Error: %.4f", thisError);
  lt.DrawLatex(ltx,lty,name);
  lty -= 0.05;

  sprintf(name,"RedChi2: %.2f", thisRedChi);
  lt.DrawLatex(ltx,lty,name);
  lty -= 0.05;

  sprintf(name,"Width: %.2f", thisWidth);
  lt.DrawLatex(ltx,lty,name);
  lty -= 0.05;

  c1.Update();
  c1.SaveAs( cName.c_str() );

return 0;
}

int main(int argc, char *argv[])
{
  std::time_t initialTime = std::time(0);
  gErrorIgnoreLevel = 2000;
  std::string inFileName = "";
  float upperEdge = 0;

  /////////// Retrieve arguments //////////////////////////
  std::vector< std::string> options;
  for(int ii=1; ii < argc; ++ii){
    options.push_back( argv[ii] );
  }

  if (argc > 1 && options.at(0).compare("-h") == 0) {
    std::cout << std::endl
         << " runDijetResonance : DijetResonance job submission" << std::endl
         << std::endl
         << " Optional arguments:" << std::endl
         << "  -h                Prints this menu" << std::endl
         << "  --file            Path to a file ending in appended" << std::endl
         << "  --upperEdge       Upper edge for final bin" << std::endl
         << "  --sysType         String tag for which sys to run" << std::endl
         << "  --RMS             Threshold RMS value to determine rebinning" << std::endl
         << "  --fit             Perform fits rather than a mean" << std::endl
         << std::endl;
    exit(1);
  }

  std::string sysType = "";
  float thresholdRMS = 1000.;
  bool f_fit = false;

  int iArg = 0;
  while(iArg < argc-1) {
    if (options.at(iArg).compare("-h") == 0) {
       // Ignore if not first argument
       ++iArg;
    } else if (options.at(iArg).compare("--file") == 0) {
       char tmpChar = options.at(iArg+1)[0];
       if (iArg+1 == argc || tmpChar == '-' ) {
         std::cout << " --file should be followed by a file or folder" << std::endl;
         return 1;
       } else {
         inFileName = options.at(iArg+1);
         iArg += 2;
       }
    } else if (options.at(iArg).compare("--sysType") == 0) {
       char tmpChar = options.at(iArg+1)[0];
       if (iArg+1 == argc || tmpChar == '-' ) {
         std::cout << " --sysType should be followed by a string" << std::endl;
         return 1;
       } else {
         sysType = options.at(iArg+1);
         iArg += 2;
       }
    } else if (options.at(iArg).compare("--upperEdge") == 0) {
       char tmpChar = options.at(iArg+1)[0];
       if (iArg+1 == argc || tmpChar == '-' ) {
         std::cout << " --upperEdge should be followed by a float" << std::endl;
         return 1;
       } else {
         upperEdge = std::stof(options.at(iArg+1));
         iArg += 2;
       }
    } else if (options.at(iArg).compare("--RMS") == 0) {
       char tmpChar = options.at(iArg+1)[0];
       if (iArg+1 == argc || tmpChar == '-' ) {
         std::cout << " --RMS should be followed by a float" << std::endl;
         return 1;
       } else {
         thresholdRMS = std::stof(options.at(iArg+1));
         iArg += 2;
       }
    } else if (options.at(iArg).compare("--fit") == 0) {
      f_fit = true;
      ++iArg;
    }else{
      std::cout << "Couldn't understand argument " << options.at(iArg) << std::endl;
      return 1;
    }
  }//while arguments


  if ( inFileName.size() == 0){
    cout << "No input file given " << endl;
    exit(1);
  }

  std::size_t pos = inFileName.find("scaled");

  if( pos == std::string::npos ){
    cout << "Only runs on scaled files " << endl;
    exit(1);
  }

  /// Get output name ///
  std::string outFileName = inFileName;
  outFileName.replace(pos, 6, "RMS");
  if (sysType.size() > 0)
    outFileName += ("."+sysType);
  cout << "Creating Output File " << outFileName << endl;

  std::string fitPlotsOutDir = outFileName;
  fitPlotsOutDir.erase(fitPlotsOutDir.find_last_of("/"));
  fitPlotsOutDir += "/RMSFits/";
  mkdir(fitPlotsOutDir.c_str(), 0777);

  std::string fitPlotsOutName = outFileName;
  fitPlotsOutName.erase(0, fitPlotsOutName.find_last_of("/"));


  // Get relevant systematics //
  TFile *inFile = TFile::Open(inFileName.c_str(), "READ");
  TIter next(inFile->GetListOfKeys());
  TKey *key;

  std::vector<TH2F*> h_2D_sys, h_2D_nominal;
  while ((key = (TKey*)next() )){
    std::string sysName = key->GetName();
    if( sysName.find(sysType) == std::string::npos)
      continue;

    TH2F* h_recoilPt_PtBal = (TH2F*) inFile->Get((sysName+"/recoilPt_PtBal_Fine").c_str());
    h_2D_sys.push_back( h_recoilPt_PtBal );
    TH2F* h_recoilPt_PtBal_nominal = (TH2F*) inFile->Get(("Nominal/recoilPt_PtBal_Fine").c_str());
    h_2D_nominal.push_back( h_recoilPt_PtBal_nominal );
  }
  cout << "numToys is " << h_2D_sys.size() << " and " << h_2D_nominal.size() << endl;

  // Get Fitting Object
  double NsigmaForFit = 1.6;
  JES_BalanceFitter* m_BalFit = new JES_BalanceFitter(NsigmaForFit);


  //Ignore any bins above upperEdge
  int largestBin = h_2D.at(0)->GetNbinsX();
  while( h_2D_sys.at(0)->GetXaxis()->GetBinLowEdge(largestBin) >= upperEdge){
    largestBin--;
  }

  vector<int> reverseBinEdges; //we start from upper end
  reverseBinEdges.push_back( largestBin );
  vector<float> values_RMS;

  // Loop over all bins //
  for( int iBin=reverseBinEdges.at(reverseBinEdges.size()-1); iBin > 0; --iBin){

    vector<float> meanValues;

    // Loop over all toys //
    for(unsigned int iH = 0; iH < h_2D_sys.size(); ++iH){
      TH1D* h_proj_sys = h_2D_sys.at(iH)->ProjectionY("h_proj_sys", iBin, reverseBinEdges.at(reverseBinEdges.size()-1), "ed" );
      TH1D* h_proj_nominal = h_2D_nominal.at(iH)->ProjectionY("h_proj_nominal", iBin, reverseBinEdges.at(reverseBinEdges.size()-1), "ed" );
      float sysVal = -1., nomVal = -1.;

      if(f_fit){
        m_BalFit->Fit(h_proj_nominal, 0); // Rebin histogram and fit
        nominalVal = m_BalFit->GetMean();
        m_BalFit->Fit(h_proj_sys, 0); // Rebin histogram and fit
        sysVal = m_BalFit->GetMean();
      }else{
        nominalVal = h_proj_nominal->GetMean();
        sysVal = h_proj_sys->GetMean();
      }
      //!! Need to check here if fit failed, and otherwise give the projection?
      meanValues.push_back((sysVal-nominalVal)/sysVal);

      // Draw this fit for the first toy //
      if( f_fit && iH == 0){
        string cName = fitPlotsOutDir+fitPlotsOutName+"_"+to_string(iBin)+"_"+to_string( reverseBinEdges.at(reverseBinEdges.size()-1) )+".png";
        SaveCanvas(iBin, reverseBinEdges.at(reverseBinEdges.size()-1), cName, m_BalFit);
      }
    }

    RMS =  TMath::RMS(meanValues.size(), &meanValues[0]);

    // Get mean value //
    // Should mean value be from actual result, not toys?
    float mean = 0.;
    for(unsigned int iV=0; iV < meanValues.size(); ++iV){
      mean += meanValues.at(iV);
    }
    mean = mean / meanValues.size();

//    // Get RMS //
//    float RMS = 0.;
//    for(unsigned int iV=0; iV < meanValues.size(); ++iV){
//      RMS += pow( meanValues.at(iV) - mean, 2);
//    }
//    RMS = sqrt( RMS / meanValues.size() );

    mu = mean / RMS / RMS;
    sig = 1.0/ RMS / RMS;
    // If RMS is below threshold, then save this bin as an edge. //
    // If above RMS, then this bin will be added with the next bin //
    //if( RMS < thresholdRMS){
    if (sig > 0 && fabs(mu)/sqrt(sig) > thres && sqrt(sig)/fabs(mu) < 1.0/sqrt(10.0)) { // 3 sigma sig + <30% error
      reverseBinEdges.push_back(iBin-1);
      values_RMS.push_back( RMS );
    }


  }//for all bins

  // Create histograms of the RMS values with the final binning //
  int numBins = reverseBinEdges.size()-1;
  Double_t newXbins[numBins];
  for(unsigned int i=0; i < reverseBinEdges.size(); ++i){
    newXbins[numBins-i] = h_2D_sys.at(0)->GetXaxis()->GetBinUpEdge(reverseBinEdges.at(i));
    if (newXbins[numBins-i] > upperEdge)
      newXbins[numBins-i] = upperEdge;
  }
  TH1D* h_RMS = new TH1D( ("RMS_"+sysType).c_str(), ("RMS_"+sysType).c_str(), numBins, newXbins);
  for(int iBin=1; iBin < h_RMS->GetNbinsX()+1; ++iBin){
    h_RMS->SetBinContent(iBin, values_RMS.at(numBins-iBin) );
  }
  TFile *outFile = TFile::Open(outFileName.c_str(), "RECREATE");
  h_RMS->Write("", TObject::kOverwrite);
  outFile->Close();
  inFile->Close();

  std::cout << "Finished Fitting after " << (std::time(0) - initialTime) << " seconds" << std::endl;

  return 0;
}
