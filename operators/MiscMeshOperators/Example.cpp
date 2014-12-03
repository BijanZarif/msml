
#include "PostProcessingOperators.h"
#include "MiscMeshOperators.h"
#include "IndexRegionOperators.h"
#include <vector>
#include <iostream>
#include <string>
#include <sstream> 
#include <VTKMeshgen.h>
#include <IOHelper.h>

using namespace MSML;

#define SMOKE_TEST_DIR_PREFIX smoke
void TestAssignRegionOperator()
{
//	std::string inputMesh("/org/share/home/mediassi/MediAssistData/Modelle/MIC/Modellbibliothek/Medical/Helios_Aktuell/Leber/LeberXSTet4.vtk");
//	std::string outputMesh("/org/share/home/mediassi/MediAssistData/Modelle/MIC/Modellbibliothek/Medical/Helios_Aktuell/Leber/LeberXSTet4Regions.vtk");
//
//
//	std::vector<std::string> regionMeshes;
//	regionMeshes.push_back("/org/share/home/mediassi/MediAssistData/Modelle/MIC/Modellbibliothek/Medical/Helios_Aktuell/Leber/LeberRegion1.vtk");


	std::string inputMesh("/home/suwelack/IPCAI/Volumes/liverMVolume.vtk");
	std::string outputMesh("/home/suwelack/IPCAI/Volumes/liverMVolumeRegions.vtk");


	std::vector<std::string> regionMeshes;
	regionMeshes.push_back("/home/suwelack/IPCAI/Surfaces/liverSurfaceXLIniFixedBC.vtk");



	string errorMessage;

	MiscMeshOperators::AssignSurfaceRegion(inputMesh.c_str(), outputMesh.c_str(), regionMeshes);

}

void TestTransformMeshBarycentric()
{
  vtkSmartPointer<vtkUnstructuredGrid> referenceGrid = IOHelper::VTKReadUnstructuredGrid("E:\\02_Data\\j_mechanic\\scenarios_1stmode99\\dispOutput0.vtu");
	vtkSmartPointer<vtkUnstructuredGrid> deformedGrid = IOHelper::VTKReadUnstructuredGrid("E:\\02_Data\\j_mechanic\\scenarios_1stmode99\\dispOutput125.vtu");
  vtkSmartPointer<vtkUnstructuredGrid> refSurface = IOHelper::VTKReadUnstructuredGrid("E:\\02_Data\\j_mechanic\\allIn100justCGALOptimizerAllOnResults\\boneMesh16.vtk");
	vtkSmartPointer<vtkUnstructuredGrid> out_surface = vtkSmartPointer<vtkUnstructuredGrid>::New();
  PostProcessingOperators::TransformMeshBarycentric(referenceGrid,out_surface, refSurface, deformedGrid, 10);
}

void TestPositionFromIndices()
{
  std::vector<unsigned int> ids;
  ids.push_back(1);
  vector<double> pos = IndexRegionOperators::PositionFromIndices("C:\\Projekte\\msml_github\\examples\\BunnyExample\\bunnyout\\liver0.vtu", ids, "points");
}

void TestExtractSurfaceMeshFromVolumeMeshByCelldataOperator()
{
	//std::string inputMesh("E:/GIT/msml/Testdata/CGALi2vExampleResults/liver_kidney_gallbladder.vtk");
	//std::string outputMesh("E:/GIT/msml/Testdata/CGALi2vExampleResults/liver_kidney_gallbladder_surface.vtk");
  std::string inputMesh("E:/GIT/msml/Testdata/3Direcadb11Labeled.vtk.tmp");
  std::string outputMesh("E:/GIT/msml/Testdata/3Direcadb11Labeled_surface.vtk");

	string errorMessage;
  std::string asd("asd");
	MiscMeshOperators::ExtractAllSurfacesByMaterial(inputMesh.c_str(), outputMesh.c_str(), true);

}

