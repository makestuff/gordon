# 
# Copyright (C) 2013 Chris McClelland
#
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; version 2 of the License.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301 USA
#
ROOT    := $(realpath ../../..)
DEPS    := argtable2
TYPE    := exe
SUBDIRS :=

EXTRA_INCS := -I../common -I$(ROOT)/../fpga-cam/userapi
EXTRA_SRC_DIRS := ../common
LINK_EXTRALIBS_REL := -L$(ROOT)/../fpga-cam/userapi -lfpgacam
LINK_EXTRALIBS_DBG := $(LINK_EXTRALIBS_REL)

-include $(ROOT)/common/top.mk
