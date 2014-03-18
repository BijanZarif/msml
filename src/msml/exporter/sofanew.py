# region gplv3preamble
# The Medical Simulation Markup Language (MSML) - Simplifying the biomechanical modeling workflow
#
# MSML has been developed in the framework of 'SFB TRR 125 Cognition-Guided Surgery'
#
# If you use this software in academic work, please cite the paper:
#   S. Suwelack, M. Stoll, S. Schalck, N.Schoch, R. Dillmann, R. Bendl, V. Heuveline and S. Speidel,
#   The Medical Simulation Markup Language (MSML) - Simplifying the biomechanical modeling workflow,
#   Medicine Meets Virtual Reality (MMVR) 2014
#
# Copyright (C) 2013-2014 see Authors.txt
#
# If you have any questions please feel free to contact us at suwelack@kit.edu
#
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <http://www.gnu.org/licenses/>.
# endregion


__authors__ = 'Stefan Suwelack, Alexander Weigl'
__license__ = 'GPLv3'
__date__ = "2014-03-13"

from warnings import warn
import os
import math

from ..model import *
from .base import XMLExporter, Exporter
from path import path

import lxml.etree as etree
from msml.model.exceptions import *


class MSMLSOFAExporterWarning(MSMLWarning): pass


class SofaExporter(XMLExporter):
    def __init__(self, msml_file):
        """
      Args:
       executer (Executer)


      """
        self.name = 'SOFAExporter'
        Exporter.__init__(self, msml_file)
        self.export_file = None
        self.working_dir = path()

    def init_exec(self, executer):
        """
     initialization by the executer, sets memory and executor member
     :param executer: msml.run.Executer
     :return:
     """
        self._executer = executer
        self._memory = self._executer._memory

    def render(self):
        """
     Builds the File (XML e.g) for the external tool
     """
        print("Converting to sofa scn")
        self.export_file = self._msml_file.filename[0:-3] + 'scn'
        print self.export_file

        import codecs

        with codecs.open(self.export_file, 'w', 'utf-8') as scnfile:  #should be open with codecs.open
            rootelement = self.write_scn()
            #tree.write(filename, pretty_print=True)
            s = etree.tostring(rootelement, encoding="utf-8")
            scnfile.write(s)


    def execute(self):
        "should execute the external tool and set the memory"
        print("Executing sofa.")
        os.system("runSofa %s" % self.export_file)


    def write_scn(self):
        processingUnit = self._msml_file.env.solver.processingUnit
        self._processing_unit = "Vec3f" if processingUnit == "CPU" else "CudaVec3f"

        self.node_root = self.createScene()

        for msmlObject in self._msml_file.scene:
            assert isinstance(msmlObject, SceneObject)

            #create object, the mesh, material regions and constraints
            objectNode = self.createObject(self.node_root, msmlObject)

            #physicsElementNode = msmlObject.find("material")
            self.createMeshTopology(self.node_root, objectNode, msmlObject)
            self.createMaterialRegions(msmlObject)

            #create simulation steps
            self.createConstraintRegions(msmlObject)

            #creat post processing request
            self.createPostProcessingRequests(msmlObject)

            #add solver
            self.createSolvers(msmlObject)

        return etree.ElementTree(self.node_root)

    def createMeshTopology(self, sceneNode, objectNode, msmlObject):
        assert isinstance(msmlObject, SceneObject)
        mesh_value = msmlObject.mesh.mesh
        mesh_type = msmlObject.mesh.type

        theFilename = self.working_dir / self.evaluate_node(mesh_value)

        # TODO currentMeshNode.get("name" )) - having a constant name for our the loader simplifies using it as source for nodes generated later.
        # TODO does not work as expected. If a single triangle exists in mesh, then for each facet of all tets a triangle is ceated... SOFA bug?

        self.sub("MechanicalObject", name="dofs", template=self._processing_unit, src="@LOADER")
        self.sub("MeshTopology", name="topo", src="@LOADER")
        #check if child is present
        #if so, check operator compatibility
        #execute operator
        #check if mesh is in MSML folder

        #if not -> copy

        if mesh_type == "linearTet":
            loaderNode = self.sub("MeshVTKLoader", name="LOADER", filename=theFilename,
                                  createSubelements=0)
        elif mesh_type == "quadraticTet":
            loaderNode = self.sub("MeshExtendedVTKLoader", name="LOADER", filename=theFilename)
        else:
            print("Mesh type must be mesh.volume.linearTetrahedron.vtk or mesh.volume.quadraticTetrahedron.vtk")

        self.sub("MechanicalObject", name="dofs", template=self._processing_unit, src="@LOADER")
        self.sub("QuadraticMeshTopology", name="topo", src="@LOADER")

        return loaderNode

    def createSolvers(self, sceneNode):
        if self._msml_file.env.solver.timeIntegration == "dynamicImplicit":
            self.sub("MyNewmarkImplicitSolver",
                     rayleighStiffness="0.2",
                     rayleighMass="0.02",
                     name="odesolver")

        elif self._msml_file.env.solver.timeIntegration == "dynamicImplicitEuler":
            self.sub("EulerImplicitSolver",
                     name="odesolver")
        else:
            warn(MSMLSOFAExporterWarning, "Error ODE solver %s not supported" %
                                          self._msml_file.env.solver.timeIntegration)

        if self._msml_file.env.solver.linearSolver == "direct":
            self.sub("SparseMKLSolver")
        elif self._msml_file.env.solver.linearSolver == "iterativeCG":
            self.sub("CGLinearSolver",
                     iterations="100", tolerance="1e-06", threshold="1e-06")
        else:
            warn(MSMLSOFAExporterWarning, "Error linear solver %s not supported" %
                                          self._msml_file.env.solver.linearSolver)


    def createMaterialRegions(self, msmlObject):
        assert isinstance(msmlObject, SceneObject)

        youngs = {}
        poissons = {}
        density = {}

        for matregion in msmlObject.material:
            assert isinstance(matregion, MaterialRegion)
            indexGroupNode = matregion.get_indices()

            assert isinstance(indexGroupNode, ObjectElement)

            indices_key = indexGroupNode.attributes["indices"]
            indices_vec = self.evaluate_node(indices_key)
            indices = '%s' % ', '.join(map(str, indices_vec))

            indices_int = [int(i) for i in indices.split(",")]

            #Get all materials
            for material in matregion:
                assert isinstance(material, ObjectElement)

                currentMaterialType = material.attributes['__tag__']
                if currentMaterialType == "indexgroup":
                    continue

                if currentMaterialType == "linearElastic":
                    currentYoungs = material.attributes["youngModulus"]
                    currentPoissons = material.attributes["poissonRatio"]  # not implemented in sofa yet!
                    for i in indices_int:  #TODO Performance (maybe generator should be make more sense)
                        youngs[i] = currentYoungs
                        poissons[i] = currentPoissons
                elif currentMaterialType == "mass":
                    currentDensity = material.attributes["density"]
                    for i in indices_int:
                        density[i] = currentDensity
                else:
                    warn(MSMLSOFAExporterWarning, "Material Type not supported %s" % currentMaterialType)

        keylist = density.keys()
        keylist.sort()

        _select = lambda x: (x[k] for k in keylist)
        _to_str = lambda x: ' '.join(_select(x))

        density_str = _to_str(density)
        youngs_str = _to_str(youngs)
        poissons_str = _to_str(poissons)


        #merge all different materials to single forcefield/density entries.
        if self.node_root.find("MeshTopology") is not None:
            elasticNode = self.sub("TetrahedronFEMForceField", template=self._processing_unit, name="FEM",
                                   listening="true", youngModulus=youngs_str,
                                   poissonRatio=poissons[keylist[0]])
            self.sub("TetrahedronSetGeometryAlgorithms",
                     name="aTetrahedronSetGeometryAlgorithm",
                     template=self._processing_unit)
            massNode = self.sub("DiagonalMass", name="meshMass")
            massNode.set("massDensity", density_str)
        elif self.node_root.find("QuadraticMeshTopology") is not None:
            eelasticNode = self.sub("QuadraticTetrahedralCorotationalFEMForceField",
                                    template=self._processing_unit, name="FEM", listening="true",
                                    setYoungModulus=youngs_str,
                                    setPoissonRatio=poissons[keylist[0]])  # TODO
            emassNode = self.sub("QuadraticMeshMatrixMass", name="meshMass", massDensity=density_str)
        else:
            warn(MSMLSOFAExporterWarning, "Current mesh topology not supported")


    def createConstraintRegions(self, msmlObject):
        assert isinstance(msmlObject, SceneObject)

        for constraint_set in (msmlObject.constraints[0], ):  #TODO take all constraints
            assert isinstance(constraint_set, ObjectConstraints)

            for constraint in constraint_set.constraints:
                assert isinstance(constraint, ObjectElement)
                currentConstraintType = constraint.tag
                indices_vec = self.evaluate_node(constraint.indices)
                indices = '%s' % ', '.join(map(str, indices_vec))

                if currentConstraintType == "fixedConstraint":
                    constraintNode = self.sub("FixedConstraint",
                                              name=constraint.id or constraint_set.name,
                                              indices=indices)

                    #elasticNode.set("setPoissonRatio", material.get("poissonRatio"))

                    #check if child is present

                    #if so, check operator compatibility

                    #execute operator

                    #check if mesh is in MSML folder

                    #if not -> copy
                elif currentConstraintType == "surfacePressure":
                    constraintNode = self.sub("Node", name="SurfaceLoad")

                    self.sub("MeshTopology", constraintNode,
                             name="SurfaceTopo",
                             position="@LOADER.position",
                             triangles="@LOADER.triangles", quads="@LOADER.quads")

                    self.sub("MechanicalObject", constraintNode, template="Vec3f", name="surfacePressDOF",
                             position="@SurfaceTopo.position")

                    surfacePressureForceFieldNode = self.sub("SurfacePressureForceField", constraintNode,
                                                             template="Vec3f",
                                                             name="surfacePressure",
                                                             pulseMode="1",
                                                             pressureSpeed=str(float(
                                                                 constraint.pressure) / 10.0),
                                                             pressure=constraint.get("pressure"),
                                                             triangleIndices=indices)

                    self.sub("BarycentricMapping", constraintNode,
                             template="undef, Vec3f",
                             name="barycentricMapSurfacePressure",
                             input="@..", output="@.")

                elif currentConstraintType == "springMeshToFixed":

                    constraintNode = self.sub(sceneNode, "Node", name="springMeshToFixed")
                    mechObj = self.sub("MechanicalObject", constraintNode, template="Vec3f",
                                       name="pointsInDeformingMesh",
                                       position=constraint.get("movingPoints"))

                    self.sub("BarycentricMapping", constraintNode,
                             template="undef, Vec3f",
                             name="barycentricMapSpringMeshToFixed",
                             input="@..",
                             output="@.")

                    displacedLandLMarks = self.sub(constraintNode, "Node",
                                                   name="fixedPointsForSpringMeshToFixed")
                    mechObj = self.sub(displacedLandLMarks, "MechanicalObject",
                                       template="Vec3f",
                                       name="fixedPoints")

                    mechObj.set("position", constraint.get("fixedPoints"))

                    forcefield = self.sub(constraintNode, "RestShapeSpringsForceField",
                                          template="Vec3f",
                                          name="Springs",
                                          external_rest_shape="fixedPointsForSpringMeshToFixed/fixedPoints",
                                          drawSpring="true",
                                          stiffness=constraint.get("stiffness"),
                                          rayleighStiffnes=constraint.get("rayleighStiffnes"))

                elif currentConstraintType == "supportingMesh":

                    constraintNode = self.sub("Node", constraintNode,
                                              name="support_" + constraint.get("name"))
                    loaderNode = self.sub("MeshVTKLoader", constraintNode,
                                          name="LOADER_supportmesh",
                                          createSubelements="0",
                                          filename=constraint.get("filename"))

                    self.sub("MechanicalObject", constraintNode,
                             name="dofs",
                             src="@LOADER_supportmesh",
                             template="Vec3f",
                             translation="0 0 0")

                    self.sub("MeshTopology", constraintNode,
                             name="topo",
                             src="@LOADER_supportmesh")

                    forcefield = self.sub("TetrahedronFEMForceField", constraintNode, listening="true",
                                          name="FEM", template="Vec3f",
                                          youngModulus=constraint.get("youngModulus"),
                                          poissonRatio=constraint.get("poissonRatio"))

                    self.sub("TetrahedronSetGeometryAlgorithms", constraintNode,
                             name="aTetrahedronSetGeometryAlgorithm",
                             template="Vec3f")

                    diagonalMass = self.sub("DiagonalMass", constraintNode,
                                            name="meshMass",
                                            massDensity=constraint.get("massDensity"))

                    self.sub("BarycentricMapping", constraintNode,
                             input="@..",
                             name="barycentricMap",
                             output="@.",
                             template="undef, Vec3f")
                else:
                    warn(MSMLSOFAExporterWarning, "Constraint Type not supported %s " % currentConstraintType)


    def createObject(self, currentSofaNode, scobj):
        assert isinstance(scobj, SceneObject)
        objectNode = self.sub("Node", name=scobj.id)
        return objectNode


    def createScene(self):
        dt = "0.05"  # TODO find dt from msmlfile > env > simulation
        root = etree.Element("Node", name="root", dt=dt)
        theGravity = "0 0 -9.81"  # TODO find gravity in msmlfile > env > simulation stepNode.get("gravity")
        if theGravity is None:
            theGravity = '0 -9.81 0'
        root.set("gravity", theGravity)
        return root

        #sofa_exporter handles displacementOutputRequest only. Other postProcessing operators need to be adressed in... ?


    def createPostProcessingRequests(self, msmlObject):
        for request in msmlObject.output:
            assert isinstance(request, ObjectElement)
            filename = self.working_dir / request.id
            if request.tag == "displacementOutputRequest":
                if sceneNode.find("MeshTopology") is not None:
                    #dispOutputNode = self.sub(currentSofaNode, "ExtendedVTKExporter" )
                    exportEveryNumberOfSteps = request.get("timestep")

                    dispOutputNode = self.sub("VTKExporter",
                                              filename=filename,
                                              exportEveryNumberOfSteps=exportEveryNumberOfSteps,
                                              XMLformat=1,
                                              edges=0,
                                              #todo export material => allows extraction of surfaces in post processing
                                              tetras=1,
                                              triangles=0,
                                              listening="true",
                                              exportAtEnd="true")

                    timeSteps = self._msml_file.env.simulation[0].iterations

                    #exportEveryNumberOfSteps = 1 in SOFA means export every second time step.
                    #exportEveryNumberOfSteps = 0 in SOFA means do not export.
                    if exportEveryNumberOfSteps == 0:
                        lastNumber = 1
                    else:
                        lastNumber = int(math.floor(timeSteps / ( int(exportEveryNumberOfSteps) + 1)))

                    filenameLastOutput = filename + str(lastNumber) + ".vtu"
                    request.set("filename", filenameLastOutput)

                elif sceneNode.find("QuadraticMeshTopology") is not None:
                    dispOutputNode = self.sub("ExtendedVTKExporter",
                                              filename=filename,
                                              exportEveryNumberOfSteps=exportEveryNumberOfSteps,
                                              #todo export material => allows extraction of surfaces in post processing
                                              tetras=0,
                                              quadraticTetras=1,
                                              listening="true",
                                              exportAtEnd="true")

                    #TODO: Fill "filename" of request taking output numbering into account (see VTKExporter)
                else:
                    print "Topolgy type not supported"

    def sub(self, tag, root=None, **kwargs):
        skwargs = {k: str(v) for k, v in kwargs.items()}
        if not root: root = self.node_root
        return etree.SubElement(root, tag, **skwargs)