void TestMergeMeshes()
{
  /*
  std::string inputCells("E:/GIT/msml/MSML_/LungsHighResResults/case1_T00_labled_combo.vtk");
  std::string inputPoints("E:/GIT/msml/MSML_/LungsHighResResults/dispOutput");
  std::string outputMesh("E:/GIT/msml/MSML_/LungsHighResResults/case1_T00_labledOutput");*/
  
  std::string inputCells("E:/SOFA_trunk/build/applications/projects/runSofa/cycytube_labled_combo.vtk-volume1.vtk");
  std::string inputPoints("E:/SOFA_trunk/build/applications/projects/runSofa/dispOutputOne");
  std::string outputMesh("E:/SOFA_trunk/build/applications/projects/runSofa/case1_T00_labledOutputOne");

  for (int i=0; i<=55;i++)
  {
    std::ostringstream ss;
    ss << i;
    string str = ss.str();

    PostProcessingOperators::MergeMeshes((inputPoints + str + ".vtu").c_str(), inputCells.c_str(), (outputMesh + str + ".vtk").c_str());
  }
}

void TestGenerateDVF()
{
  /*
  std::string inputCells("E:/GIT/msml/MSML_/LungsHighResResults/case1_T00_labled_combo.vtk");
  std::string inputPoints("E:/GIT/msml/MSML_/LungsHighResResults/dispOutput");
  std::string outputMesh("E:/GIT/msml/MSML_/LungsHighResResults/case1_T00_labledOutput");*/


  std::string ref("C:\\Projekte\\msml_github\\examples\\CGALPelvis_DKFZ_internal_fuer_MB\\output_pelvisCase_new_12.09.2014\\tetmesh.vtk");
  std::string def("C:\\Projekte\\msml_github\\examples\\CGALPelvis_DKFZ_internal_fuer_MB\\output_pelvisCase_new_12.09.2014\\disp20.vtu");
  std::string dvf("C:\\Projekte\\msml_github\\examples\\CGALPelvis_DKFZ_internal_fuer_MB\\output_pelvisCase_new_12.09.2014\\tmpTes.vtk");

  PostProcessingOperators::GenerateDVF(ref.c_str(),  def.c_str(), dvf.c_str(), 5, "", 10);
}
void TestReadCTX()
{
  IOHelper::VTKReadImage("C:\\Projekte\\msml_github\\examples\\CGALPelvis_DKFZ_internal_fuer_MB\\pelvisCase.ctx.gz"); //internal data type
}

void TestMarchingCube()
{
  VTKMeshgen::vtkMarchingCube("C:\\Projekte\\msml_github\\examples\\CGALPelvis_DKFZ_internal_fuer_MB\\output_pelvisCase_new_12.09.2014\\output_transformFullRTSSmarching_15.09.2014\\RTSS_BLASE.ctx.gz",
    "C:\\Projekte\\msml_github\\examples\\CGALPelvis_DKFZ_internal_fuer_MB\\output_pelvisCase_new_12.09.2014\\output_transformFullRTSSmarching_15.09.2014\\RTSS_BLASE.vtk", 0.5);
}


void TestApplyDVF()
{


  std::string ref = string(TESTDATA_PATH) + "\\CGALPelvis_DKFZ_internal_fuer_MB\\output_pelvisCase_new_12.09.2014\\pelvisCaseCTImage.vti";
  std::string def = "C:\\Projekte\\msml_github\\examples\\CGALPelvis_DKFZ_internal_fuer_MB\\output_pelvisCase_new_12.09.2014\\output_postprocessing_19.09.2014\\defed10.vtk";
  std::string dvf = "C:\\Projekte\\msml_github\\examples\\CGALPelvis_DKFZ_internal_fuer_MB\\output_pelvisCase_new_12.09.2014\\output_postprocessing_19.09.2014\\dvf10.vtk";
  
  PostProcessingOperators::ApplyDVF(ref.c_str(), dvf.c_str(), def.c_str(), false,  2);
}

