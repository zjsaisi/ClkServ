# ****************************************************************************
# **    IXXAT Automation GmbH
# ****************************************************************************
# **
# **       File: GeneSIS_step2.sh
# **    Summary: This shell file has to be called after each modification of the SIS 
# **                      configuration file SIScfg.wzd. It generates the SIS source code files
# **                      appropriate to the configurations stored in the SIScfg.wzd file.
# ** 
# **     Author:  Werner Abt
# **
# ****************************************************************************


# set path to SIStool folder here
SISTOOL_DIR='../../../../SIStool'

# then generate the new SIS-files out of the SIScfg.wzd
python $SISTOOL_DIR/geneSIS.py 
