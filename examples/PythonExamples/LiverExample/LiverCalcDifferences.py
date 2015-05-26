import os
import sys
sys.path.insert(0,"/opt/msml/src") # to use msml imports
import msml.api.simulation_runner as api
import numpy as np

# define infiles and output directory
msml_infile_Linear = os.path.abspath("liverLinear.msml.xml") # stiffness 3000
msml_infile_nonLinear = os.path.abspath("liverLinearStiff1000.msml.xml") # stiffness 1000
msml_outdirLinear = os.path.abspath("/tmp/MSMLResultsLiverLinear/")
msml_outdirNonLinear = os.path.abspath("/tmp/MSMLResultsLiverNonLinear/")

# run msml and get specified points
myRunner = api.SimulationRunner(msml_infile_Linear, "sofa", msml_outdirLinear)
myRunner.run()
referencePoints_Linear = myRunner.get_results('finalPointDisp', 'points')

myRunner = api.SimulationRunner(msml_infile_nonLinear, "sofa", msml_outdirNonLinear)
myRunner.run()
referencePoints_nonLinear = myRunner.get_results('finalPointDisp', 'points')

npArrayLinear = np.array(referencePoints_Linear)
npArrayNonLinear = np.array(referencePoints_nonLinear)

# calculate difference
difference = np.subtract(npArrayLinear,npArrayNonLinear)
print("difference: " + str(difference))

# calculate root mean square error
rms = 0.0
for i in range(len(difference)):
    value = np.power(difference[i],2)
    rms += value

rms = np.sqrt(rms)

print("Root mean square: " + str(rms))