.. QUIT documentation master file, created by
   sphinx-quickstart on Wed Oct  3 16:02:58 2018.
   You can adapt this file completely to your liking, but it should at least
   contain the root `toctree` directive.

Welcome to QUIT's documentation!
================================

Welcome to the QUantitative Imaging Tools, a set of C++ tools for analysing quantitative MR images. QUIT is provided as a single executable named ``qi`` which contains numerous commands for processing images (similar to ``bart`` or ``git``). These commands are grouped in the following modules:

.. toctree::
   :maxdepth: 2
   :caption: Contents:

   Docs/Contents

Citing
------

QUIT has been published in the Journal of Open Source Software. If you use it, please cite:

.. note::

    Wood, (2018). QUIT: QUantitative Imaging Tools. Journal of Open Source Software, 3(26), 656, https://doi.org/10.21105/joss.00656

Thank you.

Installation
------------

Pre-compiled binaries are provided for Linux and Mac OS X from http://github.com/spinicist/QUIT/releases.

Download the correct archive for your platform, untar it and then ensure that the binaries can be found via your `PATH` environment variable. Either edit your shell profile (e.g. ``.bashrc``) and add the QUIT directory your ``$PATH`` variable there, or copy the files to somewhere that will be on your path, e.g. ``/usr/local/bin`` (this will likely require ``sudo`` permissions).

The Linux binaries are built with Ubuntu 16.04 with GCC 7 and ``glibc`` version 2.17. If you need to run on an older version of Linux with a previous version of ``glibc`` then you may need to compile from source.

Compile From Source
-------------------

Compiling from source is recommended to keep up to date with the ``master`` branch. A ``build.sh`` script is provided that should automate this process. By default, ``build.sh`` only builds the binary commands. To install these to a single directory, you must run ``./build.sh -i``. The default install location is ``/usr/local/bin``, but this can be changed by running ``./build.sh -i -p /path/to/install/prefix/``.

For more details see the :doc:`Docs/Developer` documentation.

General Usage
-------------

If you run ``qi`` a list of available commands will be printed to the terminal. QUIT commands take their input as a combination of command-line arguments and text files passed to ``stdin``. If you run a QUIT command with no arguments then will see a message like this:

.. code-block:: bash

    ~: qi despot1
    SPGR FILE was not specified. Use --help to see usage.
    >

If you then run it with ``--help``, you will see some usage instructions:

.. code-block:: bash

    ~: qi despot1 --help
    qi despot1 {OPTIONS} [SPGR FILE]

        Calculates T1 maps from SPGR data
        http://github.com/spinicist/QUIT

    OPTIONS:

        SPGR FILE                         Path to SPGR data
        -h, --help                        Show this help message
        -v, --verbose                     Print more information
        -T[THREADS], --threads=[THREADS]  Use N threads (default=4, 0=hardware
                                            limit)
        -o[OUTPREFIX], --out=[OUTPREFIX]  Add a prefix to output filenames
        -b[B1], --B1=[B1]                 B1 map (ratio) file
        -m[MASK], --mask=[MASK]           Only process voxels within the mask
        -s[SUBREGION],
        --subregion=[SUBREGION]           Process subregion starting at voxel
                                            I,J,K with size SI,SJ,SK
        -r, --resids                      Write out residuals for each data-point
        -a[ALGO], --algo=[ALGO]           Choose algorithm (l/w/n)
        -i[ITERS], --its=[ITERS]          Max iterations for WLLS/NLLS (default
                                            15)
        -p[CLAMP PD], --clampPD=[CLAMP
        PD]                               Clamp PD between 0 and value
        -t[CLAMP T1], --clampT2=[CLAMP
        T1]                               Clamp T1 between 0 and value
        "--" can be used to terminate flag options and force all following
        arguments to be treated as positional options

The first line shows that DESPOT1 expects a single input image, in this case SPGR/FLASH/FFE, specified on the command line. However, if you naively run:

.. code-block:: bash

    ~: qi despot1 some_spgr_data.nii.gz

Nothing will happen. This is because most imaging formats do not store parameters that are required for quantitative imaging, e.g. the repetition time and flip-angles in their headers. QUIT commands hence expect to read this information in a text file passed to ``stdin``. If you create a small text file ``spgr.json`` containing the following:

.. code-block:: json

    {
        "SPGR": {
            "TR": 0.01,
            "FA": [3, 18]
        }
    }

and run the following:

.. code-block:: bash

    ~: qi despot1 some_spgr_data.nii.gz < spgr.json

then - provided your input data does contain two volumes corresponding to flip-angles 3 and 18 degrees - DESPOT1 will run, and you should see two files created (``D1_T1.nii.gz`` and ``D1_PD.nii.gz``). If you want to see what the commands are doing while running, specify the ``--verbose`` or ``-v`` options.

Common Options
--------------

The following options are supported by most, but not necessarily all, QUIT commands.

* ``--out, -o``

    Add a prefix to the output parameter files. By default, most QUIT commands write their output files using filenames in a pattern `command_PARAMETER.nii.gz`. They will overwrite any existing files with the same names. If you need to save the output from multiple runs of the same command, or want to save output to a particular directory, use this option to add an addtional prefix to the output names.

* ``--mask, -m``

    Specify a mask file, where non-zero values indicate that voxels should be processed, and zero values indicate that voxels should not be processed. The background voxels will be set to zero in the final image. This is useful for two reasons, first is to simply speed up processing for long-running commands (e.g. `qi mcdespot`), second is that outside the head fitting data is non-sensical, as there is no signal. Hence these regions appear very noisy on output maps, which can make visualization difficult.

* ``--threads, -t``

    Control the maximum number of threads used. The majority of QUIT commands are multi-threaded across voxels to improve processing times. In some parallel computing environments (e.g. Sun Grid Engine), it is possible to set the maximum number of cores available to a command, and it is hence good for CPU utilisation to match the number of threads to the number of cores. The default is 4. Note that HyperThreading may make the number of logical cores appear to be double the number of physical cores - QUIT commands are CPU bound, not IO bound, and hence gain no benefit from HyperThreading. You are better to specify the number of physical cores available rather than the number of logical cores.

* ``--subregion, -s``

    Similar to `--mask`, this command will only process a sub-region of the input images. The argument needs to be in the format `"start_i,start_j,start_k,size_i,size_j,size_k"` where `i,j,k` are voxel indices (not physical co-ordinates). This is useful to speed up processing for trial-runs of pipelines.

* ``--resids, -r``

    Most QUIT commands will write out a single root-sum-squared residual image along with their parameter maps. Use this option to also output residuals for each data-point to look for systematic offsets. Note that if multiple inputs are specified (e.g. `qi mcdespot`), then this option will write out a single cocatenated file for all input data-points in order.

* ``--B1, -b`` & ``--f0, -f``

    Several of the QUIT commands take B1 (relative flip-angle) and f0 (off-resonance in Hz) maps as correction factors.

File Formats
------------

By default, QUIT is compiled with support for NIFTI and NRRD formats. The preferred file-format is NIFTI for compatibility with FSL and SPM. By default QUIT will output ``.nii.gz`` files. This can be controlled by the `QUIT_EXT` environment variable. Valid values for this are any file extension supported by ITK that QUIT has been compiled to support, e.g. ``.nii`` or ``.nrrd``, or the FSL values ``NIFTI``, ``NIFTI_PAIR``, ``NIFTI_GZ``, ``NIFTI_PAIR_GZ``.

The `ITK <http://itk.org>`_ library supports a much wider variety of file formats, but adding support for all of these almost triples the size of the compiled binaries. Hence by default they are excluded. You can add support for more file formats by compiling QUIT yourself, see the :doc:`Docs/Developer` documentation. Note that ITK cannot write every format it can read (e.g. it can read Bruker 2dseq datasets, but it cannot write them).

Python Integration / Scripting
------------------------------

A set of ``nipype`` wrappers for all QUIT commands is provided, which can be found in the ``Python/QUIT`` subdirectory. The current recommended way to use these is to clone the QUIT git repository, then ``cd`` to the ``Python/QUIT`` directory and type ``pip install -e .``. This will allow you to update the wrappers with a ``git pull`` in the future. If anyone is an expert at Python packaging and knows how to upload the wrappers to ``pip`` or ``conda-forge``, please get in touch.


Indices and tables
------------------

* :ref:`genindex`
* :ref:`modindex`
* :ref:`search`
