Repository to hold scripts for building combined track finder firmware as well as for generating emulation that can be used to compare with the firmware output.

To generate a firmware project, run

~~~~bash
  ./make_new_emp_project.sh <directory_name>
~~~~

with an optional second argument to specify the project to build (`IntegrationTests/ReducedCombinedConfig/IRtoKF` or `IntegrationTests/CombinedBarrelConfig/IRtoKF`).

To generate the CMSSW emulation output for comparison run

~~~~bash
  ./make_cmssw_memprints.sh barrel
~~~~

to generate the output for the barrel project, or omit the barrel argument to generate the output for the reduced configuration.