namespace TestMiscMeshoperators
{
  void TestConvertLinearToQuadraticTetrahedralMesh()
  {
    MiscMeshOperators::ConvertLinearToQuadraticTetrahedralMesh((string(TESTDATA_PATH)+"/bunny_tets.vtk").c_str(), (string(TESTDATA_PATH)+ "/TestConvertLinearToQuadraticTetrahedralMesh.vtk").c_str());
  }

  void TestConvertSTLToVTK()
  {
    MiscMeshOperators::ConvertSTLToVTK((string(TESTDATA_PATH)+"/bunny_xl.stl").c_str(), (string(TESTDATA_PATH)+ "/TestConvertSTLToVTK.vtk").c_str());
  }

  void TestConvertVTKMeshToAbaqusMeshString()
  {
    std::ofstream aFile;
    aFile.open((string(TESTDATA_PATH)+ "/TestConvertVTKMeshToAbaqusMeshString.abq").c_str());
    string aReturn = MiscMeshOperators::ConvertVTKMeshToAbaqusMeshString((string(TESTDATA_PATH)+"/bunny_tets.vtk").c_str(), "aPart", "aMaterial");
    aFile << aReturn << "\n";
    aFile.close();
  }

  void TestConvertVTKPolydataToUnstructuredGrid()
  {
    MiscMeshOperators::ConvertVTKPolydataToUnstructuredGrid((string(TESTDATA_PATH)+"/bunny_polydata.vtk").c_str(),(string(TESTDATA_PATH)+ "/TestConvertVTKPolydataToUnstructuredGrid.vtu").c_str());
  }

  void TestConvertVTKToOFF()
  {
    MiscMeshOperators::ConvertVTKToOFF(IOHelper::VTKReadPolyData((string(TESTDATA_PATH)+"/bunny_polydata.vtk").c_str()),(string(TESTDATA_PATH)+ "/TestConvertVTKToOFF.off").c_str());
  }

  void TestExtractAllSurfacesByMaterial()
  {
    MiscMeshOperators::ExtractAllSurfacesByMaterial((string(TESTDATA_PATH)+"/ircad_tets_labled.vtk").c_str(), (string(TESTDATA_PATH)+ "/TestExtractAllSurfacesByMaterial.vtk").c_str(), false);
  }

  void TestExtractNodeSet()
  {
    MiscMeshOperators::ExtractNodeSet((string(TESTDATA_PATH)+"/ircad_tets_labled.vtk").c_str(), "nonExtistingNodeset-TestdataNeeded");
  } 

  void TestExtractPointPositions()
  {
    std::vector<int> indices;
    indices.push_back(0);indices.push_back(1);indices.push_back(2);
    std::vector<double> aReturn = MiscMeshOperators::ExtractPointPositions(indices, (string(TESTDATA_PATH)+"/bunny_tets.vtk").c_str());
    if (abs(aReturn[1] - 0.0660446) > 0.000001)
      throw;
  } 

  void TestExtractVectorField()
  {
    std::vector<unsigned int> indices;
    indices.push_back(0);indices.push_back(1);indices.push_back(2);
    MiscMeshOperators::ExtractVectorField((string(TESTDATA_PATH)+"/bunny_tets.vtk").c_str(), "nonExistinFieldTestdataNeeded", indices);
  }
  void TestExtractSurfaceMesh()
  {
    MiscMeshOperators::ExtractSurfaceMesh((string(TESTDATA_PATH)+"/bunny_tets.vtk").c_str(), (string(TESTDATA_PATH)+ "/TestExtractSurfaceMesh_AKA_ugrid_to_polydata.vtk").c_str());
  }

  void TestProjectSurfaceMesh()
  {
    MiscMeshOperators::ProjectSurfaceMesh((string(TESTDATA_PATH)+"/bunny_polydata_highres.vtk").c_str(), (string(TESTDATA_PATH)+ "/TestProjectSurfaceMesh.vtk").c_str(), (string(TESTDATA_PATH)+"/bunny_polydata.vtk").c_str());
  }

