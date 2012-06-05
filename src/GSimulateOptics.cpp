/*
VERSION2.1
1MARCH2012
*/
/*!  GTelArray.cpp
     Charlie Duke
     Grinnell College
 */

#include <iostream>
#include <cstdlib>
#include <fstream>
#include <string>
#include <sstream>
#include <vector>
#include <cmath>
#include <map>
#include <deque>
#include <list>
#include <iterator>
#include <algorithm>
#include <bitset>
#include <iomanip>

using namespace std;

#include "TROOT.h"
#include "TFile.h"
#include "TTree.h"
#include "TMatrixD.h"
#include "TMath.h"
#include "Math/Vector3D.h"
#include "Math/RotationX.h"
#include "Math/RotationY.h"
#include "Math/RotationZ.h"
#include "Math/Rotation3D.h"

#include "GDefinition.h"

#include "GUtilityFuncts.h"
#include "GTelescope.h"
#include "GDCTelescope.h"
#include "GSCTelescope.h"
#include "GTelescopeFactory.h"
#include "GDCTelescopeFactory.h"
#include "GReadDCStdBase.h"
#include "GReadDCStdGrISU.h"
#include "GReadPhotonBase.h"
#include "GReadPhotonGrISU.h"
#include "GArrayTel.h"
#include "GSimulateOptics.h"
#include "GRootWriter.h"

#define DEBUG(x) *oLog << #x << " = " << x << endl
#define DEBUGS(x) *oLog << "       "<< #x << " = " << x << endl

GSimulateOptics::GSimulateOptics() {
  reader    = 0;
  arrayTel  = 0;
  mArrayTel = 0;
};
/************** end of GSimulateOptics ******************/

GSimulateOptics::GSimulateOptics(GReadPhotonBase *read,
				 map<int, GArrayTel *> *mATel,
				 map<int,GRootWriter *> *mRWriter,
				 const string &headerTree)  
  :reader(read),mArrayTel(mATel),mRootWriter(mRWriter),
   sHeaderTree(headerTree) {

  iterRootWriter = mRootWriter->begin();
  rootWriter = iterRootWriter->second;
  vTelDcosGrd = 0;
  rotCoorGrdToSky = 0;
  rotGrdToTel = 0;

  sFileHeader  = "";
  fObsHgt      = 0.0;
  fGlobalEffic = 0.0;
  fWobbleE     = 0.0;
  fWobbleN     = 0.0;
  fWobbleR     = 0.0;
  fLatitude    = 0.0;
  fWobbleTE    = 0.0;
  fWobbleTN    = 0.0;
  fZnPrim      = 0.0;
  fAzPrim      = 0.0;
  fZnTel       = 0.0;
  fAzTel       = 0.0;
  iNShowers    = -1;
  iNPhotons    = -1;

  fEventNumber = 0;

  sFileHeader  = reader->getHeader();
  fObsHgt      = reader->getObsHeight();
  fGlobalEffic = reader->getGlobalEffic();

  fillAllTelTree();
};
  /************** end of GSimulateOptics ******************/

GSimulateOptics::~GSimulateOptics() {

  bool debug=false;
  if (debug) {
    *oLog << "  -- GSimulateOptics::~GSimulateOptics " << endl;
  }
  if (vTelDcosGrd) SafeDelete(vTelDcosGrd);
  if (rotCoorGrdToSky) SafeDelete(rotCoorGrdToSky);
  if (rotGrdToTel) SafeDelete(rotGrdToTel);

};
/************** end of ~GSimulateOptics******************/

