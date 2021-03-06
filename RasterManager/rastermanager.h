#ifndef TOOLS_H
#define TOOLS_H

#include "rastermanager_global.h"
#include "gdal_priv.h"
#include "benchmark.h"
#include <QString>


namespace RasterManager {

/**
 * @brief CalculateStats
 * @param pRasterBand
 */
void CalculateStats(GDALRasterBand * pRasterBand);

/**
 * @brief CheckFile
 * @param sFile
 * @param bMustExist
 */
void RM_DLL_API CheckFile(QString sFile, bool bMustExist);

/**
 * @brief G3etPrecision
 * @param num
 * @return
 */
int GetPrecision(double num);

/**
 * @brief LibCheck
 */
void RM_DLL_API LibCheck();

}


#endif // TOOLS_H
