# ****************************************************************************
# **    IXXAT Automation GmbH
# ****************************************************************************
# **
# **       File: GeneSIS_compl.sh
# **    Summary: This shell file has to be called after each modification of the target.h
# **                      file in the IEEE1588 V2 stack.  It generates a configuartion wizard file and
# **                      after that it generates the SIS source code files out of it.
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

# then generate the new SIS-files out of the SIScfg.wzd
python $SISTOOL_DIR/geneSIS.py 