bool GSimulateOptics::startSimulations(const int &numShowers,
                                         const int &numPhotons) {

  bool debug = false;
  if (debug) {
    *oLog << "  -- GSimulateOptics::startSimulations " << endl;
  }

  string sFileHeader = reader->getHeader();

  iNShowers = numShowers;
  iNPhotons = numPhotons;
  
  // set up shower loop, if iNShowers < 0 no limit
  int numShTmp = iNShowers;
  if (iNShowers < 0) numShTmp = 1;

  // set up photon loop, if iNPhotons < 0 no limit
  int numPhTmp = iNPhotons;
  if (iNPhotons < 0) numPhTmp = 1;
  
  primaryFlag = false;
  
  // Start of shower loop /////////////////////////////////
  for (int i=0; i<numShTmp; ++i) {
    if (iNShowers < 0) ++numShTmp;   // for no limit on showers, keep ahead of i
    
    // get the primary details from the reader
    primaryFlag = reader->getPrimary(&vSCore,&vSDcosGd,
                                     &fAzPrim,&fZnPrim,
				     &fPrimaryEnergy,&fPrimaryType);
    if (!primaryFlag) break;  // no more primaries available

    fDelay = 0;
    fPhotonToCameraTime = 0;
   
    fEventNumber++;

    // get the wobble offset
    makeWobbleOffset();

    if (debug) printDebugPrimary();
 
    // set primary in all ArrayTel elements
    for (iterArrayTel=mArrayTel->begin();
         iterArrayTel!=mArrayTel->end();
         iterArrayTel++) {
      iterArrayTel->second->setPrimary(vSCore,vSDcosGd,fAzPrim,fZnPrim,
                                       fEnergy,fWobbleTN,fWobbleTE,fLatitude);
    }
            
    photonFlag = false;
    int nPhotons = 0;
    
    for (int j = 0;j<numPhTmp;++j) {
      if (iNPhotons < 0) ++numPhTmp;
      
      // get photon from the reader
      photonFlag = reader->getPhoton(&vPhotonGrdLoc,&vPhotonDCosGd,
                                     &fAzPhot,&fZnPhot,
                                     &fPhotHgtEmiss,&fPhotGrdTime,
                                     &fPhotWaveLgt,&iPhotType,
                                     &iPhotTelHitNum);

      if (!photonFlag) break;  // no more photons available

      if (debug) {
        printDebugPhoton();
      }
         
      // check for active telescope number, could be subarray
      // skip if telescope not included in the array
       map<int, GArrayTel *>::iterator iterAT;
       iterAT = mArrayTel->find(iPhotTelHitNum);

       if (iterAT != mArrayTel->end() ) {
	 nPhotons++;

	 (*mArrayTel)[iPhotTelHitNum]->setPhoton(vPhotonGrdLoc,
						 vPhotonDCosGd,
						 fAzPhot,
						 fZnPhot,
						 fPhotHgtEmiss,
						 fPhotGrdTime,
						 fPhotWaveLgt,
						 iPhotType,
						 iPhotTelHitNum);     
	 // get ray tracing results
	 ROOT::Math::XYZVector vPhotonCameraLoc;
	 ROOT::Math::XYZVector vPhotonCameraDcos;
	 
	 bool bPhotonOnCamera = 
	   (*mArrayTel)[iPhotTelHitNum]->getCameraPhotonLocation(&vPhotonCameraLoc,
								 &vPhotonCameraDcos,
								 &fPhotonToCameraTime);

	 if (bPhotonOnCamera) {
	   // mRootWriter is passed in as a pointer so have to dereference
	   // add photon to the appropriate writer if photon strikes the camera 
	   (*mRootWriter)[iPhotTelHitNum]->addPhoton(vPhotonCameraLoc,
						     vPhotonCameraDcos,
						     fPhotonToCameraTime,
						     fPhotWaveLgt);
	   
	 }
	 
       }      // end of photon loop
    }
    *oLog << "    EventNumber " << fEventNumber << "   nPhotons "
	  << nPhotons << endl;     
    // add event to all writers here
    for (iterRootWriter = mRootWriter->begin();
	 iterRootWriter != mRootWriter->end();
	 iterRootWriter++) {

      unsigned int tel = (iterRootWriter->second)->getTelID();
      fDelay = (*mArrayTel)[tel]->getTimingDelay();
      fDelay = fDelay * 1.0e9 / (TMath::C());
      (iterRootWriter->second)->addEvent(fEventNumber,
					 fPrimaryType,
					 fPrimaryEnergy,
					 vSCore,
					 vSDcosGd,
					 fWobbleTE,
					 fWobbleTN,
					 fDelay,
					 fPhotonToCameraTime);
      
    }
    
  }  // end of shower loop
  
  // write all trees
  for (iterRootWriter = mRootWriter->begin(); 
       iterRootWriter != mRootWriter->end(); iterRootWriter++) {
    
    (iterRootWriter->second)->write();
  }
  
  return true;
};
/************** end of startSimulations******************/

