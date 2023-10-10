#!/usr/bin/env bash

#script to automatically generates emp track finder project

EMP_BUILD_PATH="IntegrationTests/ReducedCombinedConfig/IRtoKF"

function set_vivado2020_1() {
  source /nfs/opt/Xilinx/Vitis/2020.1/settings64.sh
}

function set_vivado2022_1() {
  source /nfs/opt/Xilinx/Vitis/2022.1/settings64.sh
}

function echo_usage() {
  echo "Usage: ./make_new_emp_project.sh PROJECT_DIRECTORY EMP_BUILD_PATH"
  echo "This script requires that you are added to the emp-fwk-users egroup and have an ssh key associated with your gitlab.cern.ch account"
}

if [ "$1" == "" ];
then
  echo "ERROR: insufficient arguments"
  echo_usage
  exit 1
#elif [ -d "$1" ];
#then
#  echo "ERROR: output directory already exists"
#  echo_usage
#  exit 1
fi

if [ "$2" != "" ];
then
  EMP_BUILD_PATH="$2"
fi

ipbb init $1
cd $1
ipbb add git ssh://git@gitlab.cern.ch:7999/p2-xware/firmware/emp-fwk.git -b v0.8.0
ipbb add git https://github.com/apollo-lhc/CM_FPGA_FW -b v2.2.1
ipbb add git https://gitlab.cern.ch/ttc/legacy_ttc.git -b v2.1
ipbb add git ssh://git@gitlab.cern.ch:7999/cms-tcds/cms-tcds2-firmware.git -b v0_1_1
ipbb add git https://gitlab.cern.ch/HPTD/tclink.git -r fda0bcf
ipbb add git https://github.com/ipbus/ipbus-firmware -b v1.9
ipbb add git https://gitlab.cern.ch/dth_p1-v2/slinkrocket_ips.git -b v03.09
ipbb add git ssh://git@gitlab.cern.ch:7999/dth_p1-v2/slinkrocket.git -b v03.10
ipbb add git https://gitlab.cern.ch/gbt-fpga/gbt-fpga.git -b gbt_fpga_6_1_0
ipbb add git https://gitlab.cern.ch/gbt-fpga/lpgbt-fpga.git -b v.2.1
ipbb add git https://:@gitlab.cern.ch:8443/gbtsc-fpga-support/gbt-sc.git -b gbt_sc_4_1
ipbb add git https://github.com/mcoshiro/firmware-hls.git -b emp_updates

#fix which address tables are referenced
cd src/CM_FPGA_FW/kernel/address_tables
#make prebuild_EMP_Cornell_rev2_p1_VU13p-1-SM_USP
rm address_table
ln -s address_table_EMP_Cornell_rev2_p1_VU13p-1-SM_USP address_table
cd -

#hot fixes
#no longer needed for emp_timing branch
#TODO: update branch with additional fixes
#cd src/firmware-hls/$EMP_BUILD_PATH/firmware/cfg
#sed -i 's/src --vhdl2008 common\/hdl\/tf_lutdat.vhd/#src --vhdl2008 common\/hdl\/tf_lutdat.vhd/' payload.dep
#sed -i 's/cm_v1/cm_v2/g' vsim.dep
#sed -i 's/vu7p/vu13p/' vsim.dep
#cd -

#build HLS
set_vivado2020_1
cd src/firmware-hls
make -C $EMP_BUILD_PATH/firmware
cd -
set_vivado2022_1

#make simulation project
ipbb proj create vivado vsim firmware-hls:$EMP_BUILD_PATH 'vsim.dep'
cd proj/vsim
ipbb vivado generate-project

#make implementation project
ipbb proj create vivado apollo firmware-hls:$EMP_BUILD_PATH 'apollo.dep'
cd proj/apollo
ipbb ipbus gendecoders
ipbb vivado generate-project synth -j8 impl -j8 package

