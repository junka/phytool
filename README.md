phytool
=======

Linux MDIO register access

Usage
-----

Unlike the original one, PHYAD was automatically filled, So read C22 register or C45 MMD.register directly.

    phytool read  IFACE/[MMD.]REG
    phytool write IFACE/[MMD.]REG <0-0xffff>
    phytool print IFACE/[[MMD.]REG]

    Clause 22:

    REG  := <0-0x1f>

    Clause 45 (not supported by all MDIO drivers):

    MMD  := <0-0x1f>
    REG  := <0-0xffff>

The `read` and `write` commands are simple register level
accessors. The `print` command will pretty-print a register. When
using the `print` command, the register is optional. If left out, the
most common registers will be shown.

Examples
--------

    ~ # phytool read eth4/4
    0x0de1

    ~ # phytool print eth0
    ieee-phy: id:0x01410eb1

       ieee-phy: reg:BMCR(0x00) val:0x1140
          flags:          -reset -loopback +aneg-enable -power-down -isolate -aneg-restart -collision-test
          speed:          1000-full

       ieee-phy: reg:BMSR(0x01) val:0x7949
          capabilities:   -100-b4 +100-f +100-h +10-f +10-h -100-t2-f -100-t2-h
          flags:          +ext-status -aneg-complete -remote-fault +aneg-capable -link -jabber +ext-register

Origin & References
-------------------

phytool is developed and maintained by Tobias Waldekranz.  
Please file bug fixes and pull requests at [GitHub][]

[GitHub]: https://github.com/wkz/phytool