void GSimulateOptics::makeWobbleOffset() {

  bool debug = false;

  if (debug) {
    *oLog << "  -- GSimulateOptics::makeWobbleOffset " << endl;
  }
  fWobbleTE = fWobbleE;
  fWobbleTN = fWobbleN;
    
  if (fWobbleR > 0.0001) {
    double wobRrand = fWobbleR*sqrt(TR3.Rndm());
    double wobTrand = (TMath::TwoPi())*TR3.Rndm();
    double delE     = wobRrand*cos(wobTrand);
    double delN     = wobRrand*sin(wobTrand);
    fWobbleTE     += delE;
    fWobbleTN     += delN;
  }
  else {
    // otherwise, wobbleETmp and wobbleNTmp are ok for the offsets
  }
 
  if (debug) {
    *oLog << "     fWobbleN E  " << fWobbleN*(TMath::RadToDeg()) 
          << "  " << fWobbleE*(TMath::RadToDeg()) << endl;
    *oLog << "     fWobbleTN TE  " << fWobbleTN*(TMath::RadToDeg()) 
          << "  " << fWobbleTE*(TMath::RadToDeg()) << endl;    
  }

};
/************** end of makeWobbleOffset ******************/
void GSimulateOptics::printDebugTelescope() {

  // print primary hit and direction: grd.coor.
  *oLog << "  -- GSimulateOptics::printDebugTelescope " << endl;

  *oLog << "        primary cord: Grd.Coor ";
  GUtilityFuncts::printGenVector(vSCore); *oLog << endl;  

  *oLog << "        primary dir.Cos: Grd.Coor ";
  GUtilityFuncts::printGenVector(vSDcosGd); *oLog << endl; 
  *oLog << "        az / zn of primary "  << fAzPrim*(TMath::RadToDeg())
        << "  /  " << fZnPrim*(TMath::RadToDeg()) << endl << endl; 

  *oLog << "        az / zn of telesc  "  << fAzTel*(TMath::RadToDeg())
        << "  /  " << fZnTel*(TMath::RadToDeg()) << endl;

  *oLog << "        teles dir.Cos: Grd.Coor  ";
  GUtilityFuncts::printGenVector(*vTelDcosGrd); *oLog << endl; 
  
  *oLog << "        teles dir.Cos: teles.coor ";
  ROOT::Math::XYZVector teldcos = (*rotGrdToTel)*(*vTelDcosGrd);
  GUtilityFuncts::printGenVector(teldcos); *oLog << endl;
  *oLog << "                   should be 0,0,1 " << endl << endl;

};
/************** end of printDebugTelescope ******************/

void GSimulateOptics::printDebugPhoton() {

  *oLog << "  -- GSimulateOptics::printDebugPhoton" << endl;
  *oLog << "         photonFlag " << photonFlag << endl;
  *oLog << "         ground hit location:grd.coor. ";
  GUtilityFuncts::printGenVector(vPhotonGrdLoc); *oLog << endl;
  *oLog << "         dir.cosines: grd.coor.    ";
  GUtilityFuncts::printGenVector(vPhotonDCosGd); *oLog << endl;
  *oLog << "         az / zn:       " << fAzPhot*(TMath::RadToDeg())
        << "  /  "<< fZnPhot*(TMath::RadToDeg()) << endl; 
 *oLog << "         hgtEmiss  arrivTime: " << fPhotHgtEmiss
        << "  " << fPhotGrdTime << endl;
  *oLog << "         wavelgt  partType: " << fPhotWaveLgt 
        << "  " << iPhotType << endl;
  *oLog << "         Telescope hit number: " << iPhotTelHitNum << endl;
};
/************** end of printDebugPhoton ******************/
void GSimulateOptics::printDebugPrimary() {

  *oLog << "  -- GSimulateOptics::printDebugPrimary" << endl;

  *oLog << "      primaryFlag " << primaryFlag << endl;
  *oLog << "      primary Az Zn " << fAzPrim*(TMath::RadToDeg()) 
        << " " << fZnPrim*(TMath::RadToDeg()) << endl;
  *oLog << "      energy        " << fEnergy << endl; 
  *oLog << "      wobble N E  " << fWobbleN*(TMath::RadToDeg()) 
        << " " << fWobbleE*(TMath::RadToDeg()) << endl;
  *oLog << "      wobbleR latitude " << fWobbleR*(TMath::RadToDeg())  
        << " " << fLatitude*(TMath::RadToDeg()) << endl;
  *oLog << "      wobbleTN TE from R " << fWobbleTN*(TMath::RadToDeg()) 
        << " " << fWobbleTE*(TMath::RadToDeg())  << endl; 
};
/************** end of printDebugPhoton ******************/

