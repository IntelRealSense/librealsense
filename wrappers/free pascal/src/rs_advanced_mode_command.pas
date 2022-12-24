{
Advanced Mode Commands header file
}
unit rs_advanced_mode_command;

{$mode ObjFPC}{$H+}

interface

type
  TSTDepthControlGroup = record
    plusIncrement: uint32;
    minusDecrement: uint32;
    deepSeaMedianThreshold: uint32;
    scoreThreshA: uint32;
    scoreThreshB: uint32;
    textureDifferenceThreshold: uint32;
    textureCountThreshold: uint32;
    deepSeaSecondPeakThreshold: uint32;
    deepSeaNeighborThreshold: uint32;
    lrAgreeThreshold: uint32;
  end;
  pSTDepthControlGroup = ^TSTDepthControlGroup;

type
  TSTRsm = record
    rsmBypass: uint32;
    diffThresh: single;
    sloRauDiffThresh: single;
    removeThresh: uint32;
  end;
  pSTRsm = ^TSTRsm;

type
  TSTRauSupportVectorControl = record
    minWest: uint32;
    minEast: uint32;
    minWEsum: uint32;
    minNorth: uint32;
    minSouth: uint32;
    minNSsum: uint32;
    uShrink: uint32;
    vShrink: uint32
  end;
  pSTRauSupportVectorControl = ^TSTRauSupportVectorControl;

type
  TSTColorControl = record
    disableSADColor: uint32;
    disableRAUColor: uint32;
    disableSLORightColor: uint32;
    disableSLOLeftColor: uint32;
    disableSADNormalize: uint32;
  end;
  pSTColorControl = ^TSTColorControl;

type
  TSTRauColorThresholdsControl = record
    rauDiffThresholdRed: uint32;
    rauDiffThresholdGreen: uint32;
    rauDiffThresholdBlue: uint32;
  end;
  pSTRauColorThresholdsControl = ^TSTRauColorThresholdsControl;

type
  TSTSloColorThresholdsControl = record
    diffThresholdRed: uint32;
    diffThresholdGreen: uint32;
    diffThresholdBlue: uint32;
  end;
  pSTSloColorThresholdsControl = ^TSTSloColorThresholdsControl;

type
  TSTSloPenaltyControl = record
    sloK1Penalty: uint32;
    sloK2Penalty: uint32;
    sloK1PenaltyMod1: uint32;
    sloK2PenaltyMod1: uint32;
    sloK1PenaltyMod2: uint32;
    sloK2PenaltyMod2: uint32;
  end;
  pTSTSloPenaltyControl = ^TSTSloPenaltyControl;

type
  TSTHdad = record
    lambdaCensus: single;
    lambdaAD: single;
    ignoreSAD: uint32;
  end;
  pSTHdad = ^TSTHdad;

type
  TSTColorCorrection = record
    colorCorrection1: single;
    colorCorrection2: single;
    colorCorrection3: single;
    colorCorrection4: single;
    colorCorrection5: single;
    colorCorrection6: single;
    colorCorrection7: single;
    colorCorrection8: single;
    colorCorrection9: single;
    colorCorrection10: single;
    colorCorrection11: single;
    colorCorrection12: single;
  end;
  pSTColorCorrection = ^TSTColorCorrection;

type
  TSTAEControl = record
    meanIntensitySetPoint: uint32;
  end;
  pSTAEControl = ^TSTAEControl;

type
  TSTDepthTableControl = record
    depthUnits: uint32;
    depthClampMin: uint32;
    depthClampMax: uint32;
    disparityMode: uint32;
    disparityShift: uint32;
  end;
  pSTDepthTableControl = ^TSTDepthTableControl;

type
  TSTCensusRadius = record
    uDiameter: uint32;
    vDiameter: uint32;
  end;
  pSTCensusRadius = ^TSTCensusRadius;

type
  TSTAFactor = record
    amplitude: single;
  end;
  pSTAFactor = ^TSTAFactor;

implementation

end.
