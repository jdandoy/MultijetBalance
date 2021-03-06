#!/usr/bin/env python

###################################################################
# calculateFinalMJBGraphs.py                                      #
# A MJB second stage python script                                #
# Author Jeff Dandoy, UChicago                                    #
#                                                                 #
# This script takes any MJBCorrection or DoubleMJBCorrection file #
# of histograms and makes them TGraphErrors using exact x-axis    #
# locations taken from a finely binned recoilPt histogram.        #
#                                                                 #
# This is necessary as the number of events is not flat within    #
# a recoilPt_binned bin. I.E. a single MJB bin can                #
# span from 1700 to 1900 GeV, even if there are few events above  #
# 1800 GeV.                                                       #
#                                                                 #
###################################################################

import os
from array import array
import argparse

from ROOT import *


def calculateFinalMJBGraphs(file):

  if not "MJB_initial" in file and not "MJB_sys_initial":
    print "Error, trying to run calculateMJBHists.py on non \"MJB_(sys_)initial\" input ", file
    print "Exiting"
    return

  inFile = TFile.Open(file, "READ");
  outFile = TFile.Open(file.replace('initial','final'), "RECREATE");

  keyList = [key.GetName() for key in inFile.GetListOfKeys()] #List of top level objects
  dirList = [key for key in keyList if "Iteration" in key] #List of all directories
  for dir in dirList:
    outFile.mkdir( dir )



  histTag = "MJB"
  if "Double" in file:
    histTag = "DoubleMJB"

  ## Get recoilPt hist ##
  nomDir = [dir for dir in dirList if "Nominal" in dir]
  if( not len(nomDir) == 1):
    print "Error, nominal directories are ", nomDir
    return
  else:
    nomDir = inFile.Get( nomDir[0] )

#This gets the x-axis from the original histogram
#!!  tmpMJBHist = nomDir.Get(histTag)

  xBinsList = [300, 360, 420, 480, 540, 600, 660, 720, 780, 840, 900, 960, 1020, 1140, 1260, 1480, 2000]
  xBins = array('d', xBinsList)
  tmpMJBHist = TH1F("tmpHist", "tmpHist", len(xBinsList)-1, xBins)

  ptBinFile = os.path.dirname(file)+'/'
#  if 'mc' in file:
#    print "Looking for ptBinTree_mc15.root in ",os.path.dirname(file)
#    ptBinFile += 'ptBinTree_mc15.root'
#  else:
#    print "Looking for ptBinTree_data15.root in ",os.path.dirname(file)
#    ptBinFile += 'ptBinTree_data15.root'

  ## Only look for data tree
  print "Looking for ptBinTree_data15.root in ",os.path.dirname(file)
  ptBinFile += 'ptBinTree_data15.root'


  if os.path.isfile(ptBinFile):
    ptFile = TFile.Open( ptBinFile , 'READ' )
    ptTree = ptFile.Get('ptBinTree')
#    xValues, xErrors = getXaxisFromTree_recoil(tmpMJBHist, ptTree)
    xValues, xErrors = getXaxisFromTree_leadJet(tmpMJBHist, ptTree)

  else:
    print "No ptBinTree file found, instead looking for recoilPt_center TH1F in ", file
    ptHist = nomDir.Get("recoilPt_center")

    if not ptHist:
      print "Error, no available histogram in the nominal directory ", nomDir.GetName(), " named recoilPt_center"
      exit(1)

    ptHist.Write()
    xValues, xErrors = getXaxisFromHist(tmpMJBHist, ptHist)

  for dir in dirList:
    oldDir = inFile.Get( dir )
    newDir = outFile.Get( dir )
    histList = [key.GetName() for key in oldDir.GetListOfKeys() if histTag in key.GetName()]
    for histName in histList:
      thisHist = oldDir.Get( histName )
      thisHist.SetDirectory(0)
      yValues = []
      yErrors = []
      for iBin in range(1, tmpMJBHist.GetNbinsX()+1):
        thisBin = thisHist.FindBin( tmpMJBHist.GetXaxis().GetBinLowEdge(iBin)+1 )
        yValues.append( thisHist.GetBinContent( thisBin) )
        yErrors.append( thisHist.GetBinError(thisBin) )
      yValues = array('d', yValues)
      yErrors = array('d', yErrors)
      thisGraph = TGraphErrors(tmpMJBHist.GetNbinsX(), xValues, yValues, xErrors, yErrors)
      thisGraph.SetMarkerStyle(28)
      thisGraph.SetMarkerSize(1)
      thisGraph.SetMarkerColor(kRed)
      thisGraph.SetLineColor(kRed)
      thisGraph.SetName( "graph_"+thisHist.GetName() )
      thisGraph.SetTitle( "TGraphErrors "+thisHist.GetTitle() )
      thisGraph.GetXaxis().SetTitle( thisHist.GetXaxis().GetTitle() )
      thisGraph.GetYaxis().SetTitle( thisHist.GetYaxis().GetTitle() )
      newDir.cd()
      thisGraph.Write()



  outFile.Close()
  inFile.Close()

