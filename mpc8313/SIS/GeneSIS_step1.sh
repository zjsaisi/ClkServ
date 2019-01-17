# ****************************************************************************
# **    IXXAT Automation GmbH
# ****************************************************************************
# **
# **       File: GeneSIS_step1.sh
# **    Summary: This shell file has to be called after each modification of the target.h
# **                      file in the IEEE1588 V2 stack.  It generates the configuartion wizard file 'SIScfg.wzd'.
# **                      This configuration file can be changed e.G. memory pool sizes.
# **                      After that it is necessary to store the changed SIScfg.wzd file and
# **                      call the shell-file 'GeneSIS_step2.sh.
# ** 
# **     Author:  Werner Abt
# **
# ****************************************************************************

# first clean old files
rm -f SISerror.c
rm -f SISmain.c
rm -f SISmbox.c
rm -f SISmpool.c
rm -f SISevent.c
rm -f SIShdls.h
rm -f *.o
rm -f SIScfg.wzd

# set path to SIStool folder here
SISTOOL_DIR='../../../../SIStool'
# set path to target file folder
TARGET_DIR='../../Proj'

# call GenSISwzd.py in the SIStool folder to create the SIScfg.wzd file
python $SISTOOL_DIR/GenSISwzd.py $TARGET_DIR/target.h
