/*  =========================================================================

    Program:   The Medical Simulation Markup Language
    Module:    Operators, MiscMeshOperators
  Authors:   Markus Stoll, Stefan Suwelack, Nicolai Schoch

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.

    =========================================================================*/

#include "MiscMeshOperators.h"
#include <iostream>
#include <sstream>

#include <string.h>

#include <stdio.h>

#include "vtkUnstructuredGrid.h"

#include "IOHelper.h"


#include <vtkTetra.h>
#include <vtkCellArray.h>
#include <vtkSmartPointer.h>
#include <vtkDataSetMapper.h>
#include <vtkActor.h>
#include <vtkRenderWindow.h>
#include <vtkRenderer.h>
#include <vtkRenderWindowInteractor.h>
#include <vtkPointData.h>
#include <vtkIdList.h>
#include <vtkVertexGlyphFilter.h>
#include <vtkPoints.h>

#include "vtkSTLWriter.h"
#include "vtkPolyData.h"
#include "vtkPoints.h"
#include "vtkCellArray.h"
#include <vtkCellData.h>
#include "vtkCleanPolyData.h"
#include "vtkUnsignedIntArray.h"

#include "vtkFloatArray.h"
#include "vtkIntArray.h"
#include "vtkCellData.h"
#include "vtkSTLReader.h"
#include "vtkKdTreePointLocator.h"
#include "vtkVoxelModeller.h"
#include "vtkPNGWriter.h"

#include <vtkDataSetSurfaceFilter.h>
#include "vtkLongLongArray.h"
#include "vtkDoubleArray.h"


#include <vtkUnstructuredGridGeometryFilter.h>
#include "vtkDataSetSurfaceFilter.h"
#include "vtkUnstructuredGridGeometryFilter.h"
#include <vtkImplicitPolyDataDistance.h>

#include <vtkImageData.h>
#include <vtkPolyDataToImageStencil.h>
#include <vtkImageStencil.h>
#include "vtkFeatureEdges.h"
#include "vtkFillHolesFilter.h"
#include "vtkCleanPolyData.h"
#include "vtkAppendFilter.h"

#include "vtkTetra.h"
#include "vtkTriangle.h"
#include "vtkGenericCell.h"
#include "vtkCellLocator.h"
#include "vtkPolyDataConnectivityFilter.h"



#include <vtkThreshold.h>
#include <vtkMergeCells.h>

#include <boost/filesystem.hpp>
#include <boost/lexical_cast.hpp>

#include "PostProcessingOperators.h"
#include "../vtk6_compat.h"

#include "../common/log.h"

#include <vtkWindowedSincPolyDataFilter.h>
#include <vtkPolyDataMapper.h>

#include <vtkImageDilateErode3D.h>
#include <vtkImageThreshold.h>
#include <vtkPolyDataNormals.h>
#include <vtkMath.h>
#include <math.h>


using namespace std;