  void TestVoxelizeSurfaceMesh()
  {
    MiscMeshOperators::VoxelizeSurfaceMesh((string(TESTDATA_PATH) + "/bunny_polydata.vtk").c_str(), (string(TESTDATA_PATH) + "/TestVoxelizeSurfaceMesh.vtk").c_str(), 100, 0, string("").c_str(), false, 0);
  }
}

void TestPostProcessingOperators()
{
    PostProcessingOperators::ApplyDVF((string(TESTDATA_PATH)+"/ircad_ct_image.vti").c_str(), (string(TESTDATA_PATH)+"/ircad_dvf.vti").c_str(), (string(TESTDATA_PATH)+"/TestApplyDVF.vtk").c_str(), true, 2.0);

    PostProcessingOperators::ColorMeshFromComparison((string(TESTDATA_PATH)+"/ircad_disp0.vtu").c_str(),(string(TESTDATA_PATH)+"/ircad_disp50.vtu").c_str(),(string(TESTDATA_PATH)+"/ColorMeshFromComparison.vtk").c_str());
    
    double error_max=-1; double error_rms=-1;
    PostProcessingOperators::CompareMeshes(error_rms, error_max, (string(TESTDATA_PATH)+"/ircad_disp0.vtu").c_str(),  (string(TESTDATA_PATH)+"/ircad_disp50.vtu").c_str(), true);
    if (error_max<1)
      throw;

    PostProcessingOperators::TransformMeshBarycentric((string(TESTDATA_PATH)+"/ircad_tris_labled.vtk").c_str(), (string(TESTDATA_PATH)+"/ircad_disp0.vtu").c_str(),  (string(TESTDATA_PATH)+"/ircad_disp50.vtu").c_str(),(string(TESTDATA_PATH)+"/TestTransformMeshBarycentric.vtu").c_str(), false);
      
    PostProcessingOperators::GenerateDVF((string(TESTDATA_PATH)+"/ircad_disp50.vtu").c_str(),  (string(TESTDATA_PATH)+"/ircad_disp0.vtu").c_str(),
      (string(TESTDATA_PATH)+"/TestGenerateDVF.vti").c_str(), 10, "", 10);
}

std::vector<std::string> LoadFileNames(std::string theFileListTxt)
{
  std::vector<std::string> fileNames;
	std::ifstream fileStream;
	fileStream.open(theFileListTxt.c_str(), std::ifstream::in);
  if (fileStream)
  {
	  std::string line;  
	  while(getline(fileStream,line))
	  {
      fileNames.push_back(line);
	  }
	  fileStream.close();
  }
  return fileNames;
}
void TestImageSumPrivateData() //TODO: Add test with open accesss data.
{
  PostProcessingOperators::ImageWeightedSum("C:\\Projekte\\msml_dkfz\\examples\\j_mechanic\\groundTruth\\*_segmentation.ctx.gz", true, "C:\\Projekte\\msml_dkfz\\examples\\j_mechanic\\groundTruth\\CTV_SUMMED_1_.vtk");
}