void GSimulateOptics::makeRotMatrixGrdToSky() {

  // not currently used
  bool debug = false;

  ////////////////////////////////////////
  // determine telescope location from wobble offsets
  // tested with matlab for fWobbleR > 0

  double wobbleETmp = fWobbleE;
  double wobbleNTmp = fWobbleN;
    
  if (fWobbleR > 0.0001) {
    double wobRrand = fWobbleR*sqrt(TR3.Rndm());
    double wobTrand = (TMath::TwoPi())*TR3.Rndm();
    double delE     = wobRrand*cos(wobTrand);
    double delN     = wobRrand*sin(wobTrand);
    wobbleETmp     += delE;
    wobbleNTmp     += delN;
  }
  else {
    // otherwise, wobbleETmp and wobbleNTmp are ok for the offsets
  }

  GUtilityFuncts::wobbleToAzZn(wobbleNTmp,wobbleETmp,fLatitude,
                               fAzPrim,fZnPrim,&fAzTel,&fZnTel);
  
  // get location of the telescope on the sky, dir cosines
  double xc=0.0, yc=0.0, zc=0.0;
  GUtilityFuncts::AzZnToXYcos(fAzTel,fZnTel,&xc,&yc);

  zc = sqrt(1 - xc*xc - yc*yc);
  vTelDcosGrd = new ROOT::Math::XYZVector(xc,yc,zc);
  if (debug) {
    *oLog << " tel.pointing in grd.coor ";
    GUtilityFuncts::printGenVector(*vTelDcosGrd); 
    *oLog << endl;
  }  

  // need a coordinate system rotation to the telescope system
  // XYZ rotations "actively" rotate the vector, not the coor.sys.
  ROOT::Math::RotationZ rz(fAzTel);
  ROOT::Math::RotationX rx(fZnTel);

  rotGrdToTel = new ROOT::Math::Rotation3D;
  *rotGrdToTel = rx*rz;

  if (debug) {
    // confirm for debugging, use matrix to rotation tel to tel coor.
    // should get unit vector
    ROOT::Math::XYZVector vTel_TelCoor = (*rotGrdToTel)*(*vTelDcosGrd);
    *oLog << " tel.location in tel coordinates using rot matrix ";
    GUtilityFuncts::printGenVector(vTel_TelCoor);
    *oLog << endl;
  }
};
/************** end of :makeRotMatrixGrdToSky ******************/

void GSimulateOptics::fillAllTelTree() {

  // cd to writer root file
  rootWriter->cdToWriteRootFile();

  // NEED TO CHECK THIS FOR MEMORY PROBLEMS
  //char *cstr = new char[sFileHeader.size() + 1];
  //strcpy( cstr ,sFileHeader.c_str() );

  // use a string instead of a character array for the
  // file header.
  string *strP = &sFileHeader;

  vector<int> telIDVector;
  vector<float> transitTimeVector;
  vector<float> telLocXGCVector;
  vector<float> telLocYGCVector;
  vector<float> telLocZGCVector;

  double telXLocGC = 0.0;
  double telYLocGC = 0.0;
  double telZLocGC = 0.0;

  //map<int,float> transitTimeMap;
  // make map of transit times
  for (iterArrayTel=mArrayTel->begin();
       iterArrayTel!=mArrayTel->end();
       iterArrayTel++) {
    int telid = iterArrayTel->first;
    double avgTransitTime = iterArrayTel->second->getAvgTransitTime();
    telIDVector.push_back(telid);
    transitTimeVector.push_back( (float)avgTransitTime );

    iterArrayTel->second->getTelLocGC(&telXLocGC,&telYLocGC,&telZLocGC);
    telLocXGCVector.push_back( (float)telXLocGC);
    telLocYGCVector.push_back( (float)telYLocGC);
    telLocZGCVector.push_back( (float)telZLocGC);
    
    //transitTimeMap[telid] = (float)avgTransitTime;
    //iterArrayTel->second->setPrimary

  }
  // create allTel tree, make branches, fill, and write the tree.
  string treeH = sHeaderTree;
  allTel = new TTree(treeH.c_str(),treeH.c_str());
 
  allTel->Branch("fileHeader",&strP);
  allTel->Branch("globalEffic",&fGlobalEffic,"globalEffic/D");
  allTel->Branch("obsHgt",&fObsHgt,"obsHgt/D");
  allTel->Branch("telIDVector",&telIDVector);
  allTel->Branch("telLocXVector",&telLocXGCVector);
  allTel->Branch("telLocYVector",&telLocYGCVector);
  allTel->Branch("telLocZVector",&telLocZGCVector);
  allTel->Branch("transitTimeVector",&transitTimeVector);

  // fill and write tree:  what about transit time map?
  // I added ProcessLine("#include map") in grOptics.cpp
  allTel->Fill();
  allTel->Write();

  //delete[] cstr;
}
/************** end of fillAllTelTree ******************/