namespace MSML {
namespace MiscMeshOperators {
std::string ConvertSTLToVTK(std::string infile, std::string outfile)
{
    ConvertSTLToVTK( infile.c_str(), outfile.c_str());
	
    return outfile;
}

bool ConvertSTLToVTK(const char* infile, const char* outfile)
{
    vtkSmartPointer<vtkPolyData> mesh = vtkSmartPointer<vtkPolyData>::New();
    ConvertSTLToVTK( infile, mesh);
    return IOHelper::VTKWritePolyData(outfile, mesh);
}

bool ConvertSTLToVTK(const char* infile, vtkPolyData* outputMesh)
{
    vtkSmartPointer<vtkSTLReader> reader =
        vtkSmartPointer<vtkSTLReader>::New();
    reader->SetFileName(infile);
    reader->Update();

    //deep copy
    outputMesh->DeepCopy(reader->GetOutput());

    return true;
}

std::string ConvertVTKToSTL(std::string infile, std::string outfile)
{
	ConvertVTKToVTU( infile.c_str(), outfile.c_str());

	return outfile;
}

bool ConvertVTKToSTL(const char* infile, const char* outfile)
{
    vtkSmartPointer<vtkPolyData> currentPolydata = IOHelper::VTKReadPolyData(infile);


    vtkSmartPointer<vtkSTLWriter> writer =
        vtkSmartPointer<vtkSTLWriter>::New();
    writer->SetFileTypeToBinary();
    writer->SetFileName(outfile);
    __SetInput(writer, currentPolydata);
    writer->Write();

    log_debug() <<"STL file written" << std::endl;
    return true;
}

//---------------------------- start of new by Nico on 2014-05-10.
std::string ConvertVTKToVTU(std::string infile, std::string outfile)
{
	ConvertVTKToVTU( infile.c_str(), outfile.c_str());
	return outfile;
}

bool ConvertVTKToVTU(const char* infile, const char* outfile )
{
  vtkSmartPointer<vtkUnstructuredGrid> grid = IOHelper::VTKReadUnstructuredGrid(infile);
  boost::filesystem::path filePath(outfile);
  if (filePath.extension() != ".vtu")
    log_error() << "File extension for XMLUnstructuredGridWriter must be .vtu" << std::endl;

  IOHelper::VTKWriteUnstructuredGrid(outfile, grid, true);

  return true;
}

/*
bool ConvertVTKToVTU(vtkUnstructuredGrid* inputMesh, vtkUnstructuredGrid* outputMesh) // is this needed at all?!
{
	  // Combine the two data sets
	  vtkSmartPointer<vtkAppendFilter> appendFilter = vtkSmartPointer<vtkAppendFilter>::New();

	  __AddInput(appendFilter, inputPolyData);

	  appendFilter->Update();

	  outputMesh->DeepCopy(appendFilter->GetOutput());

	 return true;
}
*/
//---------------------------- end of new by Nico on 2014-05-10.

bool ConvertVTKToOFF(vtkPolyData* inputMesh, const char* outfile)
{
    vtkPoints* thePoints = inputMesh->GetPoints();
    vtkIdType noPoints = thePoints->GetNumberOfPoints();

    //open file and generate textstream
    std::ofstream file(outfile);

    if (!file.is_open())
    {
        return false;
    }


    //write Header
    file << "OFF\n"
         <<	 noPoints << " " << inputMesh->GetNumberOfCells() <<" 0\n";

    log_debug() <<" write points" << std::endl;;

    double* currentPoint;

    for(int i=0; i<noPoints; i++)
    {
        currentPoint = thePoints->GetPoint(i);
        file<<currentPoint[0]<<" "<<currentPoint[1]<<" "<<currentPoint[2]<<"\n";
    }

    inputMesh->BuildCells(); //this call caused an access violation when this method was part of a SHARED (.dll) library and the ConvertVTKtoOff was called from outside the dll .


    vtkIdType* currentCellPoints;
    //currentCellPoints = new vtkIdType[3];
    vtkIdType numberOfNodes=3;

    for(int i=0; i<inputMesh->GetNumberOfCells(); i++)
    {
        inputMesh->GetCellPoints(i,numberOfNodes,currentCellPoints);
        file<<3<<" "<<currentCellPoints[0]<<" "<<currentCellPoints[1]<<" "<<currentCellPoints[2]<<"\n";
    }

    // delete [] currentCellPoints;

    file.close();

    return true; //todo: add useful return value
}


bool ConvertInpToVTK(const char* infile, const char* outfile)
{
  throw "not implemented";
  return true;
}

bool ConvertInpToVTK(const char* infile, vtkUnstructuredGrid* outputMesh )
{
  throw "not implemented";
  return true;
}

std::string VTKToInp( std::string infile, std::string outfile)
{
	VTKToInp( infile.c_str(),  outfile.c_str());
	return outfile;
}

bool VTKToInp( const char* infile, const char* outfile)
{
    return VTKToInp(IOHelper::VTKReadUnstructuredGrid(infile),  outfile);
}

bool VTKToInp( vtkUnstructuredGrid* inputMesh, const char* outfile)
{
    //open file and generate textstream
    //		 QFile file(GetCompleteFilename(filename));
    //		 if (!file.open(QFile::WriteOnly | QFile::Truncate))
    //			 return false;
    //
    //		 QTextStream out(&file);

    std::ofstream out(outfile);

    if (!out.is_open())
    {
        return false;
    }

    out << ConvertVTKMeshToAbaqusMeshString(inputMesh, std::string("Part1"), std::string("Neo-Hookean"));
    return true;
}

std::string ExtractAllSurfacesByMaterial(const char* infile, const char* outfile, bool theCutIntoPieces)
{
    vtkSmartPointer<vtkUnstructuredGrid> inputGrid = IOHelper::VTKReadUnstructuredGrid(infile);

    //cut model in pieces
    if (theCutIntoPieces)
    {
        cerr << "Experimental !" << std::endl;
        cerr << "Cutting" << std::endl;
        map<int, int> belongstTo;
        std::map<std::pair<int,int>, int> newPrivatePoints; // <oldKey, MaterialId> => newKey
        vtkIntArray* cellMaterialArray0 = (vtkIntArray*) inputGrid->GetCellData()->GetArray("Materials");


        for (int kk= 0; kk<10; kk++)
        {
            belongstTo.clear();

            for(vtkIdType i = 0; i < inputGrid->GetNumberOfCells(); i++)
            {
                ;//cerr << "Checking element " << i << std::endl;
                vtkCell* currentCell = inputGrid->GetCell(i);
                vtkIdList* currentPointIds = currentCell->GetPointIds();

                for(vtkIdType pointId = 0; pointId < currentPointIds->GetNumberOfIds(); pointId++)
                {
                    vtkIdType currentPoint = currentPointIds->GetId(pointId);

                    if (belongstTo[currentPoint] == 0)
                    {
                        belongstTo[currentPoint] = (int)*cellMaterialArray0->GetTuple(i);
                        ;//cerr << "   empty <- " << (int)*cellMaterialArray0->GetTuple(i) << std::endl;
                    }

                    else if (belongstTo[currentPoint] == (int)*cellMaterialArray0->GetTuple(i))
                        ;//cerr << "   same <-" << (int)*cellMaterialArray0->GetTuple(i) << std::endl;

                    else
                    {
                        cerr << "   replace needed" << pointId << std::endl;
                        double* cords = currentCell->GetPoints()->GetPoint(pointId);

                        if ( newPrivatePoints[std::make_pair(currentPoint,(int)*cellMaterialArray0->GetTuple(i))] == 0 )
                        {
                            vtkIdType aNewPointId = inputGrid->GetPoints()->InsertNextPoint(cords[0], cords[1], cords[2] + 0.01 * (int)*cellMaterialArray0->GetTuple(i));
                            *(currentPointIds->GetPointer(pointId)) = aNewPointId;
                            inputGrid->ReplaceCell(i,currentPointIds->GetNumberOfIds(), currentPointIds->GetPointer(0));
                            newPrivatePoints[std::make_pair(currentPoint,(int)*cellMaterialArray0->GetTuple(i))] = aNewPointId;
                        }

                        else
                        {
                            *(currentPointIds->GetPointer(pointId)) = newPrivatePoints[std::make_pair(currentPoint,(int)*cellMaterialArray0->GetTuple(i))] ;
                            inputGrid->ReplaceCell(i,currentPointIds->GetNumberOfIds(), currentPointIds->GetPointer(0));
                        }

                    }
                }
            }

            log_debug() << "There are " << inputGrid->GetNumberOfCells()  << std::endl;
        }

        IOHelper::VTKWriteUnstructuredGrid((string(outfile) + "-cut.vtk").c_str(), inputGrid);
    }

    //done cutting



    log_debug() << "There are " << inputGrid->GetNumberOfCells()  << " cells before thresholding." << std::endl;

    //get alle surfaces
    vtkIntArray* cellMaterialArray = (vtkIntArray*) inputGrid->GetCellData()->GetArray("Materials");
    map<int,int>* cellDataHist = createHist(cellMaterialArray);


    //create a surface for each material
    std::vector<vtkSmartPointer<vtkPolyData> > surfaces;

    for (map<int,int>::iterator it=cellDataHist->begin(); it!=cellDataHist->end(); it++) //filter for each material
    {
        log_debug() << it->second << " cells of MaterialId=" << it->first << " found." << std::endl;
        vtkSmartPointer<vtkThreshold> threshold = vtkSmartPointer<vtkThreshold>::New();
        __SetInput(threshold, inputGrid);
        threshold->ThresholdBetween(it->first, it->first);
        threshold->SetInputArrayToProcess(0, 0, 0, vtkDataObject::FIELD_ASSOCIATION_CELLS, "Materials");
        threshold->Update();

        //debug out surface
        stringstream itFirst;
        itFirst << it->first;
        //  IOHelper::VTKWriteUnstructuredGrid((string(outfile) + "-volume" + itFirst.str() + ".vtk").c_str(), threshold->GetOutput());

        log_debug() << "There are " << threshold->GetOutput()->GetNumberOfCells() << " cells after thresholding with " <<  it->first << std::endl;
        //extract surface
        vtkSmartPointer<vtkPolyData> mesh = vtkSmartPointer<vtkPolyData>::New();

        string error_message;
        bool result = ExtractSurfaceMesh(threshold->GetOutput(), mesh);

        //debug out surface
        //   IOHelper::VTKWritePolyData((string(outfile) + "-surface" + itFirst.str() + ".vtk").c_str(), mesh);

        surfaces.push_back(mesh);
    }

    //merge all surfaces back into a single unstuctured grid - togther with volume cells. Points are unified.
    int numberOfMehes = surfaces.size()+1;
    int numberOfPoints=inputGrid->GetNumberOfPoints();
    int numberOfCells=inputGrid->GetNumberOfCells();

    for (int i=0; i<surfaces.size(); i++)
    {
        numberOfPoints+=surfaces[i]->GetNumberOfPoints();
        numberOfCells+=surfaces[i]->GetNumberOfCells();
    }

    vtkSmartPointer<vtkMergeCells> merger = vtkSmartPointer<vtkMergeCells>::New();
    vtkSmartPointer<vtkUnstructuredGrid> unionMesh = vtkSmartPointer<vtkUnstructuredGrid>::New();
    merger->SetUnstructuredGrid(unionMesh);
    merger->SetTotalNumberOfCells(numberOfCells);
    merger->SetTotalNumberOfPoints(numberOfPoints);
    merger->MergeDuplicatePointsOn();
    merger->SetPointMergeTolerance(0.00001);
    merger->SetTotalNumberOfDataSets(numberOfMehes);

    //merge it
    for (int i=0; i<surfaces.size(); i++)
    {
        int error = merger->MergeDataSet(surfaces[i]);

        if (error)
        {
            cerr << "vtkMergeCells error during MergeDataSet with surfaces " << i;
            exit(2);
        }
    }

    int error = merger->MergeDataSet(inputGrid);

    if (error)
    {
        cerr << "vtkMergeCells error during MergeDataSet with volume.";
        exit(2);
    }

    merger->Finish();

    //save the merged data
    IOHelper::VTKWriteUnstructuredGrid(outfile, unionMesh);
    return outfile;
}


map<int,int>* createHist(vtkDataArray* theVtkDataArray)
{
    map<int,int>* hist = new map<int,int>();
    int N = theVtkDataArray->GetNumberOfTuples();

    for (int i=0; i<N; i++)
    {
        double* v = theVtkDataArray->GetTuple(i);

        //hist[(int)*v] =  hist[(int)*v] + 1 ;
        if (hist->find(*v)!=hist->end())
        {
            hist->at(*v) = hist->at(*v) + 1;
        }

        else
        {
            hist->insert(pair<int,int>(*v,1));
        }
    }

    return hist;
}

std::string  ExtractSurfaceMesh( std::string infile, std::string outfile)
{
    ExtractSurfaceMesh(infile.c_str(), outfile.c_str());
    return outfile;
}

bool ExtractSurfaceMesh( const char* infile, const char* outfile)
{
    vtkSmartPointer<vtkPolyData> mesh =
        vtkSmartPointer<vtkPolyData>::New();

    bool result = ExtractSurfaceMesh( IOHelper::VTKReadUnstructuredGrid(infile), mesh);

    //save the subdivided polydata
    IOHelper::VTKWritePolyData(outfile, mesh);
    return result;
}
/*
 Calculate a gradient on given surface, in x-direction and running over values-array
 Argument steps sets resolution of gradient (maximum number of separate regions).
*/
vector<double> GradientOnSurface(const char* inFile, vector<double> values, int steps)
{	
	vtkSmartPointer<vtkUnstructuredGrid> grid = IOHelper::VTKReadUnstructuredGrid(inFile);
	vector<double> gradient(grid->GetNumberOfCells());

	//slide box accross surface and pick cells using cell locator
	vtkSmartPointer<vtkCellLocator> locator = vtkSmartPointer<vtkCellLocator>::New();	
	locator->SetNumberOfCellsPerBucket(1);
	locator->SetNumberOfCellsPerNode(1);
	locator->SetDataSet(grid);
	locator->BuildLocator();
	
	double* boundingBox = grid->GetBounds();
	//variate x-range of bounding box (first two values)
	double xMin = boundingBox[0];
	double xMax = boundingBox[1];	
	
	int num = grid->GetNumberOfCells();
	//vtkSmartPointer<vtkFloatArray> gradientCellData = vtkSmartPointer<vtkFloatArray>::New();
	//gradientCellData->SetName("gradient");
	//gradientCellData->SetNumberOfTuples(num);
	//grid->GetCellData()->AddArray(gradientCellData);
		
	double stepWidth = (xMax-xMin)/steps;

	//compute values for each region
	//how many valueSteps for eachRegion
	int valueRegionSteps = steps/(values.size()-1);

	vector<double> vals;
	for(int i=0;i<values.size();i++)
	{
		double mapValueOffset = (values[i+1]-values[i])/valueRegionSteps;
		log_debug()<<"region step: "<<mapValueOffset<<std::endl;
		double currentValue = values[i];
		log_debug()<<"region steps: "<<valueRegionSteps<<std::endl;
		for(int step=0;step<valueRegionSteps;++step)
		{			
			log_debug()<<"current value: "<<currentValue<<std::endl;			
			vals.push_back(currentValue);
			currentValue+=mapValueOffset;
		}
	}
	int regionCounter = 0;
	double regionValue = 0;
	log_debug()<<"num vals: "<<vals.size()<<std::endl;
	double xOffset = xMin;
	for(int i=0;i<steps;++i)
	{		
		regionValue = vals[i];
		//update the bbox
		boundingBox[0] = xOffset;
		boundingBox[1] = xOffset+stepWidth;

		log_debug()<<"xmin: "<<boundingBox[0]<<" xmax: "<<boundingBox[1]<<std::endl;

		vtkSmartPointer<vtkIdList> cellsInBBox = vtkSmartPointer<vtkIdList>::New();		
		locator->FindCellsWithinBounds(boundingBox,cellsInBBox);		
		log_debug()<<"value: "<<regionValue<<std::endl;
		log_debug()<<"cells: "<<cellsInBBox->GetNumberOfIds()<<std::endl;
		for(int i=0;i<cellsInBBox->GetNumberOfIds();++i)
		{		
			//gradientCellData->SetTuple1(cellsInBBox->GetId(i),regionValue);			
			gradient[cellsInBBox->GetId(i)]=regionValue;
		}
		regionCounter++;
		xOffset+=stepWidth;
	}
	
	//IOHelper::VTKWriteUnstructuredGrid((string(inFile)+"_gradient.vtk").c_str(),grid);
	return gradient;
}


/*
	Get vector of all material numbers used in given mesh
*/
vector<unsigned int> GetMaterialNumbersFromMesh(const char* infile)
{
	 log_debug()<<"GetMaterialNumbersFromMesh"<<std::endl;
	 vtkSmartPointer<vtkUnstructuredGrid> inputGrid = IOHelper::VTKReadUnstructuredGrid(infile);

	 vtkIntArray* cellMaterialArray = (vtkIntArray*) inputGrid->GetCellData()->GetArray("Materials");
	 std::map<int,int> *matMap = createHist(cellMaterialArray);
	 vector<unsigned int> materialNumbers; 

	 for(map<int,int>::iterator it = matMap->begin(); it != matMap->end(); ++it) 
	 {				 
		 materialNumbers.push_back(it->first);
	 }
	 return materialNumbers;
}

/*
	Debug print a vector
*/
bool DebugPrint(vector<int> to_print)
{
	cerr<<"Debug print of vector:"<<std::endl;
	for(vector<int>::iterator it = to_print.begin(); it != to_print.end(); ++it)
	{    
		cerr<<*it<<" ";
	}
	cerr<<std::endl;
	return true;
}
/*
Calculate angle between two vectors. Output angle as radians.
Copied from here:
http://review.source.kitware.com/#/t/3804/
(This function should be included in recent versions of vtk)
*/
double AngleBetweenVectors(const double v1[3], const double v2[3])
{
	double cross[3];
	vtkMath::Cross(v1, v2, cross);
	return atan2(vtkMath::Norm(cross), vtkMath::Dot(v1, v2));
}
/*
   Select surface elements from a mesh based on normal direction of surface elements.
   All surface elements having a normal vector facing in same direction (including a margin) as desired input normal vector 
   will be selected and saved to outfile.
*/
string SurfaceFromVolumeAndNormalDirection(const char* infile, const char* outfile, std::vector<double> desiredNormalDir, double margin)
{
	vtkSmartPointer<vtkPolyData> mesh = IOHelper::VTKReadPolyData(infile);

	vtkSmartPointer<vtkPolyDataNormals> normalGenerator = vtkSmartPointer<vtkPolyDataNormals>::New();
	__SetInput(normalGenerator,mesh);
	normalGenerator->ComputePointNormalsOn();
	normalGenerator->ComputeCellNormalsOn();
	normalGenerator->Update();

	log_info()<<mesh->GetNumberOfCells()<<" cells before filtering with ("
			  <<desiredNormalDir[0]<<","<<desiredNormalDir[1]<<","<<desiredNormalDir[2]<<") margin:"<<margin<<std::endl;

	vtkSmartPointer<vtkPolyData> normalsMesh = normalGenerator->GetOutput();

	vtkSmartPointer<vtkCellData> cellData = normalsMesh->GetCellData();
    vtkSmartPointer<vtkDataArray> cellNormals = cellData->GetNormals();	
	int num = cellData->GetNumberOfTuples();
	vtkSmartPointer<vtkFloatArray> selNormals = vtkSmartPointer<vtkFloatArray>::New();
	selNormals->SetName("selNormals");
	selNormals->SetNumberOfTuples(num);
	normalsMesh->GetCellData()->AddArray(selNormals);
	
	double* inpNormal = &desiredNormalDir[0];	
	vtkSmartPointer<vtkThreshold> threshold = vtkSmartPointer<vtkThreshold>::New();
	
	for(int i=0;i<num;++i)
	{
		double *normal = cellNormals->GetTuple(i);
		double normAngle = vtkMath::DegreesFromRadians(AngleBetweenVectors(normal,inpNormal));					
		if(normAngle<margin)
		{
			selNormals->SetValue(i,normAngle);
		}else
		{
			selNormals->SetValue(i,180);
		}
	}
	__SetInput(threshold, normalsMesh);
	
    threshold->ThresholdBetween(0,margin);
    threshold->SetInputArrayToProcess(0, 0, 0, vtkDataObject::FIELD_ASSOCIATION_CELLS, "selNormals");
    threshold->Update();
	vtkSmartPointer<vtkUnstructuredGrid> extractedMesh = threshold->GetOutput();
	log_info()<<extractedMesh->GetNumberOfCells()<<" cells after filtering"<<std::endl;

	IOHelper::VTKWriteUnstructuredGrid(outfile,extractedMesh);
	
	return outfile;
}


string ExtractBoundarySurfaceByMaterials(const char* infile, const char* outfile, 
										 int baseMeshMaterial, std::vector<int> otherMeshesMaterial)
{	
	vtkSmartPointer<vtkUnstructuredGrid> grid = IOHelper::VTKReadUnstructuredGrid(infile);		
	vtkDataArray* cellsData = grid->GetCellData()->GetArray("Materials");	
	vtkSmartPointer<vtkPoints> p = vtkSmartPointer<vtkPoints>::New();
	vtkSmartPointer<vtkCellArray> newGridCells = vtkSmartPointer<vtkCellArray>::New();	
	vtkSmartPointer<vtkUnstructuredGrid> uGrid = vtkSmartPointer<vtkUnstructuredGrid>::New();
	vtkSmartPointer<vtkCellArray> triangles = vtkSmartPointer<vtkCellArray>::New();

	for(vtkIdType cellId = 0; cellId<grid->GetNumberOfCells();cellId++)
	{		
		if(cellsData->GetTuple1(cellId)!=baseMeshMaterial)continue;
		if(grid->GetCellType(cellId)!=10)continue;
		vtkSmartPointer<vtkIdList> cellPointIds = vtkSmartPointer<vtkIdList>::New();
		grid->GetCellPoints(cellId, cellPointIds); 		
		std::list<vtkIdType> neighbors;		
		
		vtkSmartPointer<vtkGenericCell> cell2 = vtkSmartPointer<vtkGenericCell>::New();	
		grid->GetCell(cellId,cell2);
		for(int faceIndex = 0;faceIndex<cell2->GetNumberOfFaces();faceIndex++)
		{
			vtkCell *faceCell = cell2->GetFace(faceIndex);
			vtkIdList *facePoints = faceCell->GetPointIds();
			vtkSmartPointer<vtkIdList> idList =  vtkSmartPointer<vtkIdList>::New();
			for(int fpi=0;fpi<faceCell->GetNumberOfPoints();fpi++)
			{				
				idList->InsertNextId(facePoints->GetId(fpi)); 
			}			
			vtkSmartPointer<vtkIdList> neighborCellIds = vtkSmartPointer<vtkIdList>::New(); 
			grid->GetCellNeighbors(cellId, idList, neighborCellIds);
			for(vtkIdType j = 0; j < neighborCellIds->GetNumberOfIds(); j++)
			{
				vtkIdType neighbourId = neighborCellIds->GetId(j);
				double neighbourMat = cellsData->GetTuple1(neighbourId);
				//only push if mat is ok
				if(std::find(otherMeshesMaterial.begin(), otherMeshesMaterial.end(), neighbourMat)!=otherMeshesMaterial.end())
				{										
					//save points and faces to new grid							
					vtkPoints *facePointCoords = faceCell->GetPoints();
					vtkSmartPointer<vtkTriangle> triangle = vtkSmartPointer<vtkTriangle>::New();
					for(int coordsIndex=0;coordsIndex<faceCell->GetNumberOfPoints();coordsIndex++)
					{						
						vtkIdType slotId = p->InsertNextPoint(facePointCoords->GetPoint(coordsIndex));
						triangle->GetPointIds()->SetId(coordsIndex,slotId);
					}																
					triangles->InsertNextCell(triangle);
				}		
			}
		}		
	}
	vtkSmartPointer<vtkPolyData> polydata = vtkSmartPointer<vtkPolyData>::New();
	polydata->SetPoints(p);
	polydata->SetPolys(triangles);

	int num = polydata->GetNumberOfCells();
	vtkSmartPointer<vtkIntArray> materials = vtkSmartPointer<vtkIntArray>::New();
	materials->SetNumberOfValues(num);
	materials->SetName("Materials");
	//TODO: no function to fill all values at once?
	for(int i=0;i<num;++i)
	{
		materials->SetValue(i,baseMeshMaterial);
	}
	polydata->GetCellData()->SetScalars(materials);

	vtkSmartPointer<vtkCleanPolyData> cleaner = vtkSmartPointer<vtkCleanPolyData>::New();
	__SetInput(cleaner,polydata);
	cleaner->Update();

	vtkSmartPointer<vtkPolyDataConnectivityFilter> connectivity = vtkSmartPointer<vtkPolyDataConnectivityFilter>::New();
	connectivity->SetExtractionModeToLargestRegion();
	__SetInput(connectivity,cleaner->GetOutput());
	connectivity->Update();	

	vtkSmartPointer<vtkAppendFilter> appendFilter = vtkSmartPointer<vtkAppendFilter>::New();
	__SetInput(appendFilter,connectivity->GetOutput());
	appendFilter->Update();

	IOHelper::VTKWriteUnstructuredGrid(outfile,appendFilter->GetOutput());		
	return outfile;
}


/*
	For given image: replace occurence of every value in toReplace-vector  by replaceBy
	Input: Image with values 1,2,3
	toReplace-Argument: 1,2
	replaceBy-Argument: 42
	Output: Image with values 3,42
*/
string ReplaceMaterialID(const char* infile, const char* outfile, std::vector<int> toReplace, int replaceBy)
{
	vtkSmartPointer<vtkImageData> image = IOHelper::VTKReadImage(infile); 
    vtkDataArray* pd = image->GetPointData()->GetScalars();
	double replaceValue = replaceBy;
    for (int i=0; i<pd->GetNumberOfTuples();i++)
    {		
		double* value = pd->GetTuple(i);
		//if value is contained in toReplace vector, replace by replaceBy value
		if(std::find(toReplace.begin(), toReplace.end(), *value)!=toReplace.end())
		{	
			pd->SetTuple(i,  &replaceValue);
		}        
    }
	IOHelper::VTKWriteImage(outfile,image);
	return outfile;
}

vector<double> BoundsFromMaterialID(const char* infile, int materialID)
{
	vtkSmartPointer<vtkUnstructuredGrid> dataGrid = IOHelper::VTKReadUnstructuredGrid(infile); 
	vtkSmartPointer<vtkThreshold> threshold = vtkSmartPointer<vtkThreshold>::New();
    __SetInput(threshold, dataGrid);
    threshold->ThresholdBetween(materialID,materialID);
    threshold->SetInputArrayToProcess(0, 0, 0, vtkDataObject::FIELD_ASSOCIATION_CELLS, "Materials");
    threshold->Update();
	
	vtkSmartPointer<vtkUnstructuredGrid> mesh = threshold->GetOutput();	
	double *bounds = mesh->GetBounds();
	//not really nice
	std::vector<double> vecBounds(bounds, bounds + 6);	
	return vecBounds;
}



/*
	Select volumes by their material id. All volumes with id given in group vector will be selected, 
	merged, and saved.
*/
string SelectVolumesByMaterialID(const char* infile, const char* outfile, std::vector<int> group)
{
	vtkSmartPointer<vtkUnstructuredGrid> inputGrid = IOHelper::VTKReadUnstructuredGrid(infile);	
	log_debug()<<"selecting started..."<<std::endl;

	int totalNumberVolumes = 0;
	int totalNumberCells = 0;
	int totalNumberPoints = 0;
	//collect selected volumes here
	std::vector<vtkSmartPointer<vtkUnstructuredGrid> > volumes;

	//get volumes, sum up their total number of cells/points --> needed by the merger
	//total number of cells, points must be known in advance, before merging is started
	for (std::vector<int>::iterator it = group.begin() ; it != group.end(); ++it)
	{
		int matId = *it;
		log_debug() << "selecting cells of id: "<< matId <<std::endl;
        vtkSmartPointer<vtkThreshold> threshold = vtkSmartPointer<vtkThreshold>::New();
        __SetInput(threshold, inputGrid);
		threshold->ThresholdBetween(matId,matId);
        threshold->SetInputArrayToProcess(0, 0, 0, vtkDataObject::FIELD_ASSOCIATION_CELLS, "Materials");
        threshold->Update();       
		log_debug() << "There are " << threshold->GetOutput()->GetNumberOfCells() << " cells after thresholding with " <<  matId << std::endl;     
		//extract volume
		vtkSmartPointer<vtkUnstructuredGrid> mesh = vtkSmartPointer<vtkUnstructuredGrid>::New();
		mesh->DeepCopy(threshold->GetOutput());	
		volumes.push_back(mesh);
		totalNumberVolumes++;
		totalNumberPoints+=mesh->GetNumberOfPoints();
		totalNumberCells+=mesh->GetNumberOfCells();		
	}  	
	//set up the merger
	vtkSmartPointer<vtkUnstructuredGrid> unionMesh = vtkSmartPointer<vtkUnstructuredGrid>::New();	
	vtkSmartPointer<vtkMergeCells> merger = vtkSmartPointer<vtkMergeCells>::New();
	merger->SetUnstructuredGrid(unionMesh);
	merger->MergeDuplicatePointsOn();
	merger->SetPointMergeTolerance(0.00001);	
	merger->SetTotalNumberOfCells(totalNumberCells);
	merger->SetTotalNumberOfPoints(totalNumberPoints);
	merger->SetTotalNumberOfDataSets(totalNumberVolumes);
	for(std::vector<vtkSmartPointer<vtkUnstructuredGrid> >::iterator it = volumes.begin(); it!=volumes.end(); ++it)
	{
		merger->MergeDataSet(*it);
	}
	merger->Finish();	

	//save the merged data
	log_debug()<<"saving..."<<std::endl;
	IOHelper::VTKWriteUnstructuredGrid(outfile, unionMesh);
	log_debug()<<"selecting finished"<<std::endl;	
    return outfile;
}
/*
  Morph given image cube file using this vtk filter:
  http://www.vtk.org/doc/nightly/html/classvtkImageDilateErode3D.html#details

  toDilate specifies image value to be dilated
  toErode specifies image value to be eroded
  morph_kernel specifies the kernel size in x,y,z

  output is a vti-image
*/
string MorphCube(const char *infile, const char *outfile, double toDilate, double toErode,
					std::vector<double> morph_kernel)
{
	//fail if morph_kernel does not contain exactly three values
	if(morph_kernel.size()!=3)
	{
		log_error()<<"Exactly three values are needed for kernel size!"<<std::endl;
		return infile;
	}
	//load image
	vtkSmartPointer<vtkImageData> image = IOHelper::VTKReadImage(infile);	

	//set up the morpher and morph image
	vtkSmartPointer<vtkImageDilateErode3D> dilateErode =
    vtkSmartPointer<vtkImageDilateErode3D>::New();
	__SetInput(dilateErode, image);
	dilateErode->SetDilateValue(toDilate);
	dilateErode->SetErodeValue(toErode);
	dilateErode->SetKernelSize(morph_kernel[0],morph_kernel[1],morph_kernel[2]);
	dilateErode->ReleaseDataFlagOff();
	dilateErode->Update();

	//save image
	IOHelper::VTKWriteImage(outfile,dilateErode->GetOutput());
	return outfile;
}

/*
  Smooth given surface using this vtk filter:
  http://vtk.org/Wiki/VTK/Examples/Meshes/WindowedSincPolyDataFilter
*/
const char* vtkSmoothMesh(const char* infile, const char* outfile, int iterations,
					  double feature_angle, double pass_band,bool boundary_smoothing,
					  bool feature_edge_smoothing, bool non_manifold_smoothing,
					  bool normalized_coordinates)
{		
	log_debug()<<"vtkSmoothMesh"<<std::endl;

	//set up the smoother
	vtkSmartPointer<vtkWindowedSincPolyDataFilter> smoother =
    vtkSmartPointer<vtkWindowedSincPolyDataFilter>::New();
	__SetInput(smoother, IOHelper::VTKReadPolyData(infile));
	smoother->SetNumberOfIterations(iterations);

	smoother->SetBoundarySmoothing(boundary_smoothing);
	smoother->SetFeatureEdgeSmoothing(feature_edge_smoothing);
	
	smoother->SetFeatureAngle(feature_angle);
	smoother->SetPassBand(pass_band);
	
	smoother->SetNonManifoldSmoothing(non_manifold_smoothing);
	smoother->SetNormalizeCoordinates(normalized_coordinates);
	
	//smooth surface
	smoother->Update();
	log_debug()<<"vtkSmoothMesh: smooth ok"<<std::endl;	
	
	//write smoothed surface down to file
	bool result = IOHelper::VTKWritePolyData(outfile, smoother->GetOutput());
	return outfile;
}

bool ExtractSurfaceMesh( vtkUnstructuredGrid* inputMesh, vtkPolyData* outputMesh)
{

    //extract the surface as unstructured grid
    vtkSmartPointer<vtkUnstructuredGridGeometryFilter> geom =
        vtkSmartPointer<vtkUnstructuredGridGeometryFilter>::New();
    __SetInput(geom, inputMesh);
    geom->Update();


    //color the polydata elements

	vtkSmartPointer<vtkUnstructuredGrid> currentGrid = dynamic_cast <vtkUnstructuredGrid*> (geom->GetOutput());

    int cellType = currentGrid->GetCellType(1);
    bool isQuadratic = false;

    log_debug()<<"CellType is "<<cellType<<std::endl;

    if(cellType == 22)
    {
        isQuadratic = true;
    }

    if(isQuadratic)
    {
        log_debug()<<"QuadraticMeshDetected"<<std::endl;
    }

    vtkSmartPointer<vtkDataSetSurfaceFilter> surfaceTessellator =
        vtkSmartPointer<vtkDataSetSurfaceFilter>::New();

    __SetInput(surfaceTessellator, currentGrid);

    if(isQuadratic)
    {
        surfaceTessellator->SetNonlinearSubdivisionLevel(3);
    }

    surfaceTessellator->Update();



    outputMesh->DeepCopy(surfaceTessellator->GetOutput());

    return true; //todo: add useful return value
}

bool AssignSurfaceRegion( const char* infile, const char* outfile,  std::vector<std::string> regionMeshes )
{
    //load the vtk  mesh
   vtkSmartPointer<vtkUnstructuredGrid> inputmesh = IOHelper::VTKReadUnstructuredGrid(infile);
   
    vtkSmartPointer<vtkUnstructuredGrid> outputmesh =
        vtkSmartPointer<vtkUnstructuredGrid>::New();

    unsigned int meshCount = regionMeshes.size();

    std::vector<vtkSmartPointer<vtkPolyData> > regionMeshesVec;

    for(unsigned int i=0; i<meshCount; i++)
    {
        vtkSmartPointer<vtkPolyData> currentMesh = IOHelper::VTKReadPolyData(regionMeshes[i].c_str());
        vtkSmartPointer<vtkPolyData> pushBackMesh =
            vtkSmartPointer<vtkPolyData>::New();

        pushBackMesh->DeepCopy(currentMesh);
        regionMeshesVec.push_back(currentMesh);
    }

    bool result = AssignSurfaceRegion( inputmesh , outputmesh, regionMeshesVec);

    //save the subdivided polydata
    IOHelper::VTKWriteUnstructuredGrid(outfile, outputmesh);

    return result;
}

bool AssignSurfaceRegion( vtkUnstructuredGrid* inputMesh, vtkUnstructuredGrid* outputMesh,
                          std::vector<vtkSmartPointer<vtkPolyData> >& regionMeshes)
{
    unsigned int numberOfVolumePoints = inputMesh->GetNumberOfPoints();

    //deep copy to output mesh
    outputMesh->DeepCopy(inputMesh);

    //create point data with indices
    vtkSmartPointer<vtkUnsignedIntArray> indexData =
        vtkSmartPointer<vtkUnsignedIntArray>::New();
    indexData->SetNumberOfComponents(1);
    indexData->SetName("indices");

    //create point data and reset to 0
    //create point data with indices
    vtkSmartPointer<vtkUnsignedIntArray> regionData =
        vtkSmartPointer<vtkUnsignedIntArray>::New();
    regionData->SetNumberOfComponents(1);
    regionData->SetName("indices");

    for(unsigned int i=0; i<numberOfVolumePoints; i++)
    {
        indexData->InsertNextTuple1(i);
        regionData->InsertNextTuple1(0);
    }

    inputMesh->GetPointData()->SetScalars(indexData);




    //extract surface as ug
    vtkSmartPointer<vtkUnstructuredGridGeometryFilter> geom =
        vtkSmartPointer<vtkUnstructuredGridGeometryFilter>::New();
    __SetInput(geom,inputMesh);
    geom->Update();

	vtkSmartPointer<vtkUnstructuredGrid> currentSurfaceMesh = dynamic_cast <vtkUnstructuredGrid*> (geom->GetOutput());

    //for each region mesh
    unsigned int meshCount = regionMeshes.size();
    double* currentPoint = new double[3];

    vtkSmartPointer<vtkKdTreePointLocator> kDTree =
        vtkSmartPointer<vtkKdTreePointLocator>::New();
    kDTree->SetDataSet(currentSurfaceMesh);
    kDTree->BuildLocator();


    for(unsigned int i=0; i<meshCount; i++)
    {
        vtkPoints* currentPoints = regionMeshes[i]->GetPoints();


        //iterate over points
        for( unsigned int iterPoint = 0; iterPoint < currentPoints->GetNumberOfPoints(); iterPoint++)
        {
            //for each point:: assign region to point closest to that point
            currentPoints->GetPoint(iterPoint, currentPoint);

            vtkIdType iD = kDTree->FindClosestPoint(currentPoint);

            unsigned int realId = currentSurfaceMesh->GetPointData()->GetScalars()->GetTuple1(iD);

            regionData->SetTuple1(realId, i+1);

        }

    }


    outputMesh->GetPointData()->SetScalars(regionData);


    return true;

}

std::string ConvertVTKMeshToAbaqusMeshString(std::string inputMesh,  std::string partName, std::string materialName)
{
    std::string output = ConvertVTKMeshToAbaqusMeshString( IOHelper::VTKReadUnstructuredGrid(inputMesh.c_str()),   partName,  materialName);
    return output;
}

std::string ConvertVTKMeshToAbaqusMeshString( vtkUnstructuredGrid* inputMesh,  std::string partName, std::string materialName)
{
    std::stringstream out;
    //write Header
    out << "*HEADING\n"
        << "**  \n"
        <<	 "**  ABAQUS Input Deck Generated by MediAssist FEM Plugin \n"
        <<	 "** \n";

    //part name
    out << "*Part, name="<<partName<<"-Part\n";

    //nodes
    out << "*Node \n";
    double* currentPoint;

    for(int i=0; i<inputMesh->GetNumberOfPoints(); i++)
    {
        currentPoint = inputMesh->GetPoint(i);
        out<<i+1<<", "<<currentPoint[0]<<" , "<<currentPoint[1]<<" , "<<currentPoint[2]<<"\n";
    }

    //elements
    out << "*Element, type=";
    vtkIdType* currentCellPoints;
    vtkIdType numberOfNodesPerElement;
    vtkIdType cellType = inputMesh->GetCellType(0);

    if(cellType == 24)
    {
        out<<"C3D10\n";
        numberOfNodesPerElement = 10;
        //currentCellPoints = new double[10];
    }

    else
    {
        out<<"C3D4\n";
        numberOfNodesPerElement = 4;
        // currentCellPoints = new double[4];
    }

    for(int i=0; i<inputMesh->GetNumberOfCells(); i++)
    {
        inputMesh->GetCellPoints(i, numberOfNodesPerElement,currentCellPoints);
        out<<i+1;

        for(int j=0; j<numberOfNodesPerElement; j++)
        {
            out<<" ,"<<currentCellPoints[j]+1;
        }

        out<<"\n";
    }

    ////////////////////
    out<<"*Elset, elset=SolidSectionSet, internal, generate\n";
    out<<"  1,  "<<inputMesh->GetNumberOfCells()<<",     1\n";
    out<<"** Section: "<<partName<<"-Section\n";
    out<<"*Solid Section, elset=SolidSectionSet, material="<<materialName<<"\n";

    ////////////////////////////

    //end part
    out<<"*End Part\n";

    return out.str();
}

std::string ConvertVTKPolydataToUnstructuredGrid(std::string infile, std::string outfile)
{
    log_debug()<< " Converting vtkPolydata "<<infile.c_str()<<" to vtkUnstructuredGrid "<<outfile.c_str()<<std::endl;
    bool result = ConvertVTKPolydataToUnstructuredGrid(infile.c_str(), outfile.c_str());
    return outfile;
}

bool ConvertVTKPolydataToUnstructuredGrid(const char* infile, const char* outfile )
{
    vtkSmartPointer<vtkPolyData> currentPolydata = IOHelper::VTKReadPolyData(infile);

    //load surface model to solid
    vtkSmartPointer<vtkUnstructuredGrid> mesh =
        vtkSmartPointer<vtkUnstructuredGrid>::New();

    bool returnValue = ConvertVTKPolydataToUnstructuredGrid(currentPolydata ,  mesh);

    //write output
    IOHelper::VTKWriteUnstructuredGrid(outfile, mesh);

    return returnValue;
}

bool ConvertVTKPolydataToUnstructuredGrid(vtkPolyData* inputPolyData, vtkUnstructuredGrid* outputMesh)
{

    // Combine the two data sets
    vtkSmartPointer<vtkAppendFilter> appendFilter =
        vtkSmartPointer<vtkAppendFilter>::New();

    __AddInput(appendFilter, inputPolyData);

    appendFilter->Update();

    outputMesh->DeepCopy(appendFilter->GetOutput());

    return true;

}

std::string ProjectSurfaceMesh(std::string infile, std::string outfile, std::string referenceMesh)
{
    log_debug()<< " Projecting surface mesh "<<infile.c_str()<<" to "<<referenceMesh<<" and writing results to "<<outfile<< std::endl;
    bool result = ProjectSurfaceMesh(infile.c_str(), outfile.c_str(), referenceMesh.c_str());
    return outfile;
}


bool ProjectSurfaceMesh(const char* infile, const char* outfile, const char* referenceMesh )
{
    vtkSmartPointer<vtkPolyData> currentGrid = IOHelper::VTKReadPolyData(infile);

    //load surface model to solid
    vtkSmartPointer<vtkPolyData> reference = IOHelper::VTKReadPolyData(referenceMesh);

    //	vtkSmartPointer<vtkPolyData> mesh =
    //	 vtkSmartPointer<vtkPolyData>::New();

    ProjectSurfaceMesh(currentGrid,  reference);

    //write output
    IOHelper::VTKWritePolyData(outfile, currentGrid);
    return true;
}

bool ProjectSurfaceMesh(vtkPolyData* inputMesh,  vtkPolyData* referenceMesh )
{
	log_debug()<<"Start surface projection" << std::endl;

//	outputMesh->BuildCells();
	//
	//first add a point data id to each point
	vtkPoints* thePoints = inputMesh->GetPoints();
	vtkIdType numberOfPoints = inputMesh->GetNumberOfPoints();



	//prepare octree data structure for reference mesh
	referenceMesh->BuildCells();
    vtkSmartPointer<vtkCellLocator> cellLocatorRef = vtkSmartPointer<vtkCellLocator>::New();
    cellLocatorRef->SetDataSet ( referenceMesh);
    cellLocatorRef->BuildLocator();


	//iterate over all surface point

    double currentPoint[3];
    double currentClosestPoint[3];
    double currentDisplacement[3];
    vtkSmartPointer<vtkGenericCell> currentTriangle = vtkSmartPointer<vtkGenericCell>::New();
    currentTriangle->SetCellTypeToTriangle();
    vtkIdType currentCellId;
    int currentSubId;
    double currentDistance;



	for(int i=0; i<numberOfPoints; i++)
	{
		thePoints->GetPoint ( i, currentPoint );


        cellLocatorRef->FindClosestPoint ( currentPoint, currentClosestPoint,
                                           currentTriangle, currentCellId,currentSubId,currentDistance );


        thePoints->SetPoint(i, currentClosestPoint[0], currentClosestPoint[1], currentClosestPoint[2]);
	}




    return true;
}

std::string VoxelizeSurfaceMesh(const char* infile, const char* outfile, int resolution, double isotropicVoxelSize, const char* referenceCoordinateGrid, bool disableFillHoles, double additionalIsotropicMargin)
{
    log_debug() << "Creating image from surface mesh (voxelization). "
                << "Resolution of the longest bound is "<<resolution<< std::endl;

    vtkSmartPointer<vtkPolyData> inputMesh = IOHelper::VTKReadPolyData(infile);

    vtkSmartPointer<vtkImageData> outputImage =
        vtkSmartPointer<vtkImageData>::New();

    bool result = VoxelizeSurfaceMesh(inputMesh, outputImage, resolution, isotropicVoxelSize, referenceCoordinateGrid, disableFillHoles, additionalIsotropicMargin);

    IOHelper::VTKWriteImage(outfile, outputImage);

    return string(outfile);
}

bool VoxelizeSurfaceMesh(vtkPolyData* inputMesh, vtkImageData* outputImage, int resolution, double isotropicVoxelSize, const char* referenceCoordinateGrid, bool disableFillHoles, double additionalIsotropicMargin)
{
    vtkSmartPointer<vtkImageData> whiteImage = ImageCreateGeneric(inputMesh, resolution, isotropicVoxelSize, referenceCoordinateGrid, additionalIsotropicMargin);

#if VTK_MAJOR_VERSION <= 5
    whiteImage->SetScalarTypeToUnsignedChar();
    whiteImage->AllocateScalars();
#else
    whiteImage->AllocateScalars(VTK_UNSIGNED_CHAR,1); //one value per 3d coordinate
#endif
    //TODO: move fill hole functionality to new operator.
    //detect holes
    vtkSmartPointer<vtkFeatureEdges> featureEdges =
        vtkSmartPointer<vtkFeatureEdges>::New();

    featureEdges->FeatureEdgesOff();
    featureEdges->BoundaryEdgesOn();
    featureEdges->NonManifoldEdgesOn();
    __SetInput(featureEdges, inputMesh);
    featureEdges->Update();
    int num_open_edges = featureEdges->GetOutput()->GetNumberOfCells();
    if(num_open_edges > 2 && !disableFillHoles)
    {
        double holeSize = 1e20;//bounds[1]-bounds[0];
        log_debug() <<"Number of holes is "<<num_open_edges<<", trying to close with hole filler and size of "<< holeSize<< std::endl;

        //fill holes
        vtkSmartPointer<vtkFillHolesFilter> fillHolesFilter =
            vtkSmartPointer<vtkFillHolesFilter>::New();

        __SetInput(fillHolesFilter, inputMesh);

        fillHolesFilter->SetHoleSize(holeSize);;
        fillHolesFilter->Update();
        vtkSmartPointer<vtkCleanPolyData> cleanFilter =
            vtkSmartPointer<vtkCleanPolyData>::New();

        __SetInput(cleanFilter, fillHolesFilter->GetOutput());
        cleanFilter->Update();
        //test again
        __SetInput(featureEdges, fillHolesFilter->GetOutput());
        featureEdges->Update();
        num_open_edges = featureEdges->GetOutput()->GetNumberOfCells();
        log_debug() <<"Number of holes ofter filling is "<<num_open_edges<< std::endl;

        //replace inputMesh with cleaned data
        inputMesh->DeepCopy(cleanFilter->GetOutput());

    }
    log_debug() <<"Performing voxelization (this might take while)..." << std::endl;
    // fill the image with foreground voxels:
    unsigned char inval = 255;
    unsigned char outval = 0;
    vtkIdType count = whiteImage->GetNumberOfPoints();

    for (vtkIdType i = 0; i < count; ++i) //TODO: speed up!
    {
        whiteImage->GetPointData()->GetScalars()->SetTuple1(i, inval);
    }



    // polygonal data --> image stencil:
    vtkSmartPointer<vtkPolyDataToImageStencil> pol2stenc =
        vtkSmartPointer<vtkPolyDataToImageStencil>::New();

    __SetInput(pol2stenc, inputMesh);


    pol2stenc->SetOutputOrigin(whiteImage->GetOrigin());
    pol2stenc->SetOutputSpacing(whiteImage->GetSpacing());
    pol2stenc->SetOutputWholeExtent(whiteImage->GetExtent());
    pol2stenc->Update();

    // cut the corresponding white image and set the background:
    vtkSmartPointer<vtkImageStencil> imgstenc =
        vtkSmartPointer<vtkImageStencil>::New();

    __SetInput(imgstenc, whiteImage);
    __SetStencil(imgstenc,pol2stenc->GetOutput());

    imgstenc->ReverseStencilOff();
    imgstenc->SetBackgroundValue(outval);
    imgstenc->Update();

    outputImage->DeepCopy(imgstenc->GetOutput());

    return true;
}

//boost::::list ExtractPointPositions( boost::::list indices, std::string infile)
//{
//	unsigned int indicesSize = boost::::len(indices);
//	std::vector<int> c_indices;
//	for(int i=0; i<indicesSize; i++)
//	{
//		unsigned int currentIndex = boost::::extract<int>(indices[i]);
//		c_indices.push_back(currentIndex);
//		std::cout<<"Current index "<<currentIndex;
//	}
//
//	std::vector<double> pointPositions = ExtractPointPositions( c_indices, infile.c_str());
//	boost::::list positions;
//
//	for(int i=0; i< pointPositions.size();i++)
//	{
//		positions.append(pointPositions[i]);
//	}
//
//
//}

bool ConvertVTKToGenericMesh(std::vector<double> &vertices , std::vector<unsigned int> &cellSizes, std::vector<unsigned int> &connectivity, std::string inputMesh)
{
    return ConvertVTKToGenericMesh(vertices ,cellSizes, connectivity, IOHelper::VTKReadUnstructuredGrid(inputMesh.c_str()));
}

bool ConvertVTKToGenericMesh(std::vector<double> &vertices , std::vector<unsigned int> &cellSizes, std::vector<unsigned int> &connectivity, const char* infile)
{
    return ConvertVTKToGenericMesh(vertices ,cellSizes, connectivity, IOHelper::VTKReadUnstructuredGrid(infile));
}

bool ConvertVTKToGenericMesh(std::vector<double> &vertices , std::vector<unsigned int> &cellSizes, std::vector<unsigned int> &connectivity, vtkUnstructuredGrid* inputMesh)
{
    vertices.clear();
    vtkPoints* thePoints = inputMesh->GetPoints();
    vtkIdType numberOfPoints = inputMesh->GetNumberOfPoints();
    double currentPoint[3];

    for(int i=0; i<numberOfPoints; i++)
    {
        thePoints->GetPoint(i,currentPoint);
        vertices.push_back(currentPoint[0]);
        vertices.push_back(currentPoint[1]);
        vertices.push_back(currentPoint[2]);

    }

    vtkCellArray* theCells = inputMesh->GetCells();
    vtkIdType numberOfCells = inputMesh->GetNumberOfCells();
    connectivity.clear();
    cellSizes.clear();

    for(unsigned int i=0; i<numberOfCells; i++) // iterate over all triangles
    {

        vtkSmartPointer<vtkIdList> cellPointIds =
            vtkSmartPointer<vtkIdList>::New();

        inputMesh->GetCellPoints(i, cellPointIds);
        unsigned int currentCellSize = cellPointIds->GetNumberOfIds();
        cellSizes.push_back(currentCellSize );


        for(unsigned int j=0; j<currentCellSize;j++) //original points
        {
            connectivity.push_back(cellPointIds->GetId(j));
        }
    }



    return true;
}


std::vector<double> ExtractPointPositions( std::vector<int> indices, std::string inputMesh)
{
	return ExtractPointPositions(indices, IOHelper::VTKReadUnstructuredGrid(inputMesh.c_str()));
}


std::vector<double> ExtractPointPositions( std::vector<int> indices, const char* infile)
{
    return ExtractPointPositions(indices, IOHelper::VTKReadUnstructuredGrid(infile));
}

std::vector<double> ExtractPointPositions( std::vector<int> indices, vtkUnstructuredGrid* inputMesh)
{
    std::vector<double> outputPositions;

    vtkPoints* thePoints = inputMesh->GetPoints();
    double currentPoint[3];

    for(int i=0; i<indices.size(); i++)
    {
        thePoints->GetPoint(indices[i],currentPoint);
        outputPositions.push_back(currentPoint[0]);
        outputPositions.push_back(currentPoint[1]);
        outputPositions.push_back(currentPoint[2]);

    }


    return outputPositions;

}

bool ConvertLinearToQuadraticTetrahedralMesh(std::string infile, std::string outfile)
{
	vtkSmartPointer<vtkUnstructuredGrid> outputMesh =
	 vtkSmartPointer<vtkUnstructuredGrid>::New();

  ConvertLinearToQuadraticTetrahedralMesh(IOHelper::VTKReadUnstructuredGrid(infile.c_str()), outputMesh);

  IOHelper::VTKWriteUnstructuredGrid(outfile.c_str(), outputMesh);

	return true;
}

bool ConvertLinearToQuadraticTetrahedralMesh( vtkUnstructuredGrid* inputMesh, vtkUnstructuredGrid* outputMesh)
{


	//get cells
    vtkPoints* thePoints = inputMesh->GetPoints();
    vtkCellArray* theCells = inputMesh->GetCells();
    vtkIdType numberOfPoints = inputMesh->GetNumberOfPoints();
    vtkIdType numberOfCells = inputMesh->GetNumberOfCells();


    //set of pairs that are already mapped
    typedef std::pair<unsigned int, unsigned int> PairType;
    typedef std::map< PairType, unsigned int > MapType;
    typedef std::pair< PairType, unsigned int > MapPairType;
    MapType pointMap;

    std::vector<PairType> edgeDefs;
    edgeDefs.push_back(std::make_pair(0,1));
    edgeDefs.push_back(std::make_pair(1,2));
    edgeDefs.push_back(std::make_pair(2,0));
    edgeDefs.push_back(std::make_pair(0,3));
    edgeDefs.push_back(std::make_pair(3,1));
    edgeDefs.push_back(std::make_pair(3,2));

    double currentVTKPoint[3];
    double currentVTKPoint1[3];
    double currentVTKPoint2[3];

	vtkSmartPointer<vtkPoints> newPoints =
	 vtkSmartPointer<vtkPoints>::New();
	vtkSmartPointer<vtkCellArray> newCells =
	 vtkSmartPointer<vtkCellArray>::New();




    for(unsigned int i=0; i<numberOfCells; i++) // iterate over all triangles
    {

        vtkSmartPointer<vtkIdList> cellPointIds =
            vtkSmartPointer<vtkIdList>::New();

        vtkSmartPointer<vtkIdList> newCellPointIds =
            vtkSmartPointer<vtkIdList>::New();

        inputMesh->GetCellPoints(i, cellPointIds);

        for(unsigned int j=0; j<4;j++) //original points
        {
        	unsigned int pointId = cellPointIds->GetId(j);
        	PairType currentPair = std::make_pair(pointId, pointId);

        	MapType::iterator it=pointMap.find(currentPair);

        	unsigned int currentId;

        	if(it != pointMap.end())
        	{
        		currentId = it->second;
        	}
        	else
        	{
				thePoints->GetPoint(pointId, currentVTKPoint);
				currentId = newPoints->InsertNextPoint(currentVTKPoint[0], currentVTKPoint[1], currentVTKPoint[2]);

				pointMap.insert(std::make_pair(currentPair, currentId));
        	}

        	newCellPointIds->InsertNextId(currentId);
        }


        for(unsigned int j=4; j<10;j++) //original points
        {
        	unsigned int pointId1 = cellPointIds->GetId(edgeDefs[j-4].first);
        	unsigned int pointId2 = cellPointIds->GetId(edgeDefs[j-4].second);
        	PairType currentPair = std::make_pair( pointId1, pointId2);
        	PairType currentPair2 = std::make_pair( pointId2, pointId1);

        	MapType::iterator it1=pointMap.find(currentPair);
        	MapType::iterator it2=pointMap.find(currentPair2);

        	unsigned int currentId;

        	if(it1 != pointMap.end() || it2 != pointMap.end())
        	{
        		if(it1 != pointMap.end())
        			currentId = it1->second;
        		else
        			currentId = it2->second;
        	}
        	else
        	{

				thePoints->GetPoint(pointId1, currentVTKPoint1);
				thePoints->GetPoint(pointId2, currentVTKPoint2);

				currentVTKPoint[0] = (currentVTKPoint1[0] + currentVTKPoint2[0])/2;
				currentVTKPoint[1] = (currentVTKPoint1[1] + currentVTKPoint2[1])/2;
				currentVTKPoint[2] = (currentVTKPoint1[2] + currentVTKPoint2[2])/2;

				currentId = newPoints->InsertNextPoint(currentVTKPoint[0], currentVTKPoint[1], currentVTKPoint[2]);
				pointMap.insert(std::make_pair(currentPair, currentId));

        	}


        	newCellPointIds->InsertNextId(currentId);
        }

        newCells->InsertNextCell(newCellPointIds);

    }


    outputMesh->SetPoints(newPoints);
    outputMesh->SetCells(VTK_QUADRATIC_TETRA, newCells);



	return true;
}

