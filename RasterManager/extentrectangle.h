#ifndef EXTENTRECTANGLE_H
#define EXTENTRECTANGLE_H

#include "rastermanager_global.h"
//#include "raster.h"
//#include "rastermeta.h"

class Raster;
class RasterMeta;

namespace RasterManager {

class DLL_API ExtentRectangle
{
public:
    /**
     * @brief ExtentRectangle
     *
     * Create a rectangle from top, left, bottom and right values
     *
     * @param fTop float
     * @param fLeft float
     * @param fRight float
     * @param fBottom float
     * @param dCellHeight
     * @param dCellWidth
     *
     */
    ExtentRectangle();

    /**
     * @brief Create an ExtentRectangle with individual arguments
     * @param fTop
     * @param fLeft
     * @param nRows
     * @param nCols
     * @param dCellHeight
     * @param dCellWidth
     */
    ExtentRectangle(double fTop,
                    double fLeft,
                    int nRows,
                    int nCols,
                    double dCellHeight,
                    double dCellWidth);

      /**
       * @brief Create an ExtentRectangle from an existing raster file path
       * @param psFilePath full absolute file path to an existing raster
       */
    ExtentRectangle(const char * psFilePath);

    /**
     * @brief Copy constructor for creating an extent rectangle from another extent rectangle object
     * @param pRaster an Existing extent rectangle
     */
    ExtentRectangle(ExtentRectangle &source);

    /**
     * @brief Assignment operator for setting the properties of an extent rectangle from an existing object
     * @param pRaster an Existing extent rectangle
     */
    void operator=(ExtentRectangle &source);

    /**
     * @brief Init
     * @param fTop
     * @param fLeft
     * @param nRows
     * @param nCols
     * @param dCellHeight
     * @param dCellWidth
     */
    void Init(double fTop,
              double fLeft,
              int nRows,
              int nCols,
              double dCellHeight,
              double dCellWidth);




    /**
     * @brief Union a rectangle with this one
     *
     * @param ExtentRectangle a rectangle to add to this one
     */
    void Union(ExtentRectangle * aRectangle);

    /**
     * @brief Union: Overload of the union, accepting a raster
     * @param pRaster
     */
    //void Union(Raster * pRaster);

    /**
     * @brief GetRowTranslation: Get the row translation between two rectangles
     * @param aRectangle
     */
    int GetRowTranslation(ExtentRectangle * aRectangle);

    /**
     * @brief GetColTranslation: Get the row translation between two rectangles
     * @param aRectangle
     * @return
     */
    int GetColTranslation(ExtentRectangle * aRectangle);

    /**
     * @brief GetTop
     * @return
     */
    double GetTop();
    /**
     * @brief GetLeft
     * @return
     */
    double GetLeft();
    /**
     * @brief GetRight
     * @return
     */
    double GetRight();
    /**
     * @brief GetBottom
     * @return
     */
    double GetBottom();
    /**
     * @brief GetWidth
     * @return
     */
    double GetWidth();
    /**
     * @brief GetHeight
     * @return
     */
    double GetHeight();
    /**
     * @brief GetRows
     * @return
     */
    int GetRows();
    /**
     * @brief GetCols
     * @return
     */
    int GetCols();



private:
    double top;
    double left;
    int cols;
    int rows;

    double cellHeight;
    double cellWidth;

};
}

#endif // EXTENTRECTANGLE_H
