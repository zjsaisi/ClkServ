###############################################################################
#
#
#
#
###############################################################################

.PHONY: all clean

#all:
#	$(MAKE) -C mpc8313 $@
#	$(MAKE) -C ptp/8313/ $@
#
lib:
	$(MAKE) -C ptp/8313/ $@

#clean:
#	$(MAKE) -C mpc8313 $@
#	$(MAKE) -C ptp/8313/ $@