 bool ProjectVolumeMesh(std::string inputVolumeMeshFile, std::string outputMeshFile, std::string referenceMeshFile)
{

  vtkSmartPointer<vtkPolyData> referenceMesh = IOHelper::VTKReadPolyData(referenceMeshFile.c_str());
  vtkSmartPointer<vtkUnstructuredGrid> inputMesh = IOHelper::VTKReadUnstructuredGrid(inputVolumeMeshFile.c_str());

  vtkSmartPointer<vtkUnstructuredGrid> outputMesh = vtkSmartPointer<vtkUnstructuredGrid>::New();

	ProjectVolumeMesh(inputMesh, outputMesh, referenceMesh);

  IOHelper::VTKWriteUnstructuredGrid(outputMeshFile.c_str(), outputMesh);


	return true;
}

bool ProjectVolumeMesh( vtkUnstructuredGrid* inputMesh, vtkUnstructuredGrid* outputMesh, vtkPolyData* referenceMesh)
{

	log_debug() << "Start surface projection" << std::endl;
	outputMesh->DeepCopy(inputMesh);

	log_debug() << "deep copy finished" << std::endl;

//	outputMesh->BuildCells();
	//
	//first add a point data id to each point
	vtkPoints* thePoints = outputMesh->GetPoints();
	vtkCellArray* theCells = outputMesh->GetCells();
	vtkIdType numberOfPoints = outputMesh->GetNumberOfPoints();
	vtkIdType numberOfCells = outputMesh->GetNumberOfCells();

	vtkSmartPointer<vtkLongLongArray> pointIds =
	  vtkSmartPointer<vtkLongLongArray>::New();
	pointIds->SetNumberOfComponents(1);
	pointIds->SetName("pointIds");

	vtkSmartPointer<vtkLongLongArray> regionIds =
	  vtkSmartPointer<vtkLongLongArray>::New();
	regionIds->SetNumberOfComponents(1);
	regionIds->SetName("regionIds");

	vtkSmartPointer<vtkDoubleArray> displacements =
	  vtkSmartPointer<vtkDoubleArray>::New();
	displacements->SetNumberOfComponents(3);
	displacements->SetName("displacements");

	log_debug() << "filling tuples" << std::endl;

	for(unsigned int i=0; i<numberOfPoints;i++) // iterate over all triangles
	{
		pointIds->InsertNextTuple1(i);
		regionIds->InsertNextTuple1(0);

		displacements->InsertNextTuple3(0,0,0);
	}

	outputMesh->GetPointData()->SetGlobalIds(pointIds);


	//extract the surface
	vtkSmartPointer<vtkUnstructuredGridGeometryFilter> geom =
		vtkSmartPointer<vtkUnstructuredGridGeometryFilter>::New();
	
	__SetInput(geom, outputMesh);
	
	geom->PassThroughPointIdsOn();
	geom->SetOriginalPointIdsName("pointIds");
	geom->Update();




	//prepare octree data structure for reference mesh
	referenceMesh->BuildCells();
    vtkSmartPointer<vtkCellLocator> cellLocatorRef = vtkSmartPointer<vtkCellLocator>::New();
    cellLocatorRef->SetDataSet ( referenceMesh);
    cellLocatorRef->BuildLocator();




	//iterate over all surface point
	vtkSmartPointer<vtkUnstructuredGrid> surfaceGrid = dynamic_cast <vtkUnstructuredGrid*> (geom->GetOutput());
	vtkPoints* thePointsSurface = surfaceGrid->GetPoints();
	vtkIdType numberOfPointsSurface = thePointsSurface->GetNumberOfPoints();
	vtkLongLongArray* realPointIds = (vtkLongLongArray*)surfaceGrid->GetPointData()->GetGlobalIds("pointIds");

    double currentPoint[3];
    double currentClosestPoint[3];
    double currentDisplacement[3];
    vtkSmartPointer<vtkGenericCell> currentTriangle = vtkSmartPointer<vtkGenericCell>::New();
    currentTriangle->SetCellTypeToTriangle();
    vtkIdType currentCellId;
    int currentSubId;
    double currentDistance;



	for(int i=0; i<numberOfPointsSurface; i++)
	{
		thePointsSurface->GetPoint ( i, currentPoint );


        cellLocatorRef->FindClosestPoint ( currentPoint, currentClosestPoint,
                                           currentTriangle, currentCellId,currentSubId,currentDistance );


        unsigned int realPointId = realPointIds->GetTuple1(i);

        thePoints->SetPoint(realPointId,  currentClosestPoint[0],currentClosestPoint[1],currentClosestPoint[2] );

        currentDisplacement[0] = currentClosestPoint[0] - currentPoint[0];
        currentDisplacement[1] = currentClosestPoint[1] - currentPoint[1];
        currentDisplacement[2] = currentClosestPoint[2] - currentPoint[2];

        displacements->SetTuple3(realPointId, currentDisplacement[0],currentDisplacement[1],currentDisplacement[2]);
        regionIds->SetTuple1(realPointId, 1);
	}

	outputMesh->GetPointData()->SetScalars(regionIds);
	outputMesh->GetPointData()->SetVectors(displacements);








	return true;
}

std::vector<unsigned int> ExtractNodeSet(std::string inputVolumeMeshFile, std::string nodeSetName)
{
  std::vector<unsigned int> result = ExtractNodeSet(IOHelper::VTKReadUnstructuredGrid(inputVolumeMeshFile.c_str()), nodeSetName);
	return result;
}

std::vector<unsigned int> ExtractNodeSet( vtkUnstructuredGrid* inputMeshFile, std::string nodeSetName)
{


	std::vector<unsigned int> idList;

	if(inputMeshFile->GetPointData()->HasArray(nodeSetName.c_str()))
	{
		vtkDataArray* nodeSet = inputMeshFile->GetPointData()->GetScalars(nodeSetName.c_str());

		unsigned int numberOfNodes = nodeSet->GetNumberOfTuples();

		for(int i=0; i<numberOfNodes; i++)
		{
			unsigned int myType = (unsigned int) nodeSet->GetTuple1(i);

			if(myType)
			{
				idList.push_back(i);
			}
		}

		return idList;
	}
	else
	{
		log_debug()<<"Node set with name "<<nodeSetName<<" was not found" << std::endl;
		return idList;
	}


}

std::vector<double> ExtractVectorField(std::string inputVolumeMeshFile,  std::string vectorFieldName, std::vector<unsigned int> nodeList)
{
    std::vector<double> result = ExtractVectorField(IOHelper::VTKReadUnstructuredGrid(inputVolumeMeshFile.c_str()), vectorFieldName, nodeList);
		return result;
}

std::vector<double> ExtractVectorField( vtkUnstructuredGrid* inputMeshFile,  std::string vectorFieldName, std::vector<unsigned int> nodeList)
{
	std::vector<double> vectorField;

	if(inputMeshFile->GetPointData()->HasArray(vectorFieldName.c_str()))
	{
		vtkDataArray* vectorFieldArray = inputMeshFile->GetPointData()->GetVectors(vectorFieldName.c_str());
		unsigned int numberOfNodes = nodeList.size();
		unsigned int numberOfFieldValues = vectorFieldArray->GetNumberOfTuples();

		if(numberOfNodes> numberOfFieldValues)
		{
			log_debug() <<"Indices list and vector field does not match" << std::endl;
			return vectorField;
		}

		double currentDisplacement[3];

		for(int i=0; i<numberOfNodes; i++)
		{
			vectorFieldArray->GetTuple(nodeList[i], currentDisplacement);
			vectorField.push_back( currentDisplacement[0] );
			vectorField.push_back( currentDisplacement[1] );
			vectorField.push_back( currentDisplacement[2] );
		}

		return vectorField;
	}
	else
	{
		log_debug() <<"Vector field with name "<<vectorFieldName<<" was not found" << std::endl;
		return vectorField;
	}
}


string GenerateDistanceMap3d(const char* inputUnstructuredGrid, const char*  targetImage, int resolution, double isotropicVoxelSize, const char* referenceCoordinateGrid, double additionalIsotropicMargin )
{
  vtkSmartPointer<vtkUnstructuredGrid> polydata = IOHelper::VTKReadUnstructuredGrid(inputUnstructuredGrid);
  vtkSmartPointer<vtkImageData> aDistMap = GenerateDistanceMap3d(polydata, resolution, isotropicVoxelSize, referenceCoordinateGrid, additionalIsotropicMargin);
  IOHelper::VTKWriteImage(targetImage, aDistMap);
  return targetImage;
}

vtkSmartPointer<vtkImageData> GenerateDistanceMap3d(vtkUnstructuredGrid* aUnstructuredGrid, int resolution, double isotropicVoxelSize, const char* referenceCoordinateGrid, double additionalIsotropicMargin)
{
  vtkSmartPointer<vtkImageData> image = ImageCreateGeneric(aUnstructuredGrid, resolution, isotropicVoxelSize, referenceCoordinateGrid, additionalIsotropicMargin);
  #if VTK_MAJOR_VERSION <= 5
    image->SetScalarTypeToFloat();
    image->SetNumberOfScalarComponents(1);
    image->AllocateScalars();
#else
    image->AllocateScalars(VTK_FLOAT, 3); //3c omponents per voxel 
#endif

  vtkSmartPointer<vtkCellLocator> cellLocatorRef = vtkSmartPointer<vtkCellLocator>::New();
  cellLocatorRef->SetDataSet(aUnstructuredGrid);
  cellLocatorRef->BuildLocator();

  int* dims = image->GetDimensions();
  double* spacing = image->GetSpacing();
  double* origin = image->GetOrigin();

  vtkIdType containingCellRefId;
  double closestPointInCell[3];
  int subId=0;
  double dist=0;

  for (int z = 0; z < dims[2]; z++)
  {
    log_debug() << "Generating distance map for slice " << z << "/" << dims[2] << endl;
    for (int y = 0; y < dims[1]; y++)
    {
        for (int x = 0; x < dims[0]; x++)
        {
          double pInMM[3];
          pInMM[0] = origin[0]+x*spacing[0];
          pInMM[1] = origin[1]+y*spacing[1];
          pInMM[2] = origin[2]+z*spacing[2];
          float* point = static_cast<float*>(image->GetScalarPointer(x,y,z));  
          cellLocatorRef->FindClosestPoint(pInMM,  closestPointInCell, containingCellRefId, subId, dist);
          if (dist>0)
          {
            point[0] = abs(pInMM[0] - closestPointInCell[0]);
            point[1] = abs(pInMM[1] - closestPointInCell[1]);
            point[2] = abs(pInMM[2] - closestPointInCell[2]);
          }
          else
          {
            point[0] = 0;
            point[1] = 0;
            point[2] = 0;
          }
        } //x
    } //y
  } //z
  return image;
}

string GenerateDistanceMap(const char* inputUnstructuredGrid, const char*  targetImage, int resolution, double isotropicVoxelSize, const char* referenceCoordinateGrid, double additionalIsotropicMargin )
{
  vtkSmartPointer<vtkUnstructuredGrid> polydata = IOHelper::VTKReadUnstructuredGrid(inputUnstructuredGrid);
  vtkSmartPointer<vtkImageData> aDistMap = GenerateDistanceMap(polydata, resolution, isotropicVoxelSize, referenceCoordinateGrid, additionalIsotropicMargin);
  IOHelper::VTKWriteImage(targetImage, aDistMap);
  return targetImage;
}

vtkSmartPointer<vtkImageData> GenerateDistanceMap(vtkUnstructuredGrid* aUnstructuredGrid, int resolution, double isotropicVoxelSize, const char* referenceCoordinateGrid, double additionalIsotropicMargin)
{
  vtkSmartPointer<vtkImageData> image = ImageCreateGeneric(aUnstructuredGrid, resolution, isotropicVoxelSize, referenceCoordinateGrid, additionalIsotropicMargin);
  #if VTK_MAJOR_VERSION <= 5
    image->SetScalarTypeToFloat();
    image->SetNumberOfScalarComponents(1);
    image->AllocateScalars();
#else
    image->AllocateScalars(VTK_FLOAT, 1); //1 component per voxel 
#endif

  vtkSmartPointer<vtkCellLocator> cellLocatorRef = vtkSmartPointer<vtkCellLocator>::New();
  cellLocatorRef->SetDataSet(aUnstructuredGrid);
  cellLocatorRef->BuildLocator();

  int* dims = image->GetDimensions();
  double* spacing = image->GetSpacing();
  double* origin = image->GetOrigin();

  vtkIdType containingCellRefId;
  double closestPointInCell[3];
  int subId=0;
  double dist=0;

  for (int z = 0; z < dims[2]; z++)
  {
    log_debug() << "Generating distance map for slice " << z << "/" << dims[2] << endl;
    for (int y = 0; y < dims[1]; y++)
    {
        for (int x = 0; x < dims[0]; x++)
        {
          double pInMM[3];
          pInMM[0] = origin[0]+x*spacing[0];
          pInMM[1] = origin[1]+y*spacing[1];
          pInMM[2] = origin[2]+z*spacing[2];
          float* point = static_cast<float*>(image->GetScalarPointer(x,y,z));  
          cellLocatorRef->FindClosestPoint(pInMM,  closestPointInCell, containingCellRefId, subId, dist);

          *point = dist;
        } //x
    } //y
  } //z
  return image;
}

vtkSmartPointer<vtkImageData> ImageCreateGeneric(vtkPointSet* grid, double resolution, float isotropicVoxelSize, const char* referenceCoordinateGrid, float additionalIsotropicMargin)
{
  vtkSmartPointer<vtkImageData> whiteImage;
  
  //Method A: Generate bounds, spacing and origine based on mesh:
  if (resolution>0)
  {
    whiteImage = ImageCreateWithMesh(grid, resolution);
  }

  else if(isotropicVoxelSize>0)
  {
    whiteImage = ImageCreateWithMesh(grid, 100);
    ImageChangeVoxelSize(whiteImage, isotropicVoxelSize);
  }

  //Method B: Get bounds, spacing and origin from given grid:
  else if (!string(referenceCoordinateGrid).empty())
  {
    whiteImage = ImageCreate(IOHelper::VTKReadImage(referenceCoordinateGrid));
  }
  else
  {
    log_error() << "ImageCreateGeneric: Please specify resolution, isotropicVoxelSize or referenceCoordinateGrid" << endl;
  }

  if (additionalIsotropicMargin!=0)
    ImageEnlargeIsotropic(whiteImage, additionalIsotropicMargin);

  return whiteImage;
}

vtkSmartPointer<vtkImageData> ImageCreateWithMesh(vtkPointSet* grid, double resolution)
{
  vtkSmartPointer<vtkImageData> newVTKImage = vtkSmartPointer<vtkImageData>::New();

  double bounds[6];
  double spacingArray[3]; // desired volume spacing
  double origin[3];
  int dim[3];
  double spacing = -1;
  //find longest bound
  double longestBoundValue = 0;
  grid->GetBounds(bounds);
  for (int i = 0; i < 3; i++)
  {
      double currentValue = (bounds[i * 2 + 1] - bounds[i * 2]);

      if(currentValue>longestBoundValue)
      {
          longestBoundValue = currentValue;
          spacing = currentValue / (double)resolution;
      }
  }    

  log_debug() <<"Longest bound is "<<longestBoundValue<< std::endl;
  log_debug() <<"Spacing is "<<spacing<< std::endl;;
             
  spacingArray[0] = spacing;
  spacingArray[1] = spacing;
  spacingArray[2] = spacing;
  
  // compute dimensions 
  for (int i = 0; i < 3; i++)
  {
      dim[i] = static_cast<int>(ceil((bounds[i * 2 + 1] - bounds[i * 2]) / spacing));
  }
  //Compute origin
  origin[0] = bounds[0] + spacing / 2;
  origin[1] = bounds[2] + spacing / 2;
  origin[2] = bounds[4] + spacing / 2;
      
  newVTKImage->SetSpacing(spacingArray);
  newVTKImage->SetDimensions(dim);
  newVTKImage->SetExtent(0, dim[0] - 1, 0, dim[1] - 1, 0, dim[2] - 1);
  newVTKImage->SetOrigin(origin);

  return newVTKImage;
}

vtkSmartPointer<vtkImageData> ImageCreate(vtkImageData* refImageGrid)
{

  vtkSmartPointer<vtkImageData> newVTKImage = vtkSmartPointer<vtkImageData>::New();

  int dims[3];
  dims[0] = refImageGrid->GetDimensions()[0];
  dims[1] = refImageGrid->GetDimensions()[1];
  dims[2] = refImageGrid->GetDimensions()[2];
    
  double origin[3];
  origin[0] = refImageGrid->GetOrigin()[0];
  origin[1] = refImageGrid->GetOrigin()[1];
  origin[2] = refImageGrid->GetOrigin()[2];

  double spacing[3];
  spacing[0] = refImageGrid->GetSpacing()[0];
  spacing[1] = refImageGrid->GetSpacing()[1];
  spacing[2] = refImageGrid->GetSpacing()[2];

  newVTKImage->SetDimensions(dims);
  newVTKImage->SetOrigin(origin);
  newVTKImage->SetSpacing(spacing);
  newVTKImage->SetExtent(0, dims[0] - 1, 0, dims[1] - 1, 0, dims[2] - 1);

  return newVTKImage;
}

void ImageChangeVoxelSize(vtkImageData* image, double voxelSize)
{
  double voxelSizeArr[3];
  voxelSizeArr[0] = voxelSize;
  voxelSizeArr[1] = voxelSize;
  voxelSizeArr[2] = voxelSize;
  ImageChangeVoxelSize(image, voxelSizeArr);
}

void ImageChangeVoxelSize(vtkImageData* image, double* voxelSize)
{
  double* bounds = image->GetBounds();
  double spacing[3];
  spacing[0] = voxelSize[0];
  spacing[1] = voxelSize[1];
  spacing[2] = voxelSize[2];

  int dims[3];
  for (int i = 0; i < 3; i++)
  {
      dims[i] = static_cast<int>(ceil((bounds[i * 2 + 1] - bounds[i * 2]) / spacing[i]));
  }
  image->SetDimensions(dims);
  image->SetSpacing(spacing);
  image->SetExtent(0, dims[0] - 1, 0, dims[1] - 1, 0, dims[2] - 1);
}

void ImageEnlargeIsotropic(vtkImageData* image, double enlargement)
{
  double* bounds = image->GetBounds();
  bounds[0] -= enlargement;
  bounds[1] += enlargement;
  bounds[2] -= enlargement;
  bounds[3] += enlargement;
  bounds[4] -= enlargement;
  bounds[5] += enlargement;
  
  double* origin = image->GetOrigin();
  origin[0] -= enlargement;
  origin[1] -= enlargement;
  origin[2] -= enlargement;
  image->SetOrigin(origin);

  double* spacing = image->GetSpacing();

  int dims[3];
  for (int i = 0; i < 3; i++)
  {
      dims[i] = static_cast<int>(ceil((bounds[i * 2 + 1] - bounds[i * 2]) / spacing[i]));
  }
  image->SetDimensions(dims);
  image->SetSpacing(spacing);
  image->SetExtent(0, dims[0] - 1, 0, dims[1] - 1, 0, dims[2] - 1);
}
}//end namepace MiscMeshOperators
}//end namepace MSML

