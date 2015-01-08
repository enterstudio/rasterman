#define MY_DLL_EXPORT

#include <gdal_priv.h>
#include <gdal_alg.h>
#include <ogr_api.h>
#include <QDebug>

#include "raster.h"
#include "rastermeta.h"
#include "rastermanager.h"
#include "rastermanager_interface.h"
#include "rastermanager_exception.h"
#include <QDir>
#include <QTextStream>
#include <string>
#include <limits>
#include <math.h>

namespace RasterManager {

int Raster::VectortoRaster(const char * sVectorSourcePath,
                   const char * sRasterOutputPath,
                   const char * psLayerName,
                   const char * psFieldName,
                   RasterMeta * p_rastermeta ){

    OGRRegisterAll();

    OGRDataSource * pDSVectorInput;
    pDSVectorInput = OGRSFDriverRegistrar::Open( sVectorSourcePath, FALSE );
    if (pDSVectorInput == NULL)
        return INPUT_FILE_ERROR;

    OGRLayer * poLayer = pDSVectorInput->GetLayerByName( psLayerName );

    // The type of the field.
    OGRFeature * feat1 = poLayer->GetFeature(0);
    int fieldindex = feat1->GetFieldIndex(psFieldName);
    OGRFieldType fieldType = feat1->GetFieldDefnRef(fieldindex)->GetType();

    // The data type we're going to use for the file
    GDALDataType OutputDataType = GDT_Byte;

    // Handle field types according to their type:
    switch (fieldType) {
    case OFTString:
        OutputCSVFile(poLayer, psFieldName, sRasterOutputPath);
        break;
    case OFTInteger:
        break;
    case OFTReal:
        OutputDataType = GDT_Float64;
    default:
        throw RasterManagerException(VECTOR_FIELD_NOT_VALID, "Type of field not recognized.");
        break;
    }

    // Get our projection and set the rastermeta accordingly.
    char *pszWKT = NULL;

    OGRSpatialReference* poSRS = poLayer->GetSpatialRef();

    poSRS->exportToWkt(&pszWKT);
    p_rastermeta->SetProjectionRef(pszWKT);
    CPLFree(pszWKT);
    OSRDestroySpatialReference(poSRS);

    // Create the output dataset for writing
    GDALDataset * pDSOutput = CreateOutputDS(sRasterOutputPath, p_rastermeta);

    OGRFeature * ogrFeat;
    std::vector<OGRGeometryH> ogrBurnGeometries;
    std::vector<double> dBurnValues;

    // Create a list of burn-in values
    poLayer->ResetReading();
    while( (ogrFeat = poLayer->GetNextFeature() ) != NULL ){

        OGRGeometry * ogrGeom = ogrFeat->GetGeometryRef();

        // No geometry found. Move along.
        if( ogrGeom == NULL )
        {
            OGRFeature::DestroyFeature( ogrFeat );
            continue;
        }

        ogrBurnGeometries.push_back( static_cast<OGRGeometryH>(ogrGeom->clone()) );

        if (fieldType == OFTString){
            dBurnValues.push_back( ogrFeat->GetFID() );
        }
        else {
            dBurnValues.push_back( ogrFeat->GetFieldAsDouble(psFieldName) );
        }
        OGRFeature::DestroyFeature( ogrFeat );

    }

    int band = 1;
    char **papszRasterizeOptions = NULL;
    papszRasterizeOptions =
        CSLSetNameValue( papszRasterizeOptions, "ALL_TOUCHED", "TRUE" );

    CPLErr err = GDALRasterizeGeometries( pDSOutput, 1, &band,
                                          ogrBurnGeometries.size(),
                                          &(ogrBurnGeometries[0]),
                                          NULL, NULL,
                                          &(dBurnValues[0]),
                                          NULL,
                                          NULL, NULL );

    ogrBurnGeometries.clear();
    dBurnValues.clear();

    // Done. Calculate stats and close file
    CalculateStats(pDSOutput->GetRasterBand(1));

    CSLDestroy(papszRasterizeOptions);
    GDALClose(pDSOutput);
    pDSVectorInput->Release();

    PrintRasterProperties(sRasterOutputPath);

    //This is where the implementation actually goes
    return PROCESS_OK;

}

int Raster::VectortoRaster(const char * sVectorSourcePath,
                           const char * sRasterOutputPath,
                           const char * sRasterTemplate,
                           const char * psLayerName,
                           const char * psFieldName){

    RasterMeta TemplateRaster(sRasterTemplate);
    return VectortoRaster(sVectorSourcePath, sRasterOutputPath, psLayerName, psFieldName, &TemplateRaster);

}

int Raster::VectortoRaster(const char * sVectorSourcePath,
                           const char * sRasterOutputPath,
                           double dCellWidth,
                           const char * psLayerName,
                           const char * psFieldName){

    OGRRegisterAll();
    OGRDataSource * pDSVectorInput;
    pDSVectorInput = OGRSFDriverRegistrar::Open( sVectorSourcePath, FALSE );
    if (pDSVectorInput == NULL)
        return INPUT_FILE_ERROR;

    OGRLayer * poLayer = pDSVectorInput->GetLayerByName( psLayerName );

    if (poLayer == NULL)
        return VECTOR_LAYER_NOT_FOUND;

    OGREnvelope psExtent;
    poLayer->GetExtent(&psExtent, TRUE);

    int nRows = (int)((psExtent.MaxY - psExtent.MinY) / fabs(dCellWidth));
    int nCols = (int)((psExtent.MaxX - psExtent.MinX) / fabs(dCellWidth));
\
    // We're going to create them without projections but the projection will need to be set int he next step.

    // For floats.
    double fNoDataValue = (double) std::numeric_limits<float>::lowest();
    RasterMeta TemplateRaster(psExtent.MaxY, psExtent.MinX, nRows, nCols, -dCellWidth, dCellWidth, fNoDataValue, "GTiff", GDT_Float32, "");

    pDSVectorInput->Release();
    return VectortoRaster(sVectorSourcePath, sRasterOutputPath, psLayerName, psFieldName, &TemplateRaster);

}

void Raster::OutputCSVFile(OGRLayer * poLayer, const char * psFieldName, const char * sRasterOutputPath){

    int featurecount = poLayer->GetFeatureCount();

    // use the filename with CSV added onto the end.
    QFileInfo sOutputFileInfo(sRasterOutputPath);
    QDir sNewDir = QDir(sOutputFileInfo.absolutePath());
    QString sCSVFullPath = sNewDir.filePath(sOutputFileInfo.completeBaseName() + ".csv");

    QFile csvFile(sCSVFullPath);

    if (csvFile.open(QFile::WriteOnly|QFile::Truncate))
    {
      QTextStream stream(&csvFile);

      //    Write CSV file header
      stream << "\"index\", " << "\""<< psFieldName << "\""<< "\n"; // this writes first line with two columns

      OGRFeature *poFeature;
      poLayer->ResetReading();
      poLayer->SetSpatialFilter(NULL);
      while( (poFeature = poLayer->GetNextFeature()) != NULL){
          const char * sFieldVal = poFeature->GetFieldAsString(psFieldName);
          //        write line to file
          stream << poFeature->GetFID() << ", " << "\"" << sFieldVal << "\"" << "\n"; // this writes first line with two columns
          OGRFeature::DestroyFeature( poFeature );
      }

      csvFile.close();
    }
    return;

}

/**
<PolygonFilePath> <PolygonLayerName> <TemplateRasterPath> <OutputRasterPath>
<PolygonFilePath> <PolygonLayerName> <CellSizeEtc> <OutputRasterPath>

Output type should be detected.
1) If string then byte with CSV legend
2) If integer then byte with no legend
3) if other than float32

Validations:
must be polygon geometry type (and not point or line).
must have spatial reference.

**/




}