int main( int argc, char * argv[])
{
  TestImageSumPrivateData();
  return 0;
  TestPostProcessingOperators();
  
  TestMiscMeshoperators::TestConvertLinearToQuadraticTetrahedralMesh();
  TestMiscMeshoperators::TestConvertSTLToVTK();
  TestMiscMeshoperators::TestConvertVTKMeshToAbaqusMeshString();
  TestMiscMeshoperators::TestConvertVTKPolydataToUnstructuredGrid();
  TestMiscMeshoperators::TestConvertVTKToOFF();
  TestMiscMeshoperators::TestExtractAllSurfacesByMaterial();
  TestMiscMeshoperators::TestExtractNodeSet();
  TestMiscMeshoperators::TestExtractPointPositions();
  TestMiscMeshoperators::TestExtractVectorField();
  TestMiscMeshoperators::TestExtractSurfaceMesh();
  TestMiscMeshoperators::TestProjectSurfaceMesh();
  TestMiscMeshoperators::TestVoxelizeSurfaceMesh();



	//example: mesh stl surface files with tetgen, export volume to vtk and inp (Abaqus) and export the corresponding surface to stl
  //	std::vector<std::string> inputSurfaceMeshes;
//	std::vector<std::string> outputVolumeMeshes;
//	std::vector<std::string> outputVolumeMeshesINP;
//	std::vector<std::string> outputSurfaceMeshes;
//	TestExtractSurfaceMeshFromVolumeMeshByCelldataOperator();
//
//	inputSurfaceMeshes.push_back("/org/share/home/mediassi/MediAssistData/Modelle/MIC/Modellbibliothek/Graphical/AsianDragon/Volume/AsianDragon3kTet10.inp");
//	outputVolumeMeshes.push_back("/org/share/home/mediassi/MediAssistData/Modelle/MIC/Modellbibliothek/Graphical/AsianDragon/Volume/AsianDragon3kTet10.vtk");
//
//	inputSurfaceMeshes.push_back("/org/share/home/mediassi/MediAssistData/Modelle/MIC/Modellbibliothek/Graphical/AsianDragon/Volume/AsianDragon1kTet10.inp");
//	outputVolumeMeshes.push_back("/org/share/home/mediassi/MediAssistData/Modelle/MIC/Modellbibliothek/Graphical/AsianDragon/Volume/AsianDragon1kTet10.vtk");
//
//	inputSurfaceMeshes.push_back("/org/share/home/mediassi/MediAssistData/Modelle/MIC/Modellbibliothek/Graphical/AsianDragon/Volume/AsianDragon220Tet10.inp");
//	outputVolumeMeshes.push_back("/org/share/home/mediassi/MediAssistData/Modelle/MIC/Modellbibliothek/Graphical/AsianDragon/Volume/AsianDragon220Tet10.vtk");
//
//	inputSurfaceMeshes.push_back("/org/share/home/mediassi/MediAssistData/Modelle/MIC/Modellbibliothek/Graphical/AsianDragon/Volume/AsianDragon3kTet10Projected.inp");
//	outputVolumeMeshes.push_back("/org/share/home/mediassi/MediAssistData/Modelle/MIC/Modellbibliothek/Graphical/AsianDragon/Volume/AsianDragon3kTet10Projected.vtk");
//
//	inputSurfaceMeshes.push_back("/org/share/home/mediassi/MediAssistData/Modelle/MIC/Modellbibliothek/Graphical/AsianDragon/Volume/AsianDragon1kTet10Projected.inp");
//	outputVolumeMeshes.push_back("/org/share/home/mediassi/MediAssistData/Modelle/MIC/Modellbibliothek/Graphical/AsianDragon/Volume/AsianDragon1kTet10Projected.vtk");
//
//	inputSurfaceMeshes.push_back("/org/share/home/mediassi/MediAssistData/Modelle/MIC/Modellbibliothek/Medical/Rectum/Rectum.vtk");
//	outputVolumeMeshes.push_back("/org/share/home/mediassi/MediAssistData/Modelle/MIC/Modellbibliothek/Medical/Rectum/RectumVolume.vtk");
//
//	int numberOfModels = inputSurfaceMeshes.size();
//
//	for (unsigned int i = 0; i< numberOfModels;i++)
//	{
//		string errormessage;
//		TetgenOperators::CreateVolumeMesh(inputSurfaceMeshes[i].c_str(), outputVolumeMeshes[i].c_str(), true, false, &errormessage);
////		MiscMeshOperators::VTKToInp( outputVolumeMeshes[i].c_str(), outputVolumeMeshesINP[i].c_str(),  &errormessage);
////		MiscMeshOperators::ExtractSurfaceMesh( outputVolumeMeshes[i].c_str(), outputSurfaceMeshes[i].c_str(),  &errormessage);
////		MiscMeshOperators::ConvertInpToVTK(inputSurfaceMeshes[i].c_str(), outputVolumeMeshes[i].c_str(),&errormessage );
//
//	}
	return EXIT_SUCCESS;
}













