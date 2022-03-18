# SPDX-License-Identifier: GPL-2.0+
# Copyright (c) 2016 Google, Inc
# Written by Simon Glass <sjg@chromium.org>
#
# Entry-type module for producing an image using mkimage
#

from collections import OrderedDict

from binman.entry import Entry
from dtoc import fdt_util
from patman import tools

class Entry_mkimage(Entry):
    """Binary produced by mkimage

    Properties / Entry arguments:
        - datafile: Filename for -d argument
        - args: Other arguments to pass

    The data passed to mkimage is collected from subnodes of the mkimage node,
    e.g.::

        mkimage {
            args = "-n test -T imximage";

            u-boot-spl {
            };
        };

    This calls mkimage to create an imximage with u-boot-spl.bin as the input
    file. The output from mkimage then becomes part of the image produced by
    binman.
    """
    def __init__(self, section, etype, node):
        super().__init__(section, etype, node)
        self._args = fdt_util.GetString(self._node, 'args').split(' ')
        self._mkimage_entries = OrderedDict()
        self.align_default = None
        self.ReadEntries()

    def ObtainContents(self):
        data = b''
        for entry in self._mkimage_entries.values():
            # First get the input data and put it in a file. If not available,
            # try later.
            if not entry.ObtainContents():
                return False
            data += entry.GetData()
        uniq = self.GetUniqueName()
        input_fname = tools.GetOutputFilename('mkimage.%s' % uniq)
        tools.WriteFile(input_fname, data)
        output_fname = tools.GetOutputFilename('mkimage-out.%s' % uniq)
        if self.mkimage.run_cmd('-d', input_fname, *self._args,
                                output_fname) is not None:
            self.SetContents(tools.ReadFile(output_fname))
        else:
            # Bintool is missing; just use the input data as the output
            self.record_missing_bintool(self.mkimage)
            self.SetContents(data)

        return True

    def ReadEntries(self):
        """Read the subnodes to find out what should go in this image"""
        for node in self._node.subnodes:
            entry = Entry.Create(self, node)
            entry.ReadNode()
            self._mkimage_entries[entry.name] = entry

    def SetAllowFakeBlob(self, allow_fake):
        """Set whether the sub nodes allows to create a fake blob

        Args:
            allow_fake: True if allowed, False if not allowed
        """
        for entry in self._mkimage_entries.values():
            entry.SetAllowFakeBlob(allow_fake)

    def CheckFakedBlobs(self, faked_blobs_list):
        """Check if any entries in this section have faked external blobs

        If there are faked blobs, the entries are added to the list

        Args:
            faked_blobs_list: List of Entry objects to be added to
        """
        for entry in self._mkimage_entries.values():
            entry.CheckFakedBlobs(faked_blobs_list)

    def AddBintools(self, tools):
        self.mkimage = self.AddBintool(tools, 'mkimage')