################################################
# For each bin of MJBHist find the mean value  #
# of ptHist in that range                      #
################################################
def getXaxisFromHist(MJBHist, ptHist):
  xValues = []
  xValueErrors = []
  for iBin in range(1, MJBHist.GetNbinsX()+1):
    edgeLow = MJBHist.GetXaxis().GetBinLowEdge(iBin)
    edgeUp = MJBHist.GetXaxis().GetBinUpEdge(iBin)

    ptHist.GetXaxis().SetRangeUser(edgeLow, edgeUp)
    xValues.append( ptHist.GetMean() )
    if( xValues[-1] == 0. ): #If no events, then set to average of bin edges
      xValues[-1] = (edgeUp+edgeLow)/2.

    xValueErrors.append( 0. )

  print "Using xValues", xValues
  xValueArr = array('d', xValues)
  xValueErrorArr = array('d', xValueErrors)

  return xValueArr, xValueErrorArr

def getXaxisFromTree_recoil(MJBHist, ptTree):
  xValues = []
  xValueErrors = []
  ptHist = TH1F("ptHist", "ptHist", 1, 0, 1e10)
  for iBin in range(1, MJBHist.GetNbinsX()+1):
    edgeLow = MJBHist.GetXaxis().GetBinLowEdge(iBin)*1e3
    edgeUp = MJBHist.GetXaxis().GetBinUpEdge(iBin)*1e3

    #binning doesn't matter as GetMean() is unbinned
    ptTree.Draw('recoilPt >> ptHist', 'weight*(recoilPt>='+str(edgeLow)+' && recoilPt<'+str(edgeUp)+')')
    xValues.append( ptHist.GetMean()/1e3 )
    if( xValues[-1] == 0. ): #If no events, then set to average of bin edges
      xValues[-1] = (edgeUp+edgeLow)/2./1e3

    xValueErrors.append( 0. )

  print "Using xValues", xValues
  xValueArr = array('d', xValues)
  xValueErrorArr = array('d', xValueErrors)

  return xValueArr, xValueErrorArr

def getXaxisFromTree_leadJet(MJBHist, ptTree):
  xValues = []
  xValueErrors = []
  ptHist = TH1F("ptHist", "ptHist", 1, 0, 1e7)
  for iBin in range(1, MJBHist.GetNbinsX()+1):
    edgeLow = MJBHist.GetXaxis().GetBinLowEdge(iBin)*1e3
    edgeUp = MJBHist.GetXaxis().GetBinUpEdge(iBin)*1e3

    #binning doesn't matter as GetMean() is unbinned
    ptTree.Draw('leadJetPt >> ptHist', 'weight*(recoilPt>='+str(edgeLow)+' && recoilPt<'+str(edgeUp)+')')
    xValues.append( ptHist.GetMean() )
    if( xValues[-1] == 0. ): #If no events, then set to average of bin edges
      xValues[-1] = (edgeUp+edgeLow)/2./1e3

    xValueErrors.append( 0. )

  print "Using xValues", xValues
  xValueArr = array('d', xValues)
  xValueErrorArr = array('d', xValueErrors)

  return xValueArr, xValueErrorArr

if __name__ == "__main__":
  parser = argparse.ArgumentParser(description="%prog [options]", formatter_class=argparse.ArgumentDefaultsHelpFormatter)
  parser.add_argument("-b", dest='batchMode', action='store_true', default=False, help="Batch mode for PyRoot")
  parser.add_argument("--file", dest='file', default="submitDir/hist-data12_8TeV.root",
           help="Input file name")
  args = parser.parse_args()
  calculateFinalMJBGraphs(args.file)
