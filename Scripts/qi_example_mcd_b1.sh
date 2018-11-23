#!/bin/bash -e

#
# Example Processing Script For MCDESPOT data
# By Tobias Wood, with help from Anna Coombes
#

if [ $# -ne 4 ]; then
cat << END_USAGE
Usage: $0 spgr_file.nii ssfp_file.nii b1_file.nii mask.nii

This script will produce T1, T2 and MWF maps from DESPOT data using
an external B1 map. It requires as input the SPGR, SSFP and B1 map
file names, and a mask generated by FSL BET or other means.

Assumes that registration has been done before this script is called..

You MUST edit the script to the flip-angles, TRs etc. used in your scans.

This script assumes that the two SSFP phase-cycling patterns have been
concatenated into a single file, e.g. with fslmerge. Pay attention to which
order this is done in.

By default this script uses all available cores on a machine. If you wish to
specify the number of cores/threads to use uncomment the NTHREADS variable.
END_USAGE
exit 1;
fi

SPGR_FILE="$1"
SSFP_FILE="$2"
B1_FILE="$3"
MASK_FILE="$4"

export QUIT_EXT=NIFTI
export FSLOUTPUTTYPE=NIFTI

#NTHREADS="-T4"

#
# EDIT THE VARIABLES IN THIS SECTION FOR YOUR SCAN PARAMETERS
#
# These values are for example purposes and likely won't work
# with your data.
#
# All times (e.g. TR) are specified in SECONDS, not milliseconds
#

SPGR_SEQ='"SPGR": { "FA": [2,3,4,5,6,7,9,13,18], "TR": 0.008 }'
SSFP_SEQ='"SSFP": { "TR": 0.004,
                    "FA": [12,16,21,27,33,41,52.5,70,12,16,21,27,33,41,52.5,70],
                    "PhaseInc": [0,0,0,0,0,0,0,0,180,180,180,180,180,180,180,180] }'

#
# Process DESPOT1 T1 and PD
#

echo "Processing T1."
qidespot1 -v -an -b $B1_FILE -m $MASK_FILE $SPGR_FILE $NTHREADS <<END_HIFI
{
    $SPGR_SEQ
}
END_HIFI

# Process DESPOT2-FM to get a T2 and f0 map that we will use with mcdespot

echo "Processing T2"
qidespot2fm -v -b $B1_FILE -m $MASK_FILE D1_T1.nii $SSFP_FILE $NTHREADS <<END_FM
{
    $SSFP_SEQ
}
END_FM

# Now process MCDESPOT, using the above files, B1 and f0 maps to remove as many parameters as possible.
# The SPGR echo-time is used to correct for differential decay between the components

qimcdespot -v -m $MASK_FILE -f FM_f0.nii -b $B1_FILE -M3 -S $SPGR_FILE $SSFP_FILE $NTHREADS <<END_MCD
{
    "Sequences": [
        { $SPGR_SEQ },
        { $SSFP_SEQ }
    ]
}
END_MCD
