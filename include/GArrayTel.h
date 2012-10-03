/*
VERSION2.3
3OCT2012
*/
/*!  /brief GArrayTel class contains all telescope details 
            including a pointer to a GTelescope instance
 */

#ifndef GARRAYTEL
#define GARRAYTEL

// forward declarations
class GTelescope;
class GDCTelescope;
class TH2D;
class TH1D;
class TGraph;
class TCanvas;
#include "Math/Vector3Dfwd.h"
#include "Math/GenVector/Rotation3Dfwd.h"

class GArrayTel {

  ROOT::Math::XYZVector telLocGrdGC;  //!< tel.loc.ground coor.
  ROOT::Math::XYZVector telLocGrdTC; //!< tel.loc.telescope coor.
  double fWobbleN;  //!< wobble offset celestial N (radians)
  double fWobbleE;  //!< wobble offset celestial E (radians)
  double fLatitude;
  double fpointingOffsetX; //!< tel pointing offset X (radians)
  double fpointingOffsetY; //!< tel pointing offset Y (radians)

  TelType telType;    //!< telescope type enum
  int telID;          //!< telescope array ID
  int telStd;         //!< telescope standard number

  // primary parameters
  ROOT::Math::XYZVector vSCoreGC;  //!< primary coreHit ground coor.
  ROOT::Math::XYZVector vSDcosGC;  //!< primary dirCos. ground coor.
  double fAzPrim;                  //!< primary azimuth
  double fZnPrim;                  //!< primary zenith angle
  double fEnergy;                  //!< primary energy
  double fDelay;                   //!< timing delay

  ROOT::Math::Rotation3D rotGrdToTel;  //!< rotation matrix, grd to tel coor.
  double fAzTel;          //!< azimuthal angle (radians) for telescope
  double fZnTel;          //!< zenith angle (radians) for telescope
  ROOT::Math::XYZVector vTelDcosGC; //!< tel.dirCos. ground coor.  

  // photon parameters
  ROOT::Math::XYZVector vPhotonGrdLocGC; //!< photon grd.loc.ground coors.
  ROOT::Math::XYZVector vPhotonDcosGC;   //!< photon dir.cos.ground coors.
  double fAzPhotGr;
  double fZnPhotGr;
  double fPhotHgtEmiss;
  double fPhotGrdTime;
  double fPhotWaveLgt;
  int iPhotType;
  int iPhotTelHitNum;

  ROOT::Math::XYZVector vPhotonGrdLocTC;  //!< photon grd.loc.Tel.Coor
  ROOT::Math::XYZVector vPhotonRelLocTC;  /*< photon grd.loc.rel.to tel
                                          in tel.coor. */

  ROOT::Math::XYZVector vPhotonDcosTC;  //!< photon dir.cos.telescope coors.  

  bool bOnCamera;  //!< if true, photon on camera
  ROOT::Math::XYZVector vPhotonCameraLoc;
  ROOT::Math::XYZVector vPhotonCameraDcos;
  double fTotalPhotonTime;

  // have to define storage structures, need time, two deques are ok.
  deque<float> *dStoreLoc; //!< photon camera location deque storage
  deque<float> *dStorePix; //!< pixel location deque storage

  int iPrintMode;

  // variables for testTelescope
  vector<TH2D*> vHist;
  vector<TH1D*> vHistT;

  GTelescope *tel;  //!< pointer to associated telescope 

 protected:

 public:

  /*! default constructor
   */
  GArrayTel();

  /*!  constructor
   */
  GArrayTel(const ROOT::Math::XYZVector telLocGrd,
            const double &telOffsetX,
            const double &telOffsetY,
            const TelType &teltype,
	    const int &telid,
            const int &telstd,
	    const int &printMode,
	    GTelescope *ctel);

  /*!
   */
  ~GArrayTel();

  /*!  \ brief set all primary parameters
   */
  void setPrimary(const ROOT::Math::XYZVector &vSCorec,
                  const ROOT::Math::XYZVector &vSDcosGdc,
                  const double &AzPrimc,const double &ZnPrimc,
                  const double &energyc,const double &WobbleTNc,
                  const double &WobbleTEc,
                  const double &Latitude);
  
  /*!   \brief 
   */ 
  void setPhoton(const ROOT::Math::XYZVector &pGrd,
                 const ROOT::Math::XYZVector &pDcos,
                 const double &pAz,const double &pZn,
                 const double &pHgtEmiss,const double &pTime,
                 const double &pWaveLgt, const int &pType,
                 const int &pTel);
  
  /*!
   */
  void printArrayTel();

  /*!
   */
  void setPhotonHistory(const string &rootFile,
                        const string &treeName,
                        const int &option = 0) {

    tel->setPhotonHistory(rootFile,treeName,option);
  };

  void writePhotonHistory() {

    tel->writePhotonHistory();
  };

  bool getCameraPhotonLocation(ROOT::Math::XYZVector *photonLoc,
					ROOT::Math::XYZVector *photonDcos,
			       double *photonTime);

  double getTimingDelay() {
    return fDelay;
  };

  int getTelescopeID() {
    return telID;
  };

  int getTelescopeStdNum() {
    return telStd;
  };

  TelType getTelescopeType() {
    return telType;
  };

  double getAvgTransitTime() {
    return tel->getAvgTransitTime();
  };

  void makeTelescopeTest(const string& testfile);

  void getTelLocGC(double *telLocX, double *telLocY, double *telLocZ) {
    *telLocX = telLocGrdGC.X();
    *telLocY = telLocGrdGC.Y();
    *telLocZ = telLocGrdGC.Z();
  }
};

#endif
