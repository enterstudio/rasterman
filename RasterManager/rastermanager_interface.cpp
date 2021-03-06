#define MY_DLL_EXPORT

#include "rastermanager_interface.h"
#include "raster_pitremove.h"
#include "extentrectangle.h"
#include "rasterarray.h"
#include "raster_gutpolygon.h"
#include "histogramsclass.h"
#include <stdio.h>
#include "extentrectangle.h"
#include "rastermanager.h"

#include "raster.h"
#include "gdal_priv.h"

#include <limits>
#include <math.h>
#include <string>

#include <algorithm>
#include <iostream>

namespace RasterManager {

RM_DLL_API const char * GetLibVersion(){ return RMLIBVERSION; }

RM_DLL_API const char * GetMinGDALVersion(){ return MINGDAL; }

RM_DLL_API GDALDataset * CreateOutputDS(const char * pOutputRaster,
                                        GDALDataType eDataType,
                                        bool bHasNoData,
                                        double fNoDataValue,
                                        int nCols, int nRows, double * newTransform, const char * projectionRef, const char * unit){

    RasterMeta pInputMeta(newTransform[3], newTransform[0], nRows, nCols, &newTransform[5], &newTransform[1], &fNoDataValue, NULL, &eDataType, projectionRef, unit);
    if (bHasNoData){

    }
    return CreateOutputDS(pOutputRaster, &pInputMeta);
}


RM_DLL_API GDALDataset * CreateOutputDS(QString sOutputRaster, RasterMeta * pTemplateRasterMeta){
    const QByteArray qbFileName = sOutputRaster.toLocal8Bit();
    return CreateOutputDS(qbFileName.data(), pTemplateRasterMeta);
}

RM_DLL_API GDALDataset * CreateOutputDS(const char * pOutputRaster, RasterMeta * pTemplateRastermeta){

    // Make sure the file doesn't exist. Throws an exception if it does.
    CheckFile(pOutputRaster, false);

    /* Create the new dataset. Determine the driver from the output file extension.
     * Enforce LZW compression for TIFs. The predictor 3 is used for floating point prediction.
     * Not using this value defaults the LZW to prediction to 1 which causes striping.
     */

    char **papszOptions = NULL;
    GDALDriver * pDR = NULL;

    // Always set the driver to the output Raster name (tiff, tif, img)
    pDR = GetGDALDriverManager()->GetDriverByName(GetDriverFromFileName(pOutputRaster));

    if (strcmp( pDR->GetDescription() , "GTiff") == 0){
        papszOptions = CSLSetNameValue(papszOptions, "COMPRESS", "LZW");
    }
    else {
        papszOptions = CSLSetNameValue(papszOptions, "COMPRESS", "PACKBITS");
    }

    GDALDataset * pDSOutput =  pDR->Create(pOutputRaster,
                                           pTemplateRastermeta->GetCols(),
                                           pTemplateRastermeta->GetRows(),
                                           1,
                                           *pTemplateRastermeta->GetGDALDataType(),
                                           papszOptions);

    CSLDestroy( papszOptions );

    if (pDSOutput == NULL)
        return NULL;

    if (pTemplateRastermeta->HasNoDataValue())
    {
        CPLErr er = pDSOutput->GetRasterBand(1)->SetNoDataValue(pTemplateRastermeta->GetNoDataValue());
        if (er == CE_Failure || er == CE_Fatal)
            return NULL;
    }
    else{
        CPLErr er = pDSOutput->GetRasterBand(1)->SetNoDataValue(0);
        if (er) { }
    }

    double * newTransform = pTemplateRastermeta->GetGeoTransform();
    char * projectionRef = pTemplateRastermeta->GetProjectionRef();

    // Fill the new raster set with nodatavalue
    pDSOutput->GetRasterBand(1)->Fill(pTemplateRastermeta->GetNoDataValue());

    if (newTransform != NULL)
        pDSOutput->SetGeoTransform(newTransform);
    if (projectionRef != NULL)
        pDSOutput->SetProjection(projectionRef);
    return pDSOutput;

}

extern "C" RM_DLL_API int DeleteDataset(const char * pOutputRaster, char * sErr){

    InitCInterfaceError(sErr);
    try {
        return Raster::Delete(pOutputRaster);
    }
    catch (RasterManagerException e){
        SetCInterfaceError(e, sErr);
        return e.GetErrorCode();
    }
}

extern "C" RM_DLL_API void RegisterGDAL() { GDALAllRegister();}
extern "C" RM_DLL_API void DestroyGDAL() { GDALDestroyDriverManager();}

extern "C" RM_DLL_API int BasicMath(const char * psRaster1,
                                    const char * psRaster2,
                                    const double dNumericArg,
                                    const char * psOperation,
                                    const char * psOutput,
                                    char * sErr)
{
    InitCInterfaceError(sErr);
    try {
        return Raster::RasterMath(psRaster1,
                                  psRaster2,
                                  &dNumericArg,
                                  psOperation,
                                  psOutput);
    }
    catch (RasterManagerException e){
        SetCInterfaceError(e, sErr);
        return e.GetErrorCode();
    }
}

extern "C" RM_DLL_API int RasterInvert(const char * psRaster1,
                                       const char * psRaster2,
                                       double dValue, char * sErr)
{
    InitCInterfaceError(sErr);
    try {
        return Raster::InvertRaster(
                    psRaster1,
                    psRaster2,
                    dValue);
    }
    catch (RasterManagerException e){
        SetCInterfaceError(e, sErr);
        return e.GetErrorCode();
    }
}


extern "C" RM_DLL_API int RasterFilter(
        const char * psOperation,
        const char * psInputRaster,
        const char * psOutputRaster,
        int nWidth,
        int nHeight,
        char * sErr)
{
    InitCInterfaceError(sErr);
    try {
        return Raster::FilterRaster(
                    psOperation,
                    psInputRaster,
                    psOutputRaster,
                    nWidth,
                    nHeight);
    }
    catch (RasterManagerException e){
        SetCInterfaceError(e, sErr);
        return e.GetErrorCode();
    }
}


extern "C" RM_DLL_API int Uniform(const char * psInputRaster, const char * psOutputRaster, double dValue, char * sErr)
{
    InitCInterfaceError(sErr);
    try {
        Raster theRaster(psInputRaster);
        return theRaster.Uniform(psOutputRaster, dValue);
    }
    catch (RasterManagerException e){
        SetCInterfaceError(e, sErr);
        return e.GetErrorCode();
    }
}


extern "C" RM_DLL_API int RasterFromCSVandTemplate(const char * sCSVSourcePath,
                                                   const char * psOutput,
                                                   const char * sRasterTemplate,
                                                   const char * sXField,
                                                   const char * sYField,
                                                   const char * sDataField,
                                                   char * sErr){

    InitCInterfaceError(sErr);
    try {
        return  RasterManager::Raster::CSVtoRaster(sCSVSourcePath,
                                                   psOutput,
                                                   sRasterTemplate,
                                                   sXField,
                                                   sYField,
                                                   sDataField );
    }
    catch (RasterManagerException e){
        SetCInterfaceError(e, sErr);
        return e.GetErrorCode();
    }

}


extern "C" RM_DLL_API int RasterToCSV(const char * sRasterSourcePath,
                                      const char * sOutputCSVPath,
                                      char * sErr){

    InitCInterfaceError(sErr);
    try {
        return  RasterManager::Raster::RasterToCSV(sRasterSourcePath,
                                                   sOutputCSVPath);
    }
    catch (RasterManagerException e){
        SetCInterfaceError(e, sErr);
        return e.GetErrorCode();
    }

}

extern "C" RM_DLL_API int CalcSimpleHistograms(const char * psRasterPath,
                                               const char * psHistogramPath,
                                               int nNumBins,
                                               char * sErr)
{
    int eResult = PROCESS_OK;
    InitCInterfaceError(sErr);
    try
    {
        RasterManager::HistogramsClass theHisto(psRasterPath,nNumBins);

        theHisto.writeCSV(psHistogramPath);
        eResult = PROCESS_OK;
    }
    catch (RasterManagerException ex)
    {
        SetCInterfaceError(ex, sErr);
        eResult = ex.GetErrorCode();
    }

    return eResult;
}

extern "C" RM_DLL_API int CalcHistograms(const char * psRasterPath,
                                         const char * psHistogramPath,
                                         int nNumBins,
                                         int nMinimumBin,
                                         double fBinSize,
                                         double fBinIncrement,
                                         char * sErr)
{
    int eResult = PROCESS_OK;
    InitCInterfaceError(sErr);
    try
    {
        RasterManager::HistogramsClass theHisto(psRasterPath,nNumBins, nMinimumBin, fBinSize, fBinIncrement);

        theHisto.writeCSV(psHistogramPath);
        eResult = PROCESS_OK;
    }
    catch (RasterManagerException ex)
    {
        SetCInterfaceError(ex, sErr);
        eResult = ex.GetErrorCode();
    }

    return eResult;
}


extern "C" RM_DLL_API int RasterFromCSVandExtents(const char * sCSVSourcePath,
                                                  const char * sOutput,
                                                  double dTop,
                                                  double dLeft,
                                                  int nRows,
                                                  int nCols,
                                                  double dCellWidth,
                                                  double dNoDataVal,
                                                  const char * sXField,
                                                  const char * sYField,
                                                  const char * sDataField,
                                                  char * sErr){

    InitCInterfaceError(sErr);
    try {
        return  RasterManager::Raster::CSVtoRaster(sCSVSourcePath,
                                                   sOutput,
                                                   dTop,
                                                   dLeft,
                                                   nRows,
                                                   nCols,
                                                   dCellWidth,
                                                   dNoDataVal,
                                                   sXField,
                                                   sYField,
                                                   sDataField);
    }
    catch (RasterManagerException e){
        SetCInterfaceError(e, sErr);
        return e.GetErrorCode();
    }
}

extern "C" RM_DLL_API int ExtractRasterPoints(const char * sCSVInputSourcePath,
                                              const char * sRasterInputSourcePath,
                                              const char * sCSVOutputPath,
                                              const char * sXField,
                                              const char * sYField,
                                              const char * sNodata,
                                              char * sErr)
{
    InitCInterfaceError(sErr);
    try {
        return Raster::ExtractPoints(
                    sCSVInputSourcePath,
                    sRasterInputSourcePath,
                    sCSVOutputPath,
                    QString(sXField),
                    QString(sYField),
                    QString(sNodata) );
    }
    catch (RasterManagerException e){
        SetCInterfaceError(e, sErr);
        return e.GetErrorCode();
    }
}



extern "C" RM_DLL_API int RasterNormalize(const char * psRaster1,
                                          const char * psRaster2,
                                          char * sErr)
{
    InitCInterfaceError(sErr);
    try {
        return Raster::NormalizeRaster(
                    psRaster1,
                    psRaster2);
    }
    catch (RasterManagerException e){
        SetCInterfaceError(e, sErr);
        return e.GetErrorCode();
    }
}

extern "C" RM_DLL_API int RasterEuclideanDistance(const char * psInput,
                                                  const char * psOutput, const char * psUnits, char * sErr)
{
    InitCInterfaceError(sErr);
    try {
        return Raster::EuclideanDistance(
                    psInput,
                    psOutput,
                    psUnits);
    }
    catch (RasterManagerException e){
        SetCInterfaceError(e, sErr);
        return e.GetErrorCode();
    }
}

extern "C" RM_DLL_API int CreateHillshade(const char * psInputRaster, const char * psOutputHillshade, char * sErr)
{
    InitCInterfaceError(sErr);
    try {
        Raster pDemRaster (psInputRaster);
        return pDemRaster.Hillshade(psOutputHillshade);
    }
    catch (RasterManagerException e){
        SetCInterfaceError(e, sErr);
        return e.GetErrorCode();
    }

}

extern "C" RM_DLL_API int CreateSlope(const char * psInputRaster, const char * psOutputSlope, const char * psSlopeType, char * sErr)
{
    InitCInterfaceError(sErr);
    try{
        Raster pDemRaster (psInputRaster);
        return pDemRaster.Slope(psOutputSlope, psSlopeType);
    }
    catch (RasterManagerException e){
        SetCInterfaceError(e, sErr);
        return e.GetErrorCode();
    }

}

extern "C" RM_DLL_API int Mask(const char * psInputRaster, const char * psMaskRaster, const char * psOutput, char * sErr)
{
    InitCInterfaceError(sErr);
    try{
        return Raster::RasterMask(psInputRaster, psMaskRaster, psOutput);
    }
    catch (RasterManagerException e){
        SetCInterfaceError(e, sErr);
        return e.GetErrorCode();
    }
}

extern "C" RM_DLL_API int MaskValue(const char * psInputRaster, const char * psOutput, double dMaskValue, char * sErr)
{
    InitCInterfaceError(sErr);
    try{
        return Raster::RasterMaskValue(psInputRaster, psOutput, dMaskValue);
    }
    catch (RasterManagerException e){
        SetCInterfaceError(e, sErr);
        return e.GetErrorCode();
    }
}

extern "C" RM_DLL_API int LinearThreshold(const char * psInputRaster,
                                          const char * psOutputRaster,
                                          double dLowThresh,
                                          double dLowThreshVal,
                                          double dHighThresh,
                                          double dHighThreshVal,
                                          int nKeepNodata,
                                          char * sErr){
    InitCInterfaceError(sErr);
    try{
        return Raster::LinearThreshold(psInputRaster, psOutputRaster, dLowThresh, dLowThreshVal, dHighThresh, dHighThreshVal, nKeepNodata);
    }
    catch (RasterManagerException e){
        SetCInterfaceError(e, sErr);
        return e.GetErrorCode();
    }

}

extern "C" RM_DLL_API int AreaThreshold(const char * psInputRaster,
                                        const char * psOutputRaster,
                                        double dAreaThresh,
                                        char * sErr){
    InitCInterfaceError(sErr);
    try{
        RasterArray raRaster(psInputRaster);
        return raRaster.AreaThresholdRaster(psOutputRaster, dAreaThresh);
    }
    catch (RasterManagerException e){
        SetCInterfaceError(e, sErr);
        return e.GetErrorCode();
    }
}

extern "C" RM_DLL_API int SmoothEdges(const char * psInputRaster,
                                      const char * psOutputRaster,
                                      int nCells,
                                      char * sErr){
    InitCInterfaceError(sErr);
    try{
        RasterArray raRaster(psInputRaster);
        return raRaster.SmoothEdge(psOutputRaster, nCells);
    }
    catch (RasterManagerException e){
        SetCInterfaceError(e, sErr);
        return e.GetErrorCode();
    }
}

extern "C" RM_DLL_API int SetNull(const char * psInputRaster,
                                  const char * psOutputRaster,
                                  const char * psOperator,
                                  double dThreshVal1,
                                  double dThreshVal2,
                                  char * sErr){
    InitCInterfaceError(sErr);
    try{
        Raster raRaster(psInputRaster);
        return raRaster.SetNull(psOutputRaster, psOperator, dThreshVal1, dThreshVal2);
    }
    catch (RasterManagerException e){
        SetCInterfaceError(e, sErr);
        return e.GetErrorCode();
    }
}


extern "C" RM_DLL_API int RootSumSquares(const char * psRaster1,
                                         const char * psRaster2,
                                         const char * psOutput,
                                         char * sErr)
{
    InitCInterfaceError(sErr);
    try{
        return Raster::RasterRootSumSquares(psRaster1, psRaster2, psOutput);
    }
    catch (RasterManagerException e){
        SetCInterfaceError(e, sErr);
        return e.GetErrorCode();
    }

}


extern "C" RM_DLL_API int Mosaic(const char * csRasters, const char * psOutput, char * sErr)
{
    InitCInterfaceError(sErr);
    try{
        return Raster::RasterMosaic(csRasters, psOutput);
    }
    catch (RasterManagerException e){
        SetCInterfaceError(e, sErr);
        return e.GetErrorCode();
    }

}

extern "C" RM_DLL_API int Combine(const char * csRasters, const char * psOutput,  const char * psMethod, char * sErr)
{
    InitCInterfaceError(sErr);
    try{
        return Raster::CombineRaster(csRasters, psOutput, psMethod);
    }
    catch (RasterManagerException e){
        SetCInterfaceError(e, sErr);
        return e.GetErrorCode();
    }

}

extern "C" RM_DLL_API int vector2raster(const char * sVectorSourcePath,
                                        const char * sRasterOutputPath,
                                        const char * sRasterTemplate,
                                        double dCellWidth,
                                        const char * psFieldName,
                                        char * sErr)
{
    InitCInterfaceError(sErr);
    try{
        if (dCellWidth)
            return Raster::VectortoRaster(sVectorSourcePath, sRasterOutputPath, dCellWidth, psFieldName);
        else
            return Raster::VectortoRaster(sVectorSourcePath, sRasterOutputPath, sRasterTemplate, psFieldName);
    }
    catch (RasterManagerException e){
        SetCInterfaceError(e, sErr);
        return e.GetErrorCode();
    }

}

extern "C" RM_DLL_API int Fill(const char * sRasterInput, const char * sRasterOutput, char * sErr){


    InitCInterfaceError(sErr);
    try{
        //    Mincost is the default
        FillMode nMethod = FILL_MINCOST;

        RasterPitRemoval rasterPitRemove( sRasterInput, sRasterOutput, nMethod );
        return rasterPitRemove.Run();
    }
    catch (RasterManagerException e){
        SetCInterfaceError(e, sErr);
        return e.GetErrorCode();
    }
}

extern "C" RM_DLL_API int CreateDrain(const char * sRasterInput, const char * sRasterOutput, char * sErr){
    InitCInterfaceError(sErr);
    try{
        RasterArray raInpu(sRasterInput);
        return raInpu.CreateDrain(sRasterOutput);
    }
    catch (RasterManagerException e){
        SetCInterfaceError(e, sErr);
        return e.GetErrorCode();
    }
}


extern "C" RM_DLL_API int AddGut(const char *psShpFile,
                                 const char *psInput,
                                 const char *tier1,
                                 const char *tier2, char * sErr)
{
    InitCInterfaceError(sErr);
    try{
        return Raster2Polygon::AddGut(psShpFile, psInput, tier1, tier2);
    }
    catch (RasterManagerException e){
        SetCInterfaceError(e, sErr);
        return e.GetErrorCode();
    }

}

extern "C" RM_DLL_API int IsConcurrent(const char * csRaster1, const char * csRaster2, char * sErr)
{
    InitCInterfaceError(sErr);
    try{
        RasterManager::RasterMeta rmRaster1(csRaster1);
        RasterManager::RasterMeta rmRaster2(csRaster2);

        return rmRaster1.IsConcurrent(&rmRaster2);
    }
    catch (RasterManagerException e){
        SetCInterfaceError(e, sErr);
        return e.GetErrorCode();
    }

}

extern "C" RM_DLL_API int MakeConcurrent(const char * csRasters, const char * csRasterOutputs, char * sErr)
{
    InitCInterfaceError(sErr);
    try{
        return Raster::MakeRasterConcurrent(csRasters, csRasterOutputs);
    }
    catch (RasterManagerException e){
        SetCInterfaceError(e, sErr);
        return e.GetErrorCode();
    }
}

/*******************************************************************************************************
 *******************************************************************************************************
 * Raster property methods
 */

extern "C" RM_DLL_API int GetRasterProperties(const char * ppszRaster,
                                              double & fCellHeight, double & fCellWidth,
                                              double & fLeft, double & fTop, int & nRows, int & nCols,
                                              double & fNoData, int & bHasNoData, int & nDataType, char * psUnit, char * psProjection, char * sErr)
{
    InitCInterfaceError(sErr);
    try{
        RasterManager::Raster r(ppszRaster);
        fCellHeight = r.GetCellHeight();
        fCellWidth = r.GetCellWidth();
        fLeft = r.GetLeft();
        fTop = r.GetTop();
        nRows = r.GetRows();
        nCols = r.GetCols();
        fNoData = r.GetNoDataValue();
        bHasNoData = (int) r.HasNoDataValue();
        nDataType = (int) *r.GetGDALDataType();

        const char * ccProjectionRef =  r.GetProjectionRef();
        const char * ccUnit = r.GetUnit();

        strncpy(psProjection, ccProjectionRef, ERRBUFFERSIZE);
        strncpy(psUnit, ccUnit, ERRBUFFERSIZE);

    }
    catch (RasterManagerException e){
        SetCInterfaceError(e, sErr);
        return e.GetErrorCode();
    }
    return PROCESS_OK;
}

extern "C" RM_DLL_API int RasterCompare(const char * ppszRaster1, const char * ppszRaster2, char * sErr){
    InitCInterfaceError(sErr);
    try{
        RasterManager::RasterArray rRasterArray1(ppszRaster1);
        RasterManager::RasterArray rRasterArray2(ppszRaster2);

        if (rRasterArray1 != rRasterArray2)
            return RASTER_COMPARISON;
    }
    catch (RasterManagerException e){
        SetCInterfaceError(e, sErr);
        return e.GetErrorCode();
    }
    return PROCESS_OK;
}

extern "C" RM_DLL_API void PrintRasterProperties(const char * ppszRaster)
{
    try{
        double fCellWidth = 0;
        double fCellHeight = 0;
        double fLeft = 0;
        double fTop = 0;
        double fBottom = 0;
        double fRight = 0;
        int nRows = 0;
        int nCols = 0;
        double fNoData = 0;
        double dRasterMax = 0;
        double dRasterMin = 0;
        int bHasNoData = 0;
        int divisible = 0;
        int nDataType;

        RasterManager::Raster r(ppszRaster);
        std::string projection = r.GetProjectionRef();
        std::string unit = r.GetUnit();

        fCellHeight = r.GetCellHeight();
        fCellWidth = r.GetCellWidth();
        fLeft = r.GetLeft();
        fRight = r.GetRight();
        fTop = r.GetTop();
        fBottom = r.GetBottom();
        nRows = r.GetRows();
        nCols = r.GetCols();

        dRasterMax = r.GetMaximum();
        dRasterMin = r.GetMinimum();
        divisible = r.IsDivisible();
        fNoData = r.GetNoDataValue();
        bHasNoData = (int) r.HasNoDataValue();
        nDataType = (int) *r.GetGDALDataType();

        printLine( QString("     Raster: %1").arg(ppszRaster));

        int leftPrec = GetPrecision(fLeft);
        int rightPrec = GetPrecision(fRight);
        int bottomPrec = GetPrecision(fBottom);
        int topPrec = GetPrecision(fTop);
        int cellWidthPrec = GetPrecision(fCellWidth);

        printLine( QString("       Left: %1   Right: %2").arg(fLeft, 0, 'f', leftPrec).arg(fRight, 0, 'f', rightPrec));

        printLine( QString("        Top: %1   Bottom: %2").arg(fTop, 0, 'f', topPrec).arg(fBottom, 0, 'f', bottomPrec));
        printLine( QString("       Rows: %1         Cols: %2").arg(nRows).arg(nCols));
        printLine( QString("        "));
        printLine( QString("       Cell Width: %1").arg(fCellWidth, 0, 'f', cellWidthPrec));
        printLine( QString("              Min: %1      Max: %2").arg(dRasterMin, 0, 'f', 2).arg(dRasterMax, 0, 'f', 2));

        if (divisible == 1 ){
            printLine( QString("       Divisible: True" ) );
        }
        else {
            printLine( QString("       Divisible: False" ) );
        }
        std::cout << "\n";
        switch (nDataType)
        {
        // Note 0 = unknown;
        case  1: std::cout << "\n      Data Type: 1, GDT_Byte, Eight bit unsigned integer"; break;
        case  2: std::cout << "\n      Data Type: 2, GDT_UInt16, Sixteen bit unsigned integer"; break;
        case  3: std::cout << "\n      Data Type: 3, GDT_Int16, Sixteen bit signed integer"; break;
        case  4: std::cout << "\n      Data Type: 4, GDT_UInt32, Thirty two bit unsigned integer"; break;
        case  5: std::cout << "\n      Data Type: 5, GDT_Int32, Thirty two bit signed integer"; break;
        case  6: std::cout << "\n      Data Type: 6, GDT_Float32, Thirty two bit floating point"; break;
        case  7: std::cout << "\n      Data Type: 7, GDT_Float64, Sixty four bit floating point"; break;
        case  8: std::cout << "\n      Data Type: 8, GDT_CInt16, Complex Int16"; break;
        case  9: std::cout << "\n      Data Type: 9, GDT_CInt32, Complex Int32"; break;
        case 10: std::cout << "\n      Data Type: 10, GDT_CFloat32, Complex Float32"; break;
        case 11: std::cout << "\n      Data Type: 11, GDT_CFloat64, Complex Float64"; break;
        default: std::cout << "\n      Data Type: Unknown"; break;
        }
        if (bHasNoData)
            std::cout << "\n        No Data: " << fNoData;
        else
            std::cout << "\n        No Data: none";

        std::cout << "\n     Projection: " << projection.substr(0,70) << "...";
        std::cout << "\n     Unit: " << unit;
        std::cout << "\n ";

    }
    catch (RasterManagerException e){
        //e.GetErrorCode();
    }
}

extern "C" RM_DLL_API int RasterGetStat(const char * psOperation, double * pdValue, const char * psInputRaster, char * sErr){
    InitCInterfaceError(sErr);
    try {
        RasterManager::Raster rRaster(psInputRaster);
        int eResult = rRaster.RasterStat( (Raster_Stats_Operation) GetStatFromString(psOperation), pdValue);
        return eResult;
    }
    catch (RasterManagerException e){
        SetCInterfaceError(e, sErr);
        return e.GetErrorCode();
    }
}

/*******************************************************************************************************
 * Raster copy and resample methods
 */

extern "C" RM_DLL_API int Copy(const char * ppszOriginalRaster,
                               const char *ppszOutputRaster,
                               double fNewCellSize,
                               double fLeft, double fTop, int nRows, int nCols,
                               char * sErr)
{
    InitCInterfaceError(sErr);
    try{
        RasterManager::Raster ra(ppszOriginalRaster);
        return ra.Copy(ppszOutputRaster, &fNewCellSize, fLeft, fTop, nRows, nCols, NULL, NULL);
    }
    catch (RasterManagerException e){
        SetCInterfaceError(e, sErr);
        return e.GetErrorCode();
    }
}

extern "C" RM_DLL_API int ExtendedCopy(const char * ppszOriginalRaster,
                                       const char *ppszOutputRaster,
                                       double fLeft, double fTop, int nRows, int nCols,
                                       const char * psRef,
                                       const char * psUnit,
                                       char * sErr)
{
    InitCInterfaceError(sErr);
    try{
        RasterManager::Raster ra(ppszOriginalRaster);

        double fNewCellSize = ra.GetCellWidth();
        return ra.Copy(ppszOutputRaster, &fNewCellSize, fLeft, fTop, nRows, nCols, psRef, psUnit);
    }
    catch (RasterManagerException e){
        SetCInterfaceError(e, sErr);
        return e.GetErrorCode();
    }
}

extern "C" RM_DLL_API int SpatialReferenceMatches(const char * psDS1, const char * psDS2, int & result, char * sErr)
{
    InitCInterfaceError(sErr);
    try{
        OGRSpatialReference ref1 = RasterManager::Raster::getDSRef(psDS1);
        OGRSpatialReference ref2 = RasterManager::Raster::getDSRef(psDS2);
        result = int (ref1.IsSame(&ref2));
        return PROCESS_OK;
    }
    catch (RasterManagerException e){
        SetCInterfaceError(e, sErr);
        return e.GetErrorCode();
    }
}

extern "C" RM_DLL_API int BiLinearResample(const char * ppszOriginalRaster,
                                           const char *ppszOutputRaster,
                                           double fNewCellSize,
                                           double fLeft, double fTop, int nRows, int nCols,
                                           char * sErr)
{
    InitCInterfaceError(sErr);
    try{
        RasterManager::Raster ra(ppszOriginalRaster);
        return ra.ReSample(ppszOutputRaster, fNewCellSize, fLeft, fTop, nRows, nCols);
    }
    catch (RasterManagerException e){
        SetCInterfaceError(e, sErr);
        return e.GetErrorCode();
    }
}

extern "C" RM_DLL_API const char * ExtractFileExt(const char * FileName)
{
    for (int i = strlen(FileName); i >= 0; i--) {
        if (FileName[i] == '.' ){
            return &FileName[i];
        }
    }
    return NULL;
}

extern "C" RM_DLL_API const char * GetDriverFromFilename(const char * FileName)
{
    const char * pSuffix = ExtractFileExt(FileName);

    if (pSuffix == NULL)
        return NULL;
    else
    {
        if (strcmp(pSuffix, ".tif") == 0)
        {
            return "GTiff";
        }
        else if (strcmp(pSuffix, ".img") == 0)
            return "HFA";
        else
            return NULL;
    }
}

extern "C" RM_DLL_API const char * GetDriverFromFileName(const char * psFileName)
{
    if (EndsWith(psFileName, ".tif"))
    {
        return "GTiff";
    } else if (EndsWith(psFileName, ".img"))
    {
        return "HFA";
    }
    else
        throw std::runtime_error("Unhandled raster format without a GDAL driver specification.");
}

bool EndsWith(const char * psFullString, const char * psEnding)
{
    std::string sFullString(psFullString);
    std::string sEnding(psEnding);

    // Ensure both strings are lower case
    std::transform(sFullString.begin(), sFullString.end(), sFullString.begin(), ::tolower);
    std::transform(sEnding.begin(), sEnding.end(), sEnding.begin(), ::tolower);

    if (sFullString.length() >= sEnding.length()) {
        return (0 == sFullString.compare (sFullString.length() - sEnding.length(), sEnding.length(), sEnding));
    } else {
        return false;
    }
}

extern "C" RM_DLL_API int GetSymbologyStyleFromString(const char * psStyle)
{
    QString sStyle(psStyle);

    if (QString::compare(sStyle , "DEM", Qt::CaseInsensitive) == 0)
        return GSS_DEM;
    else if (QString::compare(sStyle , "DoD", Qt::CaseInsensitive) == 0)
        return GSS_DoD;
    else if (QString::compare(sStyle , "Error", Qt::CaseInsensitive) == 0)
        return GSS_Error;
    else if (QString::compare(sStyle , "HillShade", Qt::CaseInsensitive) == 0)
        return GSS_Hlsd;
    else if (QString::compare(sStyle , "PointDensity", Qt::CaseInsensitive) == 0)
        return GSS_PtDens;
    else if (QString::compare(sStyle , "SlopeDeg", Qt::CaseInsensitive) == 0)
        return GSS_SlopeDeg;
    else if (QString::compare(sStyle , "SlopePC", Qt::CaseInsensitive) == 0)
        return GSS_SlopePer;
    else
        return -1;
}

extern "C" RM_DLL_API int GetStatOperationFromString(const char * psStats)
{
    QString sStyle(psStats);

    if (QString::compare(sStyle , "mean", Qt::CaseInsensitive) == 0)
        return STATS_MEAN;
    else if (QString::compare(sStyle , "majority", Qt::CaseInsensitive) == 0)
        return STATS_MEDIAN;
    else if (QString::compare(sStyle , "maximum", Qt::CaseInsensitive) == 0)
        return STATS_MAJORITY;
    else if (QString::compare(sStyle , "median", Qt::CaseInsensitive) == 0)
        return STATS_MINORITY;
    else if (QString::compare(sStyle , "minimum", Qt::CaseInsensitive) == 0)
        return STATS_MAXIMUM;
    else if (QString::compare(sStyle , "minority", Qt::CaseInsensitive) == 0)
        return STATS_MINIMUM;
    else if (QString::compare(sStyle , "range", Qt::CaseInsensitive) == 0)
        return STATS_STD;
    else if (QString::compare(sStyle , "std", Qt::CaseInsensitive) == 0)
        return STATS_SUM;
    else if (QString::compare(sStyle , "sum", Qt::CaseInsensitive) == 0)
        return STATS_VARIETY;
    else if (QString::compare(sStyle , "variety", Qt::CaseInsensitive) == 0)
        return STATS_RANGE;
    else
        return -1;
}

extern "C" RM_DLL_API int GetFillMethodFromString(const char * psMethod)
{
    QString sMethod(psMethod);

    if (QString::compare(sMethod , "mincost", Qt::CaseInsensitive) == 0)
        return FILL_MINCOST;
    else if (QString::compare(sMethod , "bal", Qt::CaseInsensitive) == 0)
        return FILL_BAL;
    else if (QString::compare(sMethod , "cut", Qt::CaseInsensitive) == 0)
        return FILL_CUT;
    else
        return -1;
}

extern "C" RM_DLL_API int GetMathOpFromString(const char * psOp)
{
    QString sOp(psOp);

    if (QString::compare(sOp , "add", Qt::CaseInsensitive) == 0)
        return RM_BASIC_MATH_ADD;
    else if (QString::compare(sOp , "subtract", Qt::CaseInsensitive) == 0)
        return RM_BASIC_MATH_SUBTRACT;
    else if (QString::compare(sOp , "multiply", Qt::CaseInsensitive) == 0)
        return RM_BASIC_MATH_MULTIPLY;
    else if (QString::compare(sOp , "divide", Qt::CaseInsensitive) == 0)
        return RM_BASIC_MATH_DIVIDE;
    else if (QString::compare(sOp , "sqrt", Qt::CaseInsensitive) == 0)
        return RM_BASIC_MATH_SQRT;
    else if (QString::compare(sOp , "power", Qt::CaseInsensitive) == 0)
        return RM_BASIC_MATH_POWER;
    else if (QString::compare(sOp , "std", Qt::CaseInsensitive) == 0)
        return STATS_STD;
    else if (QString::compare(sOp, "threshproperr", Qt::CaseInsensitive) == 0)
        return RM_BASIC_MATH_THRESHOLD_PROP_ERROR;
    else
        return -1;
}

extern "C" RM_DLL_API int GetStatFromString(const char * psStat)
{
    QString sMethod(psStat);

    if (QString::compare(sMethod , "mean", Qt::CaseInsensitive) == 0)
        return STATS_MEAN;
    else if (QString::compare(sMethod , "median", Qt::CaseInsensitive) == 0)
        return STATS_MEDIAN;
    else if (QString::compare(sMethod , "majority", Qt::CaseInsensitive) == 0)
        return STATS_MAJORITY;
    else if (QString::compare(sMethod , "minority", Qt::CaseInsensitive) == 0)
        return STATS_MINORITY;
    else if (QString::compare(sMethod , "maximum", Qt::CaseInsensitive) == 0)
        return STATS_MAXIMUM;
    else if (QString::compare(sMethod , "minimum", Qt::CaseInsensitive) == 0)
        return STATS_MINIMUM;
    else if (QString::compare(sMethod , "std", Qt::CaseInsensitive) == 0)
        return STATS_STD;
    else if (QString::compare(sMethod , "sum", Qt::CaseInsensitive) == 0)
        return STATS_SUM;
    else if (QString::compare(sMethod , "variety", Qt::CaseInsensitive) == 0)
        return STATS_VARIETY;
    else if (QString::compare(sMethod , "range", Qt::CaseInsensitive) == 0)
        return STATS_RANGE;
    else
        return -1;
}

extern "C" RM_DLL_API void GetReturnCodeAsString(unsigned int eErrorCode, char * sErr)
{
    const char * pRMErr = RasterManagerException::GetReturnCodeOnlyAsString(eErrorCode);
    strncpy(sErr, pRMErr, ERRBUFFERSIZE);
    sErr[ ERRBUFFERSIZE - 1 ] = 0;
}

void RM_DLL_API printLine(QString theString)
{
    std::string sString = theString.toStdString();
    std::cout << "\n" << sString;
}

RM_DLL_API void InitCInterfaceError(char * sErr){
    strncpy(sErr, "\0", ERRBUFFERSIZE);; // Set the string to NULL.
}

RM_DLL_API void SetCInterfaceError(RasterManagerException e, char * sErr){
    QString qsErr = e.GetReturnMsgAsString();
    const QByteArray qbErr = qsErr.toLocal8Bit();
    strncpy(sErr, qbErr.data(), ERRBUFFERSIZE);
    sErr[ ERRBUFFERSIZE - 1 ] = 0;
}

} // namespace
