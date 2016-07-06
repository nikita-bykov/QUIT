#!/bin/bash -eu

#
# Example Processing Script For MCDESPOT data
# By Tobias Wood, with help from Anna Coombes
#

if [ $# -ne 3 ]; then
cat << END_USAGE
Usage: $0 spgr_file.nii irspgr_file.nii ssfp_file.nii

This script will produce T1, T2 and MWF maps from DESPOT data using
DESPOT1-HIFI for B1 correction. It requires as input the SPGR, IR-SPGR and SSFP
file names. A rough processing mask will be automatically generated.

Requires FSL.

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
IRSPGR_FILE="$2"
SSFP_FILE="$3"

SPGR_ROOT="${SPGR_FILE%%.nii*}"
IRSPGR_ROOT="${IRSPGR_FILE%%.nii*}"
SSFP_ROOT="${SSFP_FILE%%.nii*}"

export QUIT_EXT=NIFTI
export FSLOUTPUTTYPE=NIFTI

# Change this line to change the default number of threads for QUIT programs
NTHREADS="-T4"

#
# EDIT THE VARIABLES IN THIS SECTION FOR YOUR SCAN PARAMETERS
#
# These values are for example purposes and likely won't work
# with your data.
#
# All times (e.g. TR) are specified in SECONDS, not milliseconds
#

SPGR_FLIP="3 4 5 6 7 9 13 18"
SPGR_TR="0.008"
SPGR_TE="0.003"

IR_SPGR_FLIP="5"
IR_SPGR_TR="0.008"
IR_SPGR_NPE="68"
IR_SPGR_K0="0"
IR_SPGR_TI="0.45"
IR_SPGR_DELAY="0.0"
IR_SPGR_EFF="1.0"

SSFP_TR="0.004208"
SSFP_FLIP="12 16 21 27 33 40 51 68 "
SSFP_PHASE="180 0"

# Motion correction
# First register the SSFP to the average volume (to approximately remove bands)

echo "Motion correction"
SPGR_MCF="${SPGR_ROOT}_mcf.nii"
IRSPGR_MCF="${IRSPGR_ROOT}_mcf.nii"
SSFP_MCF="${SSFP_ROOT}_mcf.nii"

mcflirt -in $SSFP_FILE -cost mutualinfo -meanvol -stages 4
mcflirt -in $SPGR_FILE -cost mutualinfo -reffile ${SSFP_ROOT}_mcf_mean_reg -stages 4
flirt -in $IRSPGR_FILE -ref $SSFP_MCF -cost mutualinfo -searchcost mutualinfo -interp sinc -out $IRSPGR_MCF

# Generate an approximate mask

echo "Generating mask"
MASK_FILE="${SPGR_ROOT}_mask.nii"
qimask $SPGR_MCF --fillh=2 -o $MASK_FILE

# Process DESPOT1-HIFI to get an approximate B1 map

echo "Processing HIFI."
qidespot1hifi -n -v --clamp=5.0 --mprage -m $MASK_FILE $SPGR_MCF $IRSPGR_MCF $NTHREADS <<END_HIFI
$SPGR_FLIP
$SPGR_TR
$IR_SPGR_FLIP
$IR_SPGR_TR
$IR_SPGR_NPE
$IR_SPGR_K0
$IR_SPGR_TI
$IR_SPGR_DELAY
$IR_SPGR_EFF
END_HIFI

# Fit a polynomial to the B1 map to remove WM/GM/CSF contrast

echo "Fitting B1 polynomial"
qipolyfit --mask=$MASK_FILE --order=8 HIFI_B1 | qipolyimg --order=8 --mask=$MASK_FILE HIFI_B1.nii POLY_B1.nii

# Recalculate T1/PD map using the fitted B1 map

echo "Recalculating T1 map"
qidespot1 -n -v --clampT1=5.0 --mask=$MASK_FILE --B1=POLY_B1.nii $SPGR_MCF <<END_D1
$SPGR_FLIP
$SPGR_TR
END_D1

# Process DESPOT2-FM to get a T2 and f0 map
# FM is automatically clamped between 0.001 and T1 seconds

echo "Processing FM"
qidespot2fm -n -v -m $MASK_FILE -b POLY_B1.nii D1_T1.nii $SSFP_MCF $NTHREADS <<END_FM
$SSFP_FLIP
$SSFP_PHASE
$SSFP_TR
END_FM

# Now process MCDESPOT, using the above files, B1 and f0 maps to remove as many parameters as possible.

qimcdespot -n -v -m $MASK_FILE -f FM_f0.nii -b POLY_B1.nii -M3 -S $NTHREADS -s "0 0 48 80 80 1" <<END_MCD
$SPGR_MCF
SPGR_ECHO
$SPGR_FLIP
$SPGR_TR
$SPGR_TE
$SSFP_MCF
SSFP_ECHO
$SSFP_FLIP
$SSFP_PHASE
$SSFP_TR
END
END_